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
#if !defined(WIN32)
#include<unistd.h>
#include <sys/resource.h>
#include <sys/poll.h>
#else
#include "../win32lib/win32_sub.h"
#include <process.h>
#include <WinSock2.h>
#if defined(WIN32)
extern int child_num;
extern int Child_num;
#endif
#endif
#include<time.h>
#include<errno.h>

#if defined(WIN32)
int win_exec (char **, int);
#define strdup _strdup		/* ANSI vs POSIX */
#endif

/****************************************************************************/
/* COMM SECTION */
/****************************************************************************/
/*
 * Communication library for controlling many hosts over the net.
 */
#if !defined(WIN32)
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#else
#include <signal.h>
#endif

#if defined(_bsd_) || defined(_macos_) || defined(_solaris_)
#include <signal.h>
#endif

#if defined(_macos_) || defined(_bsd_) || defined(_aix_)
int get_nprocs (void);
#endif
#if defined(_solaris_) || defined(_bsd_)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

/* This is different for the various versions. */
#include <netmist.h>

#include "netmist_version.h"
#include "netmist_defines.h"
#include "netmist_utils.h"
#include "netmist_if.h"
#include "netmist_random.h"
#include "netmist_structures.h"
#include "netmist_nodeManager.h"
#include "netmist_logger.h"
#include "netmist_fsm.h"
#include "netmist_thread.h"

static int nodeManager_listen_socket;
static int nodeManager_port;
static int peer_socket;
static char nodeManager_localname[MAXHNAME];
extern int num_work_obj;
extern int netmist_get_sock_error (void);
extern int netmist_get_error (void);
extern char *netmist_get_sock_errstr (int);
extern void R_exit (enum netmist_child_err, int);
#if defined(_linux_) || defined(_macos_) || defined(_solaris_) || defined(_bsd_) || defined(_aix_) || defined(WIN32)
static long long total_client_maxrss, total_client_oublock, total_client_inblock;
long long curr_client_maxrss, curr_oublock, curr_inblock;
float curr_us_time;;
static long long prev_oublock, prev_inblock;
static int keep_recv_count;
static double user_time, system_time, prev_user_time, prev_system_time;
volatile int cpu_warning;
volatile double cpu_percent_busy;
#endif
int NM_phase;

/*
 * Stores list of all clients that are being managed
 * by this nodeManager
 */
static int *nm_c_ids = NULL;
/*
 * Stores list of all clients specific configuration
 */
static struct nm_client_object *client_obj;
/*
 * Total number of clients that this NodeManager is 
 * managing
 */
static int nm_total_clients = -1;

extern char version[];

extern int my_gethostname (char *, int);
extern int max_proc;
extern int max_open;
extern void flip_b2f_slashes (char *, char *);


static int *c_keepalive_sockets = NULL;
static int peer_keepalive_socket;

static struct keepalive_command k_keepalive;
static struct ext_keepalive_command k_ext_keepalive;

static unsigned int keepalive_count = 0;

extern char *netmist_strerror (int);
void nodeManager_send_kill_procs (void);
static void nodeManager_shutdown (void);
static void wait_for_client_message (int command, const char *log_message);

/**
 * @brief Close the keepalive sockets.
 */
static void
close_keepalive_sockets (void)
{
    int i;
    log_file (LOG_EXEC, "Closing keepalive sockets\n");
    for (i = 0; i < nm_total_clients; i++)
    {
#if defined(WIN32)
	(void) shutdown (c_keepalive_sockets[i], SD_BOTH);
#else
	(void) shutdown (c_keepalive_sockets[i], SHUT_RDWR);
#endif
    }
#if defined(WIN32)
    (void) shutdown (peer_keepalive_socket, SD_BOTH);
#else
    (void) shutdown (peer_keepalive_socket, SHUT_RDWR);
#endif
}

/**
 * @brief Function for thread to listen for keep-alive messages.
 *
 * @param ptr : Void opaque passing of data to the listen thread.
 */
static void *
nodeManager_listen_thread_handler (void *ptr)
{
    struct pollfd *handle;
    int rc, i;
    int error_code = 0;
    char *str_error = NULL;
#if defined(_linux_) || defined(_macos_) || defined(_solaris_) || defined(_bsd_) || defined(_aix_) || defined(WIN32)
    double delta_time;
#endif

    log_file (LOG_THREAD, "listen thread started with %p\n", ptr);

    /*
     * Allocate + 1 to include Prime communication
     */

    handle = my_malloc (sizeof (struct pollfd) * (nm_total_clients + 1));
    if (handle == NULL)
    {
	log_file (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	exit (1);
    }

    for (i = 0; i < nm_total_clients; i++)
    {
	handle[i].fd = c_keepalive_sockets[i];
	handle[i].events = POLLIN;
    }

    /*
     * Add Prime socket to the handle list at the end
     */
    handle[i].fd = peer_keepalive_socket;
    handle[i].events = POLLIN;

    while (1)
    {
#if defined(WIN32)
	rc = WSAPoll (handle, nm_total_clients + 1, KEEPALIVE_POLL_TIMEOUT);
#else
	rc = poll (handle, nm_total_clients + 1, KEEPALIVE_POLL_TIMEOUT);
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

	for (i = 0; i < nm_total_clients; i++)
	{
	    if (!(handle[i].revents & POLLIN))
	    {
		continue;
	    }

	    rc = socket_receive (c_keepalive_sockets[i], (char *) g_ekc,
				 sizeof (struct ext_keepalive_command));

	    if (rc < 0 || rc == 0)
	    {
		if (fsm_state == NODEMANAGER_FSM_SHUTDOWN_PHASE)
		{
		    /* DO NOT CALL log_file() from a thread. */
 		    /*	It can cause the benchmark to crash */
		    log_file (LOG_DEBUG, "listen thread exiting..\n"); 
		    return NULL;
		}
		error_code = netmist_get_sock_error ();
		str_error = netmist_get_sock_errstr (error_code);
		log_file (LOG_ERROR,
			  "recv failure keepalive  %s %s. Child may have exited unexpectedly.\n",
			  __FUNCTION__, str_error);
		/*
		 * This can happen if Prime shuts down
		 * Since this is a valid possibility, just exit the thread
		 */

		return NULL;
	    }

	    keepalive_ext_to_int (g_kc, g_ekc);
	    print_keepalive_command (g_kc);

#if defined(_linux_) || defined(_macos_) || defined(_solaris_) || defined(_bsd_) || defined(_aix_) || defined(WIN32)

#if defined(_linux_)
#include <sys/sysinfo.h>
#endif
#if defined(WIN32)
	    extern int get_nprocs (void);
#endif
	    keep_recv_count++;
	    user_time += g_kc->k_user_time;
	    system_time += g_kc->k_system_time;
	    total_client_maxrss += g_kc->k_maxrss;
	    total_client_oublock += g_kc->k_oublock;
	    total_client_inblock += g_kc->k_inblock;
	    if (keep_recv_count == nm_total_clients)
	    {
		delta_time =
		    ((user_time - prev_user_time) +
		     (system_time - prev_system_time));
		if ((NM_phase == 1) || (NM_phase == 2))
		{
		     /* Note: Sending things to the NodeManager's log 
 		      * from a thread is dangerous.  Let the normal heartbeat 
 		      * code log this !! Just set some globals and let netmist_util loop log it.
 		      */
			curr_us_time = (float) ((delta_time * 100.0) / (float) (cmdline.keepalive * get_nprocs ()));
			curr_client_maxrss = total_client_maxrss;
			curr_inblock =  total_client_inblock - prev_inblock;
			curr_oublock = total_client_oublock - prev_oublock;
		}

		/* Warn when there is only 5% headroom left */
		if (delta_time >=
		    ((cmdline.keepalive * get_nprocs ()) * 95) / 100)
		{
		    cpu_percent_busy =
			(delta_time * 100.0) / (float) (cmdline.keepalive *
							get_nprocs ());
		    /* 
 		     * This warning will trigger netmist_util loop to log a warning 
                     */
		    if ((NM_phase == 1) || (NM_phase == 2)) /* Warmup or RUN */
		       cpu_warning = 1;
		}
		else
		{
		    cpu_warning = 0;
		    cpu_percent_busy =
			(delta_time * 100.0) / (float) (cmdline.keepalive *
							get_nprocs ());
		}
		prev_user_time = user_time;
		prev_system_time = system_time;
		prev_inblock = total_client_inblock;
		prev_oublock = total_client_oublock;

		user_time = system_time = 0;
		total_client_maxrss = 0LL;
		total_client_inblock = 0LL;
		total_client_oublock = 0LL;
		keep_recv_count = 0;
	    }
#endif
	    set_nm_client_keepalive (g_kc->k_source_id, g_kc->k_count);

	}

	/*
	 * Check for Prime at the end
	 */
	if (handle[i].revents & POLLIN)
	{
	    rc = socket_receive (peer_keepalive_socket, (char *) g_ekc,
				 sizeof (struct ext_keepalive_command));

	    if (rc < 0 || rc == 0)
	    {
		if (fsm_state == NODEMANAGER_FSM_SHUTDOWN_PHASE)
		{
		    /* DO NOT CALL log_file() from a thread. It can cause 
 		     * the benchmark to crash */
		    log_file (LOG_DEBUG, "listen thread existing..\n");
		    return NULL;
		}
		log_file (LOG_ERROR,
			  "recv failure keepalive: %s. Lost keepalive socket from the prime.\n",
			  netmist_strerror (netmist_get_sock_error ()));
		/*
		 * This can happen if Prime shuts down
		 * Since this is a valid possibility, just exit the thread
		 */
#if defined(WIN32)
		return (DWORD) 0;
#else
		return NULL;
#endif
	    }

	    keepalive_ext_to_int (g_kc, g_ekc);

	    /* The prime has sent the SIGINT via the keepalive socket mechanism */
	    if (g_kc->k_command == M_SIGINT)
	    {
		print_keepalive_command (g_kc);
		kill (getpid (), SIGINT);
		/*nodeManager_send_kill_procs (); */
	    }
	    else
	    {
		print_keepalive_command (g_kc);
		set_nm_prime_keepalive (g_kc->k_count);
	    }
	}

    }

    return NULL;
}

/**
 * @brief  Function for keep-alive thread to send keep-alive messages.
 *
 * @param ptr : Pointer to opaque data that is handed to the thread when it starts.
 */
static void *
nodeManager_keepalive_thread_handler (void *ptr)
{
    int i, ret, error = 0;
    log_file (LOG_THREAD, "keepalive thread started with %p\n", ptr);

    /*
     * Initialize this message only once
     */
    init_k_keepalive_command (&k_keepalive, 2, cmdline.client_id);


    while (1)
    {
	nap (cmdline.keepalive * 1000);

	if (fsm_state == NODEMANAGER_FSM_SHUTDOWN_PHASE)
	{
            /* DO NOT CALL log_file() from a thread. It can cause the 
 	     * benchmark to crash */
	    log_file (LOG_DEBUG, "keepalive thread exiting..\n");
	    return NULL;
	}

	keepalive_count++;
	k_keepalive.k_count = keepalive_count;
#if defined(_linux_)
	if (cpu_warning)
	{
	    k_keepalive.k_cpu_warning = 1;
	    k_keepalive.k_cpu_percent_busy = cpu_percent_busy;
	}
	else
	{
	    k_keepalive.k_cpu_warning = 0;
	    k_keepalive.k_cpu_percent_busy = 0.0;
	}
#endif
	keepalive_int_to_ext (&k_keepalive, &k_ext_keepalive);

	log_file (LOG_THREAD, "sent keepalive %u to client(s)\n",
		  keepalive_count);
	for (i = 0; i < nm_total_clients; i++)
	{
	    socket_send (c_keepalive_sockets[i], (char *) &k_ext_keepalive,
			 sizeof (struct ext_keepalive_command));
	}

	socket_send (peer_keepalive_socket, (char *) &k_ext_keepalive,
		     sizeof (struct ext_keepalive_command));
	log_file (LOG_THREAD, "sent keepalive %u to prime\n",
		  keepalive_count);


	if ((keepalive_count % KEEPALIVE_SCAN_FREQ) == 0)
	{
	    log_file (LOG_DEBUG, "Running keepalive scanner..\n");
	    ret = run_nodeManager_keepalive_scanner (keepalive_count);
	    if (ret == -1)
	    {
		log_file (LOG_ERROR, "Missing heartbeats from Prime\n");
		error = 1;
	    }
	    else if (ret > -1)
	    {
		log_file (LOG_ERROR,
			  "Missing heartbeats from client %d\n", ret);
		error = 1;
	    }

	    if (error)
	    {
		nodeManager_send_kill_procs ();
		nodeManager_shutdown ();
		exit (0);
	    }

	    log_file (LOG_DEBUG, "Success!\n");
	}
    }

    return NULL;
}


static int kill_procs_only_once = 0;
/**
 * @brief NodeManager sends a kill to all of its children.
 *
 */
void
nodeManager_send_kill_procs (void)
{
    int i, c_pid, c_id;

    if (kill_procs_only_once == 0)
    {
	kill_procs_only_once = 1;
    }
    else
    {
	return;
    }

    log_file (LOG_EXEC, "Sending kill procs from NodeManager id %d\n",
	      cmdline.client_id);

    log_file (LOG_EXEC, "Killing Clients\n");
    for (i = 0; i < nm_total_clients; i++)
    {
	c_id = nm_c_ids[i];
	c_pid = get_nm_client_pid (c_id);
	if (c_pid)
	{
	    log_file (LOG_DEBUG, "Killing Client: %d pid: %d\n", c_id, c_pid);
	    kill (c_pid, SIGINT);
	    set_nm_client_pid (c_id, 0);
	}
    }
}

/**
 * @brief Shutdown the NodeManager.
 *
 */
static void
nodeManager_shutdown (void)
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
 * @brief nodeManager tells the Prime that it has an error.
 *
 * @param  nm_id : nm_id ID
 * @param  eval : Netmist specific error value
 * @param  plat_err : Error value  (value of errno in Unix)
 */
/*
 * __doc__
 * __doc__  Function : void tell_prime_error(int nm_id, int eval, errno)
 * __doc__  Arguments: int nm ID.
 * __doc__  		   int eval: Netmist specific error value.
 * __doc__  		   int plat_err: Platform specific error value.
 * __doc__  Returns  : void
 * __doc__  Performs : nodeManager tells the Prime that it has an error.
 * __doc__
 */
void
tell_prime_error (int nm_id, enum netmist_child_err eval, int plat_err)
{
    log_file (LOG_ERROR,
	      "%d: Tell prime Netmist error %d Platform error: %d\n",
	      nm_id, eval, plat_err);

    reset_all_prime_comm_buffers ();
    g_mc->m_command = R_NM_FAILED;
    g_mc->m_child_flag = CHILD_STATE_READY;
    g_mc->m_client_number = (int) nm_id;
    g_mc->m_client_eval = eval;
    g_mc->m_client_plat_err = plat_err;

    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

    (void) socket_send (peer_socket,
			(char *) g_emc, sizeof (struct ext_prime_command));
}

/**
 * @brief nodeManager sends SYNC_OK to a client on a direct socket
 *
 * @param socket_id 
 */
static void
nodeManager_send_sync_ok_on_socket (int socket_id)
{
    reset_all_client_comm_buffers ();

    g_cc->c_command = R_SYNC_OK;
    g_cc->magic = fsm_state;

    add_crc32_client_command ();

    log_file (LOG_FSM_VERBOSE, "Sending SYNC_OK to client\n");

    client_int_to_ext (g_cc, g_ecc);

    server_send_on_socket (socket_id, (char *) g_ecc,
			   sizeof (struct ext_client_command));
}

/**
 * @brief nodeManager sends SYNC_OK to a client
 *
 * @param client_id : Send ok to this client.
 */
static void
nodeManager_send_sync_ok (int client_id)
{
    reset_all_client_comm_buffers ();

    g_cc->c_command = R_SYNC_OK;
    g_cc->magic = fsm_state;

    add_crc32_client_command ();

    log_file (LOG_FSM_VERBOSE, "Sending SYNC_OK to client %d\n", client_id);

    client_int_to_ext (g_cc, g_ecc);

    add_crc32_ext_client_command ();

    server_send (client_id, (char *) g_ecc,
		 sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends the Epoch time sync to its child.
 *
 * @param  client_id : child ID 
 */
static void
nodeManager_do_time_sync (int client_id)
{
    char string[40];
    double foobar;
    memset (string, 0, 40);
    reset_all_client_comm_buffers ();

    g_cc->c_command = R_TIME_SYNC;
    /* Because the internal format contains binary data, and is crc checked
     * it is critical that doubles and floats do not have extra bits of 
     * precision that cause the crc check to fail.
     */
    foobar = getepochtime ();
    snprintf (string, 40, "%lf", foobar);
    sscanf (string, "%lf", &foobar);
    g_cc->c_prime_eclock_time = foobar;
    g_cc->magic = fsm_state;
    add_crc32_client_command ();

    log_file (LOG_FSM_VERBOSE, "Sending time sync to client: %d\n",
	      client_id);

    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    server_send (client_id, (char *) g_ecc,
		 sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends the GO message to its child.
 *
 * @param  client_id : child ID 
 */
static void
nodeManager_send_epoch (int client_id)
{
    /*
     * We do not zero out g_cc structure since we want to relay all 
     * the information we received from Prime to clients
     */

    memset (g_ecc, 0, sizeof (struct ext_client_command));

    log_file (LOG_FSM_VERBOSE, "Sending Epoch to Client: %d\n", client_id);

    client_int_to_ext (g_cc, g_ecc);

    server_send (client_id, (char *) g_ecc,
		 sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager checks the version.
 *
 * @param  version 
 */
static int
nodeManager_version_check (const char *version)
{
    int error = 0;

    if (strncmp (version, git_version, VERS_WIDTH) != 0)
    {
	log_file (LOG_ERROR, "Version check failed. Expected: %s, Got: %s\n",
		  git_version, version);
	error = 1;
    }
    else
    {
	log_file (LOG_DEBUG, "Version check successful: %s\n", git_version);
    }

    return error;
}

/**
 * @brief NodeManager sends the GO message to its child.
 *
 * @param  index : index for child to send the GO.
 */
static void
nodeManager_send_go (int index)
{
    int files, dirs;
    int client_id;
    char *workdir, *workload_name;

    client_id = nm_c_ids[index];

    files = client_obj[index].files_per_dir;
    dirs = client_obj[index].dir_count;
    /*
     * We do not zero out g_cc structure since we want to relay all 
     * the information we received from Prime to clients
     *
     * Only *per client* information we now add is files/dir, dir, file size
     */

    memset (g_ecc, 0, sizeof (struct ext_client_command));


    /* Change the files and directories for this particular client */

    /* This is all dead-code.  YAML values over-ride these */
    /* These are passed in the import/export of the workloads. */

    g_cc->c_client_files = files;
    g_cc->c_client_dirs = dirs;
    g_cc->c_files_per_dir = files;
    g_cc->c_dir_count = dirs;

    /* These are passed in the import/export of the workloads. */
    /* This is all dead-code.  YAML values over-ride these */

    g_cc->c_pdsm_mode = cmdline.pdsm_mode;
    g_cc->c_pdsm_interval = cmdline.pdsm_interval;

    /* Tell the clients how many workloads are active */
    g_cc->c_workload_count = cmdline.workload_count;
    log_file (LOG_DEBUG, "Sending workload_count %d to client %d\n",
	      cmdline.workload_count, client_id);

    /* Tell the clients the path to the psdm file */
    strcpy (g_cc->c_unix_pdsm_file, cmdline.unix_pdsm_file);
    strcpy (g_cc->c_unix_pdsm_control_file, cmdline.unix_pdsm_control_file);
    strcpy (g_cc->c_windows_pdsm_file, cmdline.windows_pdsm_file);
    strcpy (g_cc->c_windows_pdsm_control_file,
	    cmdline.windows_pdsm_control_file);

    /* Turn on heartbeats from all clients */
    g_cc->c_heartbeat = 1;

    workdir = client_obj[index].client_workdir;
    workload_name = client_obj[index].client_workload_name;

    /* Check if workload is present in the database. 
     * If it doesn't elect this client to do op_validate()
     */
    if (check_and_add_workloads (workload_name, workdir) ==
	WORKLOAD_NOT_FOUND)
    {
	log_file (LOG_DEBUG, "Workload was NOT found\n");
	if (g_cc->c_do_validate == 1)	/* If enabled by the prime */
	{
	    g_cc->c_do_validate = 1;	/* then enable for the first child on this node */
	    log_file (LOG_DEBUG, "Client %d will do op validation\n",
		      client_id);
	}
    }
    else
    {
	log_file (LOG_DEBUG, "Workload WAS found\n");
	g_cc->c_do_validate = 0;
	log_file (LOG_DEBUG, "Client %d will *not* do op validation\n",
		  client_id);
    }

    log_file (LOG_FSM_VERBOSE, "Sending GO to Client: %d\n", client_id);

    g_cc->magic = fsm_state;
    add_crc32_client_command ();
    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    server_send (client_id, (char *) g_ecc,
		 sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager initializing the logging functionality.
 *
 */
static void
nodeManager_initialize_logging (void)
{
    char localtracename[MAXNAME];
    char logrole[MAXNAME];

    /*
     * Override input argument to start logging
     */
    cmdline.log_flag = 1;

    if (cmdline.Uflag)
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s\\netmist_NM%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s\\netmist_trace_NM%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s/netmist_NM%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s/netmist_trace_NM%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "csv");
#endif
    }
    else
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "\\tmp\\netmist_NM%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "\\tmp\\netmist_trace_C%d.%s", cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "/tmp/netmist_NM%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "/tmp/netmist_trace_NM%d.%s", cmdline.client_id, "csv");
#endif
    }
    if (cmdline.WUflag)
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s\\netmist_NM%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s\\netmist_trace_NM%d.%s", cmdline.client_windows_log_dir,
		  cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "%s/netmist_NM%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "%s/netmist_trace_NM%d.%s", &cmdline.client_log_dir[0],
		  cmdline.client_id, "csv");
#endif
    }
    else
    {
#if defined(WIN32)
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "\\tmp\\netmist_NM%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "\\tmp\\netmist_trace_C%d.%s", cmdline.client_id, "csv");
#else
	snprintf (cmdline.log_filename, sizeof (cmdline.log_filename),
		  "/tmp/netmist_NM%d.%s", cmdline.client_id, "log");
	snprintf (localtracename, sizeof (localtracename),
		  "/tmp/netmist_trace_NM%d.%s", cmdline.client_id, "csv");
#endif
    }

    /*
     * Since nodeManagers run on remote machines, we need logging!
     */
    if (cmdline.log_flag)
    {
	internal_cfg.u.nodeManager_cfg.log_handle =
	    init_log (cmdline.log_filename, "w");
	if (internal_cfg.u.nodeManager_cfg.log_handle == NULL)
	{
	    perror ("nodeManager: Failed to initialize logging");
	    exit (1);
	}
    }

    (void) snprintf (logrole, sizeof (logrole), "%s %d", "nodeManager",
		     cmdline.client_id);

    set_log_role (logrole);
    set_log_level (cmdline.tracelevel);
    set_timestamp_in_log_stdout ();
    set_timestamp_in_log_file ();
}

/**
 * @brief NodeManager validating command line arguments. Not used at this time.
 *
 */
static void
nodeManager_validate_input_args (void)
{
}

/**
 * @brief NodeManager spawns all of its children.
 *
 */
static void
nodeManager_spawn_clients (void)
{
#define MAX_ARGS 150
#define TEMP_BUFFER_SIZE 500
    char *margv[MAX_ARGS];
    char temp_buffer[TEMP_BUFFER_SIZE];
    char temp_name_buf[MAXNAME + 1];
    int arg_index = 0;
    int length;
    int client_id;
#if !defined(WIN32)
    int x;
#endif
    int value;
    char *value_c;
    int i, j;

    if ((nm_total_clients >= (max_proc - 1)))
    {
	log_file (LOG_ERROR,
		  "Spawning client failed. MAX_CHILD exceeded. Max = %d. Try more client nodes.\n",
		  max_proc);
	exit (1);
    }
    /* 
     * Check to see if there are sufficient file descriptors for all of the sockets 
     * that the NodeManager will need.
     *
     * 1 for control, 1 for keep alive send, 1 for keep alive recv
     */
    if ((nm_total_clients * 3) > (max_open - 4)) 
    {
	log_file (LOG_ERROR,
		  "Spawning client failed. MAX_OPEN exceeded. Max = %d. Try increasing max open files. \n",
		  max_open);
	exit (1);
    }
    for (i = 0; i < nm_total_clients; i++)
    {
	/*
	 * Create arguments first
	 */
	client_id = g_nmc->nm_children_ids[i];
	log_file (LOG_EXEC_VERBOSE, "Spawning, client %d\n", client_id);

	/* Process name */
	my_strncpy (temp_buffer, cmdline.execdir, TEMP_BUFFER_SIZE);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -C <vfs plugin> */
	if (cmdline.vfs_so[0] != '\0')
	{
	    margv[arg_index] = strdup ("-C");
	    if (margv[arg_index] == NULL)
	    {
		log_file (LOG_ERROR,
			  "Out of memory constructing command line\n");
		exit (1);
	    }
	    arg_index++;
	    margv[arg_index] = strdup (cmdline.vfs_so);
	    if (margv[arg_index] == NULL)
	    {
		log_file (LOG_ERROR,
			  "Out of memory constructing command line\n");
		exit (1);
	    }
	    arg_index++;

	    if (cmdline.vfs_arg[0] != '\0')
	    {
		margv[arg_index] = strdup ("-+X");
		if (margv[arg_index] == NULL)
		{
		    log_file (LOG_ERROR,
			      "Out of memory constructing command line\n");
		    exit (1);
		}
		arg_index++;
		margv[arg_index] = strdup (cmdline.vfs_arg);
		if (margv[arg_index] == NULL)
		{
		    log_file (LOG_ERROR,
			      "Out of memory constructing command line\n");
		    exit (1);
		}
		arg_index++;
	    }
	}

	/* -2 <trace level> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-2 %u",
			 cmdline.tracelevel);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -9 <keepalive period> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-9 %u",
			 cmdline.keepalive);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -x <skip init> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-x %u",
			 cmdline.skip_init);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -m 3 */
	my_strncpy (temp_buffer, "-m 3", TEMP_BUFFER_SIZE);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -n <client id> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-n %d", client_id);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -t <runtime> */
	value = g_nmc->nm_runtime;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-t %d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -p <client workdir> */
	value_c = client_obj[i].client_workdir;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-p %s", value_c);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -e <client execdir> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-e %s",
			 cmdline.execdir);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -M <nodeManager name> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-M %s",
			 nodeManager_localname);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -U <client logdir> */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-U %s",
			 cmdline.client_log_dir);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -P <node Manager listen port> */
	value = nodeManager_port;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-P %d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -W <workload name> */
	value_c = client_obj[i].client_workload_name;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-W %s", value_c);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -y <ipv6 enable> */
	value = g_nmc->nm_ipv6_enable;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-y %d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -A Password */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-A %s",
			 cmdline.password);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -Z UserDomain */
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-Z %s",
			 cmdline.usr_dom);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

	/* -+m <pdsm_mode> */
	value = g_nmc->nm_pdsm_mode;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+m");
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	log_file (LOG_DEBUG,
		  "Spawning, client %d with -+m %d for pdsm_mode\n",
		  client_id, value);

	/* -+i <pdsm_interval> */
	value = g_nmc->nm_pdsm_interval;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+i");
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	log_file (LOG_DEBUG,
		  "Spawning, client %d with -+m %d for pdsm_interval\n",
		  client_id, value);

	/* -+w <workload_count> Let everyone know the active workload count */
	value = g_nmc->nm_workload_count;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+w");
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%d", value);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;
	log_file (LOG_DEBUG,
		  "Spawning, client %d with %d for workload_count\n",
		  client_id, value);

	if (strcmp (cmdline.unix_pdsm_file, "_") != 0)
	{
	    /* -+p <pdsm_file_name> Set the pdsm file name */
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+p");
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%s",
			     cmdline.unix_pdsm_file);
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    log_file (LOG_DEBUG,
		      "Spawn: Setting pdsm file name: in command line args %s\n",
		      temp_buffer);
	}

	if (strcmp (cmdline.unix_pdsm_control_file, "_") != 0)
	{
	    /* -+C <pdsm_control_file_name> Set the pdsm remote control file name */
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+C");
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%s",
			     cmdline.unix_pdsm_control_file);
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    log_file (LOG_DEBUG,
		      "Spawn: Setting pdsm remote control file name: in command line args %s\n",
		      temp_buffer);
	}

	if (strcmp (cmdline.windows_pdsm_file, "_") != 0)
	{
	    /* -+j <pdsm_file_name> Set the windows pdsm file name */
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+j");
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    /* Flip all backslashes to forward for transport, will get flipped back at the other end */
	    flip_b2f_slashes (temp_name_buf, cmdline.windows_pdsm_file);
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%s",
			     temp_name_buf);
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    log_file (LOG_DEBUG,
		      "Spawn: Setting windows pdsm file name: in command line args %s\n",
		      temp_buffer);
	}

	if (strcmp (cmdline.windows_pdsm_control_file, "_") != 0)
	{
	    /* -+k <windows_pdsm_control_file_name> Set the windows pdsm remote control file name */
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-+k");
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    /* Flip all backslashes to forward for transport, will get flipped back at the other end */
	    flip_b2f_slashes (temp_name_buf,
			      cmdline.windows_pdsm_control_file);
	    (void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "%s",
			     temp_name_buf);
	    length = (int) strlen (temp_buffer) + 1;
	    margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	    my_strncpy (margv[arg_index], temp_buffer, length);
	    arg_index++;
	    log_file (LOG_DEBUG,
		      "Spawn: Setting windows pdsm remote control file name: in command line args %s\n",
		      temp_buffer);
	}

	/* -1 <client windows logdir> */

	/* Flip the back slashes to forward, they will get flipped back at the getop parsing.
	 * This is needed so that ssh or any shell, will not drop the back slashes as part of 
	 * command line argument handling.       
	 */
	flip_b2f_slashes (temp_name_buf, cmdline.client_windows_log_dir);
	(void) snprintf (temp_buffer, TEMP_BUFFER_SIZE, "-1 %s ",
			 temp_name_buf);
	length = (int) strlen (temp_buffer) + 1;
	margv[arg_index] = (char *) my_malloc (sizeof (char) * length);
	my_strncpy (margv[arg_index], temp_buffer, length);
	arg_index++;

#if defined(WIN32)
	log_file (LOG_DEBUG, "Windows log %s\n", temp_name_buf);
#endif

	margv[arg_index] = NULL;

#if defined(WIN32)
	win_exec (margv, arg_index);
	for (j = 0; j < arg_index; j++)
	{
	    free (margv[j]);
	}

	arg_index = 0;
#else
	x = fork ();

	/*
	 * This is a child process 
	 */
	if (x == 0)
	{

	    execv (margv[0], margv);


	    /* It should not come here */
	    log_file (LOG_ERROR, "Failed to spawn client %d\n", client_id);
	    /* XXX abort and let parent know */
	    exit (1);
	}
	else
	{
	    /*
	     * Free up memory and re-initialize for the next spawn
	     */
	    for (j = 0; j < arg_index; j++)
	    {
		free (margv[j]);
	    }

	    arg_index = 0;
	}
#endif
    }
}

/**
 * @brief NodeManager initializes its data structures for keeping track
 *        of its children from the  communication with the prime.
 *
 * @param  mc : Struct prime_command. Tells the nodemanager all of the
 *              things it needs to handle.
 */
static int
initialize_runtime_structures (struct prime_command *mc)
{
    int success;
    int i;

    /*
     * Now since we have received all the information from the 
     * prime about all the clients this instance of nodeManager
     * needs to spawn, parse nmc and populate the nodeManager data
     * structure
     */
    success = nm_add_nodeManager_object (nodeManager_localname,
					 mc->m_client_number,
					 mc->m_child_pid,
					 mc->m_child_port,
					 nodeManager_listen_socket,
					 peer_socket,
					 g_nmc->nm_num_of_children,
					 g_nmc->nm_children_ids);

    if (!success)
    {
	log_file (LOG_ERROR, "Failed to add nodeManager object\n");
	fsm_state = NODEMANAGER_FSM_ERROR_STATE;
	return 1;
    }

    nm_total_clients = g_nmc->nm_num_of_children;

    nm_c_ids = (int *) my_malloc (sizeof (int) * nm_total_clients);

    if (nm_c_ids == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate client ids\n");
	fsm_state = NODEMANAGER_FSM_ERROR_STATE;
	return 1;
    }

    /*
     * Now allocate client data structure
     */

    client_obj =
	(struct nm_client_object *)
	my_malloc (sizeof (struct nm_client_object) * nm_total_clients);
    if (client_obj == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate client_obj\n");
	fsm_state = NODEMANAGER_FSM_ERROR_STATE;
	return 1;
    }

    /*
     * Populate respective client_obj structure
     */

    for (i = 0; i < nm_total_clients; i++)
    {
	nm_c_ids[i] = g_nmc->nm_children_ids[i];
	client_obj[i].client_id = g_nmc->nm_children_ids[i];
	my_strncpy (client_obj[i].client_log_path,
		    cmdline.client_log_dir, MAXNAME);
	my_strncpy (client_obj[i].client_windows_log_path,
		    cmdline.client_windows_log_dir, MAXNAME);
	my_strncpy (client_obj[i].client_workload_name,
		    g_nmc->nm_workload_name[i], MAXWLNAME);
	my_strncpy (client_obj[i].client_workdir,
		    g_nmc->nm_workdir[i], MAXNAME);
	client_obj[i].op_rate = g_nmc->nm_my_oprate[i];

	/* More un-used dead code.  These are now passwed in the workload import/export */
	client_obj[i].dir_count = g_nmc->nm_client_dirs[i];
	client_obj[i].files_per_dir = g_nmc->nm_client_files[i];
	client_obj[i].file_size = g_nmc->nm_file_size[i];
	/* More un-used dead code.  These are now passwed in the workload import/export */

	//XXX TBD needs to add more information to g_* structures
	client_obj[i].memsize = 0;
    }

    log_file (LOG_CONFIG, "exec: %s\n", cmdline.execdir);
    log_file (LOG_DEBUG, "Unix log path: %s\n", cmdline.client_log_dir);
    log_file (LOG_DEBUG, "Windows log dir: %s\n",
	      cmdline.client_windows_log_dir);

    return 0;
}

#if defined(WIN32)
extern int parent;
int Windows_authenticate_and_login (void);
#endif

/**
 * @brief NodeManager starting its listening thread. 
 *
 */
static void
nodeManager_start_listening_thread (void)
{
    get_my_server_socket (cmdline.client_id,
			  &nodeManager_listen_socket, &nodeManager_port);
}

/**
 * @brief NodeManager comm memory allocation. 
 *
 */
static void
nodeManager_allocate_comm_structures (void)
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
		  "Failed to allocate memory for ext prime command\n");
	exit (1);
    }

    g_nmc =
	(struct nodeManager_command *)
	MALLOC (sizeof (struct nodeManager_command));
    if (g_nmc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for nodeManager command\n");
	exit (1);
    }

    g_enmc =
	(struct ext_nodeManager_command *)
	MALLOC (sizeof (struct ext_nodeManager_command));
    if (g_enmc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for ext nodeManager command\n");
	exit (1);
    }

    g_cc = (struct client_command *) MALLOC (sizeof (struct client_command));
    if (g_cc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for client command\n");
	exit (1);
    }

    g_ecc =
	(struct ext_client_command *)
	MALLOC (sizeof (struct ext_client_command));
    if (g_ecc == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for ext client command\n");
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

    log_file (LOG_EXEC_VERBOSE, "Size of SP2 text segment emulation  = %lu\n",
	      KB_NMEM_PRESSURE);
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
 * @brief NodeManager initializes the logging functionality.
 *
 */
static void
nodeManager_log_state (void)
{
    log_file (LOG_FSM, "**** %s ---> %s ****\n",
	      _fsm_state_names[fsm_prev_state], _fsm_state_names[fsm_state]);
}

/**
 * @brief NodeManager performs all of its work by stepping through its
 *        FSM.
 *
 */
void
do_all_nodeManager_work (void)
{
    abort_ifnot_nodeManager ();

    int whileloop = 1;
    int error = 0, ret = 0;
#if defined(FORK_ME)
    int x = really = 0;
#endif
    int i;
    int Windows_auth_failed = 0;

    fsm_next_state = NODEMANAGER_FSM_FORK;
    fsm_state = NODEMANAGER_FSM_FORK;
    fsm_prev_state = NODEMANAGER_FSM_FORK;

    int *c_sockets = NULL, single_socket[1];
    int c_id = 0;
#if defined(WIN32)
    int xx = 0;
#endif
    extern int max_proc;
    extern int max_open;

#if defined(RLIMIT_NPROC)
    struct rlimit rlimit_struct;
#endif

    double epoch_my_time, time_skew;
    double start_time = 0.0, end_time = 0.0, total_rrt_time =
	0.0, avg_rrt_time = 0.0;

    /* 
     * Determine the max number of children, and the max number of open files.
     */
#if !defined(WIN32)
#if defined(RLIMIT_NPROC)
    getrlimit (RLIMIT_NPROC, &rlimit_struct);
    max_proc = rlimit_struct.rlim_max;
    if (max_proc == (int) RLIM_INFINITY)
    {
	max_proc = MAXCLIENTS_PER_NM;
    }
#endif
    if (max_proc <= 0)
	max_proc = sysconf (_SC_CHILD_MAX);

    if (max_proc <= 0)
    {
#if defined(CHILD_MAX)
	max_proc = CHILD_MAX;
#endif
    }
    if (max_proc <= 0)
    {
	log_file (LOG_ERROR,
		  "Unable to obtain MAX_CHILD.. Max_child = %d Fatal\n",
		  max_proc);
	;
	exit (1);
    }
    max_open = sysconf (_SC_OPEN_MAX);

#else
    /* WINDOWS HERE: 
     * There is no hard-set limit for max open local files, and no max 
     * processes on Windows. MSDN blog indicates a limit on network open 
     * files to be 16384 per session.  We will default to WIN32_MAX_OPEN
     * and let the user over-ride by using cache limit option (-J).
     */
    max_open = WIN32_MAX_OPEN_SFS2020;
    max_proc = WIN32_MAX_PROC;
#endif

#if defined(WIN32)
    /* This is the directory where we look for "netmist_pretest.cmd" file */
    char *pretest_dir = GetParentPath (cmdline.execdir);
#endif

    /*
     * Typical FSM infinite loop
     */

    while (whileloop)
    {
	switch (fsm_state)
	{
	case NODEMANAGER_FSM_FORK:
	    /* In the past we had the prime start the nodemanager on the remote node
	     * and the first thing the nodemanager did was to fork and have the parent
	     * exit so that the prime's ssh would return, and it could then go into its
	     * listening loop.
	     * However, this doesn't work on Windows ssh as all procs are terminated
	     * when the parent thread returns regardless of how hard one tries to make
	     * it act like Unix. (So much for a quality port of Openssh)
	     * The solution is to use ssh's -fn flags to put the job in background on the
	     * prime node and so the prime gets control and drops into its listen loop.
	     * Thus, we no longer need to have the benchmark do the fork, we can let
	     * the ssh implementation handle this by using its -fn flags.
	     * Since this also works for Unix-to-Unix, we can use this as the solution
	     * for all platforms that use ssh.
	     */

/* If defined FORKME -----------------------------------------------------*/
#if defined(FORK_ME)
	  again:
#if defined(WIN32)
	    child_num++;

	    if (parent || 1)	/* Current in network user context */
	    {
		xx = Windows_authenticate_and_login ();	/* Move to real user */
		if (xx != 0)
		{
		    Windows_auth_failed = 1;
		    /* logging hasn't been initialized yet */
		    /*log_file (LOG_ERROR, "Authenticate failed\n"); */

		    log_stdout (LOG_ERROR, "Authentication failed.\n");

		    /* Let the user continue, as we did in SP2. It may
		     * run fine anyway. It would depend on the user's Windows
		     * registry settings.
		     */
		}
	    }

#endif
	    x = fork ();	/* Become a stand alone child. */
	    if (x < 0)
	    {
		if (errno == EAGAIN)
		{
		    really++;
		    if (really > 10)
		    {		/* Try ten times with sleep between */
			log_file (LOG_ERROR,
				  "Parent unable to fork !!!. Giving up.\n");
			exit (EAGAIN);
		    }

		    sleep (2);	/* Sleep a bit, and try again */
		    goto again;
		}
		log_stdout (LOG_ERROR, "Parent unable to fork. Errno %d.\n",
			    errno);
		exit (-1);
	    }

	    if (x != 0)
	    {			/* I am the parent process */
#if defined(WIN32)
		/* stop network */
		win32_close ();
#endif
		exit (0);	/* Parent exits the system. Let rsh move on */
	    }

	    /* Child also closes its handles so prime/shell can exit */
#if !defined(WIN32)
	    fclose (stdin);
	    fclose (stdout);
	    fclose (stderr);
#endif

#else
#if defined(WIN32)
	    xx = Windows_authenticate_and_login ();	/* Move to real user */
	    if (xx != 0)
	    {
		Windows_auth_failed = 1;
		/* logging hasn't been initialized yet */
		/*log_file (LOG_ERROR, "Authenticate failed\n"); */

		log_stdout (LOG_ERROR, "Authentication failed.\n");

		/* Let the user continue, as we did in SP2. It may
		 * run fine anyway. It would depend on the user's Windows
		 * registry settings.
		 */
	    }
#endif
/* End of IF defined FORK_ME ----------------------------------------*/
#endif
	    /* Initialize log handle */
	    /*
	     * We got a kill flag from the Prime
	     * This will happen only in "kill" processes that Prime spawns 
	     * to shutdown the benchmark
	     */
	    if (cmdline.kflag)
	    {
		log_stdout (LOG_EXEC, "Sending kill to NodeManager: %d \n",
			    cmdline.kill_pid);
		kill (cmdline.kill_pid, SIGINT);
		exit (0);

	    }

	    nodeManager_initialize_logging ();

#if defined(WIN32)
	    if (Windows_auth_failed == 1)
		log_file (LOG_ERROR, "Windows authentication failed.\n");
	    else
		log_file (LOG_EXEC, "Windows authentication succeeded.\n");
#endif
	    log_file (LOG_DEBUG, "do_all_node_manager: workload_count %d\n",
		      cmdline.workload_count);
	    log_file (LOG_DEBUG, "do_all_node_manager: unix_pdsm_file %s\n",
		      cmdline.unix_pdsm_file);
	    log_file (LOG_DEBUG,
		      "do_all_node_manager: windows_pdsm_file %s\n",
		      cmdline.windows_pdsm_file);
	    log_file (LOG_DEBUG,
		      "do_all_node_manager: unix_pdsm_control_file %s\n",
		      cmdline.unix_pdsm_control_file);
	    log_file (LOG_DEBUG,
		      "do_all_node_manager: windows pdsm_control_file %s\n",
		      cmdline.windows_pdsm_control_file);
	    log_file (LOG_DEBUG, "do_all_node_manager: pdsm_mode %d\n",
		      cmdline.pdsm_mode);
	    log_file (LOG_DEBUG, "do_all_node_manager: pdsm_interval %d\n",
		      cmdline.pdsm_interval);

	    /* Set global num_work_obj so that all functions called by the 
	     * nodeManager will know how many active workloads are present. 
	     */
	    num_work_obj = cmdline.workload_count;

	    log_file (LOG_DEBUG, "NM num_work_obj =  %d\n", num_work_obj);

	    /* Now we can deliver that log message */
	    if (Windows_auth_failed == 1)
		log_file (LOG_ERROR, "Authenticate failed\n");

	    fsm_prev_state = NODEMANAGER_FSM_FORK;

#ifdef NM_ATTACH_DEBUGGER
	    fsm_state = NODEMANAGER_FSM_DEBUG_STATE;
#else
	    fsm_state = NODEMANAGER_FSM_START;
#endif
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_DEBUG_STATE:
	    sleep (30);
	    fsm_prev_state = NODEMANAGER_FSM_DEBUG_STATE;
	    fsm_state = NODEMANAGER_FSM_START;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_START:

	    nodeManager_validate_input_args ();
	    nodeManager_start_listening_thread ();
	    nodeManager_allocate_comm_structures ();

	    error = 0;
	    while (1)
	    {
		ret = my_gethostname (nodeManager_localname, MAXHNAME);
		if (ret == 0)
		{
		    error = 0;
		    break;
		}
		log_file (LOG_ERROR, "my_gethostname failed: %s\n",
			  netmist_strerror (netmist_get_error ()));
		error++;
		if (error > 10)
		{
		    break;
		}
		sleep (5);
	    }

	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		nodeManager_log_state ();
		continue;
	    }

	    fsm_prev_state = NODEMANAGER_FSM_START;
	    fsm_state = NODEMANAGER_FSM_PRIME_CONNECT;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_PRIME_CONNECT:
	    sleep ((netmist_rand () % 5) + 2);
	    peer_socket = connect_peer_server (cmdline.prime_name,
					       cmdline.prime_p);

	    fsm_prev_state = NODEMANAGER_FSM_PRIME_CONNECT;
	    fsm_state = NODEMANAGER_FSM_SYNC_OK_STATE;
	    fsm_next_state = NODEMANAGER_FSM_PRIME_JOIN;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_PRIME_JOIN:
	    reset_all_prime_comm_buffers ();
	    /* send join message */
	    g_mc->m_command = R_NODEMANAGER_JOIN;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends join. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_PRIME_JOIN;
	    fsm_state = NODEMANAGER_FSM_VERSION_CHECK;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_VERSION_CHECK:
	    reset_all_client_comm_buffers ();
	    socket_receive (peer_socket, (char *) g_ecc,
			    sizeof (struct ext_client_command));
	    client_ext_to_int (g_cc, g_ecc);

	    if (g_cc->c_command != R_VERSION_CHECK)
	    {
		log_file (LOG_ERROR, "Unexpected command from Prime. "
			  "Expected VERSION_CHECK, got %d\n",
			  g_cc->c_command);
		exit (0);
	    }

	    fsm_prev_state = NODEMANAGER_FSM_VERSION_CHECK;
	    fsm_state = NODEMANAGER_FSM_VERSION_CHECK_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_VERSION_CHECK_DONE:

	    error = nodeManager_version_check (g_cc->c_client_version);

	    reset_all_prime_comm_buffers ();
	    /* send VERSION_CHECK_DONE message */

	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    if (error)
	    {
		g_mc->m_command = R_NM_FAILED;
	    }
	    else
	    {
		g_mc->m_command = R_VERSION_CHECK_DONE;
	    }

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE,
		      "NodeManager: %d sends VERSION_CHECK_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_VERSION_CHECK_DONE;

	    if (error)
	    {
		fsm_state = NODEMANAGER_FSM_ERROR_STATE;
	    }
	    else
	    {
		fsm_state = NODEMANAGER_FSM_GET_TOKEN_CONFIG;
	    }
	    nodeManager_log_state ();
	    break;


	case NODEMANAGER_FSM_GET_TOKEN_CONFIG:
	    reset_all_nodeManager_comm_buffers ();
	    socket_receive (peer_socket, (char *) g_enmc,
			    sizeof (struct ext_nodeManager_command));
	    nodeManager_ext_to_int (g_enmc, g_nmc, num_work_obj);

	    error = initialize_runtime_structures (g_mc);
	    if (error)
	    {
		continue;
	    }

	    fsm_prev_state = NODEMANAGER_FSM_GET_TOKEN_CONFIG;
	    fsm_state = NODEMANAGER_FSM_SPAWN_CLIENTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_SPAWN_CLIENTS:
	    nodeManager_spawn_clients ();

	    /* We no longer need g_nmc and g_emnc structures
	     * after this so free them to save memory footprint
	     */
	    free (g_nmc);
	    free (g_enmc);
	    g_nmc = NULL;
	    g_enmc = NULL;

	    fsm_prev_state = NODEMANAGER_FSM_SPAWN_CLIENTS;
	    fsm_state = NODEMANAGER_WAIT_FOR_CLIENTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_WAIT_FOR_CLIENTS:
	    /* Finish comm linkage */
	    c_sockets = server_wait_for_clients (nodeManager_listen_socket,
						 nm_total_clients);

	    fsm_prev_state = NODEMANAGER_WAIT_FOR_CLIENTS;
	    fsm_state = NODEMANAGER_JOIN_CLIENTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_JOIN_CLIENTS:
	    for (i = 0; i < nm_total_clients; i++)
	    {
		/*
		 * Since clients have not sent 'Join' yet,
		 * we do not have relationship established between
		 * client_id <-> socket number. So, we need to send
		 * SYNC OK message on the socket directly in this state
		 * Starting next state, this is not a problem!
		 */
		nodeManager_send_sync_ok_on_socket (c_sockets[i]);
		single_socket[0] = c_sockets[i];
		error = server_select (peer_socket, single_socket,
				       1, 0, R_CHILD_JOIN);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    fsm_prev_state = NODEMANAGER_JOIN_CLIENTS;
	    fsm_state = NODEMANAGER_KEEPALIVE_WAIT;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_KEEPALIVE_WAIT:
	    c_keepalive_sockets =
		server_wait_for_clients (nodeManager_listen_socket,
					 nm_total_clients);

	    peer_keepalive_socket = connect_peer_server (cmdline.prime_name,
							 cmdline.prime_p);

	    error =
		netmist_start_threads (nodeManager_keepalive_thread_handler,
				       nodeManager_listen_thread_handler);
	    if (error)
	    {
		log_file (LOG_ERROR, "Failed to create threads");
	    }
	    fsm_prev_state = NODEMANAGER_KEEPALIVE_WAIT;
	    fsm_state = NODEMANAGER_FSM_PRIME_READY;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_PRIME_READY:
	    reset_all_prime_comm_buffers ();

	    /* send ready message */
	    g_mc->m_command = R_NODEMANAGER_ALL_READY;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends ready. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_PRIME_READY;
	    fsm_state = NODEMANAGER_FSM_GO;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_GO:
	    wait_for_client_message (R_GO, "GO");

	    /* Got 'GO' from Prime, send it to clients */
	    for (i = 0; i < nm_total_clients; i++)
	    {
		nodeManager_send_go (i);
		nap (10);
	    }

	    /*
	     * Free up memory consumed by the op_validate logic
	     */
	    cleanup_workloads ();

	    /*
	     * Free client_obj structure
	     */
	    free (client_obj);
	    client_obj = NULL;

	    fsm_prev_state = NODEMANAGER_FSM_GO;
	    fsm_state = NODEMANAGER_FSM_GO_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_GO_DONE:
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_send_sync_ok (c_id);
		single_socket[0] = (int) get_nm_client_socket (c_id);

		error = server_select (peer_socket, single_socket,
				       1, 0, R_GO_DONE);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    reset_all_prime_comm_buffers ();
	    /* send GO_DONE message */
	    g_mc->m_command = R_GO_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE,
		      "NodeManager: %d sends GO_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_GO_DONE;
	    fsm_state = NODEMANAGER_FSM_INIT_PHASE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_INIT_PHASE:
	    NM_phase = 0;	/* Init phase */
	    wait_for_client_message (INIT_PHASE, "INIT");

	    /* Got 'INIT' from Prime, forward it to clients */
	    log_file (LOG_EXEC, "Sending INIT to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		log_file (LOG_DEBUG, "Sending INIT to Client: %d\n", c_id);
		server_send (c_id, (char *) g_ecc,
			     sizeof (struct ext_client_command));
	    }

	    fsm_prev_state = NODEMANAGER_FSM_INIT_PHASE;
	    fsm_state = NODEMANAGER_FSM_INIT_PHASE_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_INIT_PHASE_DONE:
	    error = server_select (peer_socket, c_sockets,
				   nm_total_clients, 0, INIT_PHASE_DONE);
	    if (error)
	    {
		log_file (LOG_ERROR, "Select received error\n");
		fsm_prev_state = fsm_state;
		fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		nodeManager_log_state ();
		continue;
	    }

	    reset_all_prime_comm_buffers ();
	    /* send INIT DONE message */
	    g_mc->m_command = INIT_PHASE_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends INIT_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_INIT_PHASE_DONE;
	    fsm_state = NODEMANAGER_FSM_ASK_INIT_RESULTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_ASK_INIT_RESULTS:
	    wait_for_client_message (ASK_INIT_RESULTS, "INIT results");

	    /* Got 'Ask INIT results' from Prime, send it to clients as is */
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		log_file (LOG_DEBUG, "Asking results from client: %d\n",
			  c_id);
		server_send (c_id, (char *) g_ecc,
			     sizeof (struct ext_client_command));
	    }

	    fsm_prev_state = NODEMANAGER_FSM_ASK_INIT_RESULTS;
	    fsm_state = NODEMANAGER_FSM_GET_INIT_RESULTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_GET_INIT_RESULTS:
	    log_file (LOG_EXEC, "Sending sync OK to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_send_sync_ok (c_id);
		single_socket[0] = (int) get_nm_client_socket (c_id);

		/*
		 * nodeManager waits for results from clients and then
		 * forwards them to prime
		 */
		error = server_select (peer_socket,
				       single_socket, 1, 0,
				       R_SENDING_RESULTS);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    fsm_prev_state = NODEMANAGER_FSM_GET_INIT_RESULTS;
	    fsm_state = NODEMANAGER_FSM_TIME_SYNC;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_TIME_SYNC:
	    wait_for_client_message (R_TIME_SYNC, "Time Sync");

	    epoch_my_time = getepochtime ();
	    time_skew = g_cc->c_prime_eclock_time - epoch_my_time;

	    log_file (LOG_FSM_VERBOSE, "time skew: %lf\n", time_skew);

	    start_time = getepochtime ();

	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_do_time_sync (c_id);
	    }

	    fsm_prev_state = NODEMANAGER_FSM_TIME_SYNC;
	    fsm_state = NODEMANAGER_FSM_TIME_SYNC_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_TIME_SYNC_DONE:
	    log_file (LOG_EXEC, "Sending sync OK to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_send_sync_ok (c_id);
		single_socket[0] = (int) get_nm_client_socket (c_id);

		error = server_select (peer_socket, single_socket,
				       1, 0, R_TIME_SYNC_DONE);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    end_time = getepochtime ();

	    total_rrt_time = end_time - start_time;
	    avg_rrt_time = total_rrt_time / nm_total_clients;
	    log_file (LOG_FSM_VERBOSE, "Total RRT time %lf\n",
		      total_rrt_time);
	    log_file (LOG_FSM_VERBOSE, "Avg RRT time %lf\n", avg_rrt_time);

	    reset_all_prime_comm_buffers ();
	    /* send TIME SYNC DONE message */
	    g_mc->m_command = R_TIME_SYNC_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends TIME_SYNC_DONE\n",
		      cmdline.client_id);

	    fsm_prev_state = NODEMANAGER_FSM_TIME_SYNC_DONE;
	    fsm_state = NODEMANAGER_FSM_EPOCH;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_EPOCH:
	    wait_for_client_message (R_EPOCH, "Epoch");

	    /* Got 'Epoch' from Prime, send it to clients */
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_send_epoch (c_id);
	    }

	    fsm_prev_state = NODEMANAGER_FSM_EPOCH;
	    fsm_state = NODEMANAGER_FSM_EPOCH_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_EPOCH_DONE:
	    log_file (LOG_EXEC, "Sending OK EPOCH-DONE to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		nodeManager_send_sync_ok (c_id);
		single_socket[0] = (int) get_nm_client_socket (c_id);

		error = server_select (peer_socket, single_socket,
				       1, 0, R_EPOCH_DONE);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    reset_all_prime_comm_buffers ();
	    /* send GO DONE message */
	    g_mc->m_command = R_EPOCH_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE,
		      "NodeManager: %d sends EPOCH_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_EPOCH_DONE;
	    fsm_state = NODEMANAGER_FSM_WARMUP_PHASE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_WARMUP_PHASE:
	    NM_phase = 1;	/* Init phase */
	    wait_for_client_message (WARM_PHASE, "Warmup");

	    /* Got 'WARMUP' from Prime, forward it to clients */
	    log_file (LOG_EXEC, "Sending WARMUP to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		log_file (LOG_DEBUG, "Sending WARMUP to client: %d\n", c_id);
		server_send (c_id, (char *) g_ecc,
			     sizeof (struct ext_client_command));
	    }

	    fsm_prev_state = NODEMANAGER_FSM_WARMUP_PHASE;
	    fsm_state = NODEMANAGER_FSM_WARMUP_PHASE_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_WARMUP_PHASE_DONE:
	    error = server_select (peer_socket, c_sockets,
				   nm_total_clients, 0, WARM_PHASE_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		nodeManager_log_state ();
		continue;
	    }

	    reset_all_prime_comm_buffers ();
	    /* send WARMUP/RUN done message */
	    g_mc->m_command = WARM_PHASE_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends WARMUP_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_WARMUP_PHASE_DONE;
	    fsm_state = NODEMANAGER_FSM_RUN_PHASE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_RUN_PHASE:
	    log_file (LOG_EXEC, "Entering RUN phase\n");

	    NM_phase = 2;	/* RUN phase */
	    fsm_prev_state = NODEMANAGER_FSM_RUN_PHASE;
	    fsm_state = NODEMANAGER_FSM_RUN_PHASE_DONE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_RUN_PHASE_DONE:
	    log_file (LOG_EXEC, "Waiting for RUN phase to finish\n");
	    error = server_select (peer_socket, c_sockets,
				   nm_total_clients, 0, RUN_PHASE_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		nodeManager_log_state ();
		continue;
	    }

	    reset_all_prime_comm_buffers ();
	    /* send WARMUP/RUN done message */
	    g_mc->m_command = RUN_PHASE_DONE;
	    g_mc->m_child_port = nodeManager_port;
	    g_mc->m_child_pid = getpid ();
	    g_mc->m_client_number = cmdline.client_id;
	    my_strncpy (g_mc->m_client_name, nodeManager_localname, MAXHNAME);

	    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);

	    socket_send (peer_socket, (char *) g_emc,
			 sizeof (struct ext_prime_command));

	    log_file (LOG_FSM_VERBOSE, "%d sends RUN_DONE. Port %d\n",
		      cmdline.client_id, nodeManager_port);

	    fsm_prev_state = NODEMANAGER_FSM_RUN_PHASE_DONE;
	    fsm_state = NODEMANAGER_FSM_ASK_RESULTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_ASK_RESULTS:
	    wait_for_client_message (ASK_RESULTS, "Ask Results");

	    /* Got 'Ask results' from Prime, send it to clients as is */
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		log_file (LOG_DEBUG, "Asking results from client: %d\n",
			  c_id);
		server_send (c_id, (char *) g_ecc,
			     sizeof (struct ext_client_command));
	    }

	    fsm_prev_state = NODEMANAGER_FSM_ASK_RESULTS;
	    fsm_state = NODEMANAGER_FSM_GET_RESULTS;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_GET_RESULTS:
	    for (i = 0; i < nm_total_clients; i++)
	    {
		nodeManager_send_sync_ok (nm_c_ids[i]);
		single_socket[0] = (int) get_nm_client_socket (nm_c_ids[i]);

		/*
		 * nodeManager waits for results from clients and then
		 * forwards them to prime
		 */
		error =
		    server_select (peer_socket, single_socket, 1, 0,
				   R_SENDING_RESULTS);
		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = NODEMANAGER_FSM_ERROR_STATE;
		    nodeManager_log_state ();
		    break;
		}
	    }

	    if (error)
	    {
		break;
	    }

	    fsm_prev_state = NODEMANAGER_FSM_GET_RESULTS;
	    fsm_state = NODEMANAGER_FSM_COMPLETE_PHASE;
	    nodeManager_log_state ();
	    break;


	case NODEMANAGER_FSM_COMPLETE_PHASE:
	    fsm_prev_state = NODEMANAGER_FSM_COMPLETE_PHASE;
	    fsm_state = NODEMANAGER_FSM_SHUTDOWN_PHASE;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_SHUTDOWN_PHASE:
	    NM_phase = 3;	/* Shutdown phase */
	    wait_for_client_message (SHUTDOWN_PHASE, "shutdown");

	    /* Got 'shutdown' from Prime, send it to clients as is */
	    log_file (LOG_EXEC, "Sending shutdown to clients.\n");
	    for (i = 0; i < nm_total_clients; i++)
	    {
		c_id = nm_c_ids[i];
		log_file (LOG_DEBUG, "Sending shutdown to client: %d\n",
			  c_id);
		server_send (c_id, (char *) g_ecc,
			     sizeof (struct ext_client_command));
	    }

	    nodeManager_shutdown ();

	    fsm_prev_state = NODEMANAGER_FSM_SHUTDOWN_PHASE;
	    fsm_state = NODEMANAGER_FSM_FINISHED;
	    nodeManager_log_state ();
	    break;

	case NODEMANAGER_FSM_SYNC_OK_STATE:
	    wait_for_client_message (R_SYNC_OK, "sync ok");

	    fsm_prev_state = NODEMANAGER_FSM_SYNC_OK_STATE;
	    fsm_state = fsm_next_state;
	    nodeManager_log_state ();
	    break;


	case NODEMANAGER_FSM_FINISHED:
	    exit (0);

	case NODEMANAGER_FSM_ERROR_STATE:
	    log_file (LOG_EXEC, "Killing all child procs\n");

	    nodeManager_send_kill_procs ();
	    nodeManager_shutdown ();

	    fsm_prev_state = fsm_state;
	    fsm_state = NODEMANAGER_FSM_FINISHED;
	    nodeManager_log_state ();

	    while (1)
	    {
		log_file (LOG_EXEC, "Waiting for Prime to send kill\n");
		sleep (20);
	    }

	    break;

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

    int rc =
	socket_receive (peer_socket, (char *) g_ecc,
			sizeof (struct ext_client_command));
    if (rc < (int) sizeof (struct ext_client_command))
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

    log_file (LOG_FSM_VERBOSE, "Epoch = %lf\n", g_cc->c_start_epoch);

    //Validate if the received message is of the same type as command
    if (!validate_client_command ("prime", command))
    {
	//report error back and wait for termination
	R_exit (NETMIST_BAD_CRC, 0);
    }

    if (log_message)
    {
	log_file (LOG_FSM_VERBOSE, "%s received\n", log_message);
    }
}


#if defined(_macos_) || defined(_bsd_) || defined(_aix_)
int
get_nprocs (void)
{
    int value;
    value = sysconf (_SC_NPROCESSORS_ONLN);
    if (value < 0)
	value = 1;
    return (value);
}
#endif
