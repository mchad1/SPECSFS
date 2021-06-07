/**
 *  @copyright
 *  Copyright (c) 2002-2020 by Iozone.org
 *      All rights reserved.
 *              Iozone.org
 *              7417 Crenshaw Dr.
 *              Plano, TX 75025
 *
 *      This product contains benchmarks acquired from several sources who
 *      understand and agree with Iozone's goal of creating fair and objective
 *      benchmarks to measure computer performance.
 *
 *      This copyright notice is placed here only to protect Iozone.org in the
 *      event the source is misused in any manner that is contrary to the
 *      spirit, the goals and the intent of Iozone.org
 *
 *      Founder, author, maintainer: Don Capps
 *      
 *      Author: Udayan Bapat, NetApp Inc.
 *
 */
#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "./copyright.txt"
#include "./license.txt"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#if defined(_linux_) || defined(_bsd_) || defined(_aix_)
#include <sys/resource.h>
#endif
#if !defined(WIN32)
#include<unistd.h>
#include<sys/socket.h>
#include<poll.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/timeb.h>
#include <io.h>
#include <process.h>
#include "../win32lib/win32_sub.h"
#endif
#include<sys/types.h>


/* This is different for the various versions. */
#include <netmist.h>

#include "netmist_defines.h"
#include "netmist_utils.h"
#include "netmist_copyright.h"
#include "netmist_logger.h"
#include "netmist_random.h"
#include "netmist_fsm.h"
#include "netmist_thread.h"

static int peer_socket;
static int peer_keepalive_socket;

extern struct mix_table my_mix_table[];
extern volatile long *global_reserve_memory;
extern int netmist_get_sock_error (void);
extern char *netmist_get_sock_errstr (int);
extern char *netmist_strerror (int);
extern int netmist_get_error (void);
extern void R_exit (enum netmist_child_err, int);

#if defined(WIN32)
/* Enables a getrusage type of functionality for CPU and memory stats */
/* Brutally created for a Windows port. */
#define RUSAGE_SELF 0
#define RUSAGE_THREAD 1
#include <psapi.h>
#include <time.h>

struct rusage
{
    struct timeval ru_utime;	/* user CPU time used */
    struct timeval ru_stime;	/* system CPU time used */
    long ru_maxrss;		/* maximum resident set size */
    long ru_ixrss;		/* integral shared memory size */
    long ru_idrss;		/* integral unshared data size */
    long ru_isrss;		/* integral unshared stack size */
    long ru_minflt;		/* page reclaims (soft page faults) */
    long ru_majflt;		/* page faults (hard page faults) */
    long ru_nswap;		/* swaps */
    long ru_inblock;		/* block input operations */
    long ru_oublock;		/* block output operations */
    long ru_msgsnd;		/* IPC messages sent */
    long ru_msgrcv;		/* IPC messages received */
    long ru_nsignals;		/* signals received */
    long ru_nvcsw;		/* voluntary context switches */
    long ru_nivcsw;		/* involuntary context switches */
};
int getrusage (int, struct rusage *);
#endif

/* 
 * These are the objects associated with each unique
 * workload in the mix 
 */
struct work_object OLD_workloads[20];

extern char version[];

extern int num_work_obj;
extern char my_pdsm_remote_file[MAXNAME];
extern void client_populate_results (int client_id, char *client_name);
extern void complete_client_run_work (void);
extern void complete_client_epoch_work (void);
extern void complete_client_go_work (void);
extern void complete_client_init_work (void);
int what_am_i (char *, int);

extern char client_localname[MAXNAME];

static void wait_for_client_message (int command, const char *log_message);

static struct keepalive_command k_keepalive;
static struct ext_keepalive_command k_ext_keepalive;
static unsigned int keepalive_count = 0;

/** 
 * @brief Close the keep-alive sockets
 */
static void
close_keepalive_sockets (void)
{
    log_file (LOG_EXEC, "Closing keepalive sockets\n");
#ifdef WIN32
    (void) shutdown (peer_keepalive_socket, SD_BOTH);
#else
    (void) shutdown (peer_keepalive_socket, SHUT_RDWR);
#endif
}

/** 
 * @brief Function for thread to listen for keep-alive messages
 *
 * @param ptr : Pointer to void, opaque contents for this thread's input.
 */
static void *
client_listen_thread_handler (void *ptr)
{
    int rc;
    struct pollfd *handle;
    int error_code = 0;
    char *str_error = NULL;

    log_file (LOG_THREAD, "listen thread started %p\n", ptr);

    handle = my_malloc (sizeof (struct pollfd));
    if (handle == NULL)
    {
	log_file (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	exit (1);
    }

    handle[0].fd = peer_keepalive_socket;
    handle[0].events = POLLIN;

    while (1)
    {
#if defined(WIN32)
	rc = WSAPoll (handle, 1, KEEPALIVE_POLL_TIMEOUT);
#else
	rc = poll (handle, 1, KEEPALIVE_POLL_TIMEOUT);
#endif
	if (rc < 0)
	{
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    log_file (LOG_ERROR, "poll error in %s %s\n",
		      __FUNCTION__, str_error);
	    exit (1);
	}
	else if (rc == 0)
	{
	    log_file (LOG_ERROR, "timeout in poll in %s\n", __FUNCTION__);
	    continue;
	}

	if (!(handle[0].revents & POLLIN))
	{
	    continue;
	}

	rc = socket_receive (peer_keepalive_socket, (char *) g_ekc,
			     sizeof (struct ext_keepalive_command));

	/*
	 * rc = 0 can happen if the socket is shutdown from NM or client.
	 * It can also happen if clients get into this before NM gets to 
	 * other endpoint. 
	 * In this case, sleep for 1 sec and continue
	 * If benchmark is ending, then there is no issue and the thread
	 * will be killed.
	 * Otherwise, it will correctly block once NM initializes socket
	 * at its end.
	 */
	if (rc < 0 || rc == 0)
	{
	    if (fsm_state == CLIENT_FSM_SHUTDOWN_PHASE)
	    {
		log_file (LOG_DEBUG, "listen thread exiting..\n");
		return NULL;
	    }
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    log_file (LOG_ERROR,
		      "socket_receive returned %d in %s Status: %s\n", rc,
		      __FUNCTION__, str_error);
	    return NULL;
	}

	keepalive_ext_to_int (g_kc, g_ekc);
	print_keepalive_command (g_kc);
    }

    return NULL;
}

/** 
 * @brief Function for thread to send keep alive messages
 *
 * @param ptr : Pointer to void, opaque contents for this thread's input.
 */
static void *
client_keepalive_thread_handler (void *ptr)
{
#if defined(_linux_)  || defined(_macos_) || defined (_solaris_) || defined(_bsd_) || defined(_aix_) || defined(WIN32)
    struct rusage my_usage;
#endif
    log_file (LOG_THREAD, "keepalive thread started %p\n", ptr);

    /*
     * Initialize this message only once
     */
    init_k_keepalive_command (&k_keepalive, 3, cmdline.client_id);

    while (1)
    {
	nap (cmdline.keepalive * 1000);

	if (fsm_state == CLIENT_FSM_SHUTDOWN_PHASE)
	{
	    log_file (LOG_DEBUG, "keepalive thread exiting..\n");
	    return NULL;
	}

	keepalive_count++;
	k_keepalive.k_count = keepalive_count;
#if defined(_linux_) || defined(_macos_) || defined(_solaris_) || defined(_bsd_) || defined(_aix_) || defined(WIN32)

	getrusage (RUSAGE_SELF, &my_usage);
	k_keepalive.k_user_time = (double) my_usage.ru_utime.tv_sec +
	    ((double) my_usage.ru_utime.tv_usec * 0.000001);
	k_keepalive.k_system_time = (double) my_usage.ru_stime.tv_sec +
	    ((double) my_usage.ru_stime.tv_usec * 0.000001);
	k_keepalive.k_minflt = (int) my_usage.ru_minflt;
	k_keepalive.k_majflt = (int) my_usage.ru_majflt;
	k_keepalive.k_inblock = (int) my_usage.ru_inblock;
	k_keepalive.k_oublock = (int) my_usage.ru_oublock;
	k_keepalive.k_maxrss = (int) my_usage.ru_maxrss;

#endif
	keepalive_int_to_ext (&k_keepalive, &k_ext_keepalive);

	socket_send (peer_keepalive_socket, (char *) &k_ext_keepalive,
		     sizeof (struct ext_keepalive_command));

	log_file (LOG_THREAD, "sent keepalive to nodeManager\n");
    }

    return NULL;
}

/** 
 * @brief Function to shutdown a client and release resources.
 *
 */

static void
client_shutdown (void)
{
    log_file (LOG_EXEC, "Shutting down\n");

    /*
     * Close sockets
     */
    log_file (LOG_EXEC, "Closing sockets\n");
    close_keepalive_sockets ();

    /*
     * Close file handles
     */
    log_file (LOG_EXEC, "Closing file handles\n");

    /*
     * Cleanup memory
     */
    log_file (LOG_EXEC, "Freeing memory\n");
}

/**
 * @brief Clients tell the nodeManager that warmup is done.
 *
 * __doc__
 * __doc__  Function : void send_warmup_done(void)
 * __doc__  Returns  : void
 * __doc__  Performs : Clients tell the nodeManager that warmup is done.
 * __doc__
 */
void
send_warmup_done (void)
{
    reset_all_prime_comm_buffers ();
    g_mc->m_command = WARM_PHASE_DONE;
    g_mc->m_child_port = 0;
#if defined(WIN32)
    g_mc->m_child_pid = _getpid ();
#else
    g_mc->m_child_pid = getpid ();
#endif
    g_mc->m_client_number = cmdline.client_id;
    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

    socket_send (peer_socket, (char *) g_emc,
		 sizeof (struct ext_prime_command));

    log_file (LOG_EXEC, "sending WARM UP done to nodeManager.\n");
}

/**
 * @brief Clients tell the nodeManager that I have an error.
 *
 * @param  chid : child ID 
 * @param  eval : Netmist specific error value 
 * @param  plat_err : Error value  (value of errno in Unix)
 */
/*
 * __doc__
 * __doc__  Function : void tell_nm_error(int chid, int eval, errno)
 * __doc__  Arguments: int child ID.
 * __doc__             int eval: Netmist specific error value.
 * __doc__             int plat_err: Platform specific error value.
 * __doc__  Returns  : void
 * __doc__  Performs : Clients tell the nodeManager that I have an error.
 * __doc__
 */
void
tell_nm_error (int chid, enum netmist_child_err eval, int plat_err)
{
    log_file (LOG_ERROR,
	      "%d: Tell nodeManager Netmist error %d Platform error: %d\n",
	      chid, eval, plat_err);

    reset_all_prime_comm_buffers ();
    g_mc->m_command = R_FAILED;
    g_mc->m_child_flag = CHILD_STATE_READY;
    g_mc->m_client_number = (int) chid;
    g_mc->m_client_eval = eval;
    g_mc->m_client_plat_err = plat_err;

    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

    (void) socket_send (peer_socket,
			(char *) g_emc, sizeof (struct ext_prime_command));
}

/**
 * @brief Clients tell the prime that I am this percent complete 
 *
 * @param percent : Percent complete.
 * @param chid : Child_id value.
 * @param type : Type of message. WARMUP, INIT, RUN...
 */
/*
 * __doc__
 * __doc__  Function : void 
 * __doc__             tell_prime_pct_complete(int percent, int chid, int type)
 * __doc__  Arguments: int: percent complete
 * __doc__             int: child ID.
 * __doc__             int: Command type.
 * __doc__  Returns  : void
 * __doc__  Performs : Clients tell the prime that I am this percent complete 
 * __doc__
 */
void
tell_prime_pct_complete (int percent, int chid, int type)
{
    int throttle = netmist_rand () % 5;


    log_file (LOG_DEBUG, "%d percent complete\n", percent);
    /*
     * Rate limit heartbeats for only 20% of times to reduce
     * network traffic
     */
    if (throttle != 0)
    {
	return;
    }


    log_file (LOG_DEBUG, " Sending it to Prime\n");

    reset_all_prime_comm_buffers ();
    g_mc->m_command = type;
    g_mc->m_client_number = chid;
    g_mc->m_percent_complete = percent;

    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

    socket_send (peer_socket, (char *) g_emc,
		 sizeof (struct ext_prime_command));
}

/**
 * @brief Clients tell the prime that I am alive..heartbeat
 *
 * @param  chid : Child ID
 * @param  type : type of heartbeat
 * @param  rate : current op_rate
 */
/*
 * __doc__
 * __doc__  Function : void 
 * __doc__             tell_prime_heartbeat(int chid, int type, double rate)
 * __doc__  Arguments: int Child ID
 * __doc__             int type of heartbeat
 * __doc__             double current op_rate.
 * __doc__  Returns  : void
 * __doc__  Performs : Clients tell the prime that I am alive..heartbeat
 * __doc__
 */
void
tell_prime_heartbeat (int chid, int type, double rate)
{
    int throttle = netmist_rand () % 3;

    /*
     * Rate limit heartbeats for only 30% of time time to reduce
     * network traffic
     */
    if (throttle != 0)
    {
	return;
    }

    log_file (LOG_DEBUG, "Tell prime heartbeat %lf\n", rate);

    reset_all_prime_comm_buffers ();
    g_mc->m_command = R_HEARTBEAT;
    g_mc->m_current_op_rate = rate;
    g_mc->m_client_number = chid;
    g_mc->m_heartbeat_type = type;

    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

    socket_send (peer_socket, (char *) g_emc,
		 sizeof (struct ext_prime_command));
}

/**
 * @brief Client initializes the logging capability.
 */
static void
client_initialize_logging (void)
{
    char localtracename[MAXNAME];
    char logrole[MAXNAME];

    /*
     * Override input argument to start logging
     */
    cmdline.log_flag = 1;

#if defined(WIN32)
    trim_leading_spaces (cmdline.client_windows_log_dir);
#else
    trim_leading_spaces (cmdline.client_log_dir);
#endif
    internal_cfg.u.client_cfg.log_handle = NULL;

    if (cmdline.Uflag)
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s\\netmist_C%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s\\netmist_trace_C%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s/netmist_C%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s/netmist_trace_C%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "csv");
#endif
    }
    else
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "\\tmp\\netmist_C%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "\\tmp\\netmist_trace_C%d.%s", cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "/tmp/netmist_C%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "/tmp/netmist_trace_C%d.%s", cmdline.client_id, "csv");
#endif
    }
    if (cmdline.WUflag)
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s\\netmist_C%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s\\netmist_trace_C%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s/netmist_C%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s/netmist_trace_C%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "csv");
#endif
    }
    else
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "\\tmp\\netmist_C%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "\\tmp\\netmist_trace_C%d.%s", cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "/tmp/netmist_C%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "/tmp/netmist_trace_C%d.%s", cmdline.client_id, "csv");
#endif
    }

    /*
     * Since clients run on remote machines, we need logging!
     */
    if (cmdline.log_flag)
    {
	internal_cfg.u.client_cfg.log_handle =
	    init_log (cmdline.log_filename, "w");
	if (internal_cfg.u.client_cfg.log_handle == NULL)
	{
	    perror ("Unable to open log file netmist_C...log for stdout\n");
	    exit (1);
	}
    }

    /* Reassign the handle to the log file so everything is sent to the log */
#if CLOSE_STDIN_STDOUT
    fclose (stdout);
    fclose (stderr);
#endif

#if defined(WIN32)
    (void) _dup2 (_fileno (internal_cfg.u.client_cfg.log_handle), 1);
    (void) _dup2 (_fileno (internal_cfg.u.client_cfg.log_handle), 2);
#else
    (void) dup2 (fileno (internal_cfg.u.client_cfg.log_handle), 1);
    (void) dup2 (fileno (internal_cfg.u.client_cfg.log_handle), 2);
/*
	stdout = internal_cfg.u.client_cfg.log_handle;
        stderr = internal_cfg.u.client_cfg.log_handle;
*/
#endif
    /*
       stdout = internal_cfg.u.client_cfg.log_handle;
       stderr = internal_cfg.u.client_cfg.log_handle;
     */


    (void) snprintf (logrole, sizeof (logrole), "%s %d", "Client",
		     cmdline.client_id);

    set_log_role (logrole);
    set_log_level (cmdline.tracelevel);
    set_timestamp_in_log_stdout ();
    set_timestamp_in_log_file ();
}

/**
 * @brief Validate command line input args make sense.
 */
static void
client_validate_input_args (void)
{
#if VALIDATE_INPUT_ARGS
    /*
     * Child must have a port to send to.
     */
    if (!cmdline.kflag && !cmdline.Pflag)
    {
	log_file (LOG_ERROR, "Missing prime's port number (-P) \n");
	usage ();
	exit (0);
    }

    /*
     * Child gets cmdline.client_files and cmdline.client_dirs from message passing at startup 
     * Don't need to have it passed in, and the message at comm startup will
     * over-ride anything that was passed in.
     */

    /*
     * Child must have cmdline.fsize set
     */
    if (!cmdline.setfsflag)
    {
	log_file (LOG_ERROR, "Missing cmdline.fsize\n");
	usage ();
	exit (0);
    }
    /*
     * Child must have client id set
     */
    if (!cmdline.nflag)
    {
	log_file (LOG_ERROR, "Missing client id\n");
	usage ();
	exit (0);
    }
    /*
     * Child must have client cmdline.workdir
     */
    if (!cmdline.pflag)
    {
	log_file (LOG_ERROR, "Missing client cmdline.workdir\n");
	usage ();
	exit (0);
    }
#endif
}

/**
 * @brief Initialze runtime structures. Not used at this time.
 */
static void
initialize_runtime_structures (void)
{
}

/**
 * @brief Start a listening thread. Not used at this time.
 */
static void
client_start_listening_thread (void)
{
    log_file (LOG_EXEC, "does not need a listening server socket\n");
}

/**
 * @brief NodeManager memory allocation. 
 *
 */
static void
client_allocate_comm_structures (void)
{
    g_mc = (struct prime_command *) MALLOC (sizeof (struct prime_command));
    if (g_mc == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate memory for prime command\n");
	exit (1);
    }

    g_emc =
	(struct ext_prime_command *)
	MALLOC (sizeof (struct ext_prime_command));
    if (g_emc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for ext nodeManager command\n");
	exit (1);
    }

    g_nmc = NULL;
    g_enmc = NULL;

    g_cc = (struct client_command *) MALLOC (sizeof (struct client_command));
    if (g_cc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for nodeManager command\n");
	exit (1);
    }

    g_ecc =
	(struct ext_client_command *)
	MALLOC (sizeof (struct ext_client_command));
    if (g_ecc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for ext nodeManager command\n");
	exit (1);
    }

    g_kc =
	(struct keepalive_command *)
	MALLOC (sizeof (struct keepalive_command));
    if (g_kc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for keepalive command\n");
	exit (1);
    }

    g_ekc =
	(struct ext_keepalive_command *)
	MALLOC (sizeof (struct ext_keepalive_command));
    if (g_ekc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for ext keepalive command\n");
	exit (1);
    }

    log_file (LOG_EXEC_VERBOSE,
	      "Size of internal prime comm structure = %lu\n",
	      sizeof (*g_mc));
    log_file (LOG_EXEC_VERBOSE,
	      "Size of external prime comm structure = %lu\n",
	      sizeof (*g_emc));
    log_file (LOG_EXEC_VERBOSE,
	      "Size of internal nodeManager comm structure = %lu\n",
	      sizeof (*g_nmc));
    log_file (LOG_EXEC_VERBOSE,
	      "Size of external nodeManager comm structure = %lu\n",
	      sizeof (*g_enmc));
    log_file (LOG_EXEC_VERBOSE,
	      "Size of internal client comm structure = %lu\n",
	      sizeof (*g_cc));
    log_file (LOG_EXEC_VERBOSE,
	      "Size of external client comm structure = %lu\n",
	      sizeof (*g_ecc));
}

/**
 * @brief Log the current FSM state.
 */
static void
client_log_state (void)
{
    log_file (LOG_FSM, "**** %s ---> %s ****\n",
	      _fsm_state_names[fsm_prev_state], _fsm_state_names[fsm_state]);
}

extern int Windows_authenticate_and_login (void);
extern int got_wl;

/**
 * @brief Perform all of the work of the client, walking through the FSM.
 */
void
do_all_client_work (void)
{
    abort_ifnot_client ();

    int whileloop = 1;
    int error;
    double epoch_my_time, time_skew;
    char type_buffer[256];

    fsm_next_state = CLIENT_FSM_START;
    fsm_state = CLIENT_FSM_START;
    fsm_prev_state = CLIENT_FSM_START;

    /*
     * Typical FSM infinite loop
     */
    while (whileloop)
    {
	switch (fsm_state)
	{
	case CLIENT_FSM_START:

	    client_initialize_logging ();
#if defined(WIN32)
	    error = Windows_authenticate_and_login ();	/* Move to real user */
	    if (error != 0)
	    {
		log_file (LOG_ERROR, "Authenticate failed.\n");
		exit (-1);
	    }
#endif
	    while (1)
	    {
		error = my_gethostname (client_localname, MAXHNAME);
		if (error == 0)
		{
		    break;
		}
		log_file (LOG_ERROR, "my_gethostname failed: %s\n",
			  netmist_strerror (netmist_get_error ()));
	    }

	    (void) what_am_i (type_buffer, 256);
	    log_file (LOG_EXEC, "OS Type: %s\n", type_buffer);

	    log_file (LOG_FSM, "Client FSM starts:\n\n");

	    client_validate_input_args ();
	    initialize_runtime_structures ();
	    client_start_listening_thread ();
	    client_allocate_comm_structures ();

	    fsm_prev_state = CLIENT_FSM_START;
#ifdef CLIENT_ATTACH_DEBUGGER
	    fsm_state = CLIENT_FSM_DEBUG_STATE;
#else
	    fsm_state = CLIENT_FSM_NODEMANAGER_CONNECT;
#endif
	    client_log_state ();
	    break;

	case CLIENT_FSM_DEBUG_STATE:
	    nap (30000);
	    fsm_prev_state = CLIENT_FSM_DEBUG_STATE;
	    fsm_state = CLIENT_FSM_NODEMANAGER_CONNECT;
	    client_log_state ();
	    break;

	case CLIENT_FSM_NODEMANAGER_CONNECT:
	    trim_leading_spaces (cmdline.prime_name);
	    peer_socket = connect_peer_server (cmdline.prime_name,
					       cmdline.prime_p);

	    fsm_prev_state = CLIENT_FSM_NODEMANAGER_CONNECT;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_NODEMANAGER_JOIN;
	    client_log_state ();
	    break;


	case CLIENT_FSM_NODEMANAGER_JOIN:
	    reset_all_prime_comm_buffers ();
	    /* send join message */
	    g_mc->m_command = R_CHILD_JOIN;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends join.\n");

	    fsm_prev_state = CLIENT_FSM_NODEMANAGER_JOIN;
	    fsm_state = CLIENT_OPEN_KEEPALIVE;
	    client_log_state ();
	    break;

	case CLIENT_OPEN_KEEPALIVE:
	    peer_keepalive_socket = connect_peer_server (cmdline.prime_name,
							 cmdline.prime_p);

	    error = netmist_start_threads (client_keepalive_thread_handler,
					   client_listen_thread_handler);
	    if (error)
	    {
		log_file (LOG_ERROR, "Failed to create threads");
	    }
	    fsm_prev_state = CLIENT_OPEN_KEEPALIVE;
	    fsm_state = CLIENT_FSM_GO;
	    client_log_state ();
	    break;

	case CLIENT_FSM_GO:
	    wait_for_client_message (R_GO, "GO");

	    complete_client_go_work ();

	    fsm_prev_state = CLIENT_FSM_GO;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_GO_DONE;
	    client_log_state ();

	    break;

	case CLIENT_FSM_GO_DONE:
	    reset_all_prime_comm_buffers ();
	    /* send GO1 done message */
	    g_mc->m_command = R_GO_DONE;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends Go done.\n");

	    fsm_prev_state = CLIENT_FSM_GO_DONE;
	    fsm_state = CLIENT_FSM_INIT_PHASE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_INIT_PHASE:
	    wait_for_client_message (INIT_PHASE, "INIT");

	    complete_client_init_work ();

	    fsm_prev_state = CLIENT_FSM_INIT_PHASE;
	    fsm_state = CLIENT_FSM_INIT_PHASE_DONE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_INIT_PHASE_DONE:
	    reset_all_prime_comm_buffers ();
	    /* send INIT DONE message */
	    g_mc->m_command = INIT_PHASE_DONE;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends INIT done.\n");

	    fsm_prev_state = CLIENT_FSM_INIT_PHASE_DONE;
	    fsm_state = CLIENT_FSM_INIT_RESULT_REQUEST;
	    client_log_state ();
	    break;

	case CLIENT_FSM_INIT_RESULT_REQUEST:
	    wait_for_client_message (ASK_INIT_RESULTS, "INIT results");

	    client_populate_results (cmdline.client_id, client_localname);

	    fsm_prev_state = CLIENT_FSM_RESULT_REQUEST;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_INIT_SEND_RESULT;
	    client_log_state ();
	    break;

	case CLIENT_FSM_INIT_SEND_RESULT:
	    (void) socket_send (peer_socket,
				(char *) g_emc,
				sizeof (struct ext_prime_command));

	    fsm_prev_state = CLIENT_FSM_INIT_SEND_RESULT;
	    fsm_state = CLIENT_FSM_TIME_SYNC;
	    client_log_state ();
	    break;

	case CLIENT_FSM_TIME_SYNC:
	    wait_for_client_message (R_TIME_SYNC, "time sync");

	    epoch_my_time = getepochtime ();
	    time_skew = g_cc->c_prime_eclock_time - epoch_my_time;
	    log_file (LOG_FSM_VERBOSE, "time skew: %lf\n", time_skew);

	    fsm_prev_state = CLIENT_FSM_TIME_SYNC;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_TIME_SYNC_DONE;
	    client_log_state ();

	    break;

	case CLIENT_FSM_TIME_SYNC_DONE:
	    reset_all_prime_comm_buffers ();
	    /* send TIME SYNC DONE message */
	    g_mc->m_command = R_TIME_SYNC_DONE;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends TIME SYNC done.\n");

	    fsm_prev_state = CLIENT_FSM_TIME_SYNC_DONE;
	    fsm_state = CLIENT_FSM_EPOCH;
	    client_log_state ();

	    break;

	case CLIENT_FSM_EPOCH:
	    wait_for_client_message (R_EPOCH, "Epoch");

	    complete_client_epoch_work ();

	    fsm_prev_state = CLIENT_FSM_EPOCH;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_EPOCH_DONE;
	    client_log_state ();

	    break;

	case CLIENT_FSM_EPOCH_DONE:
	    reset_all_prime_comm_buffers ();
	    /* send GO DONE message */
	    g_mc->m_command = R_EPOCH_DONE;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends Epoch done.\n");

	    fsm_prev_state = CLIENT_FSM_EPOCH_DONE;
	    fsm_state = CLIENT_FSM_WARMUP_PHASE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_WARMUP_PHASE:
	case CLIENT_FSM_RUN_PHASE:
	    wait_for_client_message (WARM_PHASE, "Warmup");

	    complete_client_run_work ();

	    fsm_prev_state = CLIENT_FSM_WARMUP_PHASE;
	    fsm_state = CLIENT_FSM_RUN_PHASE_DONE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_RUN_PHASE_DONE:
	    reset_all_prime_comm_buffers ();
	    g_mc->m_command = RUN_PHASE_DONE;
	    g_mc->m_child_port = 0;
#if defined(WIN32)
	    g_mc->m_child_pid = _getpid ();
#else
	    g_mc->m_child_pid = getpid ();
#endif
	    g_mc->m_client_number = cmdline.client_id;

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "sends RUN done.\n");

	    fsm_prev_state = CLIENT_FSM_RUN_PHASE_DONE;
	    fsm_state = CLIENT_FSM_RESULT_REQUEST;
	    client_log_state ();
	    break;

	case CLIENT_FSM_RESULT_REQUEST:
	    wait_for_client_message (ASK_RESULTS, "Result Request");

	    client_populate_results (cmdline.client_id, client_localname);

	    fsm_prev_state = CLIENT_FSM_RESULT_REQUEST;
	    fsm_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_next_state = CLIENT_FSM_SEND_RESULT;
	    client_log_state ();
	    break;

	case CLIENT_FSM_SEND_RESULT:
	    (void) socket_send (peer_socket,
				(char *) g_emc,
				sizeof (struct ext_prime_command));

	    fsm_prev_state = CLIENT_FSM_SEND_RESULT;
	    fsm_state = CLIENT_FSM_COMPLETE_PHASE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_COMPLETE_PHASE:
	    fsm_prev_state = CLIENT_FSM_COMPLETE_PHASE;
	    fsm_state = CLIENT_FSM_SHUTDOWN_PHASE;
	    client_log_state ();
	    break;

	case CLIENT_FSM_SHUTDOWN_PHASE:
	    wait_for_client_message (SHUTDOWN_PHASE, "shutdown");

	    client_shutdown ();

	    fsm_prev_state = CLIENT_FSM_SHUTDOWN_PHASE;
	    fsm_state = CLIENT_FSM_FINISHED;
	    client_log_state ();
	    break;

	case CLIENT_FSM_SYNC_OK_STATE:
	    wait_for_client_message (R_SYNC_OK, "sync ok");

	    fsm_prev_state = CLIENT_FSM_SYNC_OK_STATE;
	    fsm_state = fsm_next_state;
	    client_log_state ();
	    break;

	case CLIENT_FSM_FINISHED:
	    exit (0);

	case CLIENT_FSM_ERROR_STATE:
	default:
	    whileloop = 0;
	    break;
	}
    }
}

static void
wait_for_client_message (int command, const char *log_message)
{
    if (log_message)
    {
	log_file (LOG_FSM_VERBOSE, "waiting for %s...\n", log_message);
    }

    reset_all_client_comm_buffers ();

    unsigned int rc =
	socket_receive (peer_socket, (char *) g_ecc,
			sizeof (struct ext_client_command));
    if (rc < sizeof (struct ext_client_command))
    {
	log_file (LOG_ERROR,
		  "Incomplete client message. Received %d, Expected %d\n"
		  "aborting ...\n", rc, sizeof (struct ext_client_command));
	exit (0);
    }

    //Added for debugging. This prints the crc32 on the external buffer
    add_crc32_ext_client_command ();

    //convert external buffer into internal structure
    client_ext_to_int (g_cc, g_ecc);

    //Validate if the received message is of the same type as command
    if (!validate_client_command ("nodeManager", command))
    {
	//report error back and wait for termination
	R_exit (NETMIST_BAD_CRC, 0);
    }

    if (log_message)
    {
	log_file (LOG_FSM_VERBOSE, "%s received\n", log_message);
    }
}

#if defined(WIN32)
#include <VersionHelpers.h>

int
what_am_i (char *buffer, int size)
{
    SYSTEM_INFO siSysInfo;
    char proctype[25];
    unsigned long type;

    memset (proctype, 0, 25);
    GetSystemInfo (&siSysInfo);
    type = (unsigned long) siSysInfo.wProcessorArchitecture;
    switch (type)
    {
    case 9:
	snprintf (proctype, 25, "x64");
	break;
    case 5:
	snprintf (proctype, 25, "ARM");
	break;
    case 12:
	snprintf (proctype, 25, "ARM64");
	break;
    case 6:
	snprintf (proctype, 25, "IA64");
	break;
    case 0:
	snprintf (proctype, 25, "x86");
	break;
    case 0xffff:
	snprintf (proctype, 25, "Unknown");
	break;
    default:
	snprintf (proctype, 25, "Unknown. Type %u", type);
	break;
    };
    if (IsWindowsServer ())
    {
	snprintf (buffer, size, "Windows Server on %s", proctype);
    }
    else
    {
	snprintf (buffer, size, "Windows Client on %s", proctype);
    }
    return (0);
}
#else
#include <sys/utsname.h>
int
what_am_i (char *buffer, int size)
{
    struct utsname utsnamebuf;
    uname (&utsnamebuf);
    snprintf (buffer, size, "%s on %s", utsnamebuf.sysname,
	      utsnamebuf.machine);
    return (0);
}
#endif

#if defined(WIN32)

static void
usage_to_timeval (FILETIME * ft, struct timeval *tv)
{
    ULARGE_INTEGER time;
    time.LowPart = ft->dwLowDateTime;
    time.HighPart = ft->dwHighDateTime;

    tv->tv_sec = (long) (time.QuadPart / 10000000);
    tv->tv_usec = (long) ((time.QuadPart % 10000000) / 10);
}

/*
 * Gather some IO stats that are similar to what getrusage does.
 * Make its units in Kbytes so that the numbers will be more comparable.
 */
void
get_io_counters (struct rusage *usage)
{
    IO_COUNTERS io_counters;;
    HANDLE myHandle = GetCurrentProcess ();
    GetProcessIoCounters (myHandle, &io_counters);
    usage->ru_inblock = (long) (io_counters.ReadTransferCount / 1024LL);
    usage->ru_oublock = (long) (io_counters.WriteTransferCount / 1024LL);
}

int
getrusage (int who, struct rusage *usage)
{
    FILETIME creation_time, exit_time, kernel_time, user_time;
    PROCESS_MEMORY_COUNTERS pmc;

    memset (usage, 0, sizeof (struct rusage));

    if (who == RUSAGE_SELF)
    {
	if (!GetProcessTimes
	    (GetCurrentProcess (), &creation_time, &exit_time, &kernel_time,
	     &user_time))
	{
	    log_file (LOG_ERROR, "Error in GetProcessTimes.\n");
	    return -1;
	}

	if (!GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc)))
	{
	    log_file (LOG_ERROR, "Error in GetProcessMemoryInfo.\n");
	    return -1;
	}

	usage_to_timeval (&kernel_time, &usage->ru_stime);
	usage_to_timeval (&user_time, &usage->ru_utime);
	usage->ru_majflt = pmc.PageFaultCount;
	usage->ru_maxrss = (long) (pmc.PeakWorkingSetSize / 1024);
	get_io_counters (usage);
	return 0;
    }
    else if (who == RUSAGE_THREAD)
    {
	if (!GetThreadTimes (GetCurrentThread (), &creation_time, &exit_time,
			     &kernel_time, &user_time))
	{
	    log_file (LOG_ERROR, "Error in GetThreadTimes\n");
	    return -1;
	}
	usage_to_timeval (&kernel_time, &usage->ru_stime);
	usage_to_timeval (&user_time, &usage->ru_utime);
	return 0;
    }
    else
    {
	return -1;
    }
}

/* Windows version of get_nprocs() */
int
get_nprocs (void)
{
    int number_cpus = 1;
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    number_cpus = sysinfo.dwNumberOfProcessors;
    return (number_cpus);
}
#endif
