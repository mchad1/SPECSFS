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
 */
#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#include "./copyright.txt"
#include "./license.txt"

#include<stdio.h>
#include<stdarg.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<errno.h>
#if !defined(WIN32)
#include<unistd.h>
#if !defined(_aix_)
#include<getopt.h>
#endif
#else
extern char *optarg;
#include "../win32lib/win32_getopt.h"
#include <WinSock2.h>
#endif

#include<ctype.h>
#include<time.h>
#if !defined(WIN32)
#include<sys/time.h>
#else
#include "../win32lib/win32_sub.h"
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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#else
#include <signal.h>
#endif

#if defined(_bsd_)
#include <signal.h>
#endif

#if defined(_solaris_) || defined(_bsd_)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif


#include "netmist_defines.h"
#include "netmist_utils.h"
#include "netmist_structures.h"
#include "netmist_logger.h"

extern int NM_phase;;
_INTERNAL_CFG internal_cfg;
_CMD_LINE cmdline;

extern int signal_flag;
extern char *netmist_strerror (int);

extern void R_exit (int val);

extern int netmist_rand (void);
extern void nap (int select_msecs);
extern void prime_store_results (int client_id, struct prime_command *mc);

void process_error (struct prime_command *);

extern int netmist_get_error (void);
extern int netmist_get_sock_error (void);
extern char *netmist_get_sock_errstr (int);
void flip_f2b_slashes (char *, char *);
extern long long curr_client_maxrss, curr_oublock, curr_inblock;
extern float curr_us_time;;
extern volatile int cpu_warning;
extern volatile double cpu_percent_busy;

extern int import_yaml (char *);

#if defined(WIN32)
int w_gettimeofday (struct timeval *, struct w_timezone *);
#endif


/**
 * @brief Initialize the command line structure.
 */
void
init_command_line_args (void)
{
    cmdline.log_flag = 0;
    cmdline.log_filename[0] = '\0';
    cmdline.m_flag = 1;		/* Initialize as Prime */
    cmdline.sharing_flag = 0;
    cmdline.orflag = 0;
    cmdline.op_rate = 0;
    cmdline.pdsm_mode = 0;
    cmdline.pdsm_interval = 1;
    cmdline.workload_count = MAX_WORK_OBJ;
    cmdline.flush_flag = 1;
    cmdline.heartbeat_flag = 0;
    cmdline.Op_lat_flag = 0;
    cmdline.fd_caching_limit = 0;
    cmdline.Q_flag = 0;
    cmdline.client_files = 0;
    cmdline.fpd_flag = 0;
    cmdline.T_flag = 0;
    cmdline.client_dirs = 0;
    cmdline.nflag = 0;
    cmdline.client_id = 0;
    cmdline.real_client_id = 0;
    cmdline.gflag = 0;
    cmdline.gg_offset = 0;
    cmdline.fsize = FSIZE;
    cmdline.dflag = 0;
    cmdline.ppc = 0;
    cmdline.use_rsh = 0;
    cmdline.pflag = 0;
    cmdline.localflag = 0;
    cmdline.hflag = 0;
    cmdline.eflag = 0;
    cmdline.iflag = 0;
    cmdline.p_ext_mon_flag = 0;
    cmdline.p_ext_mon_arg_flag = 0;
    cmdline.run_time = 300;
    cmdline.run_time_flag = 0;
    cmdline.wflag = 0;
    cmdline.warm_time = 300;
    cmdline.twarn = 0;
    cmdline.wwarn = 0;
    cmdline.Mflag = 0;
    cmdline.Password_flag = 0;
    my_strncpy (cmdline.password, "abcd_1234", MAXNAME);
    my_strncpy (cmdline.unix_pdsm_file, "_", MAXNAME);
    my_strncpy (cmdline.unix_pdsm_control_file, "_", MAXNAME);
    my_strncpy (cmdline.windows_pdsm_file, "_", MAXNAME);
    my_strncpy (cmdline.windows_pdsm_control_file, "_", MAXNAME);
    cmdline.Usr_dom_flag = 0;
    cmdline.ipv6_enable = 0;
    cmdline.Uflag = 0;
    cmdline.WUflag = 0;
    cmdline.Rflag = 0;
    cmdline.Vflag = 0;
    cmdline.my_child_pid = 0;
    cmdline.bflag = 0;
    cmdline.client_memsize_mb = 1024;
    cmdline.Bflag = 0;
    cmdline.agg_set_memsize_mb = 1024;
    cmdline.kflag = 0;
    cmdline.vflag = 0;
    cmdline.lflag = 0;
    cmdline.Dflag = 0;
    cmdline.Pflag = 0;
    cmdline.do_validate = 1;
    cmdline.prime_p = 0;
    cmdline.cleanup_flag = 1;
    cmdline.skip_init = 0;
    cmdline.skip_init_flag = 0;
    cmdline.dump_files_flag = 0;
    cmdline.tracelevel = 0x105;
    cmdline.keepalive = 60;
    cmdline.vfs_so[0] = '\0';
    cmdline.vfs_arg[0] = '\0';
}

/**
 * @brief Return true if I am the Prime.
 */
int
is_prime (void)
{
    return (internal_cfg.netmist_role == NETMIST_PRIME);
}

/**
 * @brief Return true if I am a NodeManager.
 */
int
is_nodeManager (void)
{
    return (internal_cfg.netmist_role == NETMIST_NODEMANAGER);
}

/**
 * @brief Return true if I am a client.
 */
int
is_client (void)
{
    return (internal_cfg.netmist_role == NETMIST_CLIENT);
}

/**
 * @brief Abort the benchmark if i am not the Prime.
 */
void
abort_ifnot_prime (void)
{
    if (internal_cfg.netmist_role != NETMIST_PRIME)
    {
	printf ("\nDebug assertion fail %s\n", __FUNCTION__);
	exit (1);
    }
}

/**
 * @brief Abort the benchmark if i am not a nodeManager.
 */
void
abort_ifnot_nodeManager (void)
{
    if (internal_cfg.netmist_role != NETMIST_NODEMANAGER)
    {
	printf ("\nDebug assertion fail %s\n", __FUNCTION__);
	exit (1);
    }
}

/**
 * @brief Abort the benchmark if i am not a client.
 */
void
abort_ifnot_client (void)
{
    if (internal_cfg.netmist_role != NETMIST_CLIENT)
    {
	printf ("\nDebug assertion fail %s\n", __FUNCTION__);
	exit (1);
    }
}

/**
 * @brief Set my role in the world.
 *
 * @param command_line_m_flag : Specifies my role in the Universe.
 */
void
set_netmist_role (int command_line_m_flag)
{
    if (command_line_m_flag == 1)
    {
	internal_cfg.netmist_role = NETMIST_PRIME;
    }
    else if (command_line_m_flag == 2)
    {
	internal_cfg.netmist_role = NETMIST_NODEMANAGER;
    }
    else if (command_line_m_flag == 3)
    {
	internal_cfg.netmist_role = NETMIST_CLIENT;
    }
    else
    {
	printf ("\nInvalid Netmist role value %d in function %s\n",
		command_line_m_flag, __FUNCTION__);
	exit (1);
    }
}


#if defined(WIN32)
extern int parent;
#endif
int got_wl;

/** 
 * @brief  Parse the command line.
 *
 * @param argc : Argument count.
 * @param argv : Pointer to args.
 */
void
parse_command_line (int argc, char **argv)
{
    int cret;
    char *subarg;
    char temp_buf[MAXNAME];

    while ((cret =
	    getopt (argc, argv,
		    "zNKYXFGDLvihu8431:2:7:9:c:o:O:Q:T:H:l:S:n:r:s:p:e:g:f:j:t:M:R:U:b:B:d:P:w:W:EI:a:y:x:J:V:A:Z:q:m:k:C:+: "))
	   != EOF)
    {
	switch (cret)
	{
	case 'x':
	    cmdline.skip_init_flag = 1;
	    cmdline.skip_init = atoi (optarg);
	    break;
	case 'F':		/* Disable flushes */
	    cmdline.flush_flag = 0;
	    break;
	case 'G':		/* enable heartbeat messages */
	    cmdline.heartbeat_flag = 1;
	    break;
	case 'N':		/* enable op_latency collection */
	    cmdline.Op_lat_flag = 1;
	    break;
	case 'J':		/* limit file descriptor caching to this many */
	    cmdline.fd_caching_limit = atoi (optarg);
	    break;
	case 'n':		/* Unique client id */
	    cmdline.client_id = atoi (optarg);
	    cmdline.real_client_id = cmdline.client_id;
	    cmdline.nflag = 1;
	    break;
	case 'a':		/* Misaligned 4k offset */
	    cmdline.gg_offset = atoi (optarg);
	    cmdline.gg_flag = 1;
	    break;
	case 'u':		/* Use a different remote shell */
	    cmdline.use_rsh++;
	    break;
	case 'p':		/* Directory to do work */
	    cmdline.pflag++;
	    my_strncpy (cmdline.workdir, optarg, MAXNAME);
	    cmdline.workdir[strlen (optarg)] = 0;
	    break;
	case 'z':		/* Use sh to run locaally */
	    cmdline.localflag = 1;
	    break;
	case 'h':		/* Help menu */
	    usage ();
	    cmdline.hflag = 1;
	    exit (0);
	case 'e':		/* Path to executable for the test */
	    cmdline.eflag = 1;
	    my_strncpy (cmdline.execdir, optarg, MAXNAME);
	    cmdline.execdir[strlen (optarg)] = 0;
	    break;
	case 'm':		/* Prime/ NodeManager/client controlling process flag */
	    cmdline.m_flag = atoi (optarg);
	    set_netmist_role (cmdline.m_flag);
	    break;
	case 'i':		/* Show but do not run */
	    cmdline.iflag = 1;
	    break;
	case 'g':		/* Name of the token based file that contains */
	    /* the client config info */
	    cmdline.gflag = 1;
	    my_strncpy (cmdline.client_tok_filename, optarg, MAXNAME);
	    cmdline.client_tok_filename[strlen (optarg)] = 0;
	    break;
	case 'f':		/* Name of the external monitor script */
	    cmdline.p_ext_mon_flag = 1;
	    my_strncpy (cmdline.u_external_script_name, optarg, MAXNAME);
	    cmdline.u_external_script_name[strlen (optarg)] = 0;
	    break;
	case 'j':		/* Arguments to external monitor script */
	    cmdline.p_ext_mon_arg_flag++;
	    my_strncpy (cmdline.u_external_script_args, optarg, MAXNAME);
	    cmdline.u_external_script_args[strlen (optarg)] = 0;
	    break;
	case 't':		/* Timed test for this many seconds */
	    cmdline.run_time = atoi (optarg);
	    cmdline.run_time_flag = 1;
#if defined(FAST)
	    if (cmdline.run_time < 60)
	    {
		cmdline.run_time = 60;
		cmdline.twarn = 1;
	    }
#else
	    if (cmdline.run_time < 300)
	    {
		cmdline.run_time = 300;
		cmdline.twarn = 1;
	    }
#endif
	    break;
	case 'M':		/* Name of the prime */
	    cmdline.Mflag = 1;
	    my_strncpy (cmdline.prime_name, optarg, MAXHNAME);
	    cmdline.prime_name[strlen (optarg)] = 0;
	    break;
	case 'A':		/* Password */
	    cmdline.Password_flag = 1;
	    my_strncpy (cmdline.password, optarg, MAXNAME);
	    cmdline.password[strlen (optarg)] = 0;
	    if (strcmp (cmdline.password, "_") == 0)
		cmdline.Password_flag = 0;	/* Invalid cmdline.password */
	    my_strncpy (cmdline.mypassword, cmdline.password, MAXNAME);
	    cmdline.mypassword[strlen (cmdline.password)] = 0;
	    break;
	case 'Z':		/* User/domain */
	    cmdline.Usr_dom_flag = 1;
	    my_strncpy (cmdline.usr_dom, optarg, MAXNAME);
	    cmdline.usr_dom[strlen (optarg)] = 0;
	    break;
	case 'y':		/* Enable ipv6 */
	    cmdline.ipv6_enable = atoi (optarg);
	    break;
	case 'U':		/* Name of the child log directory */
	    cmdline.Uflag = 1;
	    my_strncpy (cmdline.client_log_dir, optarg, MAXNAME);
	    cmdline.client_log_dir[strlen (optarg)] = 0;
	    break;
	case 'R':		/* Name of the primes results directory */
	    cmdline.Rflag = 1;
	    my_strncpy (cmdline.prime_results_dir, optarg, MAXNAME);
	    cmdline.prime_results_dir[strlen (optarg)] = 0;
	    break;
	case 'V':		/* PID of child to kill */
	    cmdline.Vflag = 1;
	    cmdline.my_child_pid = atoi (optarg);
	    break;
	case 'k':		/* kill the tests on the clients and exit */
	    cmdline.kflag = 1;
	    cmdline.kill_pid = atoi (optarg);
	    break;
	case 'v':		/* Version info only */
	    cmdline.vflag = 1;
	    break;
	case 'L':		/* Local file system flag for debug */
	    cmdline.lflag = 1;
	    my_strncpy (cmdline.var_opt, " -L ", 256);
	    cmdline.var_opt[strlen (" -L ")] = 0;
	    break;
	case 'l':		/* Log file */
	    cmdline.log_flag = 1;
	    my_strncpy (cmdline.log_filename, optarg, MAXNAME);
	    cmdline.log_filename[strlen (optarg)] = 0;
	    break;
	case 'q':		/* license key file */
	    my_strncpy (cmdline.license_filename, optarg, MAXNAME);
	    cmdline.license_filename[strlen (optarg)] = 0;
	    break;
	case 'D':		/* Distribute the benchmark to the clients */
	    cmdline.Dflag = 1;
	    break;
	case 'P':		/* Port the prime is listening on */
	    cmdline.Pflag = 1;
	    cmdline.prime_p = atoi (optarg);
	    break;
	case 'w':		/* Warmup time */
	    cmdline.wflag = 1;
	    cmdline.warm_time = atoi (optarg);
	    if (cmdline.warm_time < 60)
	    {
		cmdline.warm_time = 60;
		cmdline.wwarn = 1;
	    }
	    break;
	case 'E':		/* export current table */
	    fprintf (stderr,
		     "Usage warning: -E deprecated. Now using YAML configuration file.\n");
	    dump_table ();
	    exit (0);
	    break;
	case 'K':		/* Cleanup flag */
	    cmdline.cleanup_flag = 0;
	    break;
	case 'Y':		/* Dump files flag */
	    cmdline.dump_files_flag = 1;
	    break;
	case '2':		/* Dump files flag */
	    cmdline.tracelevel = atoi (optarg);
	    if (cmdline.tracelevel < 0)
	    {
		log_stdout (LOG_ERROR,
			    "Negative tracelevel not supported.\n");
		exit (1);
	    }
	    break;
	    /* 
	     * Note/WARNING any arguments that have backslashes MUST be a regular 
	     * optarg, or carfully quoted.
	     */
	case '1':		/* Name of the windows child log directory */
	    memset (temp_buf, 0, MAXNAME);
	    cmdline.WUflag = 1;
	    my_strncpy (temp_buf, optarg, MAXNAME);
	    flip_f2b_slashes (cmdline.client_windows_log_dir, temp_buf);
	    cmdline.client_windows_log_dir[strlen (temp_buf)] = 0;
	    break;
#if defined(WIN32)
	case '8':
	    parent = 0;
	    break;
#endif
	case '9':		/* heartbeat period */
	    cmdline.keepalive = atoi (optarg);
	    if (cmdline.keepalive < 10)
	    {
		log_stdout (LOG_ERROR, "Heartbeats need to be > 1 min\n");
		cmdline.keepalive = 60;
	    }

	    break;
	case 'X':		/* Sharing flag */
	    cmdline.sharing_flag = 1;
	    break;
	case 'H':		/* Argument is hostname of the PIT */
	    my_strncpy (cmdline.pit_hostname, optarg, 40);
	    cmdline.pit_hostname[strlen (optarg)] = 0;
	    break;
	case 'W':		/* Argument is workload name */
	    my_strncpy (cmdline.my_tmp_workload_name, optarg, MAXNAME);
	    cmdline.my_tmp_workload_name[strlen (optarg)] = 0;
	    break;

	case 'S':		/* Argument is port of the PIT */
	    my_strncpy (cmdline.pit_service, optarg, 8);
	    cmdline.pit_service[strlen (optarg)] = 0;
	    break;

	case 'C':
	    snprintf (cmdline.vfs_so, sizeof cmdline.vfs_so, "%s", optarg);
	    break;

	    /* 
	     * The + operator is for the new extended options mechanism 
	     * Syntax is -+ followed by option leter, and if the option
	     * takes an operand then it is implemented below.
	     * NOTE: The subarg must NEVER contain any backslashes, without proper quoting !!! 
	     * They will get stripped, regardless of how many one has. 
	     */
	case '+':
	    switch (*((char *) optarg))
	    {
	    case 'm':		/* pdsm_mode */
		subarg = argv[optind++];
		cmdline.pdsm_mode = atoi (subarg);
		if (cmdline.pdsm_mode < 0)
		    cmdline.pdsm_mode = 0;
		break;
	    case 'i':		/* pdsm_interval */
		subarg = argv[optind++];
		cmdline.pdsm_interval = atoi (subarg);
		if (cmdline.pdsm_interval < 1)
		    cmdline.pdsm_interval = 1;
		break;
	    case 'j':		/* Filename for Windows PDSM log */
		subarg = argv[optind++];
		if (strcmp (subarg, "_") != 0)
		{
		    my_strncpy (temp_buf, subarg, MAXNAME);
		    flip_f2b_slashes (cmdline.windows_pdsm_file, temp_buf);
		    cmdline.
			windows_pdsm_file[strlen (cmdline.windows_pdsm_file)]
			= 0;
		}
		break;
	    case 'k':		/* Filename for Windows PDSM control file */
		subarg = argv[optind++];
		if (strcmp (subarg, "_") != 0)
		{
		    my_strncpy (temp_buf, subarg, MAXNAME);
		    flip_f2b_slashes (cmdline.windows_pdsm_control_file,
				      temp_buf);
		    cmdline.
			windows_pdsm_file[strlen
					  (cmdline.
					   windows_pdsm_control_file)] = 0;
		}
		break;
	    case 'w':		/* Number of workloads */
		got_wl++;
		subarg = argv[optind++];
		cmdline.workload_count = atoi (subarg);
		if (cmdline.workload_count < 0)
		    cmdline.workload_count = 0;
		break;
	    case 'p':		/* Path to pdsm file */
		subarg = argv[optind++];
		if (strcmp (subarg, "_") != 0)
		{
		    my_strncpy (cmdline.unix_pdsm_file, subarg, MAXNAME);
		}
		break;
	    case 'C':		/* Path to remote control pdsm file */
		subarg = argv[optind++];
		if (strcmp (subarg, "_") != 0)
		{
		    my_strncpy (cmdline.unix_pdsm_control_file, subarg,
				MAXNAME);
		}
		break;
	    case 'X':		/* VFS arg */
		subarg = argv[optind++];
		snprintf (cmdline.vfs_arg, sizeof cmdline.vfs_arg, "%s",
			  subarg);
		break;
	    case 'y':		/* Import workload from yaml file */
		subarg = argv[optind++];
		cmdline.im_flag = 1;
		cmdline.yaml_flag = 1;
		my_strncpy (cmdline.itable, subarg, MAXNAME);
		cmdline.itable[strlen (subarg)] = 0;
		import_yaml (cmdline.itable);
		break;
	    }

	}
    }
}

/*
 * @brief Safe version of strncpy() that does not pad with nulls to size N
 *
 * @param dest Pointer to buffer
 * @param src  Pointer to input buffer
 * @param size Maximum size of bytes to copy
 *
 * __doc__
 * __doc__  Function : my_strncpy( char *, char *, size_t )
 * __doc__  Arguments: Pointer to dest, src, and max bytes to copy.
 * __doc__  Returns  : char *
 * __doc__  Performs : Safely copys strings with a max byte count of N
 * __doc__             
 * __doc__             Assumes that dest has sufficient space for an 
 * __doc__             additional byte of NUL, if needed for the case where
 * __doc__             strlen(src) >= space allocated for dest.
 *
 * To whoever it was that wrote strncpy() for POSIX, geeze what a squandering
 * mess.  Think about this:  strncpy(dest,"hi",16*1024*1024)
 * Yeah, copy two bytes and then burn the CPU to the ground, zero
 * filling space after the string was terminated.  REALLY DUMB !!!
 */
char *
my_strncpy (char *dest, char *src, size_t n)
{
    size_t i;


    for (i = 0; i < n && src[i] != '\0'; i++)
	dest[i] = src[i];
    /* Copy string */
    if (i < n)			/* i was already incremented via post inc */
    {
	dest[i] = '\0';
    }
    else
    {
	/* max size > 0 */
	if (n > 0)
	{
	    /* Terminate string at dest[max] Assumes that the dest 
	       memory space is strlen(src) +1 byte for the null 
	     */
	    dest[n - 1] = '\0';
	}
    }
    return dest;
}

/**
 * @brief Look for a substring, ignoring upper/lower case 
 *
 * @param s : String to search 
 * @param find : string to find.
 */
/*
 * __doc__
 * __doc__  Function : char * my_strcasestr(char *s, char *find)
 * __doc__  Arguments: char *string, char *: string to fine.
 * __doc__  Returns  : char *: pointer to the location of the search string.
 * __doc__  Performs : Look for a substring, ignoring upper/lower case 
 * __doc__
 */
char *
my_strcasestr (char *s, char *find)
{
    char c, sc;
    size_t len;

    if ((c = *find++) != 0)
    {
	c = tolower ((unsigned char) c);
	len = strlen (find);
	do
	{
	    do
	    {
		if ((sc = *s++) == 0)
		    return (NULL);
	    }
	    while ((char) tolower ((unsigned char) sc) != c);
	}
	while (strncasecmp (s, find, len) != 0);
	s--;
    }
    return ((char *) s);
}

/**
 * @brief Compare a string while ignoring the upper/lower case.
 *
 * @param s1 : First of 2 strings to compare
 * @param s2 : Second of 2 strings to compare
 */
/*
 * __doc__
 * __doc__  Function : int my_strcasecmp(char *s1, char *s2)
 * __doc__  Arguments: char *string1, char *string2
 * __doc__  Returns  : 0 for same, difference for when mismatch.
 * __doc__  Performs : Compare a string while ignoring the upper/lower case.
 * __doc__
 */
int
my_strcasecmp (char *s1, char *s2)
{
    unsigned char *uns1 = (unsigned char *) s1;
    unsigned char *uns2 = (unsigned char *) s2;

    while (tolower (*uns1) == tolower (*uns2++))
	if (*uns1++ == '\0')
	    return (0);
    return (tolower (*uns1) - tolower (*--uns2));
}

/**
 * @brief Function that implements allocation of memory, and 
 *        zero fills it.
 *
 * @param  size : size to allocate
 */
/*
 * __doc__
 * __doc__  Function : void * my_malloc(size)
 * __doc__  Arguments: int size
 * __doc__  Returns  : pointer to the allocated memory.
 * __doc__  Performs : Function that implements allocation of memory, and 
 * __doc__             zero fills it.
 * __doc__
 */
void *
my_malloc (size_t size)
{
    void *tmp;
    long long my_size;
    int error_value = 0;

    my_size = (long long) size;

    if (my_size > 1024)
    {
	log_file (LOG_MEMORY, "** %s: Allocating %lld bytes **\n",
		  __FUNCTION__, my_size);
    }

    tmp = (void *) malloc (size);
    if (tmp == (void *) 0)
    {
	if (is_client () || is_nodeManager ())	/* If I'm a remote child */
	{
	    log_file (LOG_ERROR,
		      "Malloc failure. Trying to allocate %lld bytes of memory in the NM or Client.\n",
		      my_size);
	    error_value = netmist_get_error ();
	    R_exit (error_value);
	}
	else
	{			/* If I'm the Prime */
	    log_all (LOG_ERROR,
		     "Malloc failure. Trying to allocate %lld bytes of memory in the Prime.\n",
		     my_size);
	    exit (199);
	}
    }

    memset ((void *) tmp, 0, (size_t) size);
    return (tmp);
}

/**
 * @brief Function that implements allocation of memory
 *
 * @param  size : size to allocate
 */
/*
 * __doc__
 * __doc__  Function : void * my_malloc2(size)
 * __doc__  Arguments: int size
 * __doc__  Returns  : pointer to the allocated memory.
 * __doc__  Performs : Function that implements allocation of memory
 * __doc__
 */
void *
my_malloc2 (size_t size)
{
    void *tmp;
    long long my_size;

    my_size = (long long) size;

    if (my_size > 1024)
    {
	log_file (LOG_MEMORY, "** %s: Allocating %lld bytes **\n",
		  __FUNCTION__, my_size);
    }

    tmp = (void *) 1;
    tmp = (void *) malloc (size);
    if (tmp == (void *) 0)
    {
	if (is_client () || is_nodeManager ())	/* If I'm a remote child */
	{
	    log_file (LOG_ERROR,
		      "Malloc failure. Trying to allocate %lld bytes of memory in the NM or Client.\n",
		      my_size);
	    R_exit (netmist_get_error ());
	}
	else
	{			/* If I'm the Prime */
	    log_all (LOG_ERROR,
		     "Malloc failure. Trying to allocate %lld bytes of memory in the Prime.\n",
		     my_size);
	    exit (199);
	}
    }

    return (tmp);
}

/**
 * @brief Wrapper function to free the memory
 *
 * @param  ptr: pointer to free
 * @param  size: size of the pointer for memory tracing
 */
/*
 * __doc__
 * __doc__  Function : void my_free(ptr, size)
 * __doc__  Arguments: void *ptr, int size
 * __doc__  Returns  : None
 * __doc__  Performs : Wrapper function to free the memory
 * __doc__
 */
void
my_free (void *ptr, size_t size)
{
    long long my_size = (long long) size;
    log_file (LOG_MEMORY, "** %s: Freeing %lld bytes **\n", __FUNCTION__,
	      my_size);

    free (ptr);
}

/**
 * @brief Detect empty lines.
 *
 * @param buf : Pointer to the string.
 */
/*
 * __doc__
 * __doc__  Function : int is_empty_line(char *)
 * __doc__  Arguments: char *buf: Pointer to the string.
 * __doc__  Returns  : Int: true or false
 * __doc__  Performs : Detect empty lines.
 * __doc__
 */
int
is_empty_line (char *buf)
{
    while (buf != NULL && *buf != '\0')
    {
	if (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == '\r')
	{
	    buf++;
	}
	else
	    return 0;
    }
    return 1;
}

/**
 * @brief Detect comment lines.
 *
 * @param buf : Pointer to a string.
 */
/*
 * __doc__
 * __doc__  Function : int is_comment(char *)
 * __doc__  Arguments: char *buf: Pointer to a string.
 * __doc__  Returns  : int true or false
 * __doc__  Performs : Detect comment lines.
 * __doc__
 */
int
is_comment (char *buf)
{
    if (buf != NULL && buf[0] == '#')
	return 1;
    else
	return 0;
}

/**
 * @brief Check to see if one has access to a given directory. 
 *        Used to validate that the current specified cmdline.workdir 
 *        is accessible.
 *
 * @param where : Directory string.
 */
/* 
 * __doc__
 * __doc__  Function : int check_dir_w(char *where)
 * __doc__  Arguments: char *where: Directory string.
 * __doc__  Returns  : int success or failure.
 * __doc__  Performs : Check to see if one has access to a given directory. 
 * __doc__             Used to validate that the current specified cmdline.workdir 
 * __doc__             is accessible.
 * __doc__
 */
int
check_dir_w (char *where)
{
#if defined(WIN32)
    if (GetFileAttributes (where) != INVALID_FILE_ATTRIBUTES)
#else
    if (access (where, W_OK) == 0)
#endif
	return (0);
    else
	return (1);
}

/**
 * @brief Trim leading spaces from a string.
 *
 * @param string : Pointer to the string.
 */

void
trim_leading_spaces (char *string)
{
    int i = 0, j = 0;

    while (string[i] == ' ')
	i++;

    if (i == 0)
	return;

    while (string[j + i] != '\0')
    {
	string[j] = string[j + i];
	j++;
    }

    string[j] = '\0';
}

/**
 * @brief This function takes a host name and figures out the 
 *        ip address and sticks this address in the buffer that 
 *        was provided. This function is IPV6 aware and can 
 *        return from 4 bytes to 16 bytes in the address buffer.
 *
 * @param  host : hostname
 * @param  buffer : Pointer to buffer
 * @param  name_buf : Pointer to buffer for name.
 */
/* 
 * __doc__
 * __doc__  Function : int get_host_address(char *host, unsigned int *buffer,
 * __doc__              char *name_buf)
 * __doc__  Arguments: char *hostname
 * __doc__             unsigned int *buf: Pointer to buffer
 * __doc__             char *name_buf: Pointer to buffer for name.
 * __doc__  Returns  : Success or failure
 * __doc__  Performs : This function takes a host name and figures out the 
 * __doc__             ip address and sticks this address in the buffer that 
 * __doc__             was provided. This function is IPV6 aware and can 
 * __doc__             return from 4 bytes to 16 bytes in the address buffer.
 * __doc__
 */
int
get_host_address (char *host, unsigned int *buffer, char *name_buf)
{
    struct addrinfo *server_addr;
    struct addrinfo ai_hints;
    struct sockaddr_in *sain;
    struct sockaddr_in6 *sain6;
    int error;

    memset ((void *) &ai_hints, 0, (size_t) (sizeof (ai_hints)));
    trim_leading_spaces (host);
    /* Get host's address */
    if (cmdline.ipv6_enable)
    {
	ai_hints.ai_family = AF_INET6;
	ai_hints.ai_flags = AI_CANONNAME;
	error = getaddrinfo (host, NULL, &ai_hints, &server_addr);
	if (error != 0)
	{
	    if (is_client () || is_nodeManager ())
	    {
		log_file (LOG_ERROR,
			  "%s could not be located by getaddrinfo. Error %s\n",
			  host, gai_strerror (error));
	    }
	    else
	    {
		log_all (LOG_ERROR,
			 "\t%s could not be located by getaddrinfo. Error %s\n",
			 host, gai_strerror (error));
	    }
	    return (1);
	}
    }
    else
    {
	ai_hints.ai_family = AF_INET;
	ai_hints.ai_flags = AI_CANONNAME;
	error = getaddrinfo (host, NULL, &ai_hints, &server_addr);
	if (error != 0)
	{
	    if (is_client () || is_nodeManager ())
	    {
		log_file (LOG_ERROR,
			  "%s could not be located by getaddrinfo. Error %s\n",
			  host, gai_strerror (error));
	    }
	    else
	    {
		log_all (LOG_ERROR,
			 "\t%s could not be located by getaddrinfo. Error %s\n",
			 host, gai_strerror (error));
	    }
	    return (1);
	}
    }
    if (cmdline.ipv6_enable)
    {
	sain6 = (struct sockaddr_in6 *) (server_addr->ai_addr);
	memcpy (buffer, &sain6->sin6_addr, 16);
    }
    else
    {
	sain = (struct sockaddr_in *) (server_addr->ai_addr);
	memcpy (buffer, &sain->sin_addr, 4);
    }
    my_strncpy (name_buf, server_addr->ai_canonname, MAXHNAME);
    freeaddrinfo (server_addr);
    return (0);
}

/**
 * @brief Given a hostname, return its IP address. Works for 
 *        IPV4 and IPV6
 *
 * @param  host : hostname
 * @param  buffer : Holds IP address as a string.
 */
/*
 * __doc__
 * __doc__  Function : int get_host_ip_address(char *host, char *buffer)
 * __doc__  Arguments: char *hostname
 * __doc__             char *buffer : Holds IP address as a string.
 * __doc__  Returns  : success or failure.
 * __doc__  Performs : Given a hostname, return its IP address. Works for 
 * __doc__             IPV4 and IPV6
 * __doc__
 */
int
get_host_ip_address (char *host, char *buffer)
{
    struct addrinfo *server_addr;
    struct addrinfo ai_hints;
    struct sockaddr_in *sain;
    struct sockaddr_in6 *sain6;
    int error;
    char ibuffer[256];

    memset ((void *) &ai_hints, 0, (size_t) (sizeof (ai_hints)));
    /* Get host's address */
    if (cmdline.ipv6_enable)
    {
	ai_hints.ai_family = AF_INET6;
	error = getaddrinfo (host, NULL, &ai_hints, &server_addr);
	if (error != 0)
	{
	    (void) fprintf (stdout,
			    "%s could not be located by getaddrinfo. Error %s\n",
			    host, gai_strerror (error));
	    ;
	    return (1);
	}
    }
    else
    {
	ai_hints.ai_family = AF_INET;
	error = getaddrinfo (host, NULL, &ai_hints, &server_addr);
	if (error != 0)
	{
	    (void) fprintf (stdout,
			    "%s could not be located by getaddrinfo. Error %s\n",
			    host, gai_strerror (error));
	    ;
	    return (1);
	}
    }
    if (cmdline.ipv6_enable)
    {
	sain6 = (struct sockaddr_in6 *) (server_addr->ai_addr);
	memcpy (ibuffer, &sain6->sin6_addr, 16);
    }
    else
    {
	sain = (struct sockaddr_in *) (server_addr->ai_addr);
	memcpy (ibuffer, &sain->sin_addr, 4);
    }
    if (cmdline.ipv6_enable)
    {
	inet_ntop (AF_INET6, (void *) ibuffer, buffer,
		   sizeof (struct sockaddr_in6));
    }
    else
    {
	inet_ntop (AF_INET, (void *) ibuffer, buffer,
		   sizeof (struct sockaddr_in));
    }
    freeaddrinfo (server_addr);
    return (0);
}

/** 
 * @brief  Return my server socket fd.
 *
 * @param id : my id.
 * @param socket_id :  Pointer to my socket_id
 * @param socket_port :  Pointer to my socket_port.
 */
void
get_my_server_socket (int id, int *socket_id, int *socket_port)
{
    int port_start_value = 0;

#if SETSOCKOPTBUF
    int sockerr;
#endif
    int s, rc, tmp_port;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    char *str_error = "";
    int error_code;
#if defined(_KEEPALIVE_)
    int sol_optval = 1;
#endif
#ifdef REUSE_ADDR
    int optval = 1;
#endif
    int retry = 0;
#if SETSOCKOPTBUF
    int recv_buf_size = 65536;
#endif



    if (is_prime ())
    {
	port_start_value = HOST_LIST_PORT;
    }
    else if (is_nodeManager ())
    {
	port_start_value = NODEMANAGER_LIST_PORT;
    }
    else if (is_client ())
    {
	port_start_value = CHILD_LIST_PORT;
    }

    log_file (LOG_COMM_VERBOSE, "%s\n", __FUNCTION__);

  hereL:
    if (cmdline.ipv6_enable)
	s = (int) socket (AF_INET6, SOCK_STREAM, 0);
    else
	s = (int) socket (AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
	if (retry++ >= 300)
	{
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "socket create failed: Error %s", str_error);
	    exit (0);
	}
	else
	{
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "Retry #%d of socket create. Error = %s",
		      retry, str_error);
	    sleep (5);
	    goto hereL;
	}
    }

    retry = 0;

    log_file (LOG_COMM_VERBOSE, "socket allocated\n");

#if SETSOCKOPTBUF
    sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
			  &recv_buf_size, sizeof (int));
    if (sockerr == -1)
    {
	error_code = netmist_get_sock_error ();
	str_error = netmist_get_sock_errstr (error_code);
	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Error in setsockopt Error %s\n", str_error);
    }
#endif
#ifdef REUSE_ADDR
#if !defined(_vmware_)
    sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR,
			  (char *) &optval, sizeof (int));
    if (sockerr == -1)
    {
	error_code = netmist_get_sock_error ();
	str_error = netmist_get_sock_errstr (error_code);
	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Error in setsockopt Error %s\n", str_error);
    }
#endif
#endif
#if defined(_KEEPALIVE_)
    /*
     * In case one is behind a NAT :-)
     */
    sol_optval = 1;
    if (setsockopt (s, SOL_SOCKET, SO_KEEPALIVE, &sol_optval,
		    sizeof (int)) < 0)
    {
	error_code = netmist_get_sock_error ();
	str_error = netmist_get_sock_errstr (error_code);
	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Error in setsockopt(KEEPALIVE)\n", str_error);
    }
#endif

    tmp_port = port_start_value + id;

  next_bind_port:
    if (cmdline.ipv6_enable)
    {
	memset ((void *) (&addr6), 0,
		(size_t) (sizeof (struct sockaddr_in6)));
	addr6.sin6_port = htons (tmp_port);
	addr6.sin6_family = AF_INET6;
	/* The next line needs work */
	memset ((void *) (&addr6.sin6_addr.s6_addr[0]), 0, (size_t) 16);
    }
    else
    {
	memset ((void *) (&addr), 0, (size_t) (sizeof (struct sockaddr_in)));
	addr.sin_port = htons (tmp_port);
	addr.sin_family = AF_INET;
	memset ((void *) (&addr.sin_addr.s_addr), 0, (size_t) 4);
    }

    /*
     * Try to bind on this port unless we run out of ports
     */
    rc = -1;
    while (rc < 0)
    {
	if (cmdline.ipv6_enable)
	    rc = bind (s, (struct sockaddr *) &addr6,
		       sizeof (struct sockaddr_in6));
	else
	    rc = bind (s, (struct sockaddr *) &addr,
		       sizeof (struct sockaddr));
	if (rc < 0)
	{
	    tmp_port++;
	    if (tmp_port > PORT_END)
	    {
		log_file (LOG_ERROR, "Out of ports\n");
		exit (0);
	    }
	    goto next_bind_port;
	}
    }

    if (cmdline.ipv6_enable)
	*socket_port = ntohs (addr6.sin6_port);
    else
	*socket_port = ntohs (addr.sin_port);

    log_file (LOG_COMM_VERBOSE, "bind success: port %d\n", tmp_port);

    *socket_id = s;
}

/**
 * @brief Connet to my peer server.
 *
 * @param peer_server_name : Name of my peer server.
 * @param peer_server_port : TCP port of my peer server.
 */
int
connect_peer_server (char *peer_server_name, int peer_server_port)
{
    char *peer_role = NULL;
#if SETSOCKOPTBUF
    int sockerr;
#endif
    int rc, xx, ecount = 0;
    char *str_error = "";
    int error_code = 0;
    /*struct in_addr *ip; */
    struct sockaddr_in cs_raddr;
    struct sockaddr_in6 cs_raddr6;
    int my_socket;
    int retry_dns_count = 0;
#if SET_SOCKOPTBUF
    int recv_buf_size = 65536;
#endif

#if defined(_KEEPALIVE_)
    int sol_optval = 1;
#endif

#ifdef REUSE_ADDR
    int optval = 1;
    int sockerr;
#endif
    char name_buf[MAXHNAME];
    unsigned int addr_buf[16];
    char ok[4];

    if (is_prime ())
    {
	peer_role = "None";
    }
    else if (is_nodeManager ())
    {
	peer_role = "Prime";
    }
    else if (is_client ())
    {
	peer_role = "nodeManager";
    }

    /*
     * This function should not be called from prime
     */
    if (is_prime ())
    {
	log_all (LOG_COMM, "\nPrime doesn't connect to any server\n");
	return 0;
    }

#if defined(_solaris_) || defined(_bsd_)
  retry_connect:
#endif
    memset ((void *) ok, 0, (size_t) 4);
    memset ((void *) name_buf, 0, (size_t) MAXHNAME);

    log_file (LOG_COMM_VERBOSE, "%s\n", __FUNCTION__);

  retry_dns:
    xx = get_host_address (peer_server_name, addr_buf, name_buf);
    if (xx != 0)
    {
	if (retry_dns_count++ < 5)
	{
	    log_file (LOG_ERROR, "get_host_address failed. Retrying DNS\n");
	    nap (10);
	    goto retry_dns;
	}
	log_file (LOG_ERROR, "get_host_address failed\n");
	R_exit (BAD_DNS);
    }

    if (cmdline.ipv6_enable)
    {
	cs_raddr6.sin6_family = AF_INET6;
	cs_raddr6.sin6_port = htons (peer_server_port);
	/* The next line needs work */
	memcpy (&cs_raddr6.sin6_addr.s6_addr[0], addr_buf, 16);
	my_socket = (int) socket (AF_INET6, SOCK_STREAM, 0);
    }
    else
    {
	cs_raddr.sin_family = AF_INET;
	cs_raddr.sin_port = htons (peer_server_port);
	memcpy (&cs_raddr.sin_addr.s_addr, addr_buf, 4);
	/*cs_raddr.sin_addr.s_addr = ip->s_addr; */
	my_socket = (int) socket (AF_INET, SOCK_STREAM, 0);
    }
    if (my_socket < 0)
    {
	error_code = netmist_get_sock_error ();
	str_error = netmist_get_sock_errstr (error_code);
	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Socket failed: %s\n", str_error);
	exit (23);
    }

#if SETSOCKOPTBUF
    sockerr = setsockopt (my_socket, SOL_SOCKET, SO_RCVBUF, (char *)
			  &recv_buf_size, sizeof (int));
    if (sockerr == -1)
    {
	error_code = netmist_get_sock_error ();
	str_error = netmist_get_sock_errstr (error_code);
	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Error in setsockopt Error %s\n", str_error);
    }
#endif
#if !defined(_solaris_) && !defined(_bsd_)
/* Solaris and BSD return a positive value for the socket() call 
 * and then later claims the socket descriptor is invalid, when
 * a connect call is made.  It's insane, but then again, it is Solaris & BSD.
 * So, we close the now invalid socket descriptor, and start completely
 * over.  The second time through, it works !! It's the definition
 * of insanity.
 */
  retry_connect:
#endif
    if (cmdline.ipv6_enable)
	rc = connect (my_socket, (struct sockaddr *) &cs_raddr6,
		      sizeof (struct sockaddr_in6));
    else
	rc = connect (my_socket, (struct sockaddr *) &cs_raddr,
		      sizeof (struct sockaddr_in));
    if (rc < 0)
    {
	error_code = netmist_get_sock_error ();
	if ((ecount++ < 200) && (error_code != EISCONN))
	{

	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "connect error %s Port %d Host %s\n",
		      str_error, peer_server_port, peer_server_name);

	    if (error_code == EPERM)
	    {
		log_file (LOG_ERROR, "connect permission error %s \n",
			  str_error);
		log_file (LOG_ERROR, "Possibly local firewall issues ? \n");

	    }
#if defined (WIN32)
	    else if (error_code == WSAECONNREFUSED)
#else
	    else if (error_code == ECONNREFUSED)
#endif
	    {
		/* Prime not in the listen yet ? */
#if defined(_solaris_) || defined(_bsd_)
		/* Close your eyes, this is the Solaris & BSD crazy stuff */
		close (my_socket);
#endif
		sleep ((netmist_rand () % 5) + 1);
	    }
	    nap (100);
	    goto retry_connect;
	}

	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Connect failed %s\n", str_error);
	exit (99);
    }

    log_file (LOG_COMM_VERBOSE, "Connected to %s\n", peer_role);

    return my_socket;
}

/**
 * @brief Send this buffer via the specified socket fd for the size specifed.
 *
 * @param socket : Socket fd to send to.
 * @param buffer : Buffer to use.
 * @param size : Size of message to send.
 */
void
socket_send (int socket, char *buffer, int size)
{
    int rc, retry = 0;
    int error_code = 0;
    char *str_error = NULL;
    double start_time, end_time;

    if (socket == 0)
    {
	log_file (LOG_ERROR, "Attempt to send on uninitialized socket\n");
	return;
    }

    if (size == 0)
    {
	log_file (LOG_ERROR, "Attempt to send on zero buffer\n");
	return;
    }

  retry_send:
    start_time = getepochtime ();
    rc = send (socket, buffer, size, 0);
    end_time = getepochtime ();
    if (rc < 0)
    {
	if (retry < 1000)
	{
	    retry++;

	    nap (10);
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);

	    /* DO not retry if the circuit is dead */
#ifdef WIN32
	    if (error_code == WSAECONNABORTED)
	    {
	        log_file (LOG_ERROR,
		    "Send failed. Connection aborted. Errno: %d  Error:  %s\n",
		    error_code, str_error);
		return;
	    }
#else
	    if (error_code == ECONNABORTED)
	    {
	        log_file (LOG_ERROR,
		    "Send failed. Connection aborted. Errno: %d  Error:  %s\n",
		    error_code, str_error);
		return;
	    }
#endif
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR,
		      "Send failed. Retrying %d. Errno: %d  Error:  %s\n",
		      retry, error_code, str_error);
	    goto retry_send;
	}

	if (str_error == NULL)
	    str_error = "Null";
	log_file (LOG_ERROR, "Send failed %s\n", str_error);

	exit (26);
    }

    if (rc != size)
    {
	log_file (LOG_ERROR, "Sent bytes %u less than total %u\n", rc, size);
	size -= rc;
	buffer = buffer + rc;
	goto retry_send;
    }
    if ((end_time - start_time) > (double) 0.05)	/* 50 ms for a node local socket send is worrisome */
    {
	/* Things are getting sluggish. This may start leading to skewing of measurement window.
	 * Time to start warning the user that they are getting close to overloading this node.
	 */
	if (is_nodeManager () && (NM_phase != 0)) /* Ignore if in INIT phase */
	    log_file (LOG_EXEC,
		      "Warning: NM socket_send time > 50 ms: %10.6f\n",
		      (end_time - start_time));
    }
    if ((end_time - start_time) > (double) 1.000)	/* 1 second is a very long time for node local socket send !! */
    {
	/*
	 * If the socket_send() call is taking more than 1 second from then nodeManager 
	 * to its node local children, the benchmark is pushing TOO hard and will not 
	 * only get wrong answers, skewing, and get out of sync, it may result in the
	 * the node crashing, or forcing the admin to hit the reset button to regain 
	 * control of the node.  This can result in data loss, raid & mirror breaks, and 
	 * other evil.  It *is* time to STOP before this gets really BAD.
	 *
	 * GB: "Don't cross the streams" :-)
	 */
	if (is_nodeManager () && (NM_phase != 0))  /* Ignore if in INIT phase */
	{
	    log_file (LOG_EXEC,
		      "FATAL NM->Client overload: Error: Socket_send time > 1 sec: %10.6f\n",
		      (end_time - start_time));
	    R_exit (NETMIST_BAD_SOCKET_LATENCY);
	}
    }
}

/**
 * @brief Receive a mesage from the specifed socket for the specifed size into the buffer.
 *
 * @param socket : Socket fd.
 * @param buffer : Pointer to the buffer to receive the message.
 * @param size : Size of the message.
 */
unsigned int
socket_receive (int socket, char *buffer, unsigned int size)
{
    unsigned int tsize, read_size, rcvd;
    int rc;
    char *where;
    int error_code = 0;
    char *str_error = NULL;

    tsize = size;
    where = (char *) buffer;
    memset (where, 0, size);

    rcvd = 0;
    read_size = tsize;

    while (rcvd < tsize)
    {
	rc = recv (socket, where, read_size, 0);

	if (rc < 0)
	{
	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "receive failure in "
		      "%s - shutting down: Error: %s\n",
		      __FUNCTION__, str_error);
	    exit (0);
	}
	else if (rc == 0)
	{
	    log_file (LOG_ERROR, "%s: recv returned 0, shutting down\n",
		      __FUNCTION__);
	    return rc;

	}

	rcvd += rc;
	where += rc;
	read_size -= rc;
    }

    return size;
}

/**
 * @brief Function for this server to wait for all of the clients to check in.
 *
 * @param server_socket : Socket to wait on.
 * @param total_clients : Number of messages to wait for.
 */
int *
server_wait_for_clients (int server_socket, int total_clients)
{
    int num_done, ret, retry_count = 0, ns, port;
    char *peer_role;
    int *client_sockets;
    int outstanding_connections;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    char *str_error;
    int error_code;
    unsigned int me, me6;

    me = sizeof (addr);
    me6 = sizeof (addr6);

    if (is_prime ())
    {
	peer_role = "nodeManager";
	outstanding_connections = MAX_NMS;
    }
    else if (is_nodeManager ())
    {
	peer_role = "client";
	outstanding_connections = MAXCLIENTS_PER_NM;
    }
    else
    {
	log_file (LOG_ERROR, "Client should not be listening\n");
	return NULL;
    }

    client_sockets = my_malloc (sizeof (int) * total_clients);
    if (client_sockets == 0)
    {
	log_file (LOG_ERROR, "Failed to allocate memory for sockets\n");
	exit (99);		/* Can't ever get here */
    }

    num_done = 0;
    while (1)
    {
	if (num_done == total_clients)
	{
	    break;
	}

	ret = listen (server_socket, outstanding_connections * 2);
	if (ret != 0)
	{
	    /* You're good, if you know why this is */
	    /* needed on Windows */
	    if (signal_flag)
		exit (0);

	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "listen returned error"
		      "trying to recover\n");
	    retry_count++;

	    if (retry_count > 1000)
	    {
		if (str_error == NULL)
		    str_error = "Null";
		log_file (LOG_ERROR, "listen failure.%s Shutdown.\n",
			  str_error);
		R_exit (187);	/* XXX errno? */
	    }
	    continue;
	}

	log_file (LOG_COMM_VERBOSE, "accepting connection\n");

	if (cmdline.ipv6_enable)
	{
	    ns = (int) accept (server_socket, (void *) &addr6,
			       (socklen_t *) & me6);

	    if (ns < 0)
	    {
		error_code = netmist_get_sock_error ();
		str_error = netmist_get_sock_errstr (error_code);
		if (str_error == NULL)
		    str_error = "Null";
		log_file (LOG_ERROR, "accept error: %s\n", str_error);
	    }

	    port = addr6.sin6_port;
	}
	else
	{
	    log_file (LOG_COMM_VERBOSE, "accept size of addr: %d\n", me);
	    ns = (int) accept (server_socket, (void *) &addr,
			       (socklen_t *) & me);
	    port = addr.sin_port;

	    if (ns < 0)
	    {
		error_code = netmist_get_sock_error ();
		str_error = netmist_get_sock_errstr (error_code);
		if (str_error == NULL)
		    str_error = "Null";
		log_file (LOG_ERROR, "accept error: %s\n", str_error);
		continue;
	    }
	}

	log_file (LOG_COMM_VERBOSE,
		  "accepting connections from port %d socket %d\n", port, ns);

	client_sockets[num_done] = ns;
	num_done++;

	log_file (LOG_COMM_VERBOSE, "Entering listen for %s: "
		  "%d out of %d checked-in\n",
		  peer_role, num_done, total_clients);
    }

    return client_sockets;
}

static struct ext_prime_command ext_prime_rcv_buf;
static struct prime_command prime_rcv_buf;

/**
 * @brief  The main select poll loop.
 *
 * @param send_socket : Socket to forward data.
 * @param client_sockets : List of client socket to select on.
 * @param total_clients : Total number of clients.
 * @param result_clients : Total result clients.
 * @param state : State. 
 */
static int
server_select_child_poll (int send_socket,
			  int *client_sockets, int total_clients,
			  int result_clients, int state)
{
    struct pollfd *handle;
    char *peer_role;
    int i, rc, num_done;
    char *where;
    int rcvd;
    int read_size;
    int tsize;
    int client_id;
    int pid;
    int id = 0;
    int port;
    struct prime_command *mc;
    mc = (struct prime_command *) &prime_rcv_buf;
    int h_type;
    char *p_type = NULL;
    int error_code = 0;
    char *str_error = NULL;

    int total_loop_clients;

    time_t prev_time;
    time_t curr_time;
    time_t diff;

    /* 
     * These hash tables are used to record % completion
     * from respective states 
     */
    char init_completion[11] = { 0 };
    char warmup_completion[11] = { 0 };
    char run_completion[11] = { 0 };
    char *completion_ptr = NULL;
    int bucket = 0;
    int init_bucket = 0;
    int warmup_bucket = 0;
    int run_bucket = 0;
    int *cur_max_bucket = NULL;

    if (is_prime ())
    {
	peer_role = "nodeManager";
    }
    else if (is_nodeManager ())
    {
	peer_role = "client";
    }
    else
    {
	log_file (LOG_ERROR, "Client should not be in select\n");
	return 1;
    }

    /* 
     * For all states, except RESULTS gathering, we will get *only one*
     * message from all NodeManagers and/or all clients at any time.
     * 
     * For results, since each NodeManager will relay results from each of
     * his clients, we will get multiple messages from each node Manager
     * So adjust the terminating condition accordingly. 
     *
     * Note that, only prime needs to worry about this
     */
    if (is_prime () && (state == R_SENDING_RESULTS))
    {
	total_loop_clients = result_clients;
    }
    else
    {
	total_loop_clients = total_clients;
    }

    log_file (LOG_COMM_VERBOSE, "wait on %d messages\n", total_loop_clients);

    handle = my_malloc (sizeof (struct pollfd) * total_clients);
    if (handle == NULL)
    {
	log_file (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	exit (1);
    }

    for (i = 0; i < total_clients; i++)
    {
	handle[i].fd = client_sockets[i];
	handle[i].events = POLLIN;
    }

    num_done = 0;

    /* 
     * Record the time to be used for once a minute heartbeats.
     * It is not used for other states
     */
    prev_time = time (NULL);

    do
    {
#if defined(WIN32)
	rc = WSAPoll (handle, total_clients, RECV_POLL_TIMEOUT);
#else
	rc = poll (handle, total_clients, RECV_POLL_TIMEOUT);
#endif
	if (rc < 0 || rc == 0)
	{

	    error_code = netmist_get_sock_error ();
	    str_error = netmist_get_sock_errstr (error_code);
	    if (str_error == NULL)
		str_error = "Null";
	    log_file (LOG_ERROR, "poll error in %s %s\n",
		      __FUNCTION__, str_error);
	    return 1;
	}

	for (i = 0; i < total_clients; i++)
	{
	    if (!(handle[i].revents & POLLIN))
	    {
		continue;
	    }

	    /*
	     * Initialize receive parameters
	     */
	    if ((state == R_NODEMANAGER_JOIN) ||
		(state == R_CHILD_JOIN) ||
		(state == R_NODEMANAGER_ALL_READY) ||
		(state == R_GO_DONE) ||
		(state == R_VERSION_CHECK_DONE) ||
		(state == R_TIME_SYNC_DONE) ||
		(state == INIT_PHASE_DONE) ||
		(state == R_EPOCH_DONE) ||
		(state == WARM_PHASE_DONE) ||
		(state == RUN_PHASE_DONE) || (state == R_SENDING_RESULTS))
	    {
		tsize = sizeof (struct ext_prime_command);
		memset ((void *) (&ext_prime_rcv_buf), 0, (size_t) tsize);
		where = (char *) &ext_prime_rcv_buf;
		rcvd = 0;
		read_size = tsize;
	    }
	    else
	    {
		log_file (LOG_ERROR, "unsupported select state\n");
		exit (26);
	    }

	    /*
	     * read the buffer
	     */
	    memset (where, 0, read_size);
	    while (rcvd < tsize)
	    {
		rc = recv (client_sockets[i], where, read_size, 0);
		if (rc < 0 || rc == 0)
		{
		    if (is_prime ())
		    {
			if (rc < 0)
			{
			    error_code = netmist_get_sock_error ();
			    str_error = netmist_get_sock_errstr (error_code);
			    if (str_error == NULL)
				str_error = "Null";
			    log_file (LOG_ERROR, "receive failure in "
				      "%s - shutting down: %s\n",
				      __FUNCTION__, str_error);
			}
			else
			{
			    log_file (LOG_ERROR, "receive failure in "
				      "%s - shutting down: No error.\n",
				      __FUNCTION__);
			}
		    }
		    else
		    {
			if (rc < 0)
			{
			    error_code = netmist_get_sock_error ();
			    str_error = netmist_get_sock_errstr (error_code);
			    if (str_error == NULL)
				str_error = "Null";
			    log_file (LOG_ERROR, "receive failure in "
				      "%s - shutting down: %s\n",
				      __FUNCTION__, str_error);
			}
			else
			{
			    log_file (LOG_ERROR, "receive failure in "
				      "%s - shutting down: No error.\n",
				      __FUNCTION__);
			}
		    }
		    return 1;
		}

		rcvd += rc;
		where += rc;
		read_size -= rc;
	    }

	    prime_ext_to_int (mc, &ext_prime_rcv_buf, cmdline.Op_lat_flag);

	    /* 
	     * Fail message can come anytime so check that first
	     */
	    if (mc->m_command == R_FAILED)
	    {
		if (is_prime ())
		{
		    log_all (LOG_ERROR, "Client %d failed on error %d. "
			     "Check netmist_C%d.log and netmist_NM%d.log "
			     "for more details\n",
			     mc->m_client_number, mc->m_client_eval,
			     mc->m_client_number, mc->m_nm_number);
		    process_error (mc);
		}
		else
		{
		    log_file (LOG_ERROR, "Client %d failed on error %d. "
			      "Check netmist_C%d.log for more details\n",
			      mc->m_client_number, mc->m_client_eval,
			      mc->m_client_number);
		    log_file (LOG_COMM_VERBOSE, "Forwarding it to Prime\n");

		    /*
		     * Add NM self identifier for Prime to print
		     */
		    mc->m_nm_number = cmdline.client_id;

		    prime_int_to_ext (mc, &ext_prime_rcv_buf,
				      cmdline.Op_lat_flag);

		    socket_send (send_socket, (char *) &ext_prime_rcv_buf,
				 sizeof (struct ext_prime_command));
		}

		if (mc->m_command == R_NM_FAILED)
		{
		    if (is_prime ())
		    {
			log_all (LOG_ERROR,
				 "nodeManager %d failed on error %d. "
				 "Check netmist_NM%d.log for more details\n",
				 mc->m_client_number, mc->m_client_eval,
				 mc->m_client_number);
		    }
		}

		/*
		 * Stop receiving further messages since we need
		 * to stop the execution of the benchmark!
		 *
		 */
		return 1;
	    }

	    if (mc->m_command == R_NM_FAILED)
	    {
		if (is_prime ())
		{
		    log_all (LOG_ERROR, "NodeManager %d failed. "
			     "Check netmist_NM%d.log for more details\n",
			     mc->m_client_number, mc->m_client_number);
		}

		return 1;
	    }

	    /*
	     * Check for % completion
	     */
	    if ((mc->m_command == R_PERCENT_I_COMPLETE) ||
		(mc->m_command == R_PERCENT_W_COMPLETE) ||
		(mc->m_command == R_PERCENT_COMPLETE))
	    {
		client_id = mc->m_client_number;
		if (mc->m_command == R_PERCENT_I_COMPLETE)
		{
		    p_type = "Init";
		    completion_ptr = init_completion;
		    cur_max_bucket = &init_bucket;
		}
		else if (mc->m_command == R_PERCENT_W_COMPLETE)
		{
		    p_type = "Warm-up";
		    completion_ptr = warmup_completion;
		    cur_max_bucket = &warmup_bucket;
		}
		else if (mc->m_command == R_PERCENT_COMPLETE)
		{
		    p_type = "Run";
		    completion_ptr = run_completion;
		    cur_max_bucket = &run_bucket;
		}
		else
		{
		    log_file (LOG_ERROR, "Invalid completion time\n");
		    return 1;
		}

		/*
		 * Change 10-100 range to 1-10 for hash table look up
		 */
		bucket = mc->m_percent_complete / 10;

		/*
		 * Only forward if this is the first message on that bucket
		 */
		if (completion_ptr[bucket] == 1)
		{
		    continue;
		}

		completion_ptr[bucket] = 1;

		/*
		 * Due to randomized rate limiting logic at client heartbeat
		 * messages, it was observed that they reach here out of order
		 * This logic prevents it
		 */
		if (bucket < *cur_max_bucket)
		{
		    continue;
		}

		*cur_max_bucket = bucket;

		if (is_nodeManager ())
		{
		    log_file (LOG_DEBUG, "%s %d percent complete "
			      "from client %d\n",
			      p_type, mc->m_percent_complete, client_id);

		    log_file (LOG_COMM_VERBOSE, "Forwarding it to Prime\n");
		    socket_send (send_socket, (char *) &ext_prime_rcv_buf,
				 sizeof (struct ext_prime_command));
		}
		else
		{
		    log_all (LOG_EXEC, "%s %d percent complete "
			     "from client %d\n",
			     p_type, mc->m_percent_complete, client_id);
		}
		/* We do not want to count these messages */
		continue;
	    }

	    /*
	     * Next check for heartbeats 
	     * They can come in INIT, WARMUP, and RUN phases
	     */
	    if (mc->m_command == R_HEARTBEAT)
	    {
		client_id = mc->m_client_number;
		curr_time = time (NULL);
		diff = curr_time - prev_time;

		h_type = mc->m_heartbeat_type;


		if (is_nodeManager ())
		{
		    /*
		     * Only forward if this is coming after 1 min
		     */
		    if (diff < HEARTBEAT_TICK)
			continue;

	            if(NM_phase != 0) /* Ignore if in INIT phase */
		    {
		        log_file (LOG_EXEC, "NM client's:  Total utime+stime: %5.1f percent of max CPU/Interval. Total_maxrss %lld Total_inblock %lld Total_oublock %lld\n",
                            curr_us_time, curr_client_maxrss, curr_inblock, curr_oublock);
		        if(cpu_warning == 1)
		            log_file (LOG_EXEC, "Warning: Client's CPUs are at %5.1f busy. This can cause results to be unstable.\n",cpu_percent_busy);
		    }
		    /*
		     * change the prev_time for the next 1-min interval
		     */
		    prev_time = time (NULL);

		    if (mc->m_current_op_rate > 0.0)
		    {
			log_file (LOG_DEBUG,
				  "%s Heartbeat client %d: %.3lf Ops/sec\n",
				  heartbeat_names[h_type], client_id,
				  mc->m_current_op_rate);
		    }
		    else
		    {
			log_file (LOG_DEBUG,
				  "%s Heartbeat __/\\_/\\__ client %d\n",
				  heartbeat_names[h_type], client_id);
		    }

		    log_file (LOG_COMM_VERBOSE, "Forwarding it to Prime\n");
		    socket_send (send_socket, (char *) &ext_prime_rcv_buf,
				 sizeof (struct ext_prime_command));
		}
		else
		{
		    /*
		     * Only print if this is coming after 1 min
		     */
		    if (diff < HEARTBEAT_TICK)
			continue;

		    /*
		     * change the prev_time for the next 1-min interval
		     */
		    prev_time = time (NULL);

		    if (mc->m_current_op_rate > 0.0)
		    {
			log_all (LOG_EXEC,
				 "%s heartbeat client %d: %.3lf Ops/sec\n",
				 heartbeat_names[h_type], client_id,
				 mc->m_current_op_rate);
		    }
		    else
		    {
			log_all (LOG_EXEC,
				 "%s heartbeat __/\\_/\\__ client %d\n",
				 heartbeat_names[h_type], client_id);
		    }
		}
		/* We do not want to count these messages */
		continue;
	    }

	    if (state == R_NODEMANAGER_JOIN)
	    {
		if (mc->m_command != R_NODEMANAGER_JOIN)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_NODEMANAGER_JOIN);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
		port = mc->m_child_port;

		if ((set_nodeManager_port (id, port) == 0) ||
		    (set_nodeManager_pid (id, pid) == 0) ||
		    (set_nodeManager_socket (id, client_sockets[i]) == 0))
		{
		    log_file (LOG_ERROR, "Failed to add the %s %d "
			      "port %d pid %d socket %d\n",
			      peer_role, id, port, pid, client_sockets[i]);
		}
		else
		{
		    log_file (LOG_EXEC_VERBOSE, "Added the %s %d "
			      "port %d pid %d socket %d\n",
			      peer_role, id, port, pid, client_sockets[i]);
		}

	    }
	    else if (state == R_CHILD_JOIN)
	    {
		if (mc->m_command != R_CHILD_JOIN)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_CHILD_JOIN);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
		port = mc->m_child_port;

		if ((set_nm_client_port (id, port) == 0) ||
		    (set_nm_client_pid (id, pid) == 0) ||
		    (set_nm_client_socket (id, client_sockets[i]) == 0))
		{
		    log_file (LOG_ERROR, "Failed to add the %s %d "
			      "port %d pid %d socket %d\n",
			      peer_role, id, port, pid, client_sockets[i]);
		}
	    }
	    else if (state == R_NODEMANAGER_ALL_READY)
	    {
		if (mc->m_command != R_NODEMANAGER_ALL_READY)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_CHILD_JOIN);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == R_TIME_SYNC_DONE)
	    {
		if (mc->m_command != R_TIME_SYNC_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_TIME_SYNC_DONE);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;

	    }
	    else if (state == R_GO_DONE)
	    {
		if (mc->m_command != R_GO_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_GO_DONE);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == R_VERSION_CHECK_DONE)
	    {
		if (mc->m_command != R_VERSION_CHECK_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_VERSION_CHECK_DONE);
		    continue;
		}
	    }
	    else if (state == R_EPOCH_DONE)
	    {
		if (mc->m_command != R_EPOCH_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_EPOCH_DONE);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == INIT_PHASE_DONE)
	    {
		if (mc->m_command != INIT_PHASE_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, INIT_PHASE_DONE);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == WARM_PHASE_DONE)
	    {
		if (mc->m_command != WARM_PHASE_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d, id %u\n", mc->m_command,
			      peer_role, WARM_PHASE_DONE,
			      mc->m_client_number);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == RUN_PHASE_DONE)
	    {
		if (mc->m_command != RUN_PHASE_DONE)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, RUN_PHASE_DONE);
		    continue;
		}

		id = mc->m_client_number;
		pid = mc->m_child_pid;
	    }
	    else if (state == R_SENDING_RESULTS)
	    {
		if (mc->m_command != R_SENDING_RESULTS)
		{
		    log_file (LOG_ERROR, "Unexpected command %d from %s. "
			      "Expected %d\n", mc->m_command,
			      peer_role, R_SENDING_RESULTS);
		    continue;
		}

		/* 
		 * This is a special case
		 * 
		 * If this is a nodeManager, just forward this buffer to Prime
		 *
		 * If this is prime, store results for further
		 * processing after run completes
		 */
		id = mc->m_client_number;

		if (is_nodeManager ())
		{
		    log_file (LOG_DEBUG, "Received results from client %d\n",
			      id);
		    log_file (LOG_COMM_VERBOSE,
			      "Forwarding results to Prime\n");
		    socket_send (send_socket, (char *) &ext_prime_rcv_buf,
				 sizeof (struct ext_prime_command));

		}
		else
		{
		    prime_store_results (id, mc);
		}

	    }
	    else
	    {
		log_file (LOG_ERROR, "unsupported select state\n");
		exit (26);
	    }

	    num_done++;

	    log_file (LOG_DEBUG,
		      "%s joined. id = %d, %d out of %d\n",
		      peer_role, id, num_done, total_loop_clients);
	}
    }
    while (num_done != total_loop_clients);

    free (handle);

    return 0;
}

/**
 * @brief Server select routine that blocks on select.
 *
 * @param send_socket : Socket to forward info.
 * @param client_sockets : List of sockets to select on.
 * @param total_clients : Total number of client fds.
 * @param result_clients : Total number of results.
 * @param state : State.
 */
int
server_select (int send_socket,
	       int *client_sockets, int total_clients,
	       int result_clients, int state)
{
    int result;
    result = server_select_child_poll (send_socket,
				       client_sockets, total_clients,
				       result_clients, state);
    return result;
}

/**
 * @brief Server sends a message to the socket.
 *
 * @param socket : socket file descriptor.
 * @param send_buffer : Buffer that contains the message.
 * @param size : Size of the message to send.
 */
void
server_send_on_socket (int socket, char *send_buffer, int size)
{
    char *role, *peer_role;

    if (is_prime ())
    {
	role = "Prime";
	peer_role = "nodeManager";
    }
    else if (is_nodeManager ())
    {
	role = "nodeManager";
	peer_role = "client";
    }
    else
    {
	log_file (LOG_COMM, "Client should not be in %s\n", __FUNCTION__);
	return;
    }

    log_file (LOG_COMM, "%s: Sending to %s\n", role, peer_role);

#if USE_WRITE_FOR_SOCKET_SEND
    int full_size = size;
    int chunk = 32768;
    int total = 0;

    int curr_size = full_size;
    int keep_going = 1;

    if (curr_size < chunk)
    {
	chunk = curr_size;
    }

    do
    {
	rc = write (socket, send_buffer, chunk);
	if (rc < 0)
	{
	    if (is_prime ())
	    {
		log_all (LOG_ERROR, "write failed on socket %d\n", socket);
	    }
	    else
	    {
		log_file (LOG_ERROR, "write failed on socket %d\n", socket);
	    }
	    exit (26);
	}
	else
	{
	    total += rc;
	    curr_size -= rc;
	    send_buffer += rc;

	    /*
	     * Last frame
	     */
	    if (curr_size < chunk)
	    {
		chunk = curr_size;
	    }

	    if (curr_size == 0)
	    {
		keep_going = 0;
	    }
	}
    }
    while (keep_going);
#else
    socket_send (socket, send_buffer, size);
#endif
}

/**
 * @brief Server sends a message to the socket associated with an id.
 *
 * @param id : Id associated with the socket.
 * @param send_buffer : Buffer that contains the message.
 * @param size : Size of the message to send.
 */
void
server_send (int id, char *send_buffer, int size)
{
    char *role, *peer_role;
    int socket;

    if (is_prime ())
    {
	role = "Prime";
	peer_role = "nodeManager";
	socket = (int) get_nodeManager_socket (id);
    }
    else if (is_nodeManager ())
    {
	role = "nodeManager";
	peer_role = "client";
	socket = (int) get_nm_client_socket (id);
    }
    else
    {
	log_file (LOG_COMM, "Client should not be in %s\n", __FUNCTION__);
	return;
    }

    /* Something went horribly wrong */
    if (socket == 0)
    {
	if (is_prime ())
	{
	    log_stdout (LOG_ERROR, "Failed to get %s:%d socket in %s\n",
			peer_role, id, __FUNCTION__);
	}
	log_file (LOG_ERROR, "Failed to get %s:%d socket in %s\n",
		  peer_role, id, __FUNCTION__);
	exit (1);
    }

    log_file (LOG_COMM, "%s: Sending to %s\n", role, peer_role);

#if USE_WRITE_FOR_SOCKET_SEND
    int full_size = size;
    int chunk = 32768;
    int total = 0;

    int curr_size = full_size;
    int keep_going = 1;

    if (curr_size < chunk)
    {
	chunk = curr_size;
    }

    do
    {
	rc = write (socket, send_buffer, chunk);
	if (rc < 0)
	{
	    if (is_prime ())
	    {
		log_all (LOG_ERROR, "write failed on socket %d\n", socket);
	    }
	    else
	    {
		log_file (LOG_ERROR, "write failed on socket %d\n", socket);
	    }
	    exit (26);
	}
	else
	{
	    total += rc;
	    curr_size -= rc;
	    send_buffer += rc;

	    /*
	     * Last frame
	     */
	    if (curr_size < chunk)
	    {
		chunk = curr_size;
	    }

	    if (curr_size == 0)
	    {
		keep_going = 0;
	    }
	}
    }
    while (keep_going);
#else
    socket_send (socket, send_buffer, size);
#endif
}

/**
 * @brief  Empty function that "Forms" a prime command.
 */
void
form_prime_command (void)
{

}

/**
 * @brief Return the current hostname in a buffer.
 *
 * @param buffer : String space for hostname
 * @param len : Length of string space.
 */
/*
 * __doc__
 * __doc__  Function : int my_gethostname(char * buffer, int len)
 * __doc__  Arguments: char *buffer, int length
 * __doc__  Returns  : int success or failure.
 * __doc__  Performs : Return the current hostname in a buffer.
 * __doc__
 */
int
my_gethostname (char *buffer, int len)
{
    gethostname (buffer, len);
    return (0);
}

#if defined(WIN32)
extern int w_gettimeofday (struct timeval *, struct w_timezone *);
#endif
/*
 * Return time in seconds. Floating point.
 */
double
getepochtime (void)
{
    struct timeval tp;

#if defined(WIN32)
    if (w_gettimeofday ((struct timeval *) &tp, (struct w_timezone *) NULL))
#else
    if (gettimeofday ((struct timeval *) &tp, (struct timezone *) NULL))
#endif
    {
	perror ("gettimeofday error:");
	return ((double) 0);
    }
    return ((double) (tp.tv_sec)) + (((double) tp.tv_usec / 1000000.0));
}

/**
 * @brief Process /decode an error in the prime command.
 *
 * @param mc : Pointer to the prime_command structure.
 */
void
process_error (struct prime_command *mc)
{
    char *err_str = NULL;
    log_file (LOG_ERROR, "Prime: Failure message client %d Error = %d\n",
	      mc->m_client_number, mc->m_client_eval);
    if (mc->m_client_eval == TOO_SHORT)
    {
	log_file (LOG_ERROR, "Prime: Runtime too short on client %d\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == NOT_IMPLEMENTED)
    {
	log_file (LOG_ERROR, "Prime: Function call not implemented. %d\n",
		  mc->m_client_number);
	fflush (stdout);
	return;
    }
    if (mc->m_client_eval == BAD_WORKLOAD)
    {
	log_file (LOG_ERROR, "Prime: Client %d Bad workload definition.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == WRONG_VERS)
    {
	log_file (LOG_ERROR,
		  "Prime: Client %d is running wrong version of software.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == BAD_MIX)
    {
	log_file (LOG_ERROR, "Prime: Client %d reporting bad mix.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == LOGON_FAILURE)
    {
	log_file (LOG_ERROR, "Prime: Client %d reporting LOGON Failure.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == LOAD_GEN_ERR)
    {
	log_file (LOG_ERROR,
		  "Prime: Client %d reporting LOAD_GEN_ERR error.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == BAD_DNS)
    {
	log_file (LOG_ERROR, "Prime: Client %d reporting DNS is broken.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == NO_CREATE)
    {
	log_file (LOG_ERROR, "Prime: Client %d Unable to create file.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == NO_WORKDIR)
    {
	log_file (LOG_ERROR, "Prime: Client %d NO Workdir !.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == NETMIST_FAULT)
    {
	log_file (LOG_ERROR,
		  "Prime: Client %d SIGSEGV or SIGILL termination! Check child log for details..\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == BAD_CLOCK)
    {
	log_file (LOG_ERROR, "Prime: Client %d System clock out of sync.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == BAD_PIT_CONN)
    {
	log_file (LOG_ERROR, "Prime: Client %d Bad PIT connection.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == BAD_PIT)
    {
	log_file (LOG_ERROR, "Prime: Client %d Bad PIT.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval == PATHNAME_TOO_LONG)
    {
	log_file (LOG_ERROR, "Prime: Client %d Pathname TOO long.\n",
		  mc->m_client_number);
	return;
    }
    if (mc->m_client_eval != 0)
    {
	err_str = netmist_strerror (mc->m_client_eval);
	log_file (LOG_ERROR,
		  "Prime: Client %d returned error %d Possibly: %s\n",
		  mc->m_client_number, mc->m_client_eval, err_str);
	return;
    }
    log_file (LOG_ERROR, "Prime got unknown error type %d from client %d\n",
	      mc->m_client_eval, mc->m_client_number);
    return;
}


/**
 * @brief This is a microsecond sleep function. It will permit one 
 *        to sleep for units of time that are in the microsecond 
 *        range..
 *
 * @param  select_msecs :  Number of milliseconds to nap.
 */
/*
 * __doc__
 * __doc__  Function : void nap(int msecs)
 * __doc__  Arguments: int: number of milliseconds to nap.
 * __doc__  Returns  : void
 * __doc__  Performs : This is a microsecond sleep function. It will permit one 
 * __doc__             to sleep for units of time that are in the microsecond 
 * __doc__             range..
 * __doc__             Well.... sort of... Actually there are no existing 
 * __doc__             timers that actually let one sleep for any duration 
 * __doc__             that is shorter than a time slice. Which on most 
 * __doc__             systems in NOT in microseconds, but actually in 10-ish 
 * __doc__             millisecond range.  All of the nifty new POSIX 
 * __doc__             usleep() and nanosleep() funtions do NOT actually go 
 * __doc__             below a time slice, so they are useless examples of 
 * __doc__             group think ! Thus, nap() below does as good as one 
 * __doc__             can do, and is portable across more systems.
 * __doc__
 */

void
nap (int select_msecs)
{
#if defined(WIN32)
    Sleep (select_msecs);	/* Visual studio's Sleep(milliseconds) */
#else
    struct timeval sleeptime;
    sleeptime.tv_sec = select_msecs / 1000;
    sleeptime.tv_usec = (select_msecs % 1000) * 1000;
    select (0, NULL, NULL, NULL, &sleeptime);
#endif
}


/** 
 * @brief Strip extra slashes from the input string, and place results in dest string.
 *
 * @param dest : Destination string.
 * @param src : Input string.
 */
void
strip_extra_slashes (char *dest, char *src)
{
    size_t i;
    int k = 0;
    for (i = 0; i < strlen (src); i++)
    {
	if ((i < strlen (src) - 1) && (src[i] == '\\')
	    && (src[i + 1] == '\\'))
	{
	    dest[k] = src[i];
	    k++;
	    ;
	}
	else
	{
	    dest[k] = src[i];
	    k++;
	}
    }
}

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  CRC32 support functions.
 */

#ifdef WIN32_t
#include <stdint.h>
#define u_int32_t uint32_t
#endif

#include <sys/types.h>
#include "crc32.h"

/*
 *  CRC generator shortcut table.
 */
static const u_int32_t crctab[] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

static u_int32_t
crc32_generate_seg (void *data, size_t length, u_int32_t crc)
{
    const u_char *p = data;
    const u_int32_t *crctabp = crctab;
    int i;

    if ((int) length < 1)
	return -1;

    i = (int) length;
    while (i-- > 0)
    {
	crc ^= *p++;
	crc = (crc >> 4) ^ crctabp[crc & 0xf];
	crc = (crc >> 4) ^ crctabp[crc & 0xf];
    }
    return crc;
}

/*
 *  Generate CRC32 for data block 'data' with size 'length'.
 *
 *  Returns generated CRC32.
 */
u_int32_t
crc32_generate (void *data, size_t length)
{
    return crc32_generate_seg (data, length, 0xffffffff);
}

static u_int32_t
my_crc32_check (void *data, size_t length, u_int32_t arrived_crc)
{
    u_int32_t result = crc32_generate (data, length);
    if (is_prime ())
    {
	log_all (LOG_FSM_VERBOSE, "computed CRC result %x\n", result);
    }
    else
    {
	log_file (LOG_FSM_VERBOSE, "computed CRC result %x\n", result);
    }
    return result ^ arrived_crc;
}

/*
 *
 */
void
add_crc32_prime_command (void)
{
    /*
     * Assumes allocated structure
     */
    g_mc->crc32 = 0;
    g_mc->crc32 =
	crc32_generate ((void *) g_mc, sizeof (struct prime_command));
}

/*
 *
 */
void
add_crc32_nodeManager_command (void)
{
    /*
     * Assumes allocated structure
     */
    g_nmc->crc32 = 0;
    g_nmc->crc32 =
	crc32_generate ((void *) g_nmc, sizeof (struct nodeManager_command));
}

/*
 *
 */
void
add_crc32_client_command (void)
{
    /*
     * Assumes allocated structure
     */
    g_cc->crc32 = 0;
    g_cc->crc32 =
	crc32_generate ((void *) g_cc, sizeof (struct client_command));
    if (is_prime ())
    {
	log_all (LOG_FSM_VERBOSE, "Message Magic %u, Checksum: %x\n",
		 g_cc->magic, g_cc->crc32);
    }
    else
    {
	log_file (LOG_FSM_VERBOSE, "Message Magic %u, Checksum: %x\n",
		  g_cc->magic, g_cc->crc32);
    }
}

/*
 *
 */
void
add_crc32_ext_client_command (void)
{
    /*
     * Assumes allocated structure
     */
    u_int32_t crc32 =
	crc32_generate ((void *) g_ecc, sizeof (struct ext_client_command));
    if (is_prime ())
    {
	log_all (LOG_FSM_VERBOSE, "Ext Checksum: %x\n", crc32);
    }
    else
    {
	log_file (LOG_FSM_VERBOSE, "Ext Checksum: %x\n", crc32);
    }
}

/*
 *
 */
u_int32_t
validate_client_command (const char *from, int command)
{
    /*
     * Assumes allocated structure
     */
    u_int32_t arrived_crc = g_cc->crc32;
    g_cc->crc32 = 0;
    u_int32_t result =
	my_crc32_check ((void *) g_cc, sizeof (struct client_command),
			arrived_crc);
    if (result)
    {
	if (is_prime ())
	{
	    log_all (LOG_ERROR,
		     "Invalid Checksum: %x on client command. Magic %u, aborting\n",
		     arrived_crc, g_cc->magic);
	    return 0;
	}
	else
	{
	    log_file (LOG_ERROR,
		      "Invalid Checksum: %x on client command. Magic %u, aborting\n",
		      arrived_crc, g_cc->magic);
	    return 0;
	}
    }

    if (g_cc->c_command != command)
    {
	if (is_prime ())
	{
	    log_all (LOG_ERROR, "Invalid command from %s "
		     "Expected %u, Received %u, magic %u, aborting\n",
		     from, command, g_cc->c_command, g_cc->magic);
	    return 0;
	}
	else
	{
	    log_file (LOG_ERROR, "Invalid command from %s "
		      "Expected %u, Received %u, magic %u, aborting\n",
		      from, command, g_cc->c_command, g_cc->magic);
	    return 0;
	}
    }
    /*
     * Restore the original value in case this is forwarded
     */
    g_cc->crc32 = arrived_crc;
    return 1;
}

/*
 *
 */
void
add_crc32_keepalive_command (void)
{
    /*
     * Assumes allocated structure
     */
    g_kc->crc32 = 0;
    g_kc->crc32 =
	crc32_generate ((void *) g_kc, sizeof (struct keepalive_command));
}


/* 
 * @brief Flip forward slashes to backwards slashes.
 *        Used to decode an encoded string that was protecting backslashes
 *        from shells that strip backslashes. Flip the slashes back to what they
 *        were before getting sent over a shell as a parameter.
 *
 * @param dest : Destination string.
 * @param src : Input string.
 */
void
flip_f2b_slashes (char *dest, char *src)
{
    int i;
    for (i = 0; i < (int) strlen (src); i++)
    {
	if (src[i] == '/')
	    dest[i] = '\\';
	else
	    dest[i] = src[i];
    }
    /* Null terminate */
    dest[strlen (src)] = 0;
}

/* 
 * @brief Flip backwards slashes to forward slashes.
 *        Used to encode paths for sending as parameters to a shell that
 *        might decide to strip back-slashes.
 *
 * @param dest : Destination string.
 * @param src : Input string.
 */
void
flip_b2f_slashes (char *dest, char *src)
{
    int i;
    for (i = 0; i < (int) strlen (src); i++)
    {
	if (src[i] == '\\')
	    dest[i] = '/';
	else
	    dest[i] = src[i];
    }
    /* Null terminate */
    dest[strlen (src)] = 0;
}
