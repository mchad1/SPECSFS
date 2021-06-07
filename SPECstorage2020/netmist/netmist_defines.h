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
 *  Author: Udayan Bapat, NetApp Inc.
 *
 */
#include "./copyright.txt"
#include "./license.txt"

#ifndef __NETMIST_DEFINES_H__
#define __NETMIST_DEFINES_H__

#define START_COMM 0
#define START_WARMUP 1

/* 
 * Consume 16k of memory per proc to leave some wiggle room for making
 * future memory footprints match this release.
 */
#define KB_CMEM_PRESSURE 16

/* 
 * Consume 16k of memory per node to leave some wiggle room for making
 * future memory footprints match this release.
 * This is emulating the 16k of additional text per node.
 */

#define KB_NMEM_PRESSURE 16

/* Max clock difference between Prime and the clients */
#define MAX_TIME_DRIFT 15

/* Max message round trip cost multiplier */
/* This "ether" constant is like dark matter. The round trip cost is sampled
 * and then later used to predict the total cost, only to get surprised when
 * the round trip costs are NOT a constant but jitter wildly due to networking
 * issues.  SO... we toss in a max jitter multiplier to hopefully cover the 
 * insane amounts of jitter in round trip costs.  This is used to multiply 
 * the sampled average round trip cost.
 */
#define MAX_MSG_COST 3

#if defined(_vmware_)
#define MALLOC(x) pool_alloc(x)
#else
#define MALLOC(x) my_malloc(x)
#endif

#if defined(Windows) || defined(_linux_) || defined (_bsd_)||defined(WIN32) \
    || defined(_solaris_) || defined(_macos_)
#define _SHUTDOWN_
#endif

#if defined(_bsd_)
#define off64_t off_t
#endif

#if !defined(_macos_) && !defined(_linux_) && !defined(WIN32) && \
    !defined(_solaris_) && !defined(_aix_) && !defined(Windows) && \
    !defined(_vmware_) && !defined(off64_t)
#define off64_t long long
#endif

#if defined(_macos_)
#define off64_t off_t
#ifdef _LARGEFILE64_SOURCE
#define I_LSEEK(x,y,z)  lseek(x,(off64_t)(y),z)
#define I_OPEN(x,y,z)   open(x,(int)(y),(int)(z))
  /* 
   * Heavy magic happens with the size of the stat and statfs structures
   * about the time that 10.6 came out.
   */
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  > 1050
#define I_STAT(x,y)     stat(x,y)
#define I_STATFS(x,y)     statfs(x,y)
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  < 1050
#define I_STAT(x,y)     stat(x,y)
#define I_STATFS(x,y)     statfs(x,y)
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  == 1050
#define I_STAT(x,y)     stat64(x,y)
#define I_STATFS(x,y)     statfs64(x,y)
#endif

#define I_MMAP(a,b,c,d,e,f)     mmap((void *)(a),(size_t)(b),(int)(c), \
        (int)(d),(int)(e),(off64_t)(f))
#define I_TRUNC(x,y)  truncate(x,(off64_t)(y))
#else
#define I_LSEEK(x,y,z)  lseek(x,(off_t)(y),z)
#define I_OPEN(x,y,z)   open(x,(int)(y),(int)(z))
#define I_STAT(x,y)     stat(x,y)
#define I_STATFS(x,y)     statfs(x,y)
#define I_STATVFS(x,y)     statvfs(x,y)
#define I_MMAP(a,b,c,d,e,f)     mmap((void *)(a),(size_t)(b),(int)(c),\
        (int)(d),(int)(e),(off_t)(f))
#define I_TRUNC(x,y)  truncate(x,(off_t)(y))
#endif
#else
#ifdef _LARGEFILE64_SOURCE
#define I_LSEEK(x,y,z)  lseek64(x,(off64_t)(y),z)
#define I_OPEN(x,y,z)   open64(x,(int)(y),(int)(z))
#define I_STAT(x,y)     stat64(x,y)
#define I_STATFS(x,y)     statfs64(x,y)
#define I_STATVFS(x,y)     statvfs64(x,y)
#if defined(_linux_)
#define I_MMAP(a,b,c,d,e,f)     mmap64((void *)(a),(size_t)(b),(int)(c),\
        (int)(d),(int)(e),(off_t)(f))
#else
#define I_MMAP(a,b,c,d,e,f)     mmap64((void *)(a),(size_t)(b),(int)(c),\
        (int)(d),(int)(e),(off64_t)(f))
#endif
#define I_TRUNC(x,y)  truncate64(x,(off64_t)(y))
#else
#define I_LSEEK(x,y,z)  lseek(x,(off_t)(y),z)
#define I_OPEN(x,y,z)   open(x,(int)(y),(int)(z))
#if defined(WIN32)
#define I_STAT(x,y)     _stati64(x,y)
#else
#define I_STAT(x,y)     stat(x,y)
#endif
#define I_STATFS(x,y)     statfs(x,y)
#define I_STATVFS(x,y)     statvfs(x,y)
#define I_MMAP(a,b,c,d,e,f)     mmap((void *)(a),(size_t)(b),(int)(c),\
        (int)(d),(int)(e),(off_t)(f))
#define I_TRUNC(x,y)  truncate(x,(off_t)(y))
#endif
#endif

#define IS_SP1_WL(wl)  (my_mix_table[wl].rel_version == 1)
#define IS_SP2_WL(wl)  (my_mix_table[wl].rel_version == 2)
#define IS_SFS2020_WL(wl)  (my_mix_table[wl].rel_version == 3)

/*
 * If you don't have a licensed version...
 */
#define MAX_L_CLIENTS 50


/* Seconds between heartbeats */
#define HEARTBEAT_TICK 60

/* 
 * Maximum I/O latency during INIT before delay is introduced to slow things down.
 */
#define MAX_LATENCY 5
/* 
 * Maximum stat latency during INIT before delay is introduced to slow things down.
 */
#define MAX_STAT_LATENCY 1
/*
 * Maximum slow down between ops, to try to prevent swamping of server.
 */
#define MAX_SLOW_LATENCY 60

/* Meta-data constant */
#define META_B 8192

/* 
 * Tell subroutine to run the external script 
 */
#define SRUN 1
/* 
 * Tell subroutine to check to see if script exists and is
 * executable 
 */
#define SCHECK 2

/*
 * Max command size for external scripts 
 */
#define MAXECOMMAND 4096

/* Used to disable Nagle's queue delays. Nagle can cause the socket based 
 * messages to become SLOW 
 */
#define NODELAY 1

/* Type of client */
#define TYPE_WINDOWS 1
#define TYPE_UNIX 2
#define TYPE_WINDOWS2 3

/*
 * Minimum sizes to satisfy users of O_DIRECT
 * Linux 2.6 and later support a minimum transfer size of 512 bytes
 * for O_DIRECT. 
 * BUT.... it jumps to 4k if the disk drive has 4096 byte sectors.
 */
#define MIN_DIRECT_SIZE 512
#define MIN2_DIRECT_SIZE 4096

#define RANDOMIZE_ACROSS_LOAD_POINTS 1

/*
 * Number of tokens in the client configuration file.
 */
#define NUM_TOKENS 5

#define POWER_CHECK_LEVEL 70

/*
 * Initialize the random number generator, for reproducable
 * results.
 */

#define INIT_SEED 0x456

/*
 * For everyone else use ssh and scp.
 */
#define REMSH "ssh -o BatchMode=yes"
#define REMCP "scp"

#if defined(USE_WINEXE)
#define WINDOWS_REMOTE_FROM_UNIX "cat < /dev/null | winexe"
#endif
#define WINDOWS_REMOTE_FROM_WINDOWS "wmic.exe"

#define WINDOWS2_REMOTE_FROM_UNIX "ssh -fn -S none"
#define UNIX_REMOTE_FROM_UNIX "ssh -fn -S none"
#define UNIX_REMOTE_FROM_WINDOWS "ssh -fn -S none"

#define RW_ARRAY 100

#define FSIZE_MIN_SLOTS 20

/* 
 * Defines for the mixture of types of ops.
 * "my_workload" must be set before using these !!
 */
#define PERCENT_READ	     (my_mix_table[my_workload].percent_read)
#define PERCENT_READ_FILE    (my_mix_table[my_workload].percent_read_file)
#define PERCENT_MM_READ      (my_mix_table[my_workload].percent_mmap_read)
#define PERCENT_READ_RAND    (my_mix_table[my_workload].percent_read_rand)
#define PERCENT_WRITE        (my_mix_table[my_workload].percent_write)
#define PERCENT_WRITE_FILE   (my_mix_table[my_workload].percent_write_file)
#define PERCENT_MM_WRITE     (my_mix_table[my_workload].percent_mmap_write)
#define PERCENT_WRITE_RAND   (my_mix_table[my_workload].percent_write_rand)
#define PERCENT_RMW          (my_mix_table[my_workload].percent_rmw)
#define PERCENT_MKDIR        (my_mix_table[my_workload].percent_mkdir)
#define PERCENT_RMDIR        (my_mix_table[my_workload].percent_rmdir)
#define PERCENT_UNLINK	     (my_mix_table[my_workload].percent_unlink)
#define PERCENT_UNLINK2	     (my_mix_table[my_workload].percent_unlink2)
#define PERCENT_CREATE	     (my_mix_table[my_workload].percent_create)
#define PERCENT_APPEND       (my_mix_table[my_workload].percent_append)
#define PERCENT_LOCKING      (my_mix_table[my_workload].percent_locking)
#define PERCENT_ACCESS       (my_mix_table[my_workload].percent_access)
#define PERCENT_STAT	     (my_mix_table[my_workload].percent_stat)
#define PERCENT_NEG_STAT     (my_mix_table[my_workload].percent_neg_stat)
#define PERCENT_CHMOD	     (my_mix_table[my_workload].percent_chmod)
#define PERCENT_READDIR	     (my_mix_table[my_workload].percent_readdir)
#define PERCENT_COPYFILE     (my_mix_table[my_workload].percent_copyfile)
#define PERCENT_RENAME       (my_mix_table[my_workload].percent_rename)
#define PERCENT_STATFS       (my_mix_table[my_workload].percent_statfs)
#define PERCENT_PATHCONF     (my_mix_table[my_workload].percent_pathconf)
#define PERCENT_TRUNC        (my_mix_table[my_workload].percent_trunc)
#define PERCENT_CUSTOM1      (my_mix_table[my_workload].percent_custom1)
#define PERCENT_CUSTOM2      (my_mix_table[my_workload].percent_custom2)

/*
#define SPOT_PCT 5
#define NSPOTS (100/SPOT_PCT)
*/
#define JITTER_PCT 20

#define TRACE_STATE_SETUP 0
#define TRACE_STATE_VALIDATION 1
#define TRACE_STATE_CREATE 2
#define TRACE_STATE_WARMUP 3
#define TRACE_STATE_RUN 4
#define TRACE_STATE_CLEANUP 5

/* 
 * These are the ports that will be used for comm.
 */

#define HOST_LIST_PORT 5000
#define NODEMANAGER_LIST_PORT 5000
#define CHILD_LIST_PORT 6000
#define PORT_END 20000

#if defined(HAVE_RECV_TIMEOUT)
  /* Poll timeout is 10 mins */
  #define RECV_POLL_TIMEOUT 600000	
#else
   /* Poll timeout is infinity. */
   /* keepalive threads handle hangs now */
  #define RECV_POLL_TIMEOUT -1	
#endif 

/*#define KEEPALIVE_POLL_TIMEOUT 300000	 Poll timeout is 5 mins */

/* 5 X bigger than keepalive, and scales with respece to keepalive */
#define KEEPALIVE_POLL_TIMEOUT ((cmdline.keepalive * 1000)* 5)	

#define KEEPALIVE_SCAN_FREQ     3
#define KEEPALIVE_MISSED_COUNT  2

#define PORT_RANGE 30000
#define HARD_PORT_RANGE 65536

#define CHILD_STATE_READY 1
#define CHILD_STATE_BEGIN 2

/* 
 * Defaults for file and record size 
 */
#define FSIZE (1)		/* in Kbytes                    */
#define RSIZE (1)		/* Minimum xfer size            */

/* 
 * Status flag values .. Currently not used by test.
 */
#define NONE 0
#define INIT 1
#define INUSE 2

/* 
 * Error codes returned by children.
 *
 * (*) indicates that the secondary error code given to R_exit is
 * a VFS-layer platform-specific error code
 */
enum netmist_child_err
{
    NETMIST_NO_CHILD_ERR = 0,	/* successful return */
    LOAD_GEN_ERR = 187,		/* (*) */
    PATHNAME_TOO_LONG = 188,
    LOGON_FAILURE = 189,	/* (*) */
    NO_WORKDIR = 190,		/* (*) */
    BAD_DNS = 191,
    BAD_WORKLOAD = 192,
    BAD_PIT_CONN = 193,
    BAD_PIT = 194,
    NO_CREATE = 195,
    BAD_MIX = 196,
    WRONG_VERS = 197,
    TOO_SHORT = 199,
    NOT_IMPLEMENTED = 200,
    BAD_CLOCK = 201,
    PIPE_EXIT = 202,
    NETMIST_OOM = 203,
    NETMIST_NO_VFS = 204,
    NETMIST_FAULT = 205,
    NETMIST_BAD_CRC = 206,
    NETMIST_BAD_SOCKET_LATENCY = 207,

    NETMIST_VFS_ERR = 99,	/* (*) */
    NETMIST_OP_VFS_ERR = 100,	/* through 150 (*) */
};

/* recursion modes */
enum slinky_mode {
    SLINKY_NOOP = 0,
    SLINKY_CREATE = 1,
    SLINKY_REMOVE = 2
};

/*
 * Create a table of 3000 test op entries. Then
 * initialize according to the distribution described
 * by the mix.
 */
#define TESTOPS 3000

/* ELAP_T Collect this many seconds worth of samples */
#define ELAP_T 2.0

/* ISCALE helps with the clock resolution issues. */
/* Every ISCALE ops, inject delay to maintain requested op_rate */
#define ISCALE 50		/* always overridden */

#define COPY_CHUNK 2
#define CHUNK_SIZE (32 * 1024)

#define LOCK_RETRY 3

/*
 * Note: The PIT below is a way of getting a timer that is more
 * stable than what happens in a virtual machine, but... it may have
 * scaling issues as the number o threads goes up... this may
 * swarm the PIT server and introduce intrusion.
 */
#define DFLT_SERVICE   "PIT"	/* Default service name.               */
#ifdef _WIN32
#define INVALID_DESC   INVALID_SOCKET
#else
#define INVALID_DESC   -1	/* Invalid file (socket) descriptor.   */
#endif
#define MAXBFRSIZE     256	/* Max bfr sz to read remote TOD.      */

#define DEDUP_CHUNK 512

/*
 * Section for the parser. This parser pulls out the configuration 
 * information using a token based scheme.
 */

#define NUM_INPUT_TOKENS 10 
#define MAX_INPUT_TOKENLEN 1024
#define T_CLIENT_NAME 0
#define T_USER_NAME 1
#define T_PASSWORD 2
#define T_WORK_DIR 3
#define T_EXEC_PATH 4
#define T_WORKLOAD 5
#define T_INSTANCES 6 
#define T_LAUNCH_TYPE 7 
#define T_LOG_PATH 8 
#define T_WINDOWS_LOG_PATH 9 

#if defined(STATIC_GRANULE)
/* A 4096 byte granule size seems to be small enough to work with most
 * archetectures.
 */
#define GRANULE 4096LL
#endif

/* Region types */
#define DEDUP 1
#define COMP 2
#define RAND 3

/* 
 * Just in case there is no workdir provided 
 */
#define HOME "/tmp"

#endif
