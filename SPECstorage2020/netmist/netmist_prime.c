/*
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
#endif
#if defined(WIN32)
#include "../win32lib/win32_sub.h"
#endif
#include<time.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>

/****************************************************************************/
/* COMM SECTION */
/****************************************************************************/
/*
 * Communication library for controlling many hosts over the net.
 */
#if !defined(WIN32)
#include <sys/signal.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/poll.h>
#else
#include <signal.h>
#include <io.h>
#include <WinSock2.h>
#include <direct.h>
#endif

#if defined(_bsd_)
#include <signal.h>
#endif

#if defined(_solaris_) || defined(_bsd_)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


/* This is different for the various versions. */
#include <netmist.h>

#include "netmist_version.h"
#include "netmist_copyright.h"
#include "netmist_defines.h"
#include "netmist_utils.h"
#include "netmist_structures.h"
#include "netmist_logger.h"
#include "netmist_fsm.h"
#include "netmist_thread.h"

#include "netmist_vfs.h"

extern int netmist_get_error (void);
extern int netmist_get_sock_error (void);
extern char *netmist_get_sock_errstr (int);
extern void netmist_srand (int seed);
extern int netmist_rand (void);
extern int power_mon (int, char *);
extern void print_results (void);
extern void print_results_in_csv (int);
extern char *netmist_strerror (int);

extern unsigned int crc32_generate (void *data, size_t length);
extern unsigned int crc32_check (void *data, size_t length);

extern int num_work_obj;
extern struct mix_table my_mix_table[];

/*
 * Variables referenced by other files
 */
struct client_object *client_obj;

int num_clients;
int rsize = RSIZE;		/* in kBytes */
unsigned int lic_key;

struct work_object *workloads;
void prime_send_sigint (void);

static char option_buf[4];
static int clients_per_nm;
static int p_num_clients;
static int num_nodeManagers;

static char client_filename[MAXNAME];
static int prime_listen_socket;
static int prime_listen_port;
static double start_time, end_time, total_rrt_time, avg_rrt_time,
    c_start_epoch;
/*
 * Name of the client where the controlling process can use
 * remsh/rsh to start the client test running.
 */
static char remsh[255];

struct token
{
    int optional;		/* Set to 1 to ignore if token is not present */
    char tname[MAX_INPUT_TOKENLEN];	/* String to search for */
    char tvalue[MAX_INPUT_TOKENLEN];	/* Resulting value */
} token_objects[NUM_INPUT_TOKENS] =
{
    {
    0, "Clientname=", ""},
    {
    0, "Username=", ""},
    {
    1, "Password=", ""},
    {
    0, "Workdir=", ""},
    {
    0, "Execpath=", ""},
    {
    0, "Workload=", ""},
    {
    1, "Instances=", ""},
    {
    1, "Launchtype=", ""},
    {
    1, "Logpath=", ""},
    {
    1, "WindowsLogp=", ""}
};


int *nm_keepalive_sockets = NULL;

static struct keepalive_command k_keepalive;
static struct keepalive_command k_send_sigint;
static struct ext_keepalive_command k_ext_keepalive;
static struct ext_keepalive_command k_ext_send_sigint;

static unsigned int keepalive_count = 0;

void prime_send_kill_procs (void);
static void prime_shutdown (void);

/**
 * @brief Close keep-alive sockets.
 */
static void
close_keepalive_sockets (void)
{
    int i;
    log_all (LOG_EXEC, "Closing keepalive sockets\n");
    for (i = 0; i < num_nodeManagers; i++)
    {
	if( (nm_keepalive_sockets == NULL) || (nm_keepalive_sockets[i] == 0))
		return;	
#if defined(WIN32)
	(void) shutdown (nm_keepalive_sockets[i], SD_BOTH);
#else
	(void) shutdown (nm_keepalive_sockets[i], SHUT_RDWR);
#endif
    }
}

/**
 * @brief Function for thread that sends keep-alive messages.
 *
 * @param ptr : Pointer to opaque data that is handed to thread when it starts.
 */
static void *
prime_keepalive_thread_handler (void *ptr)
{
    int i, ret;
    log_all (LOG_THREAD, "keepalive thread started with %p\n", ptr);

    /*
     * Initialize this message only once
     */
    init_k_keepalive_command (&k_keepalive, 1, 0);

    while (1)
    {
	nap (cmdline.keepalive * 1000);

	if (fsm_state == PRIME_FSM_SHUTDOWN_PHASE)
	{
	    log_all (LOG_DEBUG, "keepalive thread exiting..\n");
	    return NULL;
	}

	keepalive_count++;
	k_keepalive.k_count = keepalive_count;
	keepalive_int_to_ext (&k_keepalive, &k_ext_keepalive);

	for (i = 0; i < num_nodeManagers; i++)
	{
	    if( (nm_keepalive_sockets == NULL) || (nm_keepalive_sockets[i] == 0))
	    {
	        log_all (LOG_ERROR, "nm_keepalive_sockets = NULL\n");
		return NULL;	
	    }
	    socket_send (nm_keepalive_sockets[i], (char *) &k_ext_keepalive,
			 sizeof (struct ext_keepalive_command));

	    log_all (LOG_THREAD, "sent keepalive %u to NodeManager %u\n",
		     keepalive_count, i);
	}

	if ((keepalive_count % KEEPALIVE_SCAN_FREQ) == 0)
	{
	    log_all (LOG_DEBUG, "Running keepalive scanner..\n");
	    ret = run_prime_keepalive_scanner (keepalive_count);
	    if (ret != -2)
	    {
		log_all (LOG_ERROR,
			 "Missing heartbeats from nodeManager %d\n", ret);
		prime_send_kill_procs ();
		prime_shutdown ();
		exit (99);
	    }

	    log_all (LOG_DEBUG, "Success!\n");
	}
    }

    return NULL;
}

/**
 * @brief  Function for a thread to listen for keep-alive messages.
 *
 * @param ptr : Pointer to opaque data, that is handed to the thread when it starts.
 */
static void *
prime_listen_thread_handler (void *ptr)
{
    struct pollfd *handle;
    int rc, i;
    int error_code = 0;
    char *str_error = NULL;

    log_all (LOG_THREAD, "listen thread started with %p\n", ptr);

    handle = my_malloc (sizeof (struct pollfd) * num_nodeManagers);
    if (handle == NULL)
    {
	log_all (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	exit (1);
    }

    for (i = 0; i < num_nodeManagers; i++)
    {
	handle[i].fd = nm_keepalive_sockets[i];
	handle[i].events = POLLIN;
    }

    while (1)
    {
#if defined(WIN32)
	rc = WSAPoll (handle, num_nodeManagers, KEEPALIVE_POLL_TIMEOUT);
#else
	rc = poll (handle, num_nodeManagers, KEEPALIVE_POLL_TIMEOUT);
#endif
	if (rc < 0 || rc == 0)
	{
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    log_all (LOG_ERROR, "poll error in %s %s\n",
		     __FUNCTION__, str_error);
	    return NULL;
	}

	for (i = 0; i < num_nodeManagers; i++)
	{
	    if (!(handle[i].revents & POLLIN))
	    {
		continue;
	    }

	    if( (nm_keepalive_sockets == NULL) || (nm_keepalive_sockets[i] == 0))
	    {
	        log_all (LOG_ERROR, "nm_keepalive_sockets = NULL\n");
		return NULL;	
	    }
	    rc = socket_receive (nm_keepalive_sockets[i], (char *) g_ekc,
				 sizeof (struct ext_keepalive_command));

	    if (rc < 0 || rc == 0)
	    {
		if (fsm_state == PRIME_FSM_SHUTDOWN_PHASE)
		{
		    log_all (LOG_DEBUG, "listen thread exiting..\n");
		    return NULL;
		}

		log_all (LOG_ERROR,
			 "recv failure keepalive. Shutdown %s.\n",
			 netmist_strerror (netmist_get_sock_error ()));
		return NULL;
	    }

	    keepalive_ext_to_int (g_kc, g_ekc);
	    print_keepalive_command (g_kc);
#if defined(_linux_)
	    if (g_kc->k_cpu_warning)
		log_all (LOG_DEBUG,
			 "CPU warning from NodeManager %d Percent busy %5.2f\n",
			 i, g_kc->k_cpu_percent_busy);
#endif
	    set_nodeManager_keepalive (g_kc->k_source_id, keepalive_count);
	}

    }

    return NULL;
}

/**
 * @brief Prime shutdown
 *
 */
static void
prime_shutdown (void)
{
    log_all (LOG_EXEC, "Shutting down\n");

    /*
     * Close sockets
     */
    log_all (LOG_EXEC, "Closing sockets\n");
    close_keepalive_sockets ();
    /*
     * Close file handles
     */
    log_all (LOG_EXEC, "Closing file handles\n");

    /*
     * Cleanup memory
     */
    log_all (LOG_EXEC, "Freeing memory\n");
}

/**
 * @brief Prime sends kill to all of the NodeManager, who in turn forward
 *        to all of their children.
 *
 */
void
prime_send_kill_procs (void)
{
#ifdef OLD_KILL_SSH
    int i;
    int sret;
    char name_buf[MAXHNAME];
    char command[MAXECOMMAND];
    char remote_command[MAXECOMMAND];

    char *nodeManager_name;
    int client_id;
    int client_pid;
#endif
    int num_nodeManagers;

#ifdef OLD_KILL_SSH
    memset ((void *) name_buf, 0, (size_t) MAXHNAME);
    memset ((void *) command, 0, (size_t) MAXECOMMAND);
#endif


    abort_ifnot_prime ();	/* Just a debug and sanity check */

    num_nodeManagers = get_nodeManager_count ();

    log_all (LOG_EXEC, "\tLaunching %d nodeManager kill processes.\n",
	     num_nodeManagers);
    /* Send a SIGINT via the keepalive socket to the nodemanagers */
    prime_send_sigint ();
    exit (1);
#ifdef OLD_KILL_SSH
    for (i = 0; i < num_nodeManagers; i++)
    {
	nodeManager_name = get_nodeManager_name (i);
	client_id = get_nodeManager_client_id (i);
	client_pid = get_nodeManager_pid (i);

	if (!client_pid)
	{
	    log_all (LOG_ERROR, "No pid for nodeManager %d\n", i);
	    continue;
	}

	if (!cmdline.localflag)
	{
#if defined(WIN32)
	    if (client_obj[client_id].client_type == TYPE_UNIX)	/*Destination is UNIX */
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, UNIX_REMOTE_FROM_WINDOWS, 255);
		    remsh[strlen (UNIX_REMOTE_FROM_WINDOWS)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s -l %s %s -A %s -Z %s -m 2 -k %d",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user, client_pid);
	    }
	    else
	    {
		/* Destination is Windows */
		my_strncpy (remote_command, WINDOWS_REMOTE_FROM_WINDOWS, 256);
		remote_command[strlen (WINDOWS_REMOTE_FROM_WINDOWS)] = 0;
		snprintf (command, sizeof (command),
			  "%s /node:\"%s\" /user:\"%s\" /password:\"%s\" process call create \"%s -A %s -Z %s -m 2 -k %d\"",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_execdir,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user, client_pid);
	    }

#else
	    /* Destination is UNIX */
	    if (client_obj[client_id].client_type == TYPE_UNIX)
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, UNIX_REMOTE_FROM_UNIX, 255);
		    remsh[strlen (UNIX_REMOTE_FROM_UNIX)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s -l %s %s -A %s -Z %s -m 2 -k %d",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user, client_pid);
	    }
	    if (client_obj[client_id].client_type == TYPE_WINDOWS2)
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, WINDOWS2_REMOTE_FROM_UNIX, 255);
		    remsh[strlen (WINDOWS2_REMOTE_FROM_UNIX)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s -l %s %s -A %s -Z %s -m 2 -k %d",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user, client_pid);
	    }
#endif
	}
	else
	{
#if defined(WIN32)
	    snprintf (command, sizeof (command),
		      "start %s -A %s -Z %s -m 2 -k %d -9 %d",
		      client_obj[client_id].client_execdir,
		      client_obj[client_id].client_password,
		      client_obj[client_id].client_dom_user,
		      client_pid, client_id);
#else
	    snprintf (command, sizeof (command),
		      "%s -A %s -Z %s -m 2 -k %d",
		      client_obj[client_id].client_execdir,
		      client_obj[client_id].client_password,
		      client_obj[client_id].client_dom_user, client_pid);
#endif
	}

	log_all (LOG_EXEC_VERBOSE, "Command: %s\n", command);

	log_all (LOG_EXEC,
		 "\tStarting kill nodeManager: %3d Host: %10s pid: %d\n", i,
		 nodeManager_name, client_pid);

	sret = system (command);
	if (sret != 0)
	{
	    log_all (LOG_ERROR,
		     "Remote command startup failed.. cleanup and exit. %d\n",
		     sret);
	    exit (99);
	}
    }
    exit (1);
#endif
}

/**
 * @brief Prime saves all of the results from the children.
 *
 * @param  client_id : child ID 
 * @param  g_mc : Pointer to the prime_command structure.
 */
void
prime_store_results (int client_id, struct prime_command *g_mc)
{
    struct result_object *res_obj;
    int u;

    if (client_id >= num_clients)
    {
	log_all (LOG_ERROR, "Received invalid client_id %d\n", client_id);
	return;
    }

    log_all (LOG_RESULTS_VERBOSE, "Prime receiving results from child %d\n",
	     client_id);

    res_obj = &client_obj[client_id].results;

    my_strncpy (res_obj->client_name, g_mc->m_client_name, MAXHNAME);
    my_strncpy (res_obj->work_obj_name, g_mc->m_workload_name, MAXWLNAME);

    res_obj->work_obj_index = g_mc->m_work_obj_index;
    res_obj->run_time = g_mc->m_run_time;
    res_obj->min_direct_size = g_mc->m_min_direct_size;
    res_obj->total_file_op_time = g_mc->m_total_file_op_time;
    res_obj->total_file_ops = g_mc->m_total_file_ops;
    res_obj->ops_per_second = g_mc->m_ops_per_second;
    res_obj->average_latency = g_mc->m_average_latency;
    res_obj->read_throughput = g_mc->m_read_throughput;
    res_obj->read_kbytes = g_mc->m_read_kbytes;
    res_obj->write_throughput = g_mc->m_write_throughput;
    res_obj->write_kbytes = g_mc->m_write_kbytes;
    res_obj->Nread_throughput = g_mc->m_Nread_throughput;
    res_obj->Nread_kbytes = g_mc->m_Nread_kbytes;
    res_obj->Nwrite_throughput = g_mc->m_Nwrite_throughput;
    res_obj->Nwrite_kbytes = g_mc->m_Nwrite_kbytes;
    res_obj->meta_r_kbytes = g_mc->m_meta_r_kbytes;
    res_obj->meta_w_kbytes = g_mc->m_meta_w_kbytes;
    res_obj->init_files = g_mc->m_init_files;
    res_obj->init_files_ws = g_mc->m_init_files_ws;
    res_obj->init_dirs = g_mc->m_init_dirs;
    res_obj->file_space_mb = g_mc->m_file_space_mb;
    res_obj->client_id = g_mc->m_client_number;
    res_obj->modified_run = g_mc->m_modified_run;
    res_obj->background = g_mc->m_background;
    res_obj->sharemode = g_mc->m_sharemode;
    res_obj->uniform_file_size_dist = g_mc->m_uniform_file_size_dist;
    res_obj->rand_dist_behavior = g_mc->m_rand_dist_behavior;
    res_obj->notify = g_mc->m_notify;
    res_obj->cipher = g_mc->m_cipher;
    res_obj->lru_on = g_mc->m_lru_on;
    /* Move banding data */
    for (u = 0; u < BUCKETS; u++)
	res_obj->bands[u] = g_mc->m_bands[u];

    res_obj->read_count = g_mc->m_read_count;
    my_strncpy (res_obj->read_string, g_mc->m_read_string, OP_STR_LEN);

    res_obj->read_file_count = g_mc->m_read_file_count;
    my_strncpy (res_obj->read_file_string, g_mc->m_read_file_string,
		OP_STR_LEN);

    res_obj->read_rand_count = g_mc->m_read_rand_count;
    my_strncpy (res_obj->read_rand_string, g_mc->m_read_rand_string,
		OP_STR_LEN);

    res_obj->write_count = g_mc->m_write_count;
    my_strncpy (res_obj->write_string, g_mc->m_write_string, OP_STR_LEN);

    res_obj->write_file_count = g_mc->m_write_file_count;
    my_strncpy (res_obj->write_file_string, g_mc->m_write_file_string,
		OP_STR_LEN);

    res_obj->open_count = g_mc->m_open_count;
    res_obj->close_count = g_mc->m_close_count;

    res_obj->mmap_write_count = g_mc->m_mmap_write_count;
    my_strncpy (res_obj->mmap_write_string, g_mc->m_mmap_write_string,
		OP_STR_LEN);

    res_obj->mmap_read_count = g_mc->m_mmap_read_count;
    my_strncpy (res_obj->mmap_read_string, g_mc->m_mmap_read_string,
		OP_STR_LEN);

    res_obj->write_rand_count = g_mc->m_write_rand_count;
    my_strncpy (res_obj->write_rand_string, g_mc->m_write_rand_string,
		OP_STR_LEN);

    res_obj->rmw_count = g_mc->m_rmw_count;
    my_strncpy (res_obj->rmw_string, g_mc->m_rmw_string, OP_STR_LEN);

    res_obj->mkdir_count = g_mc->m_mkdir_count;
    my_strncpy (res_obj->mkdir_string, g_mc->m_mkdir_string, OP_STR_LEN);

    res_obj->rmdir_count = g_mc->m_rmdir_count;
    my_strncpy (res_obj->rmdir_string, g_mc->m_rmdir_string, OP_STR_LEN);

    res_obj->unlink_count = g_mc->m_unlink_count;
    my_strncpy (res_obj->unlink_string, g_mc->m_unlink_string, OP_STR_LEN);

    res_obj->unlink2_count = g_mc->m_unlink2_count;
    my_strncpy (res_obj->unlink2_string, g_mc->m_unlink2_string, OP_STR_LEN);

    res_obj->create_count = g_mc->m_create_count;
    my_strncpy (res_obj->create_string, g_mc->m_create_string, OP_STR_LEN);

    res_obj->append_count = g_mc->m_append_count;
    my_strncpy (res_obj->append_string, g_mc->m_append_string, OP_STR_LEN);

    res_obj->lock_count = g_mc->m_locking_count;
    my_strncpy (res_obj->lock_string, g_mc->m_locking_string, OP_STR_LEN);

    res_obj->access_count = g_mc->m_access_count;
    my_strncpy (res_obj->access_string, g_mc->m_access_string, OP_STR_LEN);

    res_obj->chmod_count = g_mc->m_chmod_count;
    my_strncpy (res_obj->chmod_string, g_mc->m_chmod_string, OP_STR_LEN);

    res_obj->readdir_count = g_mc->m_readdir_count;
    my_strncpy (res_obj->readdir_string, g_mc->m_readdir_string, OP_STR_LEN);

    res_obj->stat_count = g_mc->m_stat_count;
    my_strncpy (res_obj->stat_string, g_mc->m_stat_string, OP_STR_LEN);

    res_obj->neg_stat_count = g_mc->m_neg_stat_count;
    my_strncpy (res_obj->neg_stat_string, g_mc->m_neg_stat_string,
		OP_STR_LEN);

    res_obj->copyfile_count = g_mc->m_copyfile_count;
    my_strncpy (res_obj->copyfile_string, g_mc->m_copyfile_string,
		OP_STR_LEN);

    res_obj->rename_count = g_mc->m_rename_count;
    my_strncpy (res_obj->rename_string, g_mc->m_rename_string, OP_STR_LEN);

    res_obj->statfs_count = g_mc->m_statfs_count;
    my_strncpy (res_obj->statfs_string, g_mc->m_statfs_string, OP_STR_LEN);

    res_obj->pathconf_count = g_mc->m_pathconf_count;
    my_strncpy (res_obj->pathconf_string, g_mc->m_pathconf_string,
		OP_STR_LEN);

    res_obj->trunc_count = g_mc->m_trunc_count;
    my_strncpy (res_obj->trunc_string, g_mc->m_trunc_string, OP_STR_LEN);

    res_obj->custom1_count = g_mc->m_custom1_count;
    my_strncpy (res_obj->custom1_string, g_mc->m_custom1_string, OP_STR_LEN);

    res_obj->custom2_count = g_mc->m_custom2_count;
    my_strncpy (res_obj->custom2_string, g_mc->m_custom2_string, OP_STR_LEN);

    if (cmdline.Op_lat_flag)
    {
	res_obj->read_time = g_mc->m_read_time;
	res_obj->min_read_latency = g_mc->m_min_read_latency;
	res_obj->max_read_latency = g_mc->m_max_read_latency;

	res_obj->read_file_time = g_mc->m_read_file_time;
	res_obj->min_read_file_latency = g_mc->m_min_read_file_latency;
	res_obj->max_read_file_latency = g_mc->m_max_read_file_latency;

	res_obj->read_rand_time = g_mc->m_read_rand_time;
	res_obj->min_read_rand_latency = g_mc->m_min_read_rand_latency;
	res_obj->max_read_rand_latency = g_mc->m_max_read_rand_latency;

	res_obj->write_time = g_mc->m_write_time;
	res_obj->min_write_latency = g_mc->m_min_write_latency;
	res_obj->max_write_latency = g_mc->m_max_write_latency;

	res_obj->write_file_time = g_mc->m_write_file_time;
	res_obj->min_write_file_latency = g_mc->m_min_write_file_latency;
	res_obj->max_write_file_latency = g_mc->m_max_write_file_latency;

	res_obj->mmap_write_time = g_mc->m_mmap_write_time;
	res_obj->min_mmap_write_latency = g_mc->m_min_mmap_write_latency;
	res_obj->max_mmap_write_latency = g_mc->m_max_mmap_write_latency;

	res_obj->mmap_read_time = g_mc->m_mmap_read_time;
	res_obj->min_mmap_read_latency = g_mc->m_min_mmap_read_latency;
	res_obj->max_mmap_read_latency = g_mc->m_max_mmap_read_latency;

	res_obj->write_rand_time = g_mc->m_write_rand_time;
	res_obj->min_write_rand_latency = g_mc->m_min_write_rand_latency;
	res_obj->max_write_rand_latency = g_mc->m_max_write_rand_latency;

	res_obj->rmw_time = g_mc->m_rmw_time;
	res_obj->min_rmw_latency = g_mc->m_min_rmw_latency;
	res_obj->max_rmw_latency = g_mc->m_max_rmw_latency;

	res_obj->mkdir_time = g_mc->m_mkdir_time;
	res_obj->min_mkdir_latency = g_mc->m_min_mkdir_latency;
	res_obj->max_mkdir_latency = g_mc->m_max_mkdir_latency;

	res_obj->rmdir_time = g_mc->m_rmdir_time;
	res_obj->min_rmdir_latency = g_mc->m_min_rmdir_latency;
	res_obj->max_rmdir_latency = g_mc->m_max_rmdir_latency;

	res_obj->unlink_time = g_mc->m_unlink_time;
	res_obj->min_unlink_latency = g_mc->m_min_unlink_latency;
	res_obj->max_unlink_latency = g_mc->m_max_unlink_latency;

	res_obj->unlink2_time = g_mc->m_unlink2_time;
	res_obj->min_unlink2_latency = g_mc->m_min_unlink2_latency;
	res_obj->max_unlink2_latency = g_mc->m_max_unlink2_latency;

	res_obj->create_time = g_mc->m_create_time;
	res_obj->min_create_latency = g_mc->m_min_create_latency;
	res_obj->max_create_latency = g_mc->m_max_create_latency;

	res_obj->append_time = g_mc->m_append_time;
	res_obj->min_append_latency = g_mc->m_min_append_latency;
	res_obj->max_append_latency = g_mc->m_max_append_latency;

	res_obj->lock_time = g_mc->m_locking_time;
	res_obj->min_lock_latency = g_mc->m_min_locking_latency;
	res_obj->max_lock_latency = g_mc->m_max_locking_latency;

	res_obj->access_time = g_mc->m_access_time;
	res_obj->min_access_latency = g_mc->m_min_access_latency;
	res_obj->max_access_latency = g_mc->m_max_access_latency;

	res_obj->chmod_time = g_mc->m_chmod_time;
	res_obj->min_chmod_latency = g_mc->m_min_chmod_latency;
	res_obj->max_chmod_latency = g_mc->m_max_chmod_latency;

	res_obj->readdir_time = g_mc->m_readdir_time;
	res_obj->min_readdir_latency = g_mc->m_min_readdir_latency;
	res_obj->max_readdir_latency = g_mc->m_max_readdir_latency;

	res_obj->stat_time = g_mc->m_stat_time;
	res_obj->min_stat_latency = g_mc->m_min_stat_latency;
	res_obj->max_stat_latency = g_mc->m_max_stat_latency;

	res_obj->neg_stat_time = g_mc->m_neg_stat_time;
	res_obj->min_neg_stat_latency = g_mc->m_min_neg_stat_latency;
	res_obj->max_neg_stat_latency = g_mc->m_max_neg_stat_latency;

	res_obj->copyfile_time = g_mc->m_copyfile_time;
	res_obj->min_copyfile_latency = g_mc->m_min_copyfile_latency;
	res_obj->max_copyfile_latency = g_mc->m_max_copyfile_latency;

	res_obj->rename_time = g_mc->m_rename_time;
	res_obj->min_rename_latency = g_mc->m_min_rename_latency;
	res_obj->max_rename_latency = g_mc->m_max_rename_latency;

	res_obj->statfs_time = g_mc->m_statfs_time;
	res_obj->min_statfs_latency = g_mc->m_min_statfs_latency;
	res_obj->max_statfs_latency = g_mc->m_max_statfs_latency;

	res_obj->pathconf_time = g_mc->m_pathconf_time;
	res_obj->min_pathconf_latency = g_mc->m_min_pathconf_latency;
	res_obj->max_pathconf_latency = g_mc->m_max_pathconf_latency;

	res_obj->trunc_time = g_mc->m_trunc_time;
	res_obj->min_trunc_latency = g_mc->m_min_trunc_latency;
	res_obj->max_trunc_latency = g_mc->m_max_trunc_latency;

	res_obj->custom1_time = g_mc->m_custom1_time;
	res_obj->min_custom1_latency = g_mc->m_min_custom1_latency;
	res_obj->max_custom1_latency = g_mc->m_max_custom1_latency;

	res_obj->custom2_time = g_mc->m_custom2_time;
	res_obj->min_custom2_latency = g_mc->m_min_custom2_latency;
	res_obj->max_custom2_latency = g_mc->m_max_custom2_latency;
    }

    log_all (LOG_RESULTS_VERBOSE, "\tChild %d:\n", g_mc->m_client_number);
    log_all (LOG_RESULTS_VERBOSE, "\tWorkload Name    %14s\n",
	     g_mc->m_workload_name);
    log_all (LOG_RESULTS_VERBOSE, "\tRun time	     %ld\n",
	     g_mc->m_run_time);
    log_all (LOG_RESULTS_VERBOSE, "\tMin Direct Size  %ld\n",
	     g_mc->m_min_direct_size);
    log_all (LOG_RESULTS_VERBOSE, "\tTotal file ops   %10.0f\n",
	     g_mc->m_total_file_ops);
    log_all (LOG_RESULTS_VERBOSE, "\tOps/sec	     %10.2f\n",
	     g_mc->m_ops_per_second);
    log_all (LOG_RESULTS_VERBOSE, "\tAvg Latency      %10.3f\n",
	     g_mc->m_average_latency);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->read_string,
	     g_mc->m_read_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->read_file_string,
	     g_mc->m_read_file_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->read_rand_string,
	     g_mc->m_read_rand_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->write_string,
	     g_mc->m_write_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->write_file_string,
	     g_mc->m_write_file_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->mmap_write_string,
	     g_mc->m_mmap_write_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->mmap_read_string,
	     g_mc->m_mmap_read_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->write_rand_string,
	     g_mc->m_write_rand_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->rmw_string,
	     g_mc->m_rmw_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->mkdir_string,
	     g_mc->m_mkdir_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->rmdir_string,
	     g_mc->m_rmdir_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->unlink_string,
	     g_mc->m_unlink_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->unlink2_string,
	     g_mc->m_unlink2_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->create_string,
	     g_mc->m_create_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->append_string,
	     g_mc->m_append_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->lock_string,
	     g_mc->m_locking_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->access_string,
	     g_mc->m_access_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->chmod_string,
	     g_mc->m_chmod_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->readdir_string,
	     g_mc->m_readdir_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->stat_string,
	     g_mc->m_stat_count);
    log_all (LOG_RESULTS_VERBOSE, "\t%-35s  %d\n", res_obj->neg_stat_string,
	     g_mc->m_neg_stat_count);
    log_all (LOG_RESULTS_VERBOSE, "\tOpen_count       %d\n",
	     g_mc->m_open_count);
    log_all (LOG_RESULTS_VERBOSE, "\tClose_count      %d\n",
	     g_mc->m_close_count);
}

/**
 * @brief prime sends the Warmup command to all nodeManagers
 */
static void
prime_send_command (int command, const char *message)
{
    int i;
    reset_all_client_comm_buffers ();
    g_cc->c_command = command;
    g_cc->magic = fsm_state;
    add_crc32_client_command ();
    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    for (i = 0; i < num_nodeManagers; i++)
    {
	log_all (LOG_EXEC, "Sending %s to NodeManager %d\n", message, i);
	server_send (i, (char *) g_ecc, sizeof (struct ext_client_command));
    }
}

/**
 * @brief NodeManager sends the Epoch time sync to its nodeManager.
 *
 * @param  nm_id : NodeManager's ID
 */
static void
prime_do_time_sync (int nm_id)
{
    char string[40];
    double foobar;
    memset (string, 0, 40);
    reset_all_client_comm_buffers ();
    g_cc->c_command = R_TIME_SYNC;
    foobar = getepochtime ();
    snprintf (string, 40, "%lf", foobar);
    sscanf (string, "%lf", &foobar);
    g_cc->c_prime_eclock_time = foobar;
    g_cc->magic = fsm_state;
    add_crc32_client_command ();
    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    log_all (LOG_FSM_VERBOSE, "Sending time sync to NodeManager %d\n", nm_id);

    server_send (nm_id, (char *) g_ecc, sizeof (struct ext_client_command));
}

/**
 * @brief Prime sends SYNC_OK to a nodeManager on a direct socket
 *
 * @param socket_id
 */
static void
prime_send_sync_ok_on_socket (int socket_id)
{
    reset_all_client_comm_buffers ();

    g_cc->c_command = R_SYNC_OK;
    g_cc->magic = fsm_state;

    add_crc32_client_command ();

    log_all (LOG_FSM_VERBOSE, "Sending SYNC_OK to NodeManager\n");

    client_int_to_ext (g_cc, g_ecc);

    server_send_on_socket (socket_id, (char *) g_ecc,
			   sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends version to a nodeManager
 *
 * @param  nodeManager_id : NodeManager's ID
 */
static void
prime_send_version (int nodeManager_id)
{
    reset_all_client_comm_buffers ();

    my_strncpy (g_cc->c_client_version, (char *) git_version, VERS_WIDTH);
    g_cc->c_command = R_VERSION_CHECK;

    log_all (LOG_FSM_VERBOSE, "Sending version %s to NodeManager %d\n",
	     git_version, nodeManager_id);

    client_int_to_ext (g_cc, g_ecc);

    server_send (nodeManager_id, (char *) g_ecc,
		 sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends the GO message to a nodeManager
 *
 * @param  nm_id : NodeManager's ID
 */
static void
prime_send_go (int nm_id)
{
    char string[40];
    double foobar;
    memset (string, 0, 40);
    reset_all_client_comm_buffers ();

    my_strncpy (g_cc->c_client_version, (char *) git_version, VERS_WIDTH);
    g_cc->c_flush_flag = cmdline.flush_flag;
    g_cc->c_cleanup = cmdline.cleanup_flag;
    g_cc->c_dump_files_flag = cmdline.dump_files_flag;
    g_cc->c_ipv6_enable = cmdline.ipv6_enable;
    g_cc->c_heartbeat = cmdline.heartbeat_flag;
    g_cc->c_num_clients = num_clients;
    g_cc->c_fd_caching_limit = cmdline.fd_caching_limit;
    g_cc->c_do_validate = cmdline.do_validate;
    g_cc->c_op_lat_flag = cmdline.Op_lat_flag;
    g_cc->c_gg_offset = cmdline.gg_offset;
    g_cc->c_gg_flag = cmdline.gg_flag;
    g_cc->c_pdsm_mode = cmdline.pdsm_mode;
    g_cc->c_pdsm_interval = cmdline.pdsm_interval;
    g_cc->c_workload_count = cmdline.workload_count;
    g_cc->c_command = R_GO;
    export_pit (g_cc, cmdline.pit_hostname, cmdline.pit_service);

    /* Set global num_work_obj so that all called functions know how many
     * active workloads are present.
     */
    num_work_obj = cmdline.workload_count;

    export_dist_table (g_cc, my_mix_table, num_work_obj);
    /*log_file (LOG_EXEC, "Prime num_work_obj = %d\n", num_work_obj); */

    foobar = getepochtime ();
    snprintf (string, 40, "%lf", foobar);
    sscanf (string, "%lf", &foobar);

    g_cc->c_prime_eclock_time = foobar;

    log_all (LOG_FSM_VERBOSE, "Sending GO to NodeManager %d\n", nm_id);

    g_cc->magic = fsm_state;
    add_crc32_client_command ();
    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    client_int_to_ext (g_cc, g_ecc);

    server_send (nm_id, (char *) g_ecc, sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends the GO message to a nodeManager
 *
 * @param  nm_id : NodeManager's ID
 */
static void
prime_send_epoch (int nm_id)
{
    char string[40];
    double foobar;
    memset (string, 0, 40);
    reset_all_client_comm_buffers ();

    my_strncpy (g_cc->c_client_version, git_version, VERS_WIDTH);
    g_cc->c_command = R_EPOCH;

    foobar = getepochtime ();
    snprintf (string, 40, "%lf", foobar);
    sscanf (string, "%lf", &foobar);

    g_cc->c_prime_eclock_time = foobar;

    //math based on the rrt measured in the TIME_SYNC phase
    memset (string, 0, 40);
    snprintf (string, 40, "%lf", c_start_epoch);
    sscanf (string, "%lf", &foobar);

    g_cc->c_start_epoch = foobar;

    g_cc->magic = fsm_state;
    add_crc32_client_command ();
    client_int_to_ext (g_cc, g_ecc);
    add_crc32_ext_client_command ();

    log_all (LOG_FSM_VERBOSE, "Sending epoch %lf to NodeManager %d\n",
	     g_cc->c_start_epoch, nm_id);

    server_send (nm_id, (char *) g_ecc, sizeof (struct ext_client_command));
}

/**
 * @brief NodeManager sends the the contents of the token config to 
 *        a NodeManager
 *
 * @param  nodeManager_id : NodeManager's ID 
 */
static void
prime_send_token_config (int nodeManager_id)
{
    int *client_list;
    int curr_client;
    int i;

    reset_all_nodeManager_comm_buffers ();
    g_nmc->nm_command = R_NODEMANAGER_SEND_TOKEN_FILE;
    client_list = get_nodeManager_client_list (nodeManager_id,
					       &g_nmc->nm_num_of_children);

    if ((cmdline.run_time_flag) && (cmdline.yaml_flag))
    {
	cmdline.run_time = cmdline.yaml_run_time;
	if (cmdline.run_time < 300)
	{
	    cmdline.run_time = 300;
	    cmdline.twarn = 1;
	}
	else
	    cmdline.twarn = 0;
    }

    if ((cmdline.skip_init_flag) && (cmdline.yaml_flag))
    {
	cmdline.skip_init = cmdline.yaml_skip_init;
    }

    g_nmc->nm_runtime = cmdline.run_time;

/*
    Not used since YAML now passes the warmup time that is specific for a given component.
    g_nmc->nm_warmtime = cmdline.warm_time;
*/

    g_nmc->nm_pdsm_mode = cmdline.pdsm_mode;
    g_nmc->nm_pdsm_interval = cmdline.pdsm_interval;
    g_nmc->nm_workload_count = cmdline.workload_count;
    g_nmc->nm_ipv6_enable = cmdline.ipv6_enable;
    my_strncpy (g_nmc->nm_client_log_dir, cmdline.client_log_dir, MAXNAME);
    my_strncpy (g_nmc->nm_client_windows_log_dir,
		cmdline.client_windows_log_dir, MAXNAME);

    clients_per_nm = g_nmc->nm_num_of_children;

    for (i = 0; i < g_nmc->nm_num_of_children; i++)
    {
	curr_client = client_list[i];
	my_strncpy (g_nmc->nm_workload_name[i],
		    client_obj[curr_client].client_workload_name, MAXWLNAME);
	my_strncpy (g_nmc->nm_workdir[i],
		    client_obj[curr_client].client_workdir, MAXNAME);
    }

    for (i = 0; i < g_nmc->nm_num_of_children; i++)
    {
	curr_client = client_list[i];

	g_nmc->nm_children_ids[i] = curr_client;
	g_nmc->nm_my_oprate[i] = client_obj[curr_client].op_rate;
	g_nmc->nm_client_files[i] = client_obj[curr_client].files_per_dir;
	g_nmc->nm_client_dirs[i] = client_obj[curr_client].dir_count;
	g_nmc->nm_file_size[i] = client_obj[curr_client].file_size;
    }

    /* Free the memory */
    free (client_list);

    log_all (LOG_FSM_VERBOSE, "Sending config data to NodeManager %d\n",
	     nodeManager_id);

    nodeManager_int_to_ext (g_nmc, g_enmc, cmdline.workload_count);

    server_send (nodeManager_id, (char *) g_enmc,
		 sizeof (struct ext_nodeManager_command));
}

/**
 * @brief Simple license check. This sets the variable that is
 *        used to encode the fingerprint with the users license ID.
 */
/*
 * __doc__
 * __doc__  Function : int prime_check_license_file(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Simple license check. This sets the variable that is
 * __doc__             used to encode the fingerprint with the users license ID.
 * __doc__
 */
void
prime_check_license_file (void)
{
    FILE *lic;
    int var;

#if defined(WIN32)
    if (!(_access (cmdline.license_filename, F_OK) == 0))
#else
    if (!(access (cmdline.license_filename, F_OK) == 0))
#endif
    {
	log_all (LOG_ERROR, "license file %s is missing\n",
		 cmdline.license_filename);
	exit (1);
    }

    lic = fopen (cmdline.license_filename, "r");
    if (lic == (FILE *) NULL)
    {
	log_all (LOG_ERROR, "Open on license file %s failed\n",
		 cmdline.license_filename);
	exit (1);
    }

    var = fscanf (lic, "LICENSE KEY %u\n", &lic_key);
    if (var != 1)
    {
	log_all (LOG_ERROR, "Invalid license file format.\n");
	fclose (lic);
	exit (1);
    }

    fclose (lic);
}

/**
 * @brief Parse an individual token and put result in destination 
 *
 * @param  inputlinep : input_line
 * @param  searchtokenp : search for this string.
 * @param  destinationp : destination for the result
 */
/*
 * __doc__
 * __doc__  Function : int parse_token( char *inputl, char *search, char *dest)
 * __doc__  Arguments: char *input_line
 * __doc__             char *search for this string.
 * __doc__             char *destination result
 * __doc__  Returns  : void
 * __doc__  Performs : Parse an individual token and put result in destination 
 * __doc__
 */
static int
parse_token (char *inputlinep, char *searchtokenp, char *destinationp)
{
    char *tokenp, *cp, *cd;
    int count = 0;
    tokenp = (char *) my_strcasestr (inputlinep, searchtokenp);
    if (tokenp)
    {
	cp = tokenp + strlen (searchtokenp);
	cd = destinationp;
	while ((*cp != ' ') && (*cp != (char) 0)
	       && (cp != inputlinep + strlen (inputlinep))
	       && count < MAX_INPUT_TOKENLEN)
	{
	    count++;
	    *cd++ = *cp++;
	}
	*cd = 0;
	return (0);
    }
    else
	*destinationp = 0;
    return (1);
}

/**
 * @brief Parse all known tokens from the line.
 *
 * @param input : input token
 */
/*
 * __doc__
 * __doc__  Function : int parse_all_tokens(char *input)
 * __doc__  Arguments: char *: input token
 * __doc__  Returns  : int: Success or fail
 * __doc__  Performs : Parse all known tokens from the line.
 * __doc__
 */
static int
parse_all_tokens (char *input)
{
    int i, ret;
    for (i = 0; i < NUM_INPUT_TOKENS; i++)
    {
	ret =
	    parse_token (input, token_objects[i].tname,
			 token_objects[i].tvalue);
	if (ret && !token_objects[i].optional)
	{
	    log_all (LOG_ERROR, "Parsing error while looking for %s\n",
		     token_objects[i].tname);
	    return (1);
	}
    }
    return (0);
}

/**
 * @brief Validate all tokens are there, and place their 
 *        values in the client_object table.
 *
 * @param client_num Client number
 */
/*
 * __doc__
 * __doc__  Function : int validate_all_tokens(int client_num)
 * __doc__  Arguments: int client number
 * __doc__  Returns  : int success or failure.
 * __doc__  Performs : Validate all tokens are there, and place their 
 * __doc__             values in the client_object table.
 * __doc__
 */
static int
validate_all_tokens (int client_num)
{
    char *execpath;
    int windowsclient = -1;
    int u;

    /* 
       Grab the Clientname 
     */
    if (strlen (token_objects[T_CLIENT_NAME].tvalue) == 0)
    {
	log_all (LOG_ERROR, "Clientname is missing\n");
	return (1);
    }
    my_strncpy (client_obj[client_num].client_name,
		token_objects[T_CLIENT_NAME].tvalue, MAXHNAME);

    /*
       Grab the Username.
     */
    if (strlen (token_objects[T_USER_NAME].tvalue) == 0)
    {
	log_all (LOG_ERROR, "Username is missing\n");
	return (1);
    }
    my_strncpy (client_obj[client_num].client_dom_user,
		token_objects[T_USER_NAME].tvalue, MAXHNAME);

    /*
       Grab the password, if it is specified.
     */
    if (strlen (token_objects[T_PASSWORD].tvalue) != 0)
    {
	my_strncpy (client_obj[client_num].client_password,
		    token_objects[T_PASSWORD].tvalue, MAXNAME);
    }
    else
    {
	/* invalid pass */
	my_strncpy (client_obj[client_num].client_password, "_", MAXNAME);
    }

    /*
     * Grab the Launch type (WMI or SSH), if specified 
     */
    if (strlen (token_objects[T_LAUNCH_TYPE].tvalue) != 0)
    {
	/* Use winexe to access remote Windows client */
	if (my_strcasecmp (token_objects[T_LAUNCH_TYPE].tvalue, "WMI") == 0)
	{
	    client_obj[client_num].client_type = TYPE_WINDOWS;
	    windowsclient = 1;
	}
	/* Use ssh to access remote Windows client */
	if (my_strcasecmp (token_objects[T_LAUNCH_TYPE].tvalue, "WSSH") == 0)
	{
	    client_obj[client_num].client_type = TYPE_WINDOWS2;
	    windowsclient = 2;
	}
	/* Use ssh to access remote Unix client */
	if (my_strcasecmp (token_objects[T_LAUNCH_TYPE].tvalue, "SSH") == 0)
	{
	    client_obj[client_num].client_type = TYPE_UNIX;
	    windowsclient = 0;
	}
    }

    /*
     * Grab the Work Dir Path 
     */
    if (strlen (token_objects[T_WORK_DIR].tvalue) == 0)
    {
	log_all (LOG_ERROR, "Workdir is missing\n");
	return (1);
    }
    my_strncpy (client_obj[client_num].client_workdir,
		token_objects[T_WORK_DIR].tvalue, MAXNAME);

    /*
     * Grab the Exec Path 
     */
    if (strlen (token_objects[T_EXEC_PATH].tvalue) == 0)
    {
	log_all (LOG_ERROR, "Execpath is missing\n");
	return (1);
    }
    my_strncpy (client_obj[client_num].client_execdir,
		token_objects[T_EXEC_PATH].tvalue, MAXNAME);

    /* 
     * Should the user not specify the client type in the token file, we can 
     * try to guess here by looking at the exec path. 
     */
    if (windowsclient == -1)
    {
	/* Try to detect client type */
	execpath = client_obj[client_num].client_execdir;
	for (u = 0; u < (int) strlen (execpath); u++)
	{
	    if (execpath[u] == '\\')
	    {
		windowsclient = 1;
		break;
	    }
	}
	if (windowsclient == 1)
	    client_obj[client_num].client_type = TYPE_WINDOWS;
	else
	    client_obj[client_num].client_type = TYPE_UNIX;

	windowsclient = 0;
    }

    /* 
       Grab WORKLOAD name 
     */
    if (strlen (token_objects[T_WORKLOAD].tvalue) == 0)
    {
	log_all (LOG_ERROR, "Workload is missing\n");
	return (1);
    }
    my_strncpy (client_obj[client_num].client_workload_name,
		token_objects[T_WORKLOAD].tvalue, MAXWLNAME);

    /* 
       Grab LOG_PATH, if it exists 
     */
    if (strlen (token_objects[T_LOG_PATH].tvalue) != 0)
    {
	my_strncpy (client_obj[client_num].client_log_path,
		    token_objects[T_LOG_PATH].tvalue, MAXNAME);
    }

    /* 
       Grab WINDOWS_LOG_PATH, if it exists 
     */
    if (strlen (token_objects[T_WINDOWS_LOG_PATH].tvalue) != 0)
    {
	my_strncpy (client_obj[client_num].client_windows_log_path,
		    token_objects[T_WINDOWS_LOG_PATH].tvalue, MAXNAME);
    }


    if (strlen (token_objects[T_INSTANCES].tvalue) != 0)
    {
	client_obj[client_num].instances =
	    atoi (token_objects[T_INSTANCES].tvalue);
    }
    else
    {
	client_obj[client_num].instances = 0;
    }

    return (0);
}

/**
 * @brief This is the function that reads the token file. 
 */
/*
 * __doc__
 * __doc__  Function : void read_client_tok_file(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : This is the function that reads the token file. 
 * __doc__
 */
void
read_client_tok_file (void)
{
    int client_count;
    char buf[600];
    FILE *fd;
    int ret, j;
    char *cret;
    int nm_error;

    client_count = 0;

    fd = fopen (cmdline.client_tok_filename, "r");
    if (fd == 0)
    {
	log_all (LOG_ERROR, "Bad client filename > %s <\n",
		 cmdline.client_tok_filename);
	exit (0);
    }
    while (1)
    {
	memset ((void *) buf, 0, (size_t) 600);
	cret = fgets (buf, 600, fd);
	if (cret == (char *) 0)
	{
	    break;
	}
	/*We also ignore empty line */
	if (buf[0] == '#' || is_empty_line (buf))
	    continue;
	if (buf[strlen (buf) - 1] == '\n')
	    buf[strlen (buf) - 1] = 0;;
	ret = parse_all_tokens (buf);
	if (ret)
	{
	    log_all (LOG_ERROR, "Error parsing config\n");
	    exit (1);
	}
	ret = validate_all_tokens (client_count);
	if (ret)
	{
	    log_all (LOG_ERROR, "Error validating config\n");
	    exit (1);
	}
	client_obj[client_count].client_id = client_count;

	/* Add this to NodeManager structure as well */
	nm_error =
	    prime_check_and_add_nodeManager_object (client_obj
						    [client_count].client_name,
						    client_count);
	if (nm_error == NODEMANAGER_FAILURE)
	{
	    log_all (LOG_ERROR, "Failed to add nodeManager strcture\n");
	    exit (1);
	}

	if (client_obj[client_count].instances != 0)
	{
	    j = client_obj[client_count].instances;
	    while (j > 1)
	    {
		/* 
		 * Now clone all of the fields from this entry to however 
		 * many was specified 
		 */
		client_obj[client_count].client_id = client_count;
		client_obj[client_count + 1].client_id = client_count + 1;
		my_strncpy (client_obj[client_count + 1].client_name,
			    client_obj[client_count].client_name, MAXHNAME);
		my_strncpy (client_obj[client_count + 1].client_dom_user,
			    client_obj[client_count].client_dom_user,
			    MAXNAME);
		my_strncpy (client_obj[client_count + 1].client_password,
			    client_obj[client_count].client_password,
			    MAXNAME);
		my_strncpy (client_obj[client_count + 1].client_workdir,
			    client_obj[client_count].client_workdir, MAXNAME);
		my_strncpy (client_obj[client_count + 1].client_execdir,
			    client_obj[client_count].client_execdir, MAXNAME);
		my_strncpy (client_obj[client_count + 1].client_workload_name,
			    client_obj[client_count].client_workload_name,
			    MAXWLNAME);
		my_strncpy (client_obj[client_count + 1].client_log_path,
			    client_obj[client_count].client_log_path,
			    MAXNAME);
		my_strncpy (client_obj[client_count + 1].
			    client_windows_log_path,
			    client_obj[client_count].client_windows_log_path,
			    MAXNAME);
		client_obj[client_count + 1].op_rate =
		    client_obj[client_count].op_rate;
		client_obj[client_count + 1].file_size =
		    client_obj[client_count].file_size;
		client_obj[client_count + 1].dir_count =
		    client_obj[client_count].dir_count;
		client_obj[client_count + 1].files_per_dir =
		    client_obj[client_count].files_per_dir;
		client_obj[client_count + 1].instances = 0;
		client_obj[client_count + 1].client_type =
		    client_obj[client_count].client_type;


		/* Add this to NodeManager structure as well */
		nm_error =
		    prime_check_and_add_nodeManager_object (client_obj
							    [client_count +
							     1].client_name,
							    client_count + 1);
		if (nm_error == NODEMANAGER_FAILURE)
		{
		    log_all (LOG_ERROR,
			     "Failed to add nodeManager strcture\n");
		    exit (1);
		}

		client_count++;
		j--;
		if (client_count > MAXCLIENTS)
		{
		    log_all (LOG_ERROR,
			     "Prime: Error: Increase MAXCLIENTS.\n");
		    exit (1);
		}
	    }
	}
	client_count++;

	if (client_count > MAXCLIENTS)
	{
	    log_all (LOG_ERROR, "Prime: Too many clients. "
		     "Increase MAXCLIENTS and recompile.\n");
	    exit (1);
	}
    }
    num_clients = client_count;
    if (num_clients <= 0)
    {
	/* Terminate the test if no client definition is found */
	log_all (LOG_ERROR,
		 "No client found in client token file: %s, test aborted\n",
		 client_filename);
	exit (1);
    }

    if (fd != 0)
	fclose (fd);
}

/**
 * @brief Prime scans the token file to return a total process count.
 *        This is used later to allocate memory for the client_objects
 *        This function takee no parameters.
 *
 */
/*
 * __doc__
 * __doc__  Function : int scan_client_tok_file(void)
 * __doc__  Arguments: void 
 * __doc__  Returns  : int
 * __doc__  Performs : Reads the token configuration file to compute
 * __doc__             number of clients to allocate appropriate    
 * __doc__             memory
 */
static int
scan_client_tok_file (void)
{
    int client_count = 0;
    char buf[600];
    FILE *fd;
    int ret;
    char *cret;
    long long object_size = 0LL;

    client_count = 0;

    fd = fopen (cmdline.client_tok_filename, "r");
    if (fd == 0)
    {
	log_all (LOG_ERROR, "Bad client filename > %s <\n",
		 cmdline.client_tok_filename);
	exit (0);
    }
    while (1)
    {
	memset ((void *) buf, 0, (size_t) 600);
	cret = fgets (buf, 600, fd);
	if (cret == (char *) 0)
	{
	    break;
	}
	/*We also ignore empty line */
	if (buf[0] == '#' || is_empty_line (buf))
	    continue;
	if (buf[strlen (buf) - 1] == '\n')
	    buf[strlen (buf) - 1] = 0;;
	ret = parse_all_tokens (buf);
	if (ret)
	{
	    log_all (LOG_ERROR, "Error parsing config\n");
	    exit (1);
	}
	if (strlen (token_objects[T_INSTANCES].tvalue) != 0)
	{
	    client_count += atoi (token_objects[T_INSTANCES].tvalue);
	}
	else
	{
	    client_count++;
	}
	if (client_count > MAXCLIENTS)
	{
	    log_all (LOG_ERROR, "Prime: Too many clients. "
		     "Increase MAXCLIENTS and recompile.\n");
	    exit (1);
	}
    }
    if (client_count <= 0)
    {
	/* Terminate the test if no client definition is found */
	log_all (LOG_ERROR,
		 "No client found in client token file: %s, test aborted\n",
		 client_filename);
	exit (1);
    }

    if (fd != 0)
	fclose (fd);

    if (sizeof (char *) == 8)
    {
	return (client_count);
    }
    else
    {
	object_size = client_count * sizeof (struct client_object);
	if (object_size < (2LL * 1024LL * 1024LL * 1024LL))
	{
	    return (client_count);
	}
	else
	{
	    log_all (LOG_ERROR,
		     "Too many client procs for 32 bit machine.\n ");
	    exit (1);
	}
    }
    return (client_count);
}

/**
 * @brief Prime launches the NodeManagers
 *
 * @param  num_nodeManagers : Number of NodeManagers to start
 */
static int
launch_nodeManagers (int num_nodeManagers)
{
    int i, sret, error = 0;
    char name_buf[MAXHNAME];
    char command[MAXECOMMAND];
    char remote_command[MAXECOMMAND];

    /* '-' 'C' ' ' so ' ' '-' '+' ' ' '-' 'X' ' ' arg '\0' */
    char vfs_params[11 + sizeof (cmdline.vfs_so) + sizeof (cmdline.vfs_arg)];

    char *nodeManager_name;
    int client_id;

    memset ((void *) name_buf, 0, (size_t) MAXHNAME);
    memset ((void *) command, 0, (size_t) MAXECOMMAND);

    abort_ifnot_prime ();	/* Just a debug and sanity check */

    log_all (LOG_EXEC, "\tLaunching %d nodeManager processes.\n",
	     num_nodeManagers);

    if (cmdline.sharing_flag)
    {
	my_strncpy (option_buf, "-X", 4);
	option_buf[strlen ("-X")] = 0;
    }
    else
    {
	memset ((void *) option_buf, 0, (size_t) 4);
    }

    vfs_params[0] = '\0';
    if (cmdline.vfs_so[0] != '\0')
    {
	if (cmdline.vfs_arg[0] != '\0')
	{
	    snprintf (vfs_params, sizeof vfs_params, "-C %s -+X %s",
		      cmdline.vfs_so, cmdline.vfs_arg);
	}
	else
	{
	    snprintf (vfs_params, sizeof vfs_params, "-C %s", cmdline.vfs_so);
	}
    }

    for (i = 0; i < num_nodeManagers; i++)
    {
	nodeManager_name = get_nodeManager_name (i);
	client_id = get_nodeManager_client_id (i);

	/* 
	 * Over-ride cmdline.client_log_dir with contents from token file, 
	 * if there is a log path there.
	 * This is for the case where each client might want a different path 
	 * to their log dir
	 * Example: Windows clients want C:\tmp, and Unix want /tmp.
	 * If this is not set in the token file, then the assumption is that 
	 * every client is the same type as the Prime.
	 */
	if (strlen (client_obj[client_id].client_log_path) != 0)
	{
	    my_strncpy (cmdline.client_log_dir,
			client_obj[client_id].client_log_path, MAXNAME);
	}
	if (strlen (client_obj[client_id].client_windows_log_path) != 0)
	{
	    my_strncpy (cmdline.client_windows_log_dir,
			client_obj[client_id].client_windows_log_path,
			MAXNAME);
	}

	if (!cmdline.localflag)
	{
#if defined(WIN32)
	    /*Destination is UNIX */
	    if (client_obj[client_id].client_type == TYPE_UNIX)
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, UNIX_REMOTE_FROM_WINDOWS, 255);
		    remsh[strlen (UNIX_REMOTE_FROM_WINDOWS)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s -l %s %s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -y %d -+w %d -+p '%s' -+m %d -+i %d -+C '%s' -2 %u -x %u -9 %u -1 %s -+j %s -+k %s",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  vfs_params,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user,
			  i,
			  client_obj[client_id].client_execdir,
			  cmdline.prime_name,
			  cmdline.prime_results_dir,
			  cmdline.client_log_dir,
			  prime_listen_port, cmdline.ipv6_enable,
			  cmdline.workload_count, cmdline.unix_pdsm_file,
			  cmdline.pdsm_mode, cmdline.pdsm_interval,
			  cmdline.unix_pdsm_control_file, cmdline.tracelevel,
			  cmdline.skip_init, cmdline.keepalive,
			  cmdline.client_windows_log_dir,
			  cmdline.windows_pdsm_file,
			  cmdline.windows_pdsm_control_file);
	    }
	    if (client_obj[client_id].client_type == TYPE_WINDOWS)
	    {
		/* Destination is Windows */
		my_strncpy (remote_command, WINDOWS_REMOTE_FROM_WINDOWS, 256);
		remote_command[strlen (WINDOWS_REMOTE_FROM_WINDOWS)] = 0;
		snprintf (command, sizeof (command),
			  "%s /node:\"%s\" /user:\"%s\" /password:\"%s\" process call create \"%s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -y %d -+w %d -+p %s -+m %d -+i %d -+C %s -2 %u -x %u -9 %u -1 %s -+j %s -+k %s\"",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_execdir,
			  vfs_params,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user,
			  i,
			  client_obj[client_id].client_execdir,
			  cmdline.prime_name,
			  cmdline.prime_results_dir,
			  cmdline.client_log_dir,
			  prime_listen_port, cmdline.ipv6_enable,
			  cmdline.workload_count, cmdline.unix_pdsm_file,
			  cmdline.pdsm_mode, cmdline.pdsm_interval,
			  cmdline.unix_pdsm_control_file, cmdline.tracelevel,
			  cmdline.skip_init, cmdline.keepalive,
			  cmdline.client_windows_log_dir,
			  cmdline.windows_pdsm_file,
			  cmdline.windows_pdsm_control_file);
	    }

#else
	    /* Destination is UNIX */
	    if (client_obj[client_id].client_type == TYPE_UNIX)
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, UNIX_REMOTE_FROM_UNIX, 255);
		    remsh[strlen (UNIX_REMOTE_FROM_UNIX)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s -l %s %s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -y %d -+w %d -+p '%s' -+m %d -+i %d -+C '%s' -2 %u -x %u -9 %u -1 '%s' -+j '%s' -+k '%s'",
			  remote_command,
			  nodeManager_name,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  vfs_params,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user,
			  i,
			  client_obj[client_id].client_execdir,
			  cmdline.prime_name,
			  cmdline.prime_results_dir,
			  cmdline.client_log_dir,
			  prime_listen_port, cmdline.ipv6_enable,
			  cmdline.workload_count, cmdline.unix_pdsm_file,
			  cmdline.pdsm_mode, cmdline.pdsm_interval,
			  cmdline.unix_pdsm_control_file,
			  cmdline.tracelevel, cmdline.skip_init,
			  cmdline.keepalive,
			  cmdline.client_windows_log_dir,
			  cmdline.windows_pdsm_file,
			  cmdline.windows_pdsm_control_file);
	    }
	    if (client_obj[client_id].client_type == TYPE_WINDOWS2)
	    {
		if (cmdline.use_rsh)
		{
		    my_strncpy (remsh, "rsh", 255);
		    remsh[strlen ("rsh")] = 0;
		}
		else
		{
		    my_strncpy (remsh, WINDOWS2_REMOTE_FROM_UNIX, 255);
		    remsh[strlen (WINDOWS2_REMOTE_FROM_UNIX)] = 0;
		}
		my_strncpy (remote_command, remsh, 256);
		remote_command[strlen (remsh)] = 0;
		snprintf (command, sizeof (command),
			  "%s %s %s -l %s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -y %d -+w %d -+p '%s' -+m %d -+i %d -+C '%s' -2 %u -x %u -9 %u -1 %s -+j %s -+k %s",
			  remote_command,
			  nodeManager_name,
			  vfs_params,
			  client_obj[client_id].client_dom_user,
			  client_obj[client_id].client_execdir,
			  client_obj[client_id].client_password,
			  client_obj[client_id].client_dom_user,
			  i,
			  client_obj[client_id].client_execdir,
			  cmdline.prime_name,
			  cmdline.prime_results_dir,
			  cmdline.client_log_dir,
			  prime_listen_port, cmdline.ipv6_enable,
			  cmdline.workload_count, cmdline.unix_pdsm_file,
			  cmdline.pdsm_mode, cmdline.pdsm_interval,
			  cmdline.unix_pdsm_control_file,
			  cmdline.tracelevel, cmdline.skip_init,
			  cmdline.keepalive,
			  cmdline.client_windows_log_dir,
			  cmdline.windows_pdsm_file,
			  cmdline.windows_pdsm_control_file);
	    }
#endif
	}
	else
	{
#if defined(WIN32)
	    snprintf (command, sizeof (command),
		      "start %s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -z -y %d -+w %d -+p %s -+m %d -+i %d -+C %s -2 %u -x %u -9 %u -1 %s -+j %s -+k %s",
		      client_obj[client_id].client_execdir,
		      vfs_params,
		      client_obj[client_id].client_password,
		      client_obj[client_id].client_dom_user,
		      i,
		      client_obj[client_id].client_execdir,
		      cmdline.prime_name,
		      cmdline.prime_results_dir,
		      cmdline.client_log_dir,
		      prime_listen_port, cmdline.ipv6_enable,
		      cmdline.workload_count, cmdline.unix_pdsm_file,
		      cmdline.pdsm_mode, cmdline.pdsm_interval,
		      cmdline.unix_pdsm_control_file,
		      cmdline.tracelevel, cmdline.skip_init,
		      cmdline.keepalive,
		      cmdline.client_windows_log_dir,
		      cmdline.windows_pdsm_file,
		      cmdline.windows_pdsm_control_file);
#else
	    snprintf (command, sizeof (command),
		      "%s %s -A %s -Z %s -m 2 -n %d -e %s -M %s -R %s -U %s -P %d -z -y %d -+w %d -+p '%s' -+m %d -+i %d -+C '%s' -2 %u -x %u -9 %u -1 '%s' -+j '%s' -+k '%s'",
		      client_obj[client_id].client_execdir,
		      vfs_params,
		      client_obj[client_id].client_password,
		      client_obj[client_id].client_dom_user,
		      i,
		      client_obj[client_id].client_execdir,
		      cmdline.prime_name,
		      cmdline.prime_results_dir,
		      cmdline.client_log_dir,
		      prime_listen_port, cmdline.ipv6_enable,
		      cmdline.workload_count, cmdline.unix_pdsm_file,
		      cmdline.pdsm_mode, cmdline.pdsm_interval,
		      cmdline.unix_pdsm_control_file,
		      cmdline.tracelevel, cmdline.skip_init,
		      cmdline.keepalive,
		      cmdline.client_windows_log_dir,
		      cmdline.windows_pdsm_file,
		      cmdline.windows_pdsm_control_file);
#endif
	}

	log_all (LOG_EXEC_VERBOSE, "Command: %s\n", command);

	log_all (LOG_EXEC,
		 "\tStarting test nodeManager: %3d Host: %10s\n",
		 i, nodeManager_name);

	sret = system (command);
	if (sret != 0)
	{
	    log_all (LOG_ERROR,
		     "Remote command startup failed.. cleanup and exit. %d\n",
		     sret);
	    exit (99);
	}
    }
    return error;
}

/**
 * @brief Prime initializes the logging functionality.
 *
 */
static void
prime_initialize_logging (void)
{
    internal_cfg.u.prime_cfg.log_handle = NULL;

    if (cmdline.log_flag)
    {
	internal_cfg.u.prime_cfg.log_handle =
	    init_log (cmdline.log_filename, "a");
	if (internal_cfg.u.prime_cfg.log_handle == NULL)
	{
	    log_stdout (LOG_ERROR, "%s Unable to open log file %s\n",
			cmdline.log_filename);
	    exit (1);
	}
    }

    set_log_role ("Prime");
    set_log_level (cmdline.tracelevel);
}

/**
 * @brief Prime validates the command line arguments
 *
 */
static void
prime_validate_input_args (void)
{
    reset_timestamp_in_log_file ();
    reset_timestamp_in_log_stdout ();
    /* 
     * Require ops, or client_memsize and  client count or 
     * client_filename and Aggregate data_set size size.
     */
    if (!(cmdline.gflag))
    {
	log_all (LOG_ERROR, "Missing configuration file name. (-g)\n");
	usage ();
	exit (0);
    }

    if ((cmdline.skip_init_flag || cmdline.yaml_skip_init)
	&& (cmdline.yaml_flag))
    {
	cmdline.skip_init = cmdline.yaml_skip_init;
    }

    if (cmdline.skip_init == 1)
    {
	const char *string = "**** Using an abbreviated INIT phase. ****\n";
	log_all (LOG_EXEC, string);
    }

    if (cmdline.do_validate == 0)
    {
	const char *string = "**** Op validate is disabled. ****\n";
	log_all (LOG_EXEC, string);
    }

    if (cmdline.flush_flag == 0)
    {
	const char *string = "**** Flushes are disabled. ****\n";
	log_all (LOG_EXEC, string);
    }

    num_work_obj = cmdline.workload_count;	/* global */

    if ((cmdline.run_time_flag) && (cmdline.yaml_flag))
    {
	cmdline.run_time = cmdline.yaml_run_time;
	if (cmdline.run_time < 300)
	{
	    cmdline.run_time = 300;
	    cmdline.twarn = 1;
	}
	else
	    cmdline.twarn = 0;
    }
    if (cmdline.twarn)
    {
	const char *string = "**** Minimum run time of 300 enforced. ****\n";
	log_all (LOG_EXEC, string);
    }

    if (cmdline.wwarn)
    {
	const char *string =
	    "**** Minimum warm-up time of 60 enforced. ****\n";
	log_all (LOG_EXEC, string);
    }

    set_timestamp_in_log_file ();
    set_timestamp_in_log_stdout ();
}

/**
 * @brief Prime reads in the token config file.
 *
 */
static void
prime_read_token_config_file (void)
{
    int total_procs = 0;
    /* scan to get total proc count, system wide */
    total_procs = scan_client_tok_file ();

    client_obj =
	(struct client_object *) MALLOC (sizeof (struct client_object) *
					 (total_procs + 1));

    if (client_obj == NULL)
    {
	log_all (LOG_ERROR, "Failed to allocate memory for client object\n");
	exit (1);
    }

    /*
     * Prime reads the client file to get count.
     */
    if (cmdline.gflag)
	read_client_tok_file ();

    if (num_clients <= 0)
    {
	log_all (LOG_ERROR,
		 "Something is wrong with the configuration file. When parsed, it\n");
	exit (1);
    }

    /*
     * p_num_clients is used to have all files be shared by 
     * all procs, on all clients. By using p_num_clients all 
     * of the files appear to be created, and accessed, by
     * a single client ID. (client id = 0) and no scaling 
     * is done because all files are shared by everyone. 
     */
    if (cmdline.sharing_flag)
	p_num_clients = 1;
    else
	p_num_clients = num_clients;
}

/**
 * @brief Prime initializes its runtime structures.
 *
 */
static void
initialize_runtime_structures (void)
{
    int cret;
    int mret;
    int j = 0;
    char *ubuntup;

    /*
     * Save the prime's current working directory. This is where the 
     * clients will deposit their results.
     */
    if (!cmdline.Rflag)
    {
	ubuntup = getcwd (cmdline.prime_results_dir, MAXNAME);
	if (ubuntup)
	{
	    ;
	}
    }

    /*
     * Check to see if prime's results directory exists, and is writable.
     * If not, then try to create it now...
     */
    if (check_dir_w (cmdline.prime_results_dir) != 0)
    {
	/* If it doesn't exist, try creating it */
#if defined(WIN32)
	cret = _mkdir (cmdline.prime_results_dir);
#else
	cret = mkdir (cmdline.prime_results_dir, 0777);
#endif
	if (cret < 0)
	{
	    log_all (LOG_ERROR,
		     "Unable to create prime results directory > %s <  %s\n",
		     cmdline.prime_results_dir,
		     netmist_strerror (netmist_get_error ()));
	    exit (1);
	}
    }

    /*
     * Save the prime's name. The clients use this to  deposit their 
     * results.
     */
    while (1)
    {
	mret = gethostname (cmdline.prime_name, MAXHNAME);
	if (mret == 0)
	    break;
	perror ("gethostname failure");
    }

    reset_timestamp_in_log_file ();
    reset_timestamp_in_log_stdout ();

    if ((cmdline.run_time_flag) && (cmdline.yaml_flag))
    {
	cmdline.run_time = cmdline.yaml_run_time;
	if (cmdline.run_time < 300)
	{
	    cmdline.run_time = 300;
	    cmdline.twarn = 1;
	}
	else
	    cmdline.twarn = 0;
    }
    /* 
     * Search for the workload component index.  Later this can be used to find the warm_time
     */
    for (j = 0; j < num_work_obj; j++)
    {
	if (my_strcasecmp
	    (client_obj[0].client_workload_name,
	     my_mix_table[j].workload_name) == 0)
	    break;
    }
    if ((cmdline.skip_init_flag || cmdline.yaml_skip_init)
	&& (cmdline.yaml_flag))
    {
	cmdline.skip_init = cmdline.yaml_skip_init;
    }
    if (cmdline.skip_init == 1)
    {
	const char *string = "**** Using an abbreviated INIT phase. ****\n";
	log_all (LOG_EXEC, string);
    }
    log_all (LOG_EXEC,
	     "     Test run time = %d seconds, Warmup = %d seconds.\n",
	     cmdline.run_time, my_mix_table[j].warm_time);

    log_all (LOG_EXEC, "     Results directory: %s\n",
	     cmdline.prime_results_dir);

    if (cmdline.Op_lat_flag)
    {
	log_all (LOG_EXEC, "     Op latency reporting activated\n");
    }

    if (cmdline.im_flag)
    {
	log_all (LOG_EXEC, "     Importing workloads from %s\n",
		 cmdline.itable);
    }

    log_all (LOG_EXEC_VERBOSE,
	     "     Size of internal prime comm structure = %lu\n",
	     sizeof (*g_mc));
    log_all (LOG_EXEC_VERBOSE,
	     "     Size of external prime comm structure = %lu\n",
	     sizeof (*g_emc));
    log_all (LOG_EXEC_VERBOSE,
	     "     Size of internal nodeManager comm structure = %lu\n",
	     sizeof (*g_nmc));
    log_all (LOG_EXEC_VERBOSE,
	     "     Size of external nodeManager comm structure = %lu\n",
	     sizeof (*g_enmc));
    log_all (LOG_EXEC_VERBOSE,
	     "     Size of internal client comm structure = %lu\n",
	     sizeof (*g_cc));
    log_all (LOG_EXEC_VERBOSE,
	     "     Size of external client comm structure = %lu\n\n",
	     sizeof (*g_ecc));

    if (cmdline.iflag)
	exit (0);

    set_timestamp_in_log_file ();
    set_timestamp_in_log_stdout ();

    log_all (LOG_EXEC, "Starting tests...\n");
}

/**
 * @brief Prime starts its listening thread
 *
 */
static void
prime_start_listening_thread (void)
{
    /*
     * Start prime listen 
     */
    log_all (LOG_COMM, "Start prime listen\n");

    get_my_server_socket (0, &prime_listen_socket, &prime_listen_port);
}

/**
 * @brief NodeManager comm memory allocation. 
 *
 */
static void
prime_allocate_comm_structures (void)
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
}

/**
 * @brief Prime logs its previous and current FSM state
 *
 */
static void
prime_log_state (void)
{
    log_all (LOG_FSM, "**** %s ---> %s ****\n",
	     _fsm_state_names[fsm_prev_state], _fsm_state_names[fsm_state]);
}

/**
 * @brief Entry point for the prime process 
*/
/*
 * __doc__
 * __doc__  Funcftion : void do_all_prime_work(void)
 * __doc__  Arguments: void 
 * __doc__  Performs : This is entry point of the prime process. It does the 
 * __doc__             following in order - 
 * __doc__   
 * __doc__             1) Initialize logging
* __doc__              2) Flag Validations
 * __doc__             3) Read token config file
 * __doc__             4) Allocate memory buffers for remote
 * __doc__                NodeManagers as well as other environment
 * __doc__             5) Start socket communication channel
 * __doc__             6) Launch nodeManager processes
 * __doc__
 */
void
do_all_prime_work (void)
{
    abort_ifnot_prime ();	/* Just a debug and sanity check */

    int whileloop = 1;
    int error = 0;
    int i = 0;
    int *nm_sockets = NULL, single_socket[1];

    fsm_state = PRIME_FSM_START;
    fsm_prev_state = PRIME_FSM_START;

    /* Allocate for prime's use in print_results */

    workloads =
	(struct work_object *) my_malloc (sizeof (struct work_object) *
					  cmdline.workload_count);
    /* Set global num_work_obj so that all called functions will know the 
     * number of active workloads.
     */
    num_work_obj = cmdline.workload_count;	/* global */
    /*
     * Typical FSM infinite loop
     */
    while (whileloop)
    {
	switch (fsm_state)
	{
	case PRIME_FSM_START:
	    prime_initialize_logging ();
	    log_file (LOG_DEBUG,
		      "Prime: number of workloads = %d\n", num_work_obj);

	    /*
	     * After logging is initialized, print copyright notice
	     */

	    reset_timestamp_in_log_stdout ();
	    reset_timestamp_in_log_file ();

	    print_copyright ();

	    set_timestamp_in_log_stdout ();
	    set_timestamp_in_log_file ();

	    prime_check_license_file ();
	    prime_validate_input_args ();
	    prime_read_token_config_file ();
	    prime_allocate_comm_structures ();
	    initialize_runtime_structures ();
	    prime_start_listening_thread ();

	    if (power_mon (SCHECK, "Baseline") == 0)
		power_mon (SRUN, "Baseline");

	    log_all (LOG_FSM, "Prime FSM starts:\n");
	    fsm_prev_state = PRIME_FSM_START;
#ifdef PRIME_ATTACH_DEBUGGER
	    fsm_state = PRIME_FSM_DEBUG_STATE;
#else
	    fsm_state = PRIME_FSM_SPAWN_NMS;
#endif
	    prime_log_state ();
	    break;

	case PRIME_FSM_DEBUG_STATE:
	    sleep (30);
	    fsm_prev_state = PRIME_FSM_DEBUG_STATE;
	    fsm_state = PRIME_FSM_SPAWN_NMS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_SPAWN_NMS:
	    num_nodeManagers = get_nodeManager_count ();
	    error = launch_nodeManagers (num_nodeManagers);
	    if (error)
		continue;

	    /* Finish comm linkage */
	    nm_sockets = server_wait_for_clients (prime_listen_socket,
						  num_nodeManagers);

	    fsm_prev_state = PRIME_FSM_SPAWN_NMS;
	    fsm_state = PRIME_FSM_JOIN_WAIT_NMS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_JOIN_WAIT_NMS:
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		/*
		 * Since nodeManagers have not sent 'Join' yet,
		 * we do not have relationship established between
		 * nm_id <-> socket number. So, we need to send
		 * sync OK message on the socket directly in this state
		 * Starting next state, this is not a problem!
		 */
		prime_send_sync_ok_on_socket (nm_sockets[i]);
		single_socket[0] = nm_sockets[i];

		error = server_select (0, single_socket,
				       1, 0, R_NODEMANAGER_JOIN);

		if (error)
		{
		    fsm_prev_state = fsm_state;
		    fsm_state = PRIME_FSM_ERROR_STATE;
		    prime_log_state ();
		    continue;
		}
	    }

	    fsm_prev_state = PRIME_FSM_JOIN_WAIT_NMS;
	    fsm_state = PRIME_FSM_VERSION_CHECK;
	    prime_log_state ();
	    break;

	case PRIME_FSM_VERSION_CHECK:
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		prime_send_version (i);
	    }

	    fsm_prev_state = PRIME_FSM_VERSION_CHECK;
	    fsm_state = PRIME_FSM_VERSION_CHECK_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_VERSION_CHECK_DONE:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, R_VERSION_CHECK_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    log_all (LOG_EXEC, "Version check successful: %s\n", git_version);

	    fsm_prev_state = PRIME_FSM_VERSION_CHECK_DONE;
	    fsm_state = PRIME_FSM_TOKENFILE_TO_NMS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_TOKENFILE_TO_NMS:
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		prime_send_token_config (i);
	    }

	    /*
	     * We no longer need g_nmc and g_emnc structures
	     * after this so free them to save memory footprint
	     */
	    free (g_nmc);
	    free (g_enmc);
	    g_nmc = NULL;
	    g_enmc = NULL;

	    fsm_prev_state = PRIME_FSM_TOKENFILE_TO_NMS;
	    fsm_state = PRIME_FSM_NMS_WAIT_SPAWN;
	    prime_log_state ();
	    break;

	case PRIME_FSM_NMS_WAIT_SPAWN:
	    log_all (LOG_EXEC,
		     "Waiting for each NodeManager to spawn client processes\n");
	    /*
	     * Prime will wait till all nodeManager, spawn
	     * required number of clients and get OK from them
	     */
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0,
				   R_NODEMANAGER_ALL_READY);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    fsm_prev_state = PRIME_FSM_NMS_WAIT_SPAWN;
	    fsm_state = PRIME_FSM_KEEPALIVE_WAIT;
	    prime_log_state ();
	    break;

	case PRIME_FSM_KEEPALIVE_WAIT:
	    nm_keepalive_sockets =
		server_wait_for_clients (prime_listen_socket,
					 num_nodeManagers);

	    error = netmist_start_threads (prime_keepalive_thread_handler,
					   prime_listen_thread_handler);
	    if (error)
	    {
		log_file (LOG_ERROR, "Failed to create threads");
	    }
	    fsm_prev_state = PRIME_FSM_KEEPALIVE_WAIT;
	    fsm_state = PRIME_FSM_GO;
	    prime_log_state ();
	    break;

	case PRIME_FSM_GO:
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		prime_send_go (i);
	    }

	    fsm_prev_state = PRIME_FSM_GO;
	    fsm_state = PRIME_FSM_GO_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_GO_DONE:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, R_GO_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }
	    fsm_prev_state = PRIME_FSM_GO_DONE;
	    fsm_state = PRIME_FSM_INIT_PHASE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_INIT_PHASE:
	    log_all (LOG_EXEC, "Starting INIT phase\n");
	    /*
	     * Let the external monitor know that we are entering 
	     * the INIT phase.
	     */
	    if (extern_mon (SCHECK, "INIT") == 0)
		extern_mon (SRUN, "INIT");

	    /*
	     * Let the user defined external monitor know that we 
	     * are entering the INIT phase.
	     */
	    if (u_extern_mon (SCHECK, "INIT") == 0)
		u_extern_mon (SRUN, "INIT");

	    prime_send_command (INIT_PHASE, "Init");

	    log_all (LOG_EXEC, "Waiting to finish initialization.\n");

	    fsm_prev_state = PRIME_FSM_INIT_PHASE;
	    fsm_state = PRIME_FSM_INIT_PHASE_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_INIT_PHASE_DONE:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, INIT_PHASE_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    log_all (LOG_EXEC, "Initialization finished\n");

	    fsm_prev_state = PRIME_FSM_INIT_PHASE_DONE;
	    fsm_state = PRIME_FSM_ASK_INIT_RESULTS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_ASK_INIT_RESULTS:
	    prime_send_command (ASK_INIT_RESULTS, "INIT results");

	    fsm_prev_state = PRIME_FSM_ASK_INIT_RESULTS;
	    fsm_state = PRIME_FSM_GET_INIT_RESULTS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_GET_INIT_RESULTS:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, num_clients,
				   R_SENDING_RESULTS);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    reset_timestamp_in_log_file ();
	    reset_timestamp_in_log_stdout ();

	    print_results_in_csv (ASK_INIT_RESULTS);

	    set_timestamp_in_log_file ();
	    set_timestamp_in_log_stdout ();

	    fsm_prev_state = PRIME_FSM_GET_INIT_RESULTS;
	    fsm_state = PRIME_FSM_TIME_SYNC;
	    prime_log_state ();
	    break;

	case PRIME_FSM_TIME_SYNC:
	    start_time = getepochtime ();
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		prime_do_time_sync (i);
	    }
	    fsm_prev_state = PRIME_FSM_TIME_SYNC;
	    fsm_state = PRIME_FSM_TIME_SYNC_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_TIME_SYNC_DONE:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, R_TIME_SYNC_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    end_time = getepochtime ();
	    total_rrt_time = end_time - start_time;
	    avg_rrt_time = total_rrt_time / num_nodeManagers;
	    c_start_epoch = ((getepochtime () * (double) 1000.0) +
			     ((total_rrt_time * 1000) * MAX_MSG_COST))
		/ (double) 1000.00;

	    log_all (LOG_FSM_VERBOSE, "Total RRT time %lf\n", total_rrt_time);
	    log_all (LOG_FSM_VERBOSE, "Avg RRT time %lf\n", avg_rrt_time);

	    fsm_prev_state = PRIME_FSM_TIME_SYNC_DONE;
	    fsm_state = PRIME_FSM_EPOCH;
	    prime_log_state ();
	    break;

	case PRIME_FSM_EPOCH:
	    for (i = 0; i < num_nodeManagers; i++)
	    {
		prime_send_epoch (i);
	    }

	    fsm_prev_state = PRIME_FSM_EPOCH;
	    fsm_state = PRIME_FSM_EPOCH_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_EPOCH_DONE:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, R_EPOCH_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    fsm_prev_state = PRIME_FSM_EPOCH_DONE;
	    fsm_state = PRIME_FSM_WARMUP_PHASE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_WARMUP_PHASE:
	    log_all (LOG_EXEC, "Starting WARM phase\n");
	    /* Let the external monitor script know we're entering RUN */
	    if (extern_mon (SCHECK, "WARMUP") == 0)
		extern_mon (SRUN, "WARMUP");

	    /* Let the user defined monitor script know we're entering 
	     * RUN */
	    if (u_extern_mon (SCHECK, "WARMUP") == 0)
		u_extern_mon (SRUN, "WARMUP");

	    /* Consumer defined stat script. 
	     *Here vendor can zero counters */
	    if (extern_stat (SCHECK, "ZERO") == 0)
		extern_stat (SRUN, "ZERO");

	    prime_send_command (WARM_PHASE, "warmup");

	    fsm_prev_state = PRIME_FSM_WARMUP_PHASE;
	    fsm_state = PRIME_FSM_WARMUP_PHASE_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_WARMUP_PHASE_DONE:
	    log_all (LOG_EXEC, "Waiting for WARMUP phase to finish\n");

	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, WARM_PHASE_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    fsm_prev_state = PRIME_FSM_WARMUP_PHASE_DONE;
	    fsm_state = PRIME_FSM_RUN_PHASE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_RUN_PHASE:
	    log_all (LOG_EXEC, "Starting RUN phase\n");
	    /* Let the external monitor script know we're entering RUN */
	    if (extern_mon (SCHECK, "RUNNING") == 0)
		extern_mon (SRUN, "RUNNING");

	    /* Let the user defined monitor script know we're entering 
	     * RUN */
	    if (u_extern_mon (SCHECK, "RUNNING") == 0)
		u_extern_mon (SRUN, "RUNNING");

	    /* Consumer defined stat script. 
	     *Here vendor can zero counters */
	    if (extern_stat (SCHECK, "ZERO") == 0)
		extern_stat (SRUN, "ZERO");

	    fsm_prev_state = PRIME_FSM_RUN_PHASE;
	    fsm_state = PRIME_FSM_RUN_PHASE_DONE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_RUN_PHASE_DONE:
	    log_all (LOG_EXEC, "Waiting for RUN phase to finish\n");

	    error = server_select (0, nm_sockets,
				   num_nodeManagers, 0, RUN_PHASE_DONE);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    log_all (LOG_EXEC, "Tests finished\n");

	    fsm_prev_state = PRIME_FSM_WARMUP_PHASE_DONE;
	    fsm_state = PRIME_FSM_ASK_RESULTS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_ASK_RESULTS:
	    prime_send_command (ASK_RESULTS, "Asking results");

	    fsm_prev_state = PRIME_FSM_ASK_RESULTS;
	    fsm_state = PRIME_FSM_GET_RESULTS;
	    prime_log_state ();
	    break;

	case PRIME_FSM_GET_RESULTS:
	    error = server_select (0, nm_sockets,
				   num_nodeManagers, num_clients,
				   R_SENDING_RESULTS);
	    if (error)
	    {
		fsm_prev_state = fsm_state;
		fsm_state = PRIME_FSM_ERROR_STATE;
		prime_log_state ();
		continue;
	    }

	    reset_timestamp_in_log_file ();
	    reset_timestamp_in_log_stdout ();

	    print_results ();
	    print_results_in_csv (ASK_RESULTS);

	    set_timestamp_in_log_file ();
	    set_timestamp_in_log_stdout ();

	    fsm_prev_state = PRIME_FSM_GET_RESULTS;
	    fsm_state = PRIME_FSM_COMPLETE_PHASE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_COMPLETE_PHASE:
	    log_all (LOG_EXEC, "Client results ready\n");
	    /* Consumer defined monitor script */
	    if (extern_mon (SCHECK, "STOP") == 0)
		extern_mon (SRUN, "STOP");

	    /* User defined monitor script */
	    if (u_extern_mon (SCHECK, "STOP") == 0)
		u_extern_mon (SRUN, "STOP");

	    /* Consumer defined stat script. 
	     * This is so vendor can collect stats */
	    if (extern_stat (SCHECK, "STOP") == 0)
		extern_stat (SRUN, "STOP");

	    fsm_prev_state = PRIME_FSM_COMPLETE_PHASE;
	    fsm_state = PRIME_FSM_SHUTDOWN_PHASE;
	    prime_log_state ();
	    break;

	case PRIME_FSM_SHUTDOWN_PHASE:
	    prime_send_command (SHUTDOWN_PHASE, "shutdown");

	    prime_shutdown ();

	    fsm_prev_state = PRIME_FSM_SHUTDOWN_PHASE;
	    fsm_state = PRIME_FSM_FINISHED;
	    prime_log_state ();
	    break;

	case PRIME_FSM_FINISHED:
	    exit (0);

	case PRIME_FSM_ERROR_STATE:
	    prime_send_kill_procs ();
	    prime_shutdown ();

	    fsm_prev_state = PRIME_FSM_ERROR_STATE;
	    fsm_state = PRIME_FSM_FINISHED;
	    prime_log_state ();
	    exit (99);
	    break;

	default:
	    whileloop = 0;
	    break;
	}
    }
}

/**
 * @brief Function to send SIGINT to nodemanagers
 *
 */
void
prime_send_sigint (void)
{
    int i;

    /*
     * Initialize this message only once
     */
    init_k_send_sigint_command (&k_send_sigint, 4, 0);

    keepalive_int_to_ext (&k_send_sigint, &k_ext_send_sigint);

    for (i = 0; i < num_nodeManagers; i++)
    {
	if((nm_keepalive_sockets == NULL ) || (nm_keepalive_sockets[i] == 0))
	{
	    log_all (LOG_ERROR, "Prime: Unable to send over NULL socket fd.\n");
	    continue;
	}
	socket_send (nm_keepalive_sockets[i], (char *) &k_ext_send_sigint,
		     sizeof (struct ext_keepalive_command));

	log_all (LOG_THREAD, "Sent SIGINT to NodeManager %u\n", i);
    }
}
