
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
 *      Contributor: Udayan Bapat, NetApp, Inc.
 */
#include "./copyright.txt"
#include "./license.txt"

#define PDSM_RO_ACTIVE
#define PDSM_CO_ACTIVE

#if defined(WIN32)
extern int w_gettimeofday (struct timeval *, struct w_timezone *);
#endif
/* 
 * Turn this on to enable insanly fast run configuration mode. 
 * The cmdline.run_time period (see -t) can go down to 60 seconds.
 */
/*
#define FAST
*/
#if defined(WIN32)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4133)
#pragma warning(disable:4244)
#pragma warning(disable:4102)
#pragma warning(disable:4018)

#define _CRT_RAND_S

#include <win32_sub.h>
#include <win32_getopt.h>
#include <time.h>
#endif


#ifdef WIN32
#include <stdint.h>
#define u_int32_t uint32_t
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_linux_)
#include <fcntl.h>
#include <sys/inotify.h>
#endif

#if defined(_solaris_)
#include <port.h>
#include <fcntl.h>
#endif

#if !defined(WIN32)
#include <sys/file.h>
#endif

#if defined(_solaris_)
#include <time.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

#elif !defined(WIN32)
#include <sys/time.h>
#include <sys/signal.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#endif

#if defined(_macos_)
#include <signal.h>
#include <sys/mount.h>
#include <sys/param.h>
#endif
#if defined(_linux_)
#include <time.h>
#endif

#include <string.h>
#if defined(WIN32)
#define strdup _strdup		/* ANSI vs POSIX */
#endif

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#if !defined(WIN32)
#include <strings.h>
#endif

#include <inttypes.h>

/* This is different for the various versions. */
#include <netmist.h>

#include "netmist_version.h"
#include "netmist_defines.h"
#include "netmist_copyright.h"
#include "netmist_utils.h"
#include "netmist_prime.h"
#include "netmist_nodeManager.h"
#include "netmist_client.h"
#include "netmist_structures.h"
#include "netmist_logger.h"
#include "netmist_fdcache.h"
#include "netmist_methodtable.h"

#include "netmist_vfs.h"

/* XXX Hack to help stage to the VFS API */
#if defined(WIN32)
#include "netmist_vfs_fs_win32.c"
#else
#include "netmist_vfs_fs_posix.c"
#endif

/* This is different for FULL versus "lite" versions. */
#include "./import.c"

#include "yaml.h"

#include "import_yaml.c"

#if !defined(WIN32)
#include <sys/resource.h>
#endif

extern int max_licensed_clients;
extern unsigned int lic_key;
extern int rsize;
extern long long file_accesses;
extern long long fd_accesses;
extern long long fd_cache_misses;
int num_work_obj;
int netmist_engine_major_version = 3;
int netmist_engine_minor_version = 0;
void prime_check_license_file (void);

extern void print_md5 (char *, unsigned *);
extern unsigned *md5 (char *, int);

#if defined(WIN32)
  /*
   * Name of the power monitor script 
   */
char power_script[] = "netmist_power.cmd";
  /*
   * Name of the external monitor script 
   */
char mon_script[] = "netmist_mon.cmd";

  /*
   * Name of the external stat script 
   */
char stat_script[] = "netmist_stat.cmd";
#else
  /*
   * Name of the power monitor script 
   */
char power_script[] = "./netmist_power.sh";
  /*
   * Name of the external monitor script 
   */
char mon_script[] = "./netmist_mon.sh";

  /*
   * Name of the external stat script 
   */
char stat_script[] = "./netmist_stat.sh";
#endif
/*
 * Name of the external client startup script 
 * 
 * Everyone else gets a standard Korn shell type
 * script file.
 *
 * REMEMBER !!! This thing gets started from the default
 * location of a remote login. 
 * So... be sure you put this thing where it belongs.
 *
 * Note: This script can map shares in Unix, 
 * but things may get weird in SUA or in an Active Directory
 * Domain, where logins may land you in strange lands.
 * 
 */
#if defined (WIN32)
char pretest_script[] = "netmist_pretest.cmd";
#else
char pretest_script[] = "./netmist_pretest.sh";
#endif

extern int lru_on;

/*
 * Define the mix. The defaults are imported from netmist.h
 * All of these can be set to new values by importing a new 
 * set of workload definitions from a file.
 */
struct mix_table my_mix_table[MAX_WORK_OBJ];
#if !defined(BUILT_IN_WORKLOADS)
extern struct mix_table my_stdmix_table[];
#endif

/****************************** Plug-ins ***************************/
/* A new plugin will need these functions implemented.             */
/* There are generic implementations for all of these, and they    */
/* are what is used for the current workloads.                     */
/****************************** Plug-ins ***************************/
/*
	read, read_rand, read_file, mmap_read, write, write_rand, 
	write_file, mmap_write, rmw, init_dir, init_file, init_empty_file,
	init_dir, mkdir, unlink, append, locking, access, stat, chmod, 
	remove_file, remove_dir, readdir, rename, copyfile, statfs,
	stat_workdir, make_stat, fill_buf, prefix_op, postfix_op,
	pathconf.
*/
char *netmist_strerror (int);
int netmist_get_error (void);
int geometric_rand (int);
int netmist_rand (void);
unsigned long long netmist_llrand (void);
int picker (int, int);
void set_direct_flag (long, int, enum netmist_vfs_open_flags *);
void set_fadvise_seq (long, int, enum netmist_vfs_open_flags *);
void set_fadvise_rand (long, int, enum netmist_vfs_open_flags *);
void set_fadvise_dont_need (long, int, enum netmist_vfs_open_flags *);
void set_osync_flag (long, int, enum netmist_vfs_open_flags *);
void slinky (int, int, int, struct netmist_vfs_dir **, int);
void make_chaff (struct netmist_vfs_dir *, int, int, int);
void chaff_init_empty_file (char *);
time_t pdsm_cur_stat_mod_time;

void close_cached_file_desc (void);

/*
 * Buckets used for histograms 
*/

void hist_insert (double);
void child_dump_hist (FILE *);
void dump_hist (void);

unsigned int check_sum (struct mix_table *, unsigned int);


void set_pdsm_attributes (void);
int check_op_total (void);
int modified_run;
void purge_file (struct file_object *);
unsigned long long get_next_fsize (long long);
void push_name (struct netmist_vfs_dir *, char *);
uint64_t get_next_read_size (void);
uint64_t get_next_write_size (void);
void drain_list (void);

char *my_strncpy (char *, char *, size_t);
int quiet_now;

long long fixed_value = 0LL;
long long save_epoch = 0LL;
extern int global_start_enc;

/************************** Plug-ins ************************************/
/* This is where your default plug-in goes so that the gen_op( ) will   */
/* go through your plug-in implementation of the functions.             */
/* 								        */
/* Please remember, these will be over-ridden by the workload fs        */
/* type. The default is POSIX, but others may exist 	                */
/* The client_startup will look at the mix_table.fs_type, and will      */
/* replace all of these pointers, with the ones for the specific 	*/
/* workload.								*/
/* 								        */
/************************** Plug-ins ************************************/
volatile long *global_reserve_c_memory;
volatile long *global_reserve_n_memory;
struct method_table *Netmist_Ops;
struct work_object *work_obj;
extern struct work_object *workloads;
char remcp[] = REMCP;

unsigned long long avg_read_size, avg_write_size, avg_file_size;

int my_workload;

int init_phase = 0;
int in_validate = 0;
int init_heartbeats = 0;

time_t delivered_heartbeat, current_time;
time_t delivered_log_heartbeat;

int read_dist_array[RW_ARRAY];
int write_dist_array[RW_ARRAY];
int file_size_dist_array[RW_ARRAY];

double read_throughput, write_throughput, Nread_throughput, Nwrite_throughput;
double read_kbytes, write_kbytes, Nread_kbytes, Nwrite_kbytes;
double meta_r_kbytes, meta_w_kbytes;
double file_space_mb;

#if defined(PDSM_RO_ACTIVE)
char my_pdsm_file[MAXNAME];
struct pdsm_remote_stats *pdsm_stats;
#if defined(WIN32)
HANDLE pdsm_file_fd;
#else
int pdsm_file_fd;
#endif
int pdsm_mode;			/* 0 = over-write, 1 = append */
int pdsm_interval;
#endif

#if defined(PDSM_CO_ACTIVE)
int pdsm_control_once;
char my_pdsm_control_file[MAXNAME];
struct pdsm_remote_control *pdsm_control;
#if defined(WIN32)
HANDLE pdsm_control_file_fd;
LARGE_INTEGER largeoffset;
#else
int pdsm_control_file_fd;
#endif
int pdsm_get_control_data (struct pdsm_remote_control *);
int pdsm_put_control_data (struct pdsm_remote_control *);
#endif

#if defined(WIN32)
char wmiUser[MAXNAME] = "administrator";
char wmiDomain[MAXNAME] = "fsperf";
char wmiPassword[MAXNAME] = "Lab$Man01";
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#endif

/**
 * @brief Initialize the size distribution mechanism
*/
/*
 * __doc__
 * __doc__  Function : void init_dist_tables(void)
 * __doc__  Arguments: void 
 * __doc__  Performs : Initialize the size distribution mechanism
 * __note__ Notes    : my_workload is already set by now !!
 * __doc__
 */
void
init_dist_tables (void)
{
    int i, j, k;
    unsigned long long total = 0LL;
    unsigned long long fixed_file_size = 0LL;

    /*--------------------------------------------------------------------*/
    /* read size */
    /*--------------------------------------------------------------------*/
    k = 0;
    /* Fill in the read slots into the percentage table */
    for (i = 0; i < MAX_DIST_ELEM; i++)
    {
	for (j = my_mix_table[my_workload].read_dist[i].percent; j > 0; j--)
	{
	    read_dist_array[k] = i;
	    k++;
	}
    }
    /* 
     * Calculate the average write size for both the fixed size, and the distribution table.
     */
    total = 0LL;
    for (i = 0; i < RW_ARRAY; i++)
    {
	total +=
	    (my_mix_table[my_workload].read_dist[read_dist_array[i]].
	     size_min +
	     my_mix_table[my_workload].read_dist[read_dist_array[i]].
	     size_max) / 2LL;
    }
    avg_read_size = (unsigned long long) (total / (long long) RW_ARRAY);
    log_file (LOG_EXEC, "Average read size = %lld Kib\n",
	      avg_read_size / 1024LL);

    /*--------------------------------------------------------------------*/
    /* write size */
    /*--------------------------------------------------------------------*/
    k = 0;

    /* Fill in the write slots into the percentage table */
    for (i = 0; i < MAX_DIST_ELEM; i++)
    {
	for (j = my_mix_table[my_workload].write_dist[i].percent; j > 0; j--)
	{
	    write_dist_array[k] = i;	/* Store the slot number, not the value */
	    k++;
	}
    }
    /* 
     * Calculate the average write size for both the fixed size, and the distribution table.
     */
    total = 0LL;
    for (i = 0; i < RW_ARRAY; i++)
    {
	total +=
	    (my_mix_table[my_workload].write_dist[write_dist_array[i]].
	     size_min +
	     my_mix_table[my_workload].write_dist[write_dist_array[i]].
	     size_max) / 2LL;
    }
    avg_write_size = (unsigned long long) (total / (long long) RW_ARRAY);
    log_file (LOG_EXEC, "Average write size = %lld Kib\n",
	      avg_write_size / 1024LL);

    /*--------------------------------------------------------------------*/
    /* file size */
    /*--------------------------------------------------------------------*/
    k = 0;

    /* Fill in the file_size slots into the percentage table */
    for (i = 0; i < MAX_DIST_ELEM; i++)
    {
	if (my_mix_table[my_workload].use_file_size_dist)
	{
	    for (j = my_mix_table[my_workload].file_size_dist[i].percent;
		 j > 0; j--)
	    {
		file_size_dist_array[k] = i;	/* Store the slot number, not the value */
		k++;
	    }
	}
	else
	{
	    fixed_file_size = my_mix_table[my_workload].file_size * 1024LL;
	}
    }
    /* 
     * Calculate the average file size for both the fixed size, and the distribution table.
     */
    total = 0LL;
    for (i = 0; i < RW_ARRAY; i++)
    {
	if (my_mix_table[my_workload].use_file_size_dist)
	{
	    total +=
		(my_mix_table[my_workload].
		 file_size_dist[file_size_dist_array[i]].size_min +
		 my_mix_table[my_workload].
		 file_size_dist[file_size_dist_array[i]].size_max) / 2LL;
	}
	else
	{
	    total += fixed_file_size;
	}
    }
    avg_file_size = (unsigned long long) (total / (long long) RW_ARRAY);
    log_file (LOG_EXEC, "Average file size = %lld Kib\n",
	      avg_file_size / 1024LL);
}

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

#if defined(_bsd_)
#include <signal.h>
#endif

#if defined(_solaris_) || defined(_bsd_)
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include "netmist_if.h"
#include "netmist_random.h"

long long buckets[BUCKETS];
long long global_buckets[BUCKETS];
long long bucket_val[BUCKETS] = { 20, 40, 60, 80, 100,
    200, 400, 600, 800, 1000,
    2000, 4000, 6000, 8000, 10000,
    12000, 14000, 16000, 18000, 20000,
    40000, 60000, 80000, 100000,
    200000, 400000, 600000, 800000, 1000000,
    2000000, 4000000, 6000000, 8000000, 10000000,
    20000000, 30000000, 60000000, 90000000, 120000000, 120000001
};

int child_port;

long *mem_buf;
char *main_buf;
char *copy_file_buf;

int bad_clock = 0;

long long time_skew;
int special;
int ubuntu;

int block_mode = 0;
int max_open;
int max_proc;
int rot;
int report_name;
extern int cached_open_count;
extern long long lru_promote_count;
extern long long lru_reuse_count;
int guard_band = 14;		/* Reserve so that we don't cache too many fds */
int Multi_load_disp = 0;
int junk;
char *big_buf;
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr_in6 sockaddr_in6_t;
struct addrinfo *ai;
int aiErr;
struct addrinfo hints;
struct addrinfo *aiHead;
struct sockaddr_in6 *pSadrIn6;
int one_time_unlink2;
int *unlink2_dir_array;

int spot_pct, min_acc_per_spot, acc_mult_spot;
int nspots;
int percent_affinity;
int spot_shape;

long long hit_in_spot = 1000;	/* default . Is calculated later */
unsigned long jitter;

long long spot_size;
long long offset_in_spot;
/*
sockaddr_in6_t  *pSadrIn6;
*/
int once;
int ecount;

int map_index;


FILE *tracefile;
int LogonUser_res = 0;
int ImpersonateLoggedOnUser_res = 0;
int retry1_counter, retry2_counter;

char *my_strcasestr (char *, char *);
int my_strcasecmp (char *, char *);
void *my_malloc (size_t);

void init_space (void);
void dump_file_objects (FILE *);

int dir_exist (void);
int is_empty_line (char *);
int is_comment (char *);


#if defined(_solaris_)
int pit_gettimeofday (struct timeval *, char *, char *);
void pit (int, struct timeval *);
#endif

#if !defined(_solaris_)
int pit_gettimeofday (struct timeval *, char *, char *);
void pit (int, struct timeval *);
#endif

#if defined(_solaris_)
double m_gettimeofday (struct timeval *);
#else
double m_gettimeofday (struct timeval *);
#endif

#ifndef _WIN32
typedef int SOCKET;
#define closesocket close
#endif

SOCKET openSckt (const char *, const char *, unsigned int);

int power_mon (int, char *);
int extern_mon (int, char *);
int u_extern_mon (int, char *);
int extern_stat (int, char *);

void tell_prime_pct_complete (int, int, int);
void tell_prime_heartbeat (int, int, double);

void tell_prime_results (int);
void tell_nm_error (int, enum netmist_child_err, int);
void tell_prime_error (int, enum netmist_child_err eval, int);
void send_warmup_done (void);

void make_name (char *, int, int, int);
void make_short_name (char *, int, int);
static int prename_len (void);
static int postname_len (void);
static void init_dist (void);
static char get_ch1 (void);
void netmist_srand (int);
int netmist_gseed (void);

void _start_encryption (unsigned long long, int, int);
void _end_encryption (void);
unsigned char _crypt_char (unsigned char);
unsigned char _decrypt_char (unsigned char);


/*
 * Each client is assigned an ID when the comm is set up.
 */
int c_chid;
int phase = INIT_PHASE;

void prime_dump_hist (struct result_object *, FILE *);
void prime_dump_hist_in_csv (struct result_object *, FILE *);

extern struct client_object *client_obj;

int max_xfer_size;		/* Scope: Global:  In bytes     */

int bad_name_option;		/* Used by client to kill bad workload names */

int test_ops = TESTOPS;

/*
 * There are one of these work_objects for each unique
 * workload 
 */

extern int num_clients;

/*
 * The test array of possible ops to be performed.
 */
struct test
{
    int op_type;
    int status_flags;		/* Future */
    int writer_id;		/* Future */
};

/*
 * Global counters for statistics. Needs to be in 
 * shared memory if the client forks.
 */

double total_rmw_ops;
double total_meta_r_ops;
double total_meta_w_ops;
double total_read_bytes;
double total_read_rand_bytes;
double total_write_bytes;
double total_copyfile_bytes;
double total_write_rand_bytes;
double total_rmw_bytes;
double total_file_ops;
double before_total_file_ops;
double prev_file_ops;
double ops_this_tick;
double total_file_op_time;
double my_gen_op_time;
double sanity_time;
double total_nap_time;
long long total_init_kbytes;
float total_init_time;
float avg_init_mb_per_sec;
int global_reset_counter;

/*
 * Pointer off to the test entries array. Allocated dynamically.
 */
struct test *tests;
struct test *testd;

/*
 * Prototypes
 */
double gettime (void);
void clean_exit (enum netmist_child_err, int);
void sig_clean_exit (int);
void sig_outch (int);
void sig_term (int);
void pipe_elog (int);
void R_exit (enum netmist_child_err, int);

/*
 * Needed externs
 */
#if !defined(WIN32)
extern int errno;
#endif

/*
 * The total number of operations to perform. In the future it
 * may be infinite, as the test may terminate based on run
 * time.
 */
int number_ops;

/*
 * Used to create unique directory structure for each client or 
 * child process of a client.
 */
int impersonate;

/* 
 * Name of benchmark. "Generic Remote All-purpose Filesystem Test" 
 */
#if defined(SPEC_DIST)
char testname[] = "SPECstorage(TM) Solution 2020";
char short_testname[] = "SPECstorage(TM) Solution 2020";
#endif
#if defined(PRO)
char testname[] = "SPECstorage(TM) Solution 2020";
char short_testname[] = "SPECstorage(TM) Solution 2020";
#endif
#if defined(LITE)
char testname[] = "SPECstorage(TM) Solution 2020 Lite";
char short_testname[] = "SPECstorage(TM) Solution 2020 Lite";
#endif

char bin_testname[] = "netmist";

int gdir_count;
int gfile_count, init_new_file;
int gempty_count;
int my_ops, my_warmup_ops;

/* 
 * Used for latency statistics
 */
double my_time_res = 9999999.99;
double cur_time, my_run_time, last_heart_beat;
#if defined(PDSM_RO_ACTIVE)
double last_pdsm_beat;
#endif

double my_start_time, post_time;
double my_start_time2;
double my_cur_time, my_last_time;

/* Keep track of time spent napping */
double seconds_to_nap, total_seconds_napped;
int naps_taken;

int tick;

char client_name[MAXHNAME];
char client_localname[MAXHNAME];
extern char client_filename[MAXNAME];

/*
 * Prototypes and forward declarations.
 */

#include "netmist_workfun.h"

void generic_null (void);
void clear_stats (void);

void usage (void);

void init_tests (int);
void create_tests (int);
int gen_op (int, int);
void client_init_files (int);
void client_init_block (char *);
void client_validate_ops (void);

void client_remove_files (int);
int lock_file (struct file_object *);
#if defined(WIN32)
int Windows_authenticate_and_login (void);
#endif

void purge_cache (void);
void dump_table (void);

void nap (int);
void make_data_layout (long *, int, struct file_object *, long long, int,
		       int);
void make_cypher (char *, int);
void make_non_dedup (long *, int);
void make_buf (int, int, int, char *, long long, long long, long long,
	       long long, long long, int, int);

#if !defined(WIN32)
#include <dlfcn.h>
void *vfs_so_handle = NULL;
#endif

static const struct netmist_vfs_v0 *vfst;
static struct netmist_vfs_rock *vfsr;
#define VFS(fn, ...) (vfst->netmist_vfs_ ## fn(vfsr, __VA_ARGS__))

/* Special case: no arguments other than rock */
static inline struct netmist_vfs_dir *
netmist_vfs_root (void)
{
    return vfst->netmist_vfs_root (vfsr);
}

/* Special case: no arguments, even rock */
const char *
netmist_vfs_errstr (int e)
{
    return vfst->netmist_vfs_errstr (e);
}

/*
 * The working directory is where almost all of the benchmark takes place; it's
 * a descendant of the VFS root (of course), made with some slinky() action at
 * initialization for its construction.  After initialization, almost all VFS
 * references will be via this directory handle or something derived from it.
 * See setup_workdir and its callers.
 */
static struct netmist_vfs_dir *netmist_vfs_workdir = NULL;

/* Special case: no arguments, even rock 
 * Windows has special error value handling for Socket function errors.
 */
int
netmist_get_sock_error ()
{
#if defined(WIN32)
    return WSAGetLastError ();
#else
    return errno;
#endif
}

/* Special case: no arguments, even rock 
 * Windows has special error value handling for Socket function errors.
 */
char *
netmist_get_sock_errstr (int err)
{
#if defined(WIN32)
    return win32_WSAstrerror (err);
#else
    return strerror (err);
#endif
}

/* Exported for fdcache. */
int
netmist_vfs_close (struct netmist_vfs_object **o)
{
    return VFS (close, o);
}

/* A very common special case of walk */
static inline int
netmist_vfs_walkme (struct netmist_vfs_dir **me, char *rfn)
{
    return VFS (walk, *me, rfn, me);
}

/**
 * @brief Main entry point for Prime and remote children.
 *
 * @param argc : Argument count
 * @param argv : Pointer to the args
 */

/*
 * __doc__
 * __doc__  Function : int main( int argc, char ** argv)
 * __doc__  Arguments: Argument count, the args
 * __doc__  Performs : Main entry point for Prime and remote children.
 * __note__            Entry point where the controlling process starts the
 * __note__            universe. The clients also enter here but take a 
 * __note__            different path to their destiny.
 * __doc__
 */
int
main (int argc, char **argv)
{

#if defined(RLIMIT_NPROC)
    struct rlimit rlimit_struct;
#endif

#if defined(USE_BUILTIN_WORKLOADS)
    /* 
     * Start with the built-in workloads. May change contents if importing workloads. 
     */
    /* Moved to always import from YAML in Storage2020 and later.
     */
    memcpy (my_mix_table, my_stdmix_table,
	    sizeof (struct mix_table) * NUM_INTERNAL_WORK_OBJ);
#endif

    /*
     * Initialize default state of all command line options
     */
    init_command_line_args ();

    /*
     * Handy signal handler used to cleanup when things go wrong.
     */
    signal (SIGINT, sig_clean_exit);
    signal (SIGSEGV, sig_outch);
    signal (SIGILL, sig_outch);


#if defined(WIN32_TIME_DEBUG)
    time_t ltime;
    FILETIME SystemTimeAsFileTime;
    ULARGE_INTEGER SystemTimeINT;

    GetSystemTimeAsFileTime (&SystemTimeAsFileTime);
    SystemTimeINT.HighPart = SystemTimeAsFileTime.dwHighDateTime;
    SystemTimeINT.LowPart = SystemTimeAsFileTime.dwLowDateTime;
    printf ("Windows time %llu \n",
	    (((uint64_t) SystemTimeINT.QuadPart / 10LL) -
	     DELTA_EPOCH_IN_MICROSECS) / 1000000LL);
    time (&ltime);
    printf ("Unix time %llu \n", (uint64_t) ltime);
#endif

#if defined(WIN32)
    /*
     * Warning: Evil M$ Black magic here !! 
     * https://docs.microsoft.com/en-us/windows/console/registering-a-control-handler-function
     */
    SetConsoleCtrlHandler ((PHANDLER_ROUTINE) sig_clean_exit, TRUE);
#endif
#if !defined(WIN32)
    signal (SIGPIPE, pipe_elog);
#endif
    my_strncpy (cmdline.workdir, HOME, sizeof (cmdline.workdir));
    if (argc == 1)
    {
	usage ();
	exit (0);
    }
#if !defined(WIN32)
    /*
     * If any process gets a broken pipe (on its sockets), then log and 
     * terminate.
     * This should **NEVER** happen, but it can if the operating system
     * on the clients starts closing sockets with software resets....
     */
    signal (SIGPIPE, pipe_elog);
#endif

    /* Setup default */
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
	log_stdout (LOG_EXEC,
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
    /* >>>>>>  I don't think my_workload has been setup yet !!!  <<<<<
     * So... we will setup some rational limits, and re-initialize this after
     * my_workload has been setup.

     if ((IS_SP1_WL (my_workload)) || IS_SP2_WL (my_workload))
     max_open = WIN32_MAX_OPEN;

     if (IS_SFS2020_WL (my_workload))
     max_open = WIN32_MAX_OPEN_SFS2020;
     */

    max_open = WIN32_MAX_OPEN_SFS2020;
    max_proc = WIN32_MAX_PROC;
#endif

#ifdef MERSENNE
    /* Initialize the 64 bit random number generator */
    init_by_array64 (init, length);
#endif

    /* Create some defaults for whatever platform */
    /* Windows */
    my_strncpy (cmdline.client_windows_log_dir, "C:\\tmp", MAXNAME);
    cmdline.client_windows_log_dir[strlen ("C:\\tmp")] = 0;

    /* Unix */
    my_strncpy (cmdline.client_log_dir, "/tmp", MAXNAME);
    cmdline.client_log_dir[strlen ("/tmp")] = 0;

#if defined(WIN32)
    win32_init ();
#endif

    my_strncpy (cmdline.pit_service, "PIT", 8);	/* Default */
    cmdline.pit_service[strlen ("PIT")] = 0;

    /*
     * This parses all command line arguments
     */
    parse_command_line (argc, argv);

    /*
     * Print version and exit
     */
    if (cmdline.vflag)
    {
	reset_timestamp_in_log_stdout ();
	log_stdout (LOG_EXEC, "%s %s I/O Engine: %d.%d\n", short_testname,
		    git_version, netmist_engine_major_version,
		    netmist_engine_minor_version);
	log_stdout (LOG_EXEC, "Build time: %s\n", git_date);
	exit (0);
    }

    /* YAML */
    /* Move to point after my_workload is setup. 
       cmdline.fsize = my_mix_table[my_workload].file_size;
     */

    if (is_prime ())
    {
	do_all_prime_work ();
    }
    else if (is_nodeManager ())
    {
	do_all_nodeManager_work ();
    }
    else if (is_client ())
    {
	do_all_client_work ();
    }
    else
    {				/* This should not happen ever! */
	log_stdout (LOG_EXEC, "Undefined state!!!\n");
	exit (1);
    }

    return (0);
}

/**
 * @brief Gather up all of the results associated with each 
 *        client proc and save these in the Client_#_results files.
*/
/*
 * __doc__
 * __doc__  Function : void print_results(void)
 * __doc__  Arguments: void 
 * __doc__  Performs : Gather up all of the results associated with each 
 * __doc__             client proc and save these in the Client_#_results files.
 * __doc__             Only called by the Prime.
 */
void
print_results (void)
{
    struct result_object *here;
    double total_latency = 0.0;
    double total_ops_per_second = 0.0;
    double average_latency, ops_per_second;
    double total_read_throughput = 0.0;
    double total_write_throughput = 0.0;
    double total_throughput = 0.0;
    double count = 0.0;
    unsigned int sum_val = 0;
    unsigned long long total_file_space = 0LL;
    int u, i;
    int worked = 0;
    double skip = 0.0;
    char ch_filename[400];
    FILE *ch_out;

    for (i = 0; i < num_clients; i++)
    {
	here = &client_obj[i].results;

#if defined(WIN32)
	snprintf (ch_filename, sizeof ch_filename,
		  "%s\\%s%d_results", cmdline.prime_results_dir,
		  "Client_", here->client_id);
#else
	snprintf (ch_filename, sizeof ch_filename,
		  "%s/%s%d_results", cmdline.prime_results_dir,
		  "Client_", here->client_id);
#endif
	ch_out = fopen (ch_filename, "a+");
	if (ch_out < (FILE *) 0)
	{
	    log_stdout (LOG_ERROR, "Error opening children logs: %s\n",
			netmist_strerror (netmist_get_error ()));
	    exit (1);
	}

	fprintf (ch_out,
		 "--------------------------------------------------------------------\n");
	fprintf (ch_out, "Client %30s                  ID: %10d\n",
		 here->client_name, here->client_id);
	fprintf (ch_out, "Workload Name:                   %14s\n",
		 here->work_obj_name);
	fprintf (ch_out, "Run time                         %10ld\n",
		 here->run_time);
	fprintf (ch_out, "Ops/sec                          %10.2f\n",
		 here->ops_per_second);
	fprintf (ch_out, "Avg Latency                      %10.3f\n",
		 here->average_latency);
	fprintf (ch_out, "Total file ops                   %10.0f\n",
		 here->total_file_ops);
	fprintf (ch_out,
		 "Read throughput                  %10.3f Kbytes/sec\n",
		 here->read_throughput / 1024);
	fprintf (ch_out, "Read kbytes                      %10.3f Kbytes\n",
		 here->read_kbytes / 1024);
	fprintf (ch_out,
		 "Write throughput                 %10.3f Kbytes/sec\n",
		 here->write_throughput / 1024);
	fprintf (ch_out, "Write Kbytes                     %10.3f Kbytes\n",
		 here->write_kbytes / 1024);
	fprintf (ch_out,
		 "Native_read throughput           %10.3f Kbytes/sec\n",
		 here->Nread_throughput / 1024);
	fprintf (ch_out, "Native read kbytes               %10.3f Kbytes\n",
		 here->Nread_kbytes / 1024);
	fprintf (ch_out,
		 "Native_write throughput          %10.3f Kbytes/sec\n",
		 here->Nwrite_throughput / 1024);
	fprintf (ch_out, "Native_write Kbytes              %10.3f Kbytes\n",
		 here->Nwrite_kbytes / 1024);
	fprintf (ch_out,
		 "Meta_write throughput            %10.3f Kbytes/sec\n",
		 (here->meta_w_kbytes / 1024) / here->run_time);
	fprintf (ch_out, "Meta_write Kbytes                %10.3f Kbytes\n",
		 here->meta_w_kbytes / 1024);
	fprintf (ch_out,
		 "Meta_read throughput             %10.3f Kbytes/sec\n",
		 (here->meta_r_kbytes / 1024) / here->run_time);
	fprintf (ch_out, "Meta_read Kbytes                 %10.3f Kbytes\n",
		 here->meta_r_kbytes / 1024);
	fprintf (ch_out, "Initialized dirs                 %10d \n",
		 here->init_dirs);
	fprintf (ch_out, "Initialized files                %10d \n",
		 here->init_files);
	fprintf (ch_out, "Initialized files with space     %10d \n",
		 here->init_files_ws);
	fprintf (ch_out, "Per proc file space             ~%10.0f Mbytes\n",
		 here->file_space_mb);
	fprintf (ch_out, "Min Direct I/O size =            %10ld\n",
		 here->min_direct_size);

	/* Write */
	fprintf (ch_out, "\nDetails\n");
	/*
	 * Here are the outputs for all of the standard workloads. 
	 */

	fprintf (ch_out, "\t%-35s ops = %10ld", here->write_string,
		 here->write_count);
	if (cmdline.Op_lat_flag && (here->write_count != 0)
	    && (here->write_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->write_time / (double) here->write_count,
		     here->min_write_latency, here->max_write_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Write file */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->write_file_string,
		 here->write_file_count);
	if (cmdline.Op_lat_flag && (here->write_file_count != 0)
	    && (here->write_file_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->write_file_time / (double) here->write_file_count,
		     here->min_write_file_latency,
		     here->max_write_file_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* mmap write */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->mmap_write_string,
		 here->mmap_write_count);
	if (cmdline.Op_lat_flag && (here->mmap_write_count != 0)
	    && (here->mmap_write_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->mmap_write_time / (double) here->mmap_write_count,
		     here->min_mmap_write_latency,
		     here->max_mmap_write_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* mmap read */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->mmap_read_string,
		 here->mmap_read_count);
	if (cmdline.Op_lat_flag && (here->mmap_read_count != 0)
	    && (here->mmap_read_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->mmap_read_time / (double) here->mmap_read_count,
		     here->min_mmap_read_latency,
		     here->max_mmap_read_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* read */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->read_string,
		 here->read_count);
	if (cmdline.Op_lat_flag && (here->read_count != 0)
	    && (here->read_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->read_time / (double) here->read_count,
		     here->min_read_latency, here->max_read_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* read file */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->read_file_string,
		 here->read_file_count);
	if (cmdline.Op_lat_flag && (here->read_file_count != 0)
	    && (here->read_file_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->read_file_time / (double) here->read_file_count,
		     here->min_read_file_latency,
		     here->max_read_file_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* mkdir */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->mkdir_string,
		 here->mkdir_count);
	if (cmdline.Op_lat_flag && (here->mkdir_count != 0)
	    && (here->mkdir_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->mkdir_time / (double) here->mkdir_count,
		     here->min_mkdir_latency, here->max_mkdir_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* rmdir */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->rmdir_string,
		 here->rmdir_count);
	if (cmdline.Op_lat_flag && (here->rmdir_count != 0)
	    && (here->rmdir_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->rmdir_time / (double) here->rmdir_count,
		     here->min_rmdir_latency, here->max_rmdir_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* unlink */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->unlink_string,
		 here->unlink_count);
	if (cmdline.Op_lat_flag && (here->unlink_count != 0)
	    && (here->unlink_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->unlink_time / (double) here->unlink_count,
		     here->min_unlink_latency, here->max_unlink_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* unlink2 */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->unlink2_string,
		 here->unlink2_count);
	if (cmdline.Op_lat_flag && (here->unlink2_count != 0)
	    && (here->unlink2_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->unlink2_time / (double) here->unlink2_count,
		     here->min_unlink2_latency, here->max_unlink2_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* create */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->create_string,
		 here->create_count);
	if (cmdline.Op_lat_flag && (here->create_count != 0)
	    && (here->create_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->create_time / (double) here->create_count,
		     here->min_create_latency, here->max_create_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* stat */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->stat_string,
		 here->stat_count);
	if (cmdline.Op_lat_flag && (here->stat_count != 0)
	    && (here->stat_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->stat_time / (double) here->stat_count,
		     here->min_stat_latency, here->max_stat_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* neg_stat */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->neg_stat_string,
		 here->neg_stat_count);
	if (cmdline.Op_lat_flag && (here->neg_stat_count != 0)
	    && (here->neg_stat_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->neg_stat_time / (double) here->neg_stat_count,
		     here->min_neg_stat_latency, here->max_neg_stat_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* append */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->append_string,
		 here->append_count);
	if (cmdline.Op_lat_flag && (here->append_count != 0)
	    && (here->append_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->append_time / (double) here->append_count,
		     here->min_append_latency, here->max_append_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Lock */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->lock_string,
		 here->lock_count);
	if (cmdline.Op_lat_flag && (here->lock_count != 0)
	    && (here->lock_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->lock_time / (double) here->lock_count,
		     here->min_lock_latency, here->max_lock_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* append */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->access_string,
		 here->access_count);
	if (cmdline.Op_lat_flag && (here->access_count != 0)
	    && (here->access_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->access_time / (double) here->access_count,
		     here->min_access_latency, here->max_access_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* chmod */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->chmod_string,
		 here->chmod_count);
	if (cmdline.Op_lat_flag && (here->chmod_count != 0)
	    && (here->chmod_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->chmod_time / (double) here->chmod_count,
		     here->min_chmod_latency, here->max_chmod_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* readdir */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->readdir_string,
		 here->readdir_count);
	if (cmdline.Op_lat_flag && (here->readdir_count != 0)
	    && (here->readdir_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->readdir_time / (double) here->readdir_count,
		     here->min_readdir_latency, here->max_readdir_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* write rand */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->write_rand_string,
		 here->write_rand_count);
	if (cmdline.Op_lat_flag && (here->write_rand_count != 0)
	    && (here->write_rand_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->write_rand_time / (double) here->write_rand_count,
		     here->min_write_rand_latency,
		     here->max_write_rand_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Read rand */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->read_rand_string,
		 here->read_rand_count);
	if (cmdline.Op_lat_flag && (here->read_rand_count != 0)
	    && (here->read_rand_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->read_rand_time / (double) here->read_rand_count,
		     here->min_read_rand_latency,
		     here->max_read_rand_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* rmw */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->rmw_string,
		 here->rmw_count);
	if (cmdline.Op_lat_flag && (here->rmw_count != 0)
	    && (here->rmw_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->rmw_time / (double) here->rmw_count,
		     here->min_rmw_latency, here->max_rmw_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	fprintf (ch_out, "\t%-35s ops = %10ld", "Open file",
		 here->open_count);
	fprintf (ch_out, "\tAvg Latency:   Not collected\n");

	fprintf (ch_out, "\t%-35s ops = %10ld", "Close file",
		 here->close_count);
	fprintf (ch_out, "\tAvg Latency:   Not collected\n");

	/* Copyfile */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->copyfile_string,
		 here->copyfile_count);
	if (cmdline.Op_lat_flag && (here->copyfile_count != 0)
	    && (here->copyfile_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->copyfile_time / (double) here->copyfile_count,
		     here->min_copyfile_latency, here->max_copyfile_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Rename */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->rename_string,
		 here->rename_count);
	if (cmdline.Op_lat_flag && (here->rename_count != 0)
	    && (here->rename_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->rename_time / (double) here->rename_count,
		     here->min_rename_latency, here->max_rename_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Statfs */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->statfs_string,
		 here->statfs_count);
	if (cmdline.Op_lat_flag && (here->statfs_count != 0)
	    && (here->statfs_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->statfs_time / (double) here->statfs_count,
		     here->min_statfs_latency, here->max_statfs_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* Pathconf */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->pathconf_string,
		 here->pathconf_count);
	if (cmdline.Op_lat_flag && (here->pathconf_count != 0)
	    && (here->pathconf_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->pathconf_time / (double) here->pathconf_count,
		     here->min_pathconf_latency, here->max_pathconf_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* trunc */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->trunc_string,
		 here->trunc_count);
	if (cmdline.Op_lat_flag && (here->trunc_count != 0)
	    && (here->trunc_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->trunc_time / (double) here->trunc_count,
		     here->min_trunc_latency, here->max_trunc_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* custom1 */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->custom1_string,
		 here->custom1_count);
	if (cmdline.Op_lat_flag && (here->custom1_count != 0)
	    && (here->custom1_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->custom1_time / (double) here->custom1_count,
		     here->min_custom1_latency, here->max_custom1_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");

	/* custom2 */
	fprintf (ch_out, "\t%-35s ops = %10ld", here->custom2_string,
		 here->custom2_count);
	if (cmdline.Op_lat_flag && (here->custom2_count != 0)
	    && (here->custom2_time != 0.0))
	    fprintf (ch_out,
		     " \tAvg Latency: %10.6f  Min Latency: %10.6f  Max Latency: %10.6f\n",
		     here->custom2_time / (double) here->custom2_count,
		     here->min_custom2_latency, here->max_custom2_latency);
	else
	    fprintf (ch_out, " \tAvg Latency:   Not collected\n");


	worked = fprintf (ch_out, "\n");
	if (worked < 0)
	{
	    log_stdout (LOG_ERROR, "Writing %s log failed: %s\n", ch_filename,
			netmist_strerror (netmist_get_error ()));
	}
	if (cmdline.Op_lat_flag)
	{
	    prime_dump_hist (here, ch_out);
	    fprintf (ch_out, "\n");
	}

	fflush (ch_out);
	fclose (ch_out);

	if (cmdline.Op_lat_flag)
	{
	    for (u = 0; u < BUCKETS; u++)
		global_buckets[u] += here->bands[u];
	}
	if (here->background != 0)
	{
	    skip += 1.0;
	}
	else
	{
	    total_ops_per_second += here->ops_per_second;
	    total_latency += here->average_latency;
	    total_read_throughput += here->read_throughput;
	    total_write_throughput += here->write_throughput;
	    total_file_space += here->file_space_mb;
	    total_throughput +=
		here->read_throughput + here->write_throughput;
	    /* Totals for each workload type */
	    here->work_obj_name[strlen (here->work_obj_name)] = 0;
	    my_strncpy (workloads[here->work_obj_index].work_obj_name,
			here->work_obj_name, MAXWLNAME);
	    workloads[here->
		      work_obj_index].work_obj_name[strlen (here->
							    work_obj_name)] =
		0;

	    workloads[here->work_obj_index].total_ops_per_sec +=
		here->ops_per_second;
	    workloads[here->work_obj_index].total_latency +=
		here->average_latency;
	    workloads[here->work_obj_index].total_count += 1.0;
	}
	if (here->modified_run)	/* set this if any client was modified */
	    modified_run = 1;
	count += 1.0;
    }
    count = count - skip;
    average_latency = total_latency / count;
    ops_per_second = total_ops_per_second;
    log_all (LOG_RESULTS,
	     "\t----------------------------------------------------------------\n");
    log_all (LOG_RESULTS, "\t%-39s %11.3f Milli-seconds\n",
	     "Overall average latency", average_latency);
    log_all (LOG_RESULTS, "\t%-7s %-31s %11.3f Ops/sec\n", "Overall",
	     short_testname, ops_per_second);
    log_all (LOG_RESULTS, "\t%-38s ~%11.3f Kbytes/sec\n",
	     "Overall Read_throughput", total_read_throughput / 1024);
    log_all (LOG_RESULTS, "\t%-38s ~%11.3f Kbytes/sec\n",
	     "Overall Write_throughput", total_write_throughput / 1024);
    log_all (LOG_RESULTS, "\t%-38s ~%11.3f Kbytes/sec\n",
	     "Overall throughput", total_throughput / 1024);
    log_all (LOG_RESULTS, "\t%-38s ~%011.1llu Mbytes\n",
	     "Total file space initialized", total_file_space);
    log_all (LOG_RESULTS,
	     "\t----------------------------------------------------------------\n");
    sum_val = check_sum (my_mix_table, lic_key);
    /* 
     * Licensed means that we users will obey the rules and not 
     * modify things.
     */
    if (modified_run)
	sum_val = 0;

    log_all (LOG_RESULTS, "\t%-30s  %u\n",
	     "Registered Finger Print", (unsigned int) sum_val);
    log_all (LOG_RESULTS,
	     "\t----------------------------------------------------------------\n");
    log_all (LOG_RESULTS, "\tLatency Bands:\n");

    if (cmdline.Op_lat_flag)
    {
	dump_hist ();
	log_all (LOG_RESULTS,
		 "\t----------------------------------------------------------------\n");
    }
    /* Scan... looking to see if there was more than one workload contributing */
    for (i = 0; i < num_work_obj; i++)
    {
	if (workloads[i].total_count != 0.0)
	    Multi_load_disp++;
    }
    if (Multi_load_disp > 1)
    {
	log_all (LOG_RESULTS, "\t%-30s\n\n", "Contributing workloads:");

	for (i = 0; i < num_work_obj; i++)
	{
	    if (workloads[i].total_count != 0.0)
	    {
		log_all (LOG_RESULTS,
			 "\t%-14s Procs %4.0f   Ops/sec %7.1f  Avg Latency %10.3f\n",
			 workloads[i].work_obj_name,
			 workloads[i].total_count,
			 workloads[i].total_ops_per_sec,
			 workloads[i].total_latency /
			 workloads[i].total_count);
	    }
	}
	log_all (LOG_RESULTS,
		 "\t----------------------------------------------------------------\n");
    }
}

/**
 * @brief Gather up all of the results associated with each 
 *  client proc and generate comma seperated CSV file.
*/
/*
 * __doc__
 * __doc__  Function : void print_results_in_csv(void)
 * __doc__  Arguments: void 
 * __doc__  Performs : Gather up all of the results associated with each 
 * __doc__             client proc and generate comma seperated CSV file.
 * __doc__
 */
void
print_results_in_csv (int which_results)
{
    struct result_object *here;
    int i;
    char ch_filename[400];
    FILE *ch_out;
    double timestamp = getepochtime ();
    char *file_string;

    if (which_results == ASK_INIT_RESULTS)
    {
	file_string = "Clients_init";
    }
    else
    {
	file_string = "Clients";
    }

#if defined(WIN32)
    snprintf (ch_filename, sizeof ch_filename,
	      "%s\\%s.csv", cmdline.prime_results_dir, file_string);
#else
    snprintf (ch_filename, sizeof ch_filename,
	      "%s/%s.csv", cmdline.prime_results_dir, file_string);
#endif

    ch_out = fopen (ch_filename, "a");
    if (ch_out < (FILE *) 0)
    {
	log_stdout (LOG_ERROR, "Error opening .csv file: %s\n",
		    netmist_strerror (netmist_get_error ()));
	exit (1);
    }

    fprintf (ch_out,
	     "Timestamp, Name,Id,workdir,Workload,Runtime,Ops/sec,Avg latency (ms),"
	     "Total file ops,Read throughput (KiBytes/s),Read KiBytes,"
	     "Write throughput (KiBytes/s),Write KiBytes,"
	     "Native_read throughput (KiBytes/s),Native read KiBytes,"
	     "Native_write throughput (KiBytes/s),Native_write KiBytes,"
	     "Meta_write throughput (KiBytes/s),Meta_write KiBytes,"
	     "Meta_read throughput (KiBytes/s),Meta_read KiBytes,"
	     "Initialized dirs,Initialized files,"
	     "Initialized files with space,Per proc file space,"
	     "Min Direct I/O size,"
	     "Write ops,Write latency (s),Write_min_latency, Write_max_latency,"
	     "Write_file ops,Write_file latency (s),Write_file_min_latency,Write_file_max_latency,"
	     "Mmap_write ops,Mmap_write latency (s),Mmap_write_min_latency,Mmap_write_max_latency,"
	     "Mmap_read ops,Mmap_read latency (s),Mmap_read_min_latency,Mmap_read_max_latency,"
	     "Read ops,Read latency (s),Read_min_latency,Read_max_latency,"
	     "Read_file ops,Read_file latency (s),Read_file_min_latency,Read_file_max_latency,"
	     "Mkdir ops,Mkdir latency (s),Mkdir_min_latency,Mkdir_max_latency,"
	     "Rmdir ops,Rmdir latency (s),Rmdir_min_latency,Rmdir_max_latency,"
	     "Unlink ops,Unlink latency (s),Unlink_min_latency,Unlink_max_latency,"
	     "Unlink2 ops,Unlink2 latency (s),Unlink2_min_latency,Unlink2_max_latency,"
	     "Create ops,Create latency (s),Create_min_latency,Create_max_latency,"
	     "Stat ops,Stat latency (s),Stat_min_latency,Stat_max_latency,"
	     "Neg_stat ops,Neg_stat latency (s),Neg_stat_min_latency,Neg_stat_max_latency,"
	     "Append ops,Append latency (s),Append_min_latency,Append_max_latency,"
	     "Lock ops,Lock latency (s),Lock_min_latency,Lock_max_latency,"
	     "Access ops,Access latency (s),Access_min_latency,Access_max_latency,"
	     "Chmod ops,Chmod latency (s),Chmod_min_latency,Chmod_max_latency,"
	     "Readdir ops,Readdir latency (s),Readdir_min_latency,Readdir_max_latency,"
	     "Random_write ops,Random_write latency (s),Random_write_min_latency,Random_write_max_latency,"
	     "Random_read ops,Random_read latency (s),Random_read_min_latency,Random_read_max_latency,"
	     "Read_modify_write ops,Read_modify_write latency (s),Read_modify_write_min_latency,read_modify_write_max_latency,"
	     "Open file ops,Open file latency (s),"
	     "Close file ops,Close file latency (s),"
	     "Copyfile ops,Copyfile latency (s),Copyfile_min_latency,Copyfile_max_latency,"
	     "Rename ops,Rename latency (s),Rename_min_latency,Rename_max_latency,"
	     "Statfs ops,Statfs latency (s),Statfs_min_latency,Statfs_max_latency,"
	     "Pathconf ops,Pathconf latency (s),Pathconf_min_latency,Pathconf_max_latency,"
	     "Truncate ops,Truncate latency (s),Truncate_min_latency,Truncate_max_latency,"
	     "Custom1 ops,Custom1 latency (s),Custom1_min_latency,Custom1_max_latency,"
	     "Custom2 ops,Custom2 latency (s),Custom2_min_latency,Custom2_max_latency,"
	     "Band1 20us,Band1 40us,Band1 60us,Band1 80us,Band1 100us,"
	     "Band2 200us,Band2 400us,Band2 600us,Band2 800us,Band2 1ms,"
	     "Band3 2ms,Band3 4ms,Band3 6ms,Band3 8ms,Band1 10ms,"
	     "Band4 12ms,Band4 14ms,Band4 16ms,Band4 18ms,Band4 20ms,"
	     "Band5 40ms,Band5 60ms,Band5 80ms,Band5 100ms,"
	     "Band6 200ms,Band6 400ms,Band6 600ms,Band6 800ms,"
	     "Band7 2s,Band7 4s,Band7 6s,Band7 8s,Band7 10s,"
	     "Band8 20s,Band8 40s,Band8 60s,Band8 80s,Band8 120s,"
	     "Band 9 120+s" "\n");

    for (i = 0; i < num_clients; i++)
    {
	here = &client_obj[i].results;

	fprintf (ch_out, "%.0lf,", timestamp);
	fprintf (ch_out, "%s,%d,", here->client_name, here->client_id);
	fprintf (ch_out, "%s,%s,", client_obj[i].client_workdir,
		 here->work_obj_name);
	fprintf (ch_out, "%ld,", here->run_time);
	fprintf (ch_out, "%.2f,", here->ops_per_second);
	fprintf (ch_out, "%.3f,", here->average_latency);

	fprintf (ch_out, "%f,", here->total_file_ops);
	fprintf (ch_out, "%.3f,", here->read_throughput / 1024);
	fprintf (ch_out, "%.3f,", here->read_kbytes / 1024);

	fprintf (ch_out, "%.3f,", here->write_throughput / 1024);
	fprintf (ch_out, "%.3f,", here->write_kbytes / 1024);

	fprintf (ch_out, "%.3f,", here->Nread_throughput / 1024);
	fprintf (ch_out, "%.3f,", here->Nread_kbytes / 1024);

	fprintf (ch_out, "%.3f,", here->Nwrite_throughput / 1024);
	fprintf (ch_out, " %.3f,", here->Nwrite_kbytes / 1024);

	fprintf (ch_out, "%.3f,",
		 (here->meta_w_kbytes / 1024) / here->run_time);
	fprintf (ch_out, "%.3f,", here->meta_w_kbytes / 1024);

	fprintf (ch_out, "%.3f,",
		 (here->meta_r_kbytes / 1024) / here->run_time);
	fprintf (ch_out, "%.3f,", here->meta_r_kbytes / 1024);

	fprintf (ch_out, "%d,", here->init_dirs);
	fprintf (ch_out, "%d,", here->init_files);

	fprintf (ch_out, "%d,", here->init_files_ws);
	fprintf (ch_out, " ~%.0f,", here->file_space_mb);

	fprintf (ch_out, "%ld,", here->min_direct_size);

	/*
	 * Here are the outputs for all of the standard workloads. 
	 */

	/* Write */
	fprintf (ch_out, "%ld,", here->write_count);
	if (cmdline.Op_lat_flag && (here->write_count != 0)
	    && (here->write_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->write_time / (double) here->write_count);
	    fprintf (ch_out, "%.6f, %.6f,", here->min_write_latency,
		     here->max_write_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Write file */
	fprintf (ch_out, "%ld,", here->write_file_count);
	if (cmdline.Op_lat_flag && (here->write_file_count != 0)
	    && (here->write_file_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->write_file_time / (double) here->write_file_count);
	    fprintf (ch_out, "%.6f, %.6f,", here->min_write_file_latency,
		     here->max_write_file_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* mmap write */
	fprintf (ch_out, "%ld,", here->mmap_write_count);
	if (cmdline.Op_lat_flag && (here->mmap_write_count != 0)
	    && (here->mmap_write_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->mmap_write_time / (double) here->mmap_write_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_mmap_write_latency,
		     here->max_mmap_write_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* mmap read */
	fprintf (ch_out, "%ld,", here->mmap_read_count);
	if (cmdline.Op_lat_flag && (here->mmap_read_count != 0)
	    && (here->mmap_read_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->mmap_read_time / (double) here->mmap_read_count);
	    fprintf (ch_out, "%.6f, %.6f,", here->min_mmap_read_latency,
		     here->max_mmap_read_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* read */
	fprintf (ch_out, "%ld,", here->read_count);
	if (cmdline.Op_lat_flag && (here->read_count != 0)
	    && (here->read_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->read_time / (double) here->read_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_read_latency,
		     here->max_read_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* read file */
	fprintf (ch_out, "%ld,", here->read_file_count);
	if (cmdline.Op_lat_flag && (here->read_file_count != 0)
	    && (here->read_file_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->read_file_time / (double) here->read_file_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_read_file_latency,
		     here->max_read_file_latency);
	}
	else
	{
	    fprintf (ch_out, " ,");
	}

	/* mkdir */
	fprintf (ch_out, "%ld,", here->mkdir_count);
	if (cmdline.Op_lat_flag && (here->mkdir_count != 0)
	    && (here->mkdir_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->mkdir_time / (double) here->mkdir_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_mkdir_latency,
		     here->max_mkdir_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* rmdir */
	fprintf (ch_out, "%ld,", here->rmdir_count);
	if (cmdline.Op_lat_flag && (here->rmdir_count != 0)
	    && (here->rmdir_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->rmdir_time / (double) here->rmdir_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_rmdir_latency,
		     here->max_rmdir_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* unlink */
	fprintf (ch_out, "%ld,", here->unlink_count);
	if (cmdline.Op_lat_flag && (here->unlink_count != 0)
	    && (here->unlink_time != 0.0))
	{
	    fprintf (ch_out, "%6f,",
		     here->unlink_time / (double) here->unlink_count);
	    fprintf (ch_out, "%6f,%.6f,", here->min_unlink_latency,
		     here->max_unlink_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* unlink2 */
	fprintf (ch_out, "%ld,", here->unlink2_count);
	if (cmdline.Op_lat_flag && (here->unlink2_count != 0)
	    && (here->unlink2_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->unlink2_time / (double) here->unlink2_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_unlink2_latency,
		     here->max_unlink2_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* create */
	fprintf (ch_out, "%ld,", here->create_count);
	if (cmdline.Op_lat_flag && (here->create_count != 0)
	    && (here->create_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->create_time / (double) here->create_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_create_latency,
		     here->max_create_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* stat */
	fprintf (ch_out, "%ld,", here->stat_count);
	if (cmdline.Op_lat_flag && (here->stat_count != 0)
	    && (here->stat_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->stat_time / (double) here->stat_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_stat_latency,
		     here->max_stat_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* neg_stat */
	fprintf (ch_out, "%ld,", here->neg_stat_count);
	if (cmdline.Op_lat_flag && (here->neg_stat_count != 0)
	    && (here->neg_stat_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->neg_stat_time / (double) here->neg_stat_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_neg_stat_latency,
		     here->max_neg_stat_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* append */
	fprintf (ch_out, "%ld,", here->append_count);
	if (cmdline.Op_lat_flag && (here->append_count != 0)
	    && (here->append_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->append_time / (double) here->append_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_append_latency,
		     here->max_append_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Lock */
	fprintf (ch_out, "%ld,", here->lock_count);
	if (cmdline.Op_lat_flag && (here->lock_count != 0)
	    && (here->lock_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->lock_time / (double) here->lock_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_lock_latency,
		     here->max_lock_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* append */
	fprintf (ch_out, "%ld,", here->access_count);
	if (cmdline.Op_lat_flag && (here->access_count != 0)
	    && (here->access_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->access_time / (double) here->access_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_access_latency,
		     here->max_access_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* chmod */
	fprintf (ch_out, "%ld,", here->chmod_count);
	if (cmdline.Op_lat_flag && (here->chmod_count != 0)
	    && (here->chmod_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->chmod_time / (double) here->chmod_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_chmod_latency,
		     here->max_chmod_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* readdir */
	fprintf (ch_out, "%ld,", here->readdir_count);
	if (cmdline.Op_lat_flag && (here->readdir_count != 0)
	    && (here->readdir_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->readdir_time / (double) here->readdir_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_readdir_latency,
		     here->max_readdir_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* write rand */
	fprintf (ch_out, "%ld,", here->write_rand_count);
	if (cmdline.Op_lat_flag && (here->write_rand_count != 0)
	    && (here->write_rand_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->write_rand_time / (double) here->write_rand_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_write_rand_latency,
		     here->max_write_rand_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Read rand */
	fprintf (ch_out, "%ld,", here->read_rand_count);
	if (cmdline.Op_lat_flag && (here->read_rand_count != 0)
	    && (here->read_rand_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->read_rand_time / (double) here->read_rand_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_read_rand_latency,
		     here->max_read_rand_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* rmw */
	fprintf (ch_out, "%ld,", here->rmw_count);
	if (cmdline.Op_lat_flag && (here->rmw_count != 0)
	    && (here->rmw_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->rmw_time / (double) here->rmw_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_rmw_latency,
		     here->max_rmw_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	fprintf (ch_out, "%ld,", here->open_count);
	fprintf (ch_out, " ,");

	fprintf (ch_out, "%ld,", here->close_count);
	fprintf (ch_out, " ,");

	/* Copyfile */
	fprintf (ch_out, "%ld,", here->copyfile_count);
	if (cmdline.Op_lat_flag && (here->copyfile_count != 0)
	    && (here->copyfile_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->copyfile_time / (double) here->copyfile_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_copyfile_latency,
		     here->max_copyfile_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Rename */
	fprintf (ch_out, "%ld,", here->rename_count);
	if (cmdline.Op_lat_flag && (here->rename_count != 0)
	    && (here->rename_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->rename_time / (double) here->rename_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_rename_latency,
		     here->max_rename_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Statfs */
	fprintf (ch_out, "%ld,", here->statfs_count);
	if (cmdline.Op_lat_flag && (here->statfs_count != 0)
	    && (here->statfs_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->statfs_time / (double) here->statfs_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_statfs_latency,
		     here->max_statfs_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* Pathconf */
	fprintf (ch_out, "%ld,", here->pathconf_count);
	if (cmdline.Op_lat_flag && (here->pathconf_count != 0)
	    && (here->pathconf_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->pathconf_time / (double) here->pathconf_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_pathconf_latency,
		     here->max_pathconf_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* trunc */
	fprintf (ch_out, "%ld,", here->trunc_count);
	if (cmdline.Op_lat_flag && (here->trunc_count != 0)
	    && (here->trunc_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->trunc_time / (double) here->trunc_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_trunc_latency,
		     here->max_trunc_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* custom1 */
	fprintf (ch_out, "%ld,", here->custom1_count);
	if (cmdline.Op_lat_flag && (here->custom1_count != 0)
	    && (here->custom1_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->custom1_time / (double) here->custom1_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_custom1_latency,
		     here->max_custom1_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	/* custom2 */
	fprintf (ch_out, "%ld,", here->custom2_count);
	if (cmdline.Op_lat_flag && (here->custom2_count != 0)
	    && (here->custom2_time != 0.0))
	{
	    fprintf (ch_out, "%.6f,",
		     here->custom2_time / (double) here->custom2_count);
	    fprintf (ch_out, "%.6f,%.6f,", here->min_custom2_latency,
		     here->max_custom2_latency);
	}
	else
	{
	    fprintf (ch_out, " , , ,");
	}

	if (cmdline.Op_lat_flag)
	{
	    prime_dump_hist_in_csv (here, ch_out);
	}

	fprintf (ch_out, "\n");
	fflush (ch_out);
    }

    fprintf (ch_out, "\n\n\n\n\n");
    fclose (ch_out);
}

/**
 * @brief This dumps the internal workloads to an ascii file, 
 *        for editing and later import.
*/
/* 
 * __doc__
 * __doc__  Function : void dump_table(void)
 * __doc__  Arguments: void 
 * __doc__  Performs : This dumps the internal workloads to an ascii file, 
 * __doc__             for editing and later import.
 * __doc__
 */
void
dump_table (void)
{
    /* Switched to YAML configurattion in SPECstorage2020, making this un-needed */
#if defined(BUILT_IN_WORKLOADS)
    int i, j;
    for (j = 0; j < MAX_WORK_OBJ; j++)
    {
	if (strlen (my_mix_table[j].workload_name) == 0)
	    continue;
	printf ("Workload name %14s\n", my_mix_table[j].workload_name);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].read_string, my_mix_table[j].percent_read);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].read_file_string,
		my_mix_table[j].percent_read_file);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].mmap_read_string,
		my_mix_table[j].percent_mmap_read);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].read_rand_string,
		my_mix_table[j].percent_read_rand);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].write_string, my_mix_table[j].percent_write);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].write_file_string,
		my_mix_table[j].percent_write_file);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].mmap_write_string,
		my_mix_table[j].percent_mmap_write);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].write_rand_string,
		my_mix_table[j].percent_write_rand);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].rmw_string, my_mix_table[j].percent_rmw);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].mkdir_string, my_mix_table[j].percent_mkdir);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].rmdir_string, my_mix_table[j].percent_rmdir);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].unlink_string,
		my_mix_table[j].percent_unlink);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].unlink2_string,
		my_mix_table[j].percent_unlink2);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].create_string,
		my_mix_table[j].percent_create);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].append_string,
		my_mix_table[j].percent_append);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].locking_string,
		my_mix_table[j].percent_locking);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].access_string,
		my_mix_table[j].percent_access);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].stat_string, my_mix_table[j].percent_stat);
	printf ("Percent %-35s %d\n",
		my_mix_table[j].neg_stat_string,
		my_mix_table[j].percent_neg_stat);
	printf ("Percent %-35s %d\n", my_mix_table[j].chmod_string,
		my_mix_table[j].percent_chmod);
	printf ("Percent %-35s %d\n", my_mix_table[j].readdir_string,
		my_mix_table[j].percent_readdir);
	printf ("Percent %-35s %d\n", my_mix_table[j].copyfile_string,
		my_mix_table[j].percent_copyfile);
	printf ("Percent %-35s %d\n", my_mix_table[j].rename_string,
		my_mix_table[j].percent_rename);
	printf ("Percent %-35s %d\n", my_mix_table[j].statfs_string,
		my_mix_table[j].percent_statfs);
	printf ("Percent %-35s %d\n", my_mix_table[j].pathconf_string,
		my_mix_table[j].percent_pathconf);
	printf ("Percent %-35s %d\n", my_mix_table[j].trunc_string,
		my_mix_table[j].percent_trunc);
	printf ("Percent %-35s %d\n", my_mix_table[j].custom1_string,
		my_mix_table[j].percent_custom1);
	printf ("Percent %-35s %d\n", my_mix_table[j].custom2_string,
		my_mix_table[j].percent_custom2);
	;
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    printf ("Read elem %d xfer min size %d\n", i,
		    my_mix_table[j].read_dist[i].size_min);
	    printf ("Read elem %d xfer max size %d\n", i,
		    my_mix_table[j].read_dist[i].size_max);
	    printf ("Read elem %d xfer percent %d\n", i,
		    my_mix_table[j].read_dist[i].percent);
	}
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    printf ("Write elem %d xfer min size %d\n", i,
		    my_mix_table[j].write_dist[i].size_min);
	    printf ("Write elem %d xfer max size %d\n", i,
		    my_mix_table[j].write_dist[i].size_max);
	    printf ("Write elem %d xfer percent %d\n", i,
		    my_mix_table[j].write_dist[i].percent);
	}
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    printf ("File size elem %d min size %lld\n", i,
		    my_mix_table[j].file_size_dist[i].size_min);
	    printf ("File size elem %d max size %lld\n", i,
		    my_mix_table[j].file_size_dist[i].size_max);
	    printf ("File size elem %d percent %d\n", i,
		    my_mix_table[j].file_size_dist[i].percent);
	}
	printf ("Min_pre_name_length %d\n",
		my_mix_table[j].min_pre_name_length);
	printf ("Max_pre_name_length %d\n",
		my_mix_table[j].max_pre_name_length);
	printf ("Min_post_name_length %d\n",
		my_mix_table[j].min_post_name_length);
	printf ("Max_post_name_length %d\n",
		my_mix_table[j].max_post_name_length);
	printf ("Percent write commit %d\n", my_mix_table[j].percent_commit);
	printf ("Percent direct %d\n", my_mix_table[j].percent_direct);
	printf ("Percent fadvise seq %d\n",
		my_mix_table[j].percent_fadvise_seq);
	printf ("Percent fadvise rand %d\n",
		my_mix_table[j].percent_fadvise_rand);
	printf ("Percent fadvise dont_need %d\n",
		my_mix_table[j].percent_fadvise_dont_need);
	printf ("Align          %d\n", my_mix_table[j].align);
	printf ("Percent osync %d\n", my_mix_table[j].percent_osync);
	printf ("Percent geometric %d\n", my_mix_table[j].percent_geometric);
	printf ("Percent compress %d\n", my_mix_table[j].percent_compress);
	printf ("Percent dedup %d\n", my_mix_table[j].percent_dedup);
	printf ("Percent dedup_within %d\n",
		my_mix_table[j].percent_dedup_within);
	printf ("Percent dedup_across %d\n",
		my_mix_table[j].percent_dedup_across);
	printf ("Dedupe group count %d\n", my_mix_table[j].dedup_group_count);
	printf ("Percent per_spot %d\n", my_mix_table[j].percent_per_spot);
	printf ("Min acc_per_spot %d\n", my_mix_table[j].min_acc_per_spot);
	printf ("Acc mult_spot %d\n", my_mix_table[j].acc_mult_spot);
	printf ("Percent affinity %d\n", my_mix_table[j].percent_affinity);
	printf ("Spot shape %d\n", my_mix_table[j].spot_shape);
	printf ("Dedup Granule size %d\n",
		my_mix_table[j].dedup_granule_size);
	printf ("Dedup gran rep limit %d\n",
		my_mix_table[j].dedup_gran_rep_limit);
	printf ("Use file size dist %d\n",
		my_mix_table[j].use_file_size_dist);
	printf ("Comp Granule size %d\n", my_mix_table[j].comp_granule_size);
	printf ("Background %d\n", my_mix_table[j].background);
	printf ("Sharemode %d\n", my_mix_table[j].sharemode);
	printf ("Uniform size dist %d\n",
		my_mix_table[j].uniform_file_size_dist);
	printf ("Rand dist behavior %d\n",
		my_mix_table[j].rand_dist_behavior);
	printf ("Cipher behavior %d\n", my_mix_table[j].cipher);
	printf ("Notification percent %d\n", my_mix_table[j].notify);
	printf ("LRU %d\n", my_mix_table[j].lru_on);
	printf ("Pattern version %d\n", my_mix_table[j].patt_vers);
	printf ("Init rate enable %d\n", my_mix_table[j].init_rate_enable);
	printf ("Init read flag %d\n", my_mix_table[j].init_read);
	printf ("Extra dir levels %d\n", my_mix_table[j].extra_dir_levels);
	printf ("Chaff count %d\n", my_mix_table[j].chaff_count);
	printf ("Shared buckets %d\n", my_mix_table[j].shared_buckets);
	printf ("Unlink2_no_recreate %d\n",
		my_mix_table[j].unlink2_no_recreate);
	printf ("File_size %d\n", my_mix_table[j].file_size);
	printf ("Dir_count %d\n", my_mix_table[j].dir_count);
	printf ("Warm_time %d\n", my_mix_table[j].warm_time);
	printf ("Files_per_dir %d\n", my_mix_table[j].files_per_dir);
	printf ("Release version %d\n", my_mix_table[j].rel_version);
	printf ("Platform type %s\n", my_mix_table[j].platform_type);
	printf ("FS type       %s\n", my_mix_table[j].fs_type);
    }
#endif
}

/**
 * @brief Reverse the bits in a data type where the number of 
 *        bits to reverse.
 *
 * @param num : Number of bits to reverse
 */
/*
 * __doc__
 * __doc__  Function : int reverse_bits(int number)
 * __doc__  Arguments: int number. Number of bits to reverse
 * __doc__  Performs : Reverse the bits in a data type where the number of 
 * __doc__             bits to reverse.
 * __doc__
 */
static int
reverse_bits (int num)
{
    unsigned int size = sizeof (num) * 8, i = 0;
    int result = 0;
    for (i = 0; i < size; i++)
    {
	if (num & (1 << i))
	    result |= (1 << (size - i - 1));
    }

    log_file (LOG_DEBUG, "Number %x is reversed to %x\n", num, result);
    return result;
}

/**
 * @brief Performs the Warmup and Run phases.
 */
void
complete_client_run_work (void)
{
    double my_last_tick, elap_tick, time_per_tick;
    int prev_pct_complete;
    int i_scale = ISCALE;	/* auto scaling for sleep injection */
    int loops, save_loops = 0, pct_complete;
    int xxx = 0;
#if defined(WIN32)
    DWORD ret = 0;
#else
    int ret;
#endif
#if defined(PDSM_RO_ACTIVE)
    int my_pdsm_time;
#endif

    double time_of_last_tick = 0.0;
    double ach_ops_per_second = 0.0;
    double tmp_time = 0;
    double start_epoch, time_skew, curr_epoch;

    int inject = 0;		/* loop count for how many possible injections */

    start_epoch = internal_cfg.u.client_cfg.start_epoch;
    time_skew = internal_cfg.u.client_cfg.time_skew;
    curr_epoch = internal_cfg.u.client_cfg.curr_epoch;
    log_file (LOG_FSM_VERBOSE,
	      "time_skew %lf start_epoch %lf curr_epoch %lf\n", time_skew,
	      start_epoch, curr_epoch);

#if defined(PDSM_RO_ACTIVE)
    double time_used;
#endif

    if ((curr_epoch + time_skew) >= start_epoch)
    {
	log_file (LOG_FSM_VERBOSE,
		  "Child late start: My time %lf, Start time %lf. Diff %lf\n",
		  (curr_epoch + time_skew), start_epoch,
		  (curr_epoch + time_skew) - start_epoch);

    }
    else
    {
	log_file (LOG_FSM_VERBOSE,
		  "Child waiting for Epoch start: Child time %lf, "
		  "Start time %lf. Diff %lf\n",
		  (curr_epoch + time_skew), start_epoch,
		  start_epoch - (curr_epoch + time_skew));

    }

    if (cmdline.sharing_flag)
	netmist_srand ((cmdline.client_id + 1) * 3);	/* 
							 * Now re-seed, for the 
							 * shared file case 
							 */
    /*
     * Now everyone starts at the same time.
     */
    tick = 0;
    my_last_tick = 0.0;
    prev_pct_complete = 0;
    if (cmdline.op_rate > 0.0)
    {
	i_scale = cmdline.op_rate * 0.05;	/* ops in burst = rate * time -- sets basic 
						 * interval to 0.05 seconds 
						 */
	if (i_scale < 4)
	    i_scale = 4;	/* Can't let it be 0, and there is no point 
				 * in hitting every operation at low rates 
				 */
    }

    naps_taken = 0;
    total_seconds_napped = 0.0;
    total_file_ops = 0.0;
    prev_file_ops = 0.0;
    loops = 1;

    /*
     * Now wait for the our Epoch time() to match the desired Epoch time() 
     * from the Prime. Then start the Warmup phase.
     */
    while ((curr_epoch + time_skew) < start_epoch)
    {
	curr_epoch = getepochtime ();

	log_file (LOG_FSM_VERBOSE,
		  "Waiting for our current time to match start Epoch. "
		  "My time %lf, Start time %lf\n",
		  (curr_epoch + time_skew), start_epoch);


	nap (10);
    }

    /* Cleanup dir_lock files after barrier, so that initialization 
     * only happens once, in the case where cmdline.sharing_flag is on.
     * In this particular case, there are parallel procs doing init
     * and using dir_lock (file creation) as the semaphore. But, if 
     * they drain their list of dir_lock files too soon, then another
     * proc could think that it needed to initialized the same files.
     */
    if (cmdline.sharing_flag)
	drain_list ();
    log_file (LOG_EXEC, "Starting WARMUP phase.\n");

    phase = WARM_PHASE;

    clear_stats ();

    my_start_time = gettime ();
    time_of_last_tick = my_start_time;

    sanity_time = gettime ();
    while (1)
    {
	my_gen_op_time = gettime ();

	/* While this may seem nuts, someone could have ntp running, or 
	 * manually set the clock back in time.  By doing do, the latency 
	 * calculations will be wrong.  Some negative times, mixed with 
	 * some positive times to skew the results to whatever someone 
	 * might want.  Thus, this code prevents tampering with the 
	 * passage of time.   Dr. Who would be proud !
	 * Note: I personally debugged a case where time moved backwards.
	 */
	if ((my_gen_op_time + (double) 1.0) < sanity_time)
	{
	    log_file (LOG_ERROR, "Time is moving backwards !!! \n");
	    sanity_time = my_gen_op_time;
	    R_exit (BAD_CLOCK, 0);
	}
	gen_op (test_ops, impersonate);

	if ((xxx % 100 == 0))
	{
	    log_file (LOG_DEBUG, "FD list length %d\n", fd_list_length ());

	}
	xxx++;

	/*-----------------*/
	/* heartbeat       */
	/*-----------------*/
	if (cmdline.heartbeat_flag &&
	    ((int) (my_gen_op_time - last_heart_beat) >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.client_id, WARM_BEAT,
				  ach_ops_per_second);
	    last_heart_beat = my_gen_op_time;
	}
	/*------------------------------------------*/
	/* Now check percent of warm time permitted */
	/*------------------------------------------*/
	cur_time = gettime ();
	my_run_time = cur_time - my_start_time;
	pct_complete =
	    (int) (((my_run_time +
		     ELAP_T) * 100.0) / my_mix_table[my_workload].warm_time);
	if (pct_complete > 100)
	    pct_complete = 100;

	if (pct_complete && (pct_complete != prev_pct_complete)
	    && ((pct_complete % 10) == 0) && cmdline.heartbeat_flag)
	{
	    tell_prime_pct_complete (pct_complete, cmdline.client_id,
				     R_PERCENT_W_COMPLETE);
	    prev_pct_complete = pct_complete;
	}
	/*------------------------------------------*/
	if (my_last_tick == 0.0)
	    my_last_tick = gettime ();
	elap_tick = my_gen_op_time - my_last_tick;
	if (elap_tick > (double) ELAP_T)
	{
	    my_last_tick = my_gen_op_time;
	    tick = 0;
	    save_loops = loops;
	    loops = 1;
	}
	else
	{
	    tick = 1;
	}
	if (save_loops == 0)
	    save_loops = 1;
	my_warmup_ops++;
	if (tick == 0)
	{
	    ops_this_tick = total_file_ops - prev_file_ops;
	    prev_file_ops = total_file_ops;
	    post_time = gettime ();


	    /* Stop when we have reached our defined warmup time */
	    if (post_time - my_start_time >=
		(double) my_mix_table[my_workload].warm_time)
	    {
		log_file (LOG_EXEC_VERBOSE,
			  "Stopping warmup by reaching warm time duration: %lld\n",
			  (long long) my_mix_table[my_workload].warm_time);
		break;
	    }

	    /* 
	     * Stop when we have reached the absolute time of Epoch + warmup 
	     * time 
	     */
	    if (((getepochtime () + time_skew) - start_epoch) >
		(double) my_mix_table[my_workload].warm_time)
	    {
		log_file (LOG_EXEC_VERBOSE,
			  "Stopping warmup by Epoch warm time at %lf Runtime = %lf\n",
			  (start_epoch + my_mix_table[my_workload].warm_time),
			  (cur_time - my_start_time));
		break;
	    }

	    time_per_tick = post_time - time_of_last_tick;
	    time_of_last_tick = post_time;
	    if (time_per_tick == (double) 0.0)
		time_per_tick = (double) 0.001;

	    ach_ops_per_second = (double) ops_this_tick / time_per_tick;
	    if (ach_ops_per_second == (double) 0.0)
		ach_ops_per_second = (double) 1.000;
#if defined(PDSM_RO_ACTIVE)
	    if (pdsm_stats)
	    {
		time_used = (cur_time - my_start_time);
		if (time_used != 0)
		{
		    pdsm_stats->read_throughput =
			(total_read_bytes / 1024.0) / (float) (cur_time -
							       my_start_time);
		    pdsm_stats->write_throughput =
			(total_write_bytes / 1024.0) / (float) (cur_time -
								my_start_time);
		    pdsm_stats->meta_throughput =
			(((total_meta_r_ops +
			   total_meta_w_ops) * META_B) / 1024.0) /
			(float) (cur_time - my_start_time);

		    pdsm_stats->run_time = post_time - my_start_time;
		}
		else
		{
		    pdsm_stats->read_throughput = 0;
		    pdsm_stats->write_throughput = 0;
		    pdsm_stats->meta_throughput = 0;
		    pdsm_stats->run_time = 0;
		}
		pdsm_stats->epoch_time = time (NULL);
		pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
		pdsm_stats->achieved_op_rate = ach_ops_per_second;
		pdsm_stats->cur_state = WARM_BEAT;
		pdsm_stats->client_id = cmdline.client_id;
		pdsm_stats->mode = pdsm_mode;
		my_strncpy (pdsm_stats->client_name, client_localname,
			    MAXHNAME);

		pdsm_stats->client_id = cmdline.client_id;
		pdsm_stats->client_name[strlen (client_localname)] = 0;
		my_strncpy (pdsm_stats->workload_name,
			    work_obj[my_workload].work_obj_name, MAXWLNAME);
		pdsm_stats->workload_name[strlen
					  (work_obj
					   [my_workload].work_obj_name)] = 0;

		pdsm_stats->read_op_count =
		    work_obj[my_workload].read_op_count;
		pdsm_stats->read_op_time = work_obj[my_workload].read_op_time;

		my_mix_table[my_workload].read_string[strlen
						      (my_mix_table
						       [my_workload].read_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_string,
			    my_mix_table[my_workload].read_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_string[strlen
				(my_mix_table[my_workload].read_string)] = 0;

		pdsm_stats->read_file_op_count =
		    work_obj[my_workload].read_file_op_count;
		pdsm_stats->read_file_op_time =
		    work_obj[my_workload].read_file_op_time;

		my_mix_table[my_workload].read_file_string[strlen
							   (my_mix_table
							    [my_workload].
							    read_file_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_file_string,
			    my_mix_table[my_workload].read_file_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_file_string[strlen
				     (my_mix_table[my_workload].
				      read_file_string)] = 0;

		pdsm_stats->read_rand_op_count =
		    work_obj[my_workload].read_rand_op_count;
		pdsm_stats->read_rand_op_time =
		    work_obj[my_workload].read_rand_op_time;

		my_mix_table[my_workload].read_rand_string[strlen
							   (my_mix_table
							    [my_workload].
							    read_rand_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_rand_string,
			    my_mix_table[my_workload].read_rand_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_rand_string[strlen
				     (my_mix_table[my_workload].
				      read_rand_string)] = 0;

		pdsm_stats->write_op_count =
		    work_obj[my_workload].write_op_count;
		pdsm_stats->write_op_time =
		    work_obj[my_workload].write_op_time;

		my_mix_table[my_workload].write_string[strlen
						       (my_mix_table
							[my_workload].write_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_string,
			    my_mix_table[my_workload].write_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_string[strlen
				 (my_mix_table[my_workload].write_string)] =
		    0;

		pdsm_stats->write_file_op_count =
		    work_obj[my_workload].write_file_op_count;
		pdsm_stats->write_file_op_time =
		    work_obj[my_workload].write_file_op_time;

		my_mix_table[my_workload].write_file_string[strlen
							    (my_mix_table
							     [my_workload].
							     write_file_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_file_string,
			    my_mix_table[my_workload].write_file_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_file_string[strlen
				      (my_mix_table[my_workload].
				       write_file_string)] = 0;

		pdsm_stats->open_op_count =
		    work_obj[my_workload].open_op_count;
		pdsm_stats->close_op_count =
		    work_obj[my_workload].close_op_count;
		pdsm_stats->mmap_write_op_count =
		    work_obj[my_workload].mmap_write_op_count;
		pdsm_stats->mmap_write_op_time =
		    work_obj[my_workload].mmap_write_op_time;

		my_mix_table[my_workload].mmap_write_string[strlen
							    (my_mix_table
							     [my_workload].
							     mmap_write_string)]
		    = 0;
		my_strncpy (pdsm_stats->mmap_write_string,
			    my_mix_table[my_workload].mmap_write_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mmap_write_string[strlen
				      (my_mix_table[my_workload].
				       mmap_write_string)] = 0;

		pdsm_stats->mmap_read_op_count =
		    work_obj[my_workload].mmap_read_op_count;
		pdsm_stats->mmap_read_op_time =
		    work_obj[my_workload].mmap_read_op_time;

		my_mix_table[my_workload].mmap_read_string[strlen
							   (my_mix_table
							    [my_workload].
							    mmap_read_string)]
		    = 0;
		my_strncpy (pdsm_stats->mmap_read_string,
			    my_mix_table[my_workload].mmap_read_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mmap_read_string[strlen
				     (my_mix_table[my_workload].
				      mmap_read_string)] = 0;

		pdsm_stats->write_rand_op_count =
		    work_obj[my_workload].write_rand_op_count;
		pdsm_stats->write_rand_op_time =
		    work_obj[my_workload].write_rand_op_time;

		my_mix_table[my_workload].write_rand_string[strlen
							    (my_mix_table
							     [my_workload].
							     write_rand_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_rand_string,
			    my_mix_table[my_workload].write_rand_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_rand_string[strlen
				      (my_mix_table[my_workload].
				       write_rand_string)] = 0;

		pdsm_stats->rmw_op_count = work_obj[my_workload].rmw_op_count;
		pdsm_stats->rmw_op_time = work_obj[my_workload].rmw_op_time;

		my_mix_table[my_workload].rmw_string[strlen
						     (my_mix_table
						      [my_workload].rmw_string)]
		    = 0;
		my_strncpy (pdsm_stats->rmw_string,
			    my_mix_table[my_workload].rmw_string, OP_STR_LEN);
		pdsm_stats->
		    rmw_string[strlen (my_mix_table[my_workload].rmw_string)]
		    = 0;

		pdsm_stats->mkdir_op_count =
		    work_obj[my_workload].mkdir_op_count;
		pdsm_stats->mkdir_op_time =
		    work_obj[my_workload].mkdir_op_time;

		my_mix_table[my_workload].mkdir_string[strlen
						       (my_mix_table
							[my_workload].mkdir_string)]
		    = 0;
		my_strncpy (pdsm_stats->mkdir_string,
			    my_mix_table[my_workload].mkdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mkdir_string[strlen
				 (my_mix_table[my_workload].mkdir_string)] =
		    0;

		pdsm_stats->rmdir_op_count =
		    work_obj[my_workload].rmdir_op_count;
		pdsm_stats->rmdir_op_time =
		    work_obj[my_workload].rmdir_op_time;

		my_mix_table[my_workload].rmdir_string[strlen
						       (my_mix_table
							[my_workload].rmdir_string)]
		    = 0;
		my_strncpy (pdsm_stats->rmdir_string,
			    my_mix_table[my_workload].rmdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    rmdir_string[strlen
				 (my_mix_table[my_workload].rmdir_string)] =
		    0;

		pdsm_stats->unlink_op_count =
		    work_obj[my_workload].unlink_op_count;
		pdsm_stats->unlink_op_time =
		    work_obj[my_workload].unlink_op_time;

		my_mix_table[my_workload].unlink_string[strlen
							(my_mix_table
							 [my_workload].unlink_string)]
		    = 0;
		my_strncpy (pdsm_stats->unlink_string,
			    my_mix_table[my_workload].unlink_string,
			    OP_STR_LEN);
		pdsm_stats->
		    unlink_string[strlen
				  (my_mix_table[my_workload].unlink_string)] =
		    0;
		pdsm_stats->unlink2_op_count =
		    work_obj[my_workload].unlink2_op_count;
		pdsm_stats->unlink2_op_time =
		    work_obj[my_workload].unlink2_op_time;

		my_mix_table[my_workload].unlink2_string[strlen
							 (my_mix_table
							  [my_workload].
							  unlink2_string)] =
		    0;
		my_strncpy (pdsm_stats->unlink2_string,
			    my_mix_table[my_workload].unlink2_string,
			    OP_STR_LEN);
		pdsm_stats->
		    unlink2_string[strlen
				   (my_mix_table[my_workload].
				    unlink2_string)] = 0;
		pdsm_stats->create_op_count =
		    work_obj[my_workload].create_op_count;
		pdsm_stats->create_op_time =
		    work_obj[my_workload].create_op_time;

		my_mix_table[my_workload].create_string[strlen
							(my_mix_table
							 [my_workload].create_string)]
		    = 0;
		my_strncpy (pdsm_stats->create_string,
			    my_mix_table[my_workload].create_string,
			    OP_STR_LEN);
		pdsm_stats->
		    create_string[strlen
				  (my_mix_table[my_workload].create_string)] =
		    0;
		pdsm_stats->append_op_count =
		    work_obj[my_workload].append_op_count;
		pdsm_stats->append_op_time =
		    work_obj[my_workload].append_op_time;
		my_mix_table[my_workload].
		    append_string[strlen
				  (my_mix_table[my_workload].append_string)] =
		    0;
		my_strncpy (pdsm_stats->append_string,
			    my_mix_table[my_workload].append_string,
			    OP_STR_LEN);
		pdsm_stats->
		    append_string[strlen
				  (my_mix_table[my_workload].append_string)] =
		    0;
		pdsm_stats->lock_op_count =
		    work_obj[my_workload].lock_op_count;
		pdsm_stats->lock_op_time = work_obj[my_workload].lock_op_time;
		my_mix_table[my_workload].locking_string[strlen
							 (my_mix_table
							  [my_workload].
							  locking_string)] =
		    0;
		my_strncpy (pdsm_stats->locking_string,
			    my_mix_table[my_workload].locking_string,
			    OP_STR_LEN);
		pdsm_stats->
		    locking_string[strlen
				   (my_mix_table[my_workload].
				    locking_string)] = 0;
		pdsm_stats->access_op_count =
		    work_obj[my_workload].access_op_count;
		pdsm_stats->access_op_time =
		    work_obj[my_workload].access_op_time;
		my_mix_table[my_workload].
		    access_string[strlen
				  (my_mix_table[my_workload].access_string)] =
		    0;
		my_strncpy (pdsm_stats->access_string,
			    my_mix_table[my_workload].access_string,
			    OP_STR_LEN);
		pdsm_stats->
		    access_string[strlen
				  (my_mix_table[my_workload].access_string)] =
		    0;
		pdsm_stats->chmod_op_count =
		    work_obj[my_workload].chmod_op_count;
		pdsm_stats->chmod_op_time =
		    work_obj[my_workload].chmod_op_time;
		my_mix_table[my_workload].
		    chmod_string[strlen
				 (my_mix_table[my_workload].chmod_string)] =
		    0;
		my_strncpy (pdsm_stats->chmod_string,
			    my_mix_table[my_workload].chmod_string,
			    OP_STR_LEN);
		pdsm_stats->
		    chmod_string[strlen
				 (my_mix_table[my_workload].chmod_string)] =
		    0;
		pdsm_stats->readdir_op_count =
		    work_obj[my_workload].readdir_op_count;
		pdsm_stats->readdir_op_time =
		    work_obj[my_workload].readdir_op_time;
		my_mix_table[my_workload].
		    readdir_string[strlen
				   (my_mix_table[my_workload].
				    readdir_string)] = 0;
		my_strncpy (pdsm_stats->readdir_string,
			    my_mix_table[my_workload].readdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    readdir_string[strlen
				   (my_mix_table[my_workload].
				    readdir_string)] = 0;
		pdsm_stats->stat_op_count =
		    work_obj[my_workload].stat_op_count;
		pdsm_stats->stat_op_time = work_obj[my_workload].stat_op_time;
		my_mix_table[my_workload].stat_string[strlen
						      (my_mix_table
						       [my_workload].stat_string)]
		    = 0;
		my_strncpy (pdsm_stats->stat_string,
			    my_mix_table[my_workload].stat_string,
			    OP_STR_LEN);
		pdsm_stats->
		    stat_string[strlen
				(my_mix_table[my_workload].stat_string)] = 0;
		pdsm_stats->neg_stat_op_count =
		    work_obj[my_workload].neg_stat_op_count;
		pdsm_stats->neg_stat_op_time =
		    work_obj[my_workload].neg_stat_op_time;
		my_mix_table[my_workload].
		    neg_stat_string[strlen
				    (my_mix_table
				     [my_workload].neg_stat_string)] = 0;
		my_strncpy (pdsm_stats->neg_stat_string,
			    my_mix_table[my_workload].neg_stat_string,
			    OP_STR_LEN);
		pdsm_stats->
		    neg_stat_string[strlen
				    (my_mix_table
				     [my_workload].neg_stat_string)] = 0;
		pdsm_stats->copyfile_op_count =
		    work_obj[my_workload].copyfile_op_count;
		pdsm_stats->copyfile_op_time =
		    work_obj[my_workload].copyfile_op_time;
		my_mix_table[my_workload].
		    copyfile_string[strlen
				    (my_mix_table[my_workload].
				     copyfile_string)] = 0;
		my_strncpy (pdsm_stats->copyfile_string,
			    my_mix_table[my_workload].copyfile_string,
			    OP_STR_LEN);
		pdsm_stats->
		    copyfile_string[strlen
				    (my_mix_table[my_workload].
				     copyfile_string)] = 0;
		pdsm_stats->rename_op_count =
		    work_obj[my_workload].rename_op_count;
		pdsm_stats->rename_op_time =
		    work_obj[my_workload].rename_op_time;
		my_mix_table[my_workload].
		    rename_string[strlen
				  (my_mix_table[my_workload].rename_string)] =
		    0;
		my_strncpy (pdsm_stats->rename_string,
			    my_mix_table[my_workload].rename_string,
			    OP_STR_LEN);
		pdsm_stats->
		    rename_string[strlen
				  (my_mix_table[my_workload].rename_string)] =
		    0;
		pdsm_stats->statfs_op_count =
		    work_obj[my_workload].statfs_op_count;
		pdsm_stats->statfs_op_time =
		    work_obj[my_workload].statfs_op_time;
		my_mix_table[my_workload].
		    statfs_string[strlen
				  (my_mix_table[my_workload].statfs_string)] =
		    0;
		my_strncpy (pdsm_stats->statfs_string,
			    my_mix_table[my_workload].statfs_string,
			    OP_STR_LEN);
		pdsm_stats->
		    statfs_string[strlen
				  (my_mix_table[my_workload].statfs_string)] =
		    0;
		pdsm_stats->pathconf_op_count =
		    work_obj[my_workload].pathconf_op_count;
		pdsm_stats->pathconf_op_time =
		    work_obj[my_workload].pathconf_op_time;
		my_mix_table[my_workload].
		    pathconf_string[strlen
				    (my_mix_table[my_workload].
				     pathconf_string)] = 0;
		my_strncpy (pdsm_stats->pathconf_string,
			    my_mix_table[my_workload].pathconf_string,
			    OP_STR_LEN);
		pdsm_stats->
		    pathconf_string[strlen
				    (my_mix_table[my_workload].
				     pathconf_string)] = 0;

		pdsm_stats->trunc_op_count =
		    work_obj[my_workload].trunc_op_count;
		pdsm_stats->trunc_op_time =
		    work_obj[my_workload].trunc_op_time;
		my_mix_table[my_workload].trunc_string[strlen
						       (my_mix_table
							[my_workload].
							trunc_string)] = 0;
		my_strncpy (pdsm_stats->trunc_string,
			    my_mix_table[my_workload].trunc_string,
			    OP_STR_LEN);
		pdsm_stats->trunc_string[strlen
					 (my_mix_table[my_workload].
					  trunc_string)] = 0;

		pdsm_stats->custom1_op_count =
		    work_obj[my_workload].custom1_op_count;
		pdsm_stats->custom1_op_time =
		    work_obj[my_workload].custom1_op_time;
		my_mix_table[my_workload].custom1_string[strlen
							 (my_mix_table
							  [my_workload].custom1_string)]
		    = 0;
		my_strncpy (pdsm_stats->custom1_string,
			    my_mix_table[my_workload].custom1_string,
			    OP_STR_LEN);
		pdsm_stats->custom1_string[strlen
					   (my_mix_table
					    [my_workload].custom1_string)] =
		    0;

		pdsm_stats->custom2_op_count =
		    work_obj[my_workload].custom2_op_count;
		pdsm_stats->custom2_op_time =
		    work_obj[my_workload].custom2_op_time;
		my_mix_table[my_workload].custom2_string[strlen
							 (my_mix_table
							  [my_workload].
							  custom2_string)] =
		    0;
		my_strncpy (pdsm_stats->custom2_string,
			    my_mix_table[my_workload].custom2_string,
			    OP_STR_LEN);
		pdsm_stats->
		    custom2_string[strlen
				   (my_mix_table[my_workload].
				    custom2_string)] = 0;
	    }
	    if (pdsm_file_fd)
	    {
		my_pdsm_time = (int) (cur_time - last_pdsm_beat);
		if (my_pdsm_time >= pdsm_interval)
		{
		    if (pdsm_mode == 0)
		    {
#if defined(WIN32)
			largeoffset.QuadPart =
			    sizeof (struct pdsm_remote_stats) *
			    cmdline.client_id;
			SetFilePointerEx (pdsm_file_fd, largeoffset, NULL,
					  FILE_BEGIN);
#else
			I_LSEEK (pdsm_file_fd,
				 sizeof (struct pdsm_remote_stats) *
				 cmdline.client_id, SEEK_SET);
#endif
		    }
#if defined(WIN32)
		    WriteFile (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
		    ret =
			write (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats));
#endif
		    if (ret <= 0)
			log_file (LOG_ERROR,
				  "Writing of pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
		    if (my_pdsm_time >= (2 * pdsm_interval))
			fsync (pdsm_file_fd);
#endif
		    last_pdsm_beat = cur_time;
		}
	    }
#endif
#if defined(PDSM_CO_ACTIVE)
	    if (pdsm_control && my_mix_table[my_workload].shared_buckets == 1)	/* BEWARE THE DIRTY 
										 * HACK (N.P.) 
										 */
	    {
		set_pdsm_attributes ();
	    }
#endif
	}
	if (cmdline.op_rate > 0.0)
	{
	    /* Inject a nap every i_scale ops, scaling nap to fit op_rate */
	    if (inject % i_scale == 0)
	    {
		if ((double) (total_file_ops) / cmdline.op_rate > my_run_time)
		{
		    seconds_to_nap =
			(double) total_file_ops / cmdline.op_rate -
			my_run_time;

		    log_file (LOG_DEBUG,
			      "Nap for %f ms %f %d %f %f %d Warmup \n",
			      seconds_to_nap * 1000, total_file_ops,
			      i_scale, cmdline.op_rate, my_run_time,
			      my_warmup_ops);

		    /* 
		     * Need to count naps and accumulated nap time to report 
		     * at end 
		     */
		    nap (seconds_to_nap * 1000);
		    total_seconds_napped += seconds_to_nap;
		    naps_taken++;
		}
		inject = 1;
	    }
	    inject++;
	}
	loops++;
    }
    if ((pct_complete != 100) && (prev_pct_complete != 100)
	&& cmdline.heartbeat_flag)
    {
	tell_prime_pct_complete (100, cmdline.client_id,
				 R_PERCENT_W_COMPLETE);
    }

    log_file (LOG_EXEC,
	      "Napped %d times for a total of %f seconds in warmup\n",
	      naps_taken, total_seconds_napped);

    if (lru_on)
    {
	log_file (LOG_EXEC,
		  "Post_warmup: Client file_accesses %lld, fd_cache_accesses %lld, fd_cache_misses %lld \n",
		  file_accesses, fd_accesses, fd_cache_misses);
	log_file (LOG_EXEC,
		  "Percent fd_cache misses %lld%%, Percent misses of all file accesses %lld%%\n",
		  (fd_cache_misses * 100LL) / (fd_accesses ? fd_accesses : 1),
		  (fd_cache_misses * 100LL) /
		  (file_accesses ? file_accesses : 1));
	log_file (LOG_EXEC,
		  "fd cache list length %d Cached_open_count %d LRU promote count %lld\n",
		  fd_list_length (), cached_open_count, lru_promote_count);
	log_file (LOG_EXEC, "LRU reuse count %lld \n", lru_reuse_count);
    }
    /* Reset back to zero for the RUN phase */
    log_file (LOG_EXEC, "Ending WARMUP phase.\n");

    fd_cache_misses = fd_accesses = file_accesses = lru_promote_count =
	lru_reuse_count = 0;

    naps_taken = 0;
    total_seconds_napped = 0.0;
    total_file_ops = 0.0;
    prev_file_ops = 0.0;
    clear_stats ();

    /* Sends an intermediate message from 
     * Client -> NM -> Prime to allow MON scripts @ Prime */
    send_warmup_done ();

    /* Now for the measurement phase */
    log_file (LOG_EXEC, "Starting RUN phase.\n");

    loops = 1;

    phase = RUN_PHASE;

    my_start_time = gettime ();
    sanity_time = gettime ();
    while (1)
    {
	my_gen_op_time = gettime ();

	/* While this may seem crazy, someone could have ntp running, or 
	 * manually set the clock back in time.  By doing do, the latency 
	 * calculations will be wrong.  Some negative times, mixed with 
	 * some positive times to skew the results to whatever someone 
	 * might want.  Thus, this code prevents tampering with the 
	 * passage of time.   Dr. Who would be proud !
	 * Note: I personally debugged a case where time moved backwards.
	 */
	if ((my_gen_op_time + (double) 1.0) < sanity_time)
	{
	    log_file (LOG_ERROR, "Time is moving backwards !!! \n");
	    sanity_time = my_gen_op_time;
	    R_exit (BAD_CLOCK, 0);
	}
	before_total_file_ops = total_file_ops;
	gen_op (test_ops, impersonate);
	cur_time = gettime ();
	if (tmp_time < (cur_time - my_gen_op_time))
	    tmp_time = ((cur_time - my_gen_op_time) - my_time_res);
	else
	    tmp_time = (cur_time - my_gen_op_time);
	my_run_time = cur_time - my_start_time;

	/*-----------------*/
	/* heartbeat       */
	/*-----------------*/
	if (cmdline.heartbeat_flag &&
	    ((int) (my_gen_op_time - last_heart_beat) >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.client_id, RUN_BEAT,
				  ach_ops_per_second);
	    last_heart_beat = my_gen_op_time;
	}
	/*----------------------------*/
	/* For the combined workloads */
	/*----------------------------*/
	if (total_file_ops > before_total_file_ops)	/* No op, then no 
							 * adding time */
	    total_file_op_time += tmp_time;

	/*-----------------------------*/
	/* For the particular workload */
	/*-----------------------------*/
	if (total_file_ops > before_total_file_ops)	/* No op, then no 
							 * adding time */
	    work_obj[my_workload].total_file_op_time += tmp_time;

	if (my_last_tick == 0.0)
	    my_last_tick = gettime ();
	elap_tick = my_gen_op_time - my_last_tick;
	if (elap_tick > (double) ELAP_T)
	{
	    my_last_tick = my_gen_op_time;
	    tick = 0;
	    save_loops = loops;
	    loops = 1;
	}
	else
	{
	    tick = 1;
	}

	if (tick == 0)
	{

	    pct_complete =
		(int) (((my_run_time + ELAP_T) * 100.0) / cmdline.run_time);
	    if (pct_complete > 100)
		pct_complete = 100;

	    if (pct_complete && (pct_complete != prev_pct_complete)
		&& ((pct_complete % 10) == 0) && cmdline.heartbeat_flag)
	    {
		tell_prime_pct_complete (pct_complete, cmdline.client_id,
					 R_PERCENT_COMPLETE);
		prev_pct_complete = pct_complete;
	    }
	}

	/* Stop when we have reached our defined run time */
	if (cur_time - my_start_time >= (double) cmdline.run_time)
	{
	    cmdline.run_time = cur_time - my_start_time;
	    log_file (LOG_EXEC, "Ending RUN phase.\n");

	    break;
	}

	/* Stop at the absolute Epoch start + my_mix_table[my_workload].warm_time + cmdline.run_time */
	if (((getepochtime () + time_skew) - start_epoch) >
	    (double) (my_mix_table[my_workload].warm_time + cmdline.run_time))
	{
	    log_file (LOG_EXEC, "Ending RUN phase.\n");
	    break;
	}
	my_ops++;

	if (tick == 0)
	{
	    ops_this_tick = total_file_ops - prev_file_ops;
	    prev_file_ops = total_file_ops;
	    post_time = cur_time;

	    time_per_tick = post_time - time_of_last_tick;
	    if (time_per_tick == (double) 0.0)
		time_per_tick = (double) 0.001;
	    time_of_last_tick = post_time;

	    ach_ops_per_second = (double) ops_this_tick / time_per_tick;
	    if (ach_ops_per_second == (double) 0.0)
		ach_ops_per_second = (double) 1.000;
#if defined(PDSM_RO_ACTIVE)
	    /*
	     * If the PDSM mechansim is active....
	     * then update the stats.
	     */
	    if (pdsm_stats)
	    {
		time_used = (cur_time - my_start_time);
		if (time_used != 0)
		{
		    pdsm_stats->read_throughput =
			(total_read_bytes / 1024.0) / (float) (cur_time -
							       my_start_time);
		    pdsm_stats->write_throughput =
			(total_write_bytes / 1024.0) / (float) (cur_time -
								my_start_time);
		    pdsm_stats->meta_throughput =
			(((total_meta_r_ops +
			   total_meta_w_ops) * META_B) / 1024.0) /
			(float) (cur_time - my_start_time);
		    pdsm_stats->run_time = post_time - my_start_time;
		}
		else
		{
		    pdsm_stats->read_throughput = 0;
		    pdsm_stats->write_throughput = 0;
		    pdsm_stats->meta_throughput = 0;
		    pdsm_stats->run_time = 0;
		}
		pdsm_stats->epoch_time = time (NULL);
		pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
		pdsm_stats->achieved_op_rate = ach_ops_per_second;
		pdsm_stats->cur_state = RUN_BEAT;
		pdsm_stats->client_id = cmdline.client_id;
		pdsm_stats->mode = pdsm_mode;

		my_strncpy (pdsm_stats->client_name, client_localname,
			    MAXHNAME);
		pdsm_stats->client_name[strlen (client_localname)] = 0;

		work_obj[my_workload].
		    work_obj_name[strlen
				  (work_obj[my_workload].work_obj_name)] = 0;
		my_strncpy (pdsm_stats->workload_name,
			    work_obj[my_workload].work_obj_name, MAXWLNAME);
		pdsm_stats->
		    workload_name[strlen
				  (work_obj[my_workload].work_obj_name)] = 0;
		pdsm_stats->read_op_count =
		    work_obj[my_workload].read_op_count;
		pdsm_stats->read_op_time = work_obj[my_workload].read_op_time;

		my_mix_table[my_workload].read_string[strlen
						      (my_mix_table
						       [my_workload].read_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_string,
			    my_mix_table[my_workload].read_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_string[strlen
				(my_mix_table[my_workload].read_string)] = 0;
		pdsm_stats->read_file_op_count =
		    work_obj[my_workload].read_file_op_count;
		pdsm_stats->read_file_op_time =
		    work_obj[my_workload].read_file_op_time;

		my_mix_table[my_workload].read_file_string[strlen
							   (my_mix_table
							    [my_workload].
							    read_file_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_file_string,
			    my_mix_table[my_workload].read_file_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_file_string[strlen
				     (my_mix_table[my_workload].
				      read_file_string)] = 0;

		pdsm_stats->read_rand_op_count =
		    work_obj[my_workload].read_rand_op_count;
		pdsm_stats->read_rand_op_time =
		    work_obj[my_workload].read_rand_op_time;

		my_mix_table[my_workload].read_rand_string[strlen
							   (my_mix_table
							    [my_workload].
							    read_rand_string)]
		    = 0;
		my_strncpy (pdsm_stats->read_rand_string,
			    my_mix_table[my_workload].read_rand_string,
			    OP_STR_LEN);
		pdsm_stats->
		    read_rand_string[strlen
				     (my_mix_table[my_workload].
				      read_file_string)] = 0;
		pdsm_stats->write_op_count =
		    work_obj[my_workload].write_op_count;
		pdsm_stats->write_op_time =
		    work_obj[my_workload].write_op_time;

		my_mix_table[my_workload].write_string[strlen
						       (my_mix_table
							[my_workload].write_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_string,
			    my_mix_table[my_workload].write_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_string[strlen
				 (my_mix_table[my_workload].write_string)] =
		    0;

		pdsm_stats->write_file_op_count =
		    work_obj[my_workload].write_file_op_count;
		pdsm_stats->write_file_op_time =
		    work_obj[my_workload].write_file_op_time;

		my_mix_table[my_workload].write_file_string[strlen
							    (my_mix_table
							     [my_workload].
							     write_file_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_file_string,
			    my_mix_table[my_workload].write_file_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_file_string[strlen
				      (my_mix_table[my_workload].
				       write_file_string)] = 0;

		pdsm_stats->open_op_count =
		    work_obj[my_workload].open_op_count;
		pdsm_stats->close_op_count =
		    work_obj[my_workload].close_op_count;
		pdsm_stats->mmap_write_op_count =
		    work_obj[my_workload].mmap_write_op_count;
		pdsm_stats->mmap_write_op_time =
		    work_obj[my_workload].mmap_write_op_time;

		my_mix_table[my_workload].mmap_write_string[strlen
							    (my_mix_table
							     [my_workload].
							     mmap_write_string)]
		    = 0;
		my_strncpy (pdsm_stats->mmap_write_string,
			    my_mix_table[my_workload].mmap_write_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mmap_write_string[strlen
				      (my_mix_table[my_workload].
				       mmap_write_string)] = 0;

		pdsm_stats->mmap_read_op_count =
		    work_obj[my_workload].mmap_read_op_count;
		pdsm_stats->mmap_read_op_time =
		    work_obj[my_workload].mmap_read_op_time;

		my_mix_table[my_workload].mmap_read_string[strlen
							   (my_mix_table
							    [my_workload].
							    mmap_read_string)]
		    = 0;
		my_strncpy (pdsm_stats->mmap_read_string,
			    my_mix_table[my_workload].mmap_read_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mmap_read_string[strlen
				     (my_mix_table[my_workload].
				      mmap_read_string)] = 0;

		pdsm_stats->write_rand_op_count =
		    work_obj[my_workload].write_rand_op_count;
		pdsm_stats->write_rand_op_time =
		    work_obj[my_workload].write_rand_op_time;

		my_mix_table[my_workload].write_rand_string[strlen
							    (my_mix_table
							     [my_workload].
							     write_rand_string)]
		    = 0;
		my_strncpy (pdsm_stats->write_rand_string,
			    my_mix_table[my_workload].write_rand_string,
			    OP_STR_LEN);
		pdsm_stats->
		    write_rand_string[strlen
				      (my_mix_table[my_workload].
				       write_rand_string)] = 0;

		pdsm_stats->rmw_op_count = work_obj[my_workload].rmw_op_count;
		pdsm_stats->rmw_op_time = work_obj[my_workload].rmw_op_time;

		my_mix_table[my_workload].rmw_string[strlen
						     (my_mix_table
						      [my_workload].rmw_string)]
		    = 0;
		my_strncpy (pdsm_stats->rmw_string,
			    my_mix_table[my_workload].rmw_string, OP_STR_LEN);
		pdsm_stats->
		    rmw_string[strlen (my_mix_table[my_workload].rmw_string)]
		    = 0;

		pdsm_stats->mkdir_op_count =
		    work_obj[my_workload].mkdir_op_count;
		pdsm_stats->mkdir_op_time =
		    work_obj[my_workload].mkdir_op_time;

		my_mix_table[my_workload].mkdir_string[strlen
						       (my_mix_table
							[my_workload].mkdir_string)]
		    = 0;
		my_strncpy (pdsm_stats->mkdir_string,
			    my_mix_table[my_workload].mkdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    mkdir_string[strlen
				 (my_mix_table[my_workload].mkdir_string)] =
		    0;

		pdsm_stats->rmdir_op_count =
		    work_obj[my_workload].rmdir_op_count;
		pdsm_stats->rmdir_op_time =
		    work_obj[my_workload].rmdir_op_time;

		my_mix_table[my_workload].rmdir_string[strlen
						       (my_mix_table
							[my_workload].rmdir_string)]
		    = 0;
		my_strncpy (pdsm_stats->rmdir_string,
			    my_mix_table[my_workload].rmdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    rmdir_string[strlen
				 (my_mix_table[my_workload].rmdir_string)] =
		    0;

		pdsm_stats->unlink_op_count =
		    work_obj[my_workload].unlink_op_count;
		pdsm_stats->unlink_op_time =
		    work_obj[my_workload].unlink_op_time;

		my_mix_table[my_workload].unlink_string[strlen
							(my_mix_table
							 [my_workload].unlink_string)]
		    = 0;
		my_strncpy (pdsm_stats->unlink_string,
			    my_mix_table[my_workload].unlink_string,
			    OP_STR_LEN);
		pdsm_stats->
		    unlink_string[strlen
				  (my_mix_table[my_workload].unlink_string)] =
		    0;

		pdsm_stats->unlink2_op_count =
		    work_obj[my_workload].unlink2_op_count;
		pdsm_stats->unlink2_op_time =
		    work_obj[my_workload].unlink2_op_time;

		my_mix_table[my_workload].unlink2_string[strlen
							 (my_mix_table
							  [my_workload].
							  unlink2_string)] =
		    0;
		my_strncpy (pdsm_stats->unlink2_string,
			    my_mix_table[my_workload].unlink2_string,
			    OP_STR_LEN);
		pdsm_stats->
		    unlink2_string[strlen
				   (my_mix_table[my_workload].
				    unlink2_string)] = 0;
		pdsm_stats->create_op_count =
		    work_obj[my_workload].create_op_count;
		pdsm_stats->create_op_time =
		    work_obj[my_workload].create_op_time;

		my_mix_table[my_workload].create_string[strlen
							(my_mix_table
							 [my_workload].create_string)]
		    = 0;
		my_strncpy (pdsm_stats->create_string,
			    my_mix_table[my_workload].create_string,
			    OP_STR_LEN);
		pdsm_stats->
		    create_string[strlen
				  (my_mix_table[my_workload].create_string)] =
		    0;

		pdsm_stats->append_op_count =
		    work_obj[my_workload].append_op_count;
		pdsm_stats->append_op_time =
		    work_obj[my_workload].append_op_time;

		my_mix_table[my_workload].append_string[strlen
							(my_mix_table
							 [my_workload].append_string)]
		    = 0;
		my_strncpy (pdsm_stats->append_string,
			    my_mix_table[my_workload].append_string,
			    OP_STR_LEN);
		pdsm_stats->
		    append_string[strlen
				  (my_mix_table[my_workload].append_string)] =
		    0;

		pdsm_stats->lock_op_count =
		    work_obj[my_workload].lock_op_count;
		pdsm_stats->lock_op_time = work_obj[my_workload].lock_op_time;

		my_mix_table[my_workload].locking_string[strlen
							 (my_mix_table
							  [my_workload].
							  locking_string)] =
		    0;
		my_strncpy (pdsm_stats->locking_string,
			    my_mix_table[my_workload].locking_string,
			    OP_STR_LEN);
		pdsm_stats->
		    locking_string[strlen
				   (my_mix_table[my_workload].
				    locking_string)] = 0;
		pdsm_stats->access_op_count =
		    work_obj[my_workload].access_op_count;
		pdsm_stats->access_op_time =
		    work_obj[my_workload].access_op_time;

		my_mix_table[my_workload].access_string[strlen
							(my_mix_table
							 [my_workload].access_string)]
		    = 0;
		my_strncpy (pdsm_stats->access_string,
			    my_mix_table[my_workload].access_string,
			    OP_STR_LEN);
		pdsm_stats->
		    access_string[strlen
				  (my_mix_table[my_workload].access_string)] =
		    0;

		pdsm_stats->chmod_op_count =
		    work_obj[my_workload].chmod_op_count;
		pdsm_stats->chmod_op_time =
		    work_obj[my_workload].chmod_op_time;

		my_mix_table[my_workload].chmod_string[strlen
						       (my_mix_table
							[my_workload].chmod_string)]
		    = 0;
		my_strncpy (pdsm_stats->chmod_string,
			    my_mix_table[my_workload].chmod_string,
			    OP_STR_LEN);
		pdsm_stats->
		    chmod_string[strlen
				 (my_mix_table[my_workload].chmod_string)] =
		    0;

		pdsm_stats->readdir_op_count =
		    work_obj[my_workload].readdir_op_count;
		pdsm_stats->readdir_op_time =
		    work_obj[my_workload].readdir_op_time;

		my_mix_table[my_workload].readdir_string[strlen
							 (my_mix_table
							  [my_workload].
							  readdir_string)] =
		    0;
		my_strncpy (pdsm_stats->readdir_string,
			    my_mix_table[my_workload].readdir_string,
			    OP_STR_LEN);
		pdsm_stats->
		    readdir_string[strlen
				   (my_mix_table[my_workload].
				    readdir_string)] = 0;
		pdsm_stats->stat_op_count =
		    work_obj[my_workload].stat_op_count;
		pdsm_stats->stat_op_time = work_obj[my_workload].stat_op_time;

		my_mix_table[my_workload].stat_string[strlen
						      (my_mix_table
						       [my_workload].stat_string)]
		    = 0;
		my_strncpy (pdsm_stats->stat_string,
			    my_mix_table[my_workload].stat_string,
			    OP_STR_LEN);
		pdsm_stats->
		    stat_string[strlen
				(my_mix_table[my_workload].stat_string)] = 0;
		pdsm_stats->neg_stat_op_count =
		    work_obj[my_workload].neg_stat_op_count;
		pdsm_stats->neg_stat_op_time =
		    work_obj[my_workload].neg_stat_op_time;

		my_mix_table[my_workload].neg_stat_string[strlen
							  (my_mix_table
							   [my_workload].neg_stat_string)]
		    = 0;
		my_strncpy (pdsm_stats->neg_stat_string,
			    my_mix_table[my_workload].neg_stat_string,
			    OP_STR_LEN);
		pdsm_stats->
		    neg_stat_string[strlen
				    (my_mix_table
				     [my_workload].neg_stat_string)] = 0;

		pdsm_stats->copyfile_op_count =
		    work_obj[my_workload].copyfile_op_count;
		pdsm_stats->copyfile_op_time =
		    work_obj[my_workload].copyfile_op_time;

		my_mix_table[my_workload].copyfile_string[strlen
							  (my_mix_table
							   [my_workload].
							   copyfile_string)] =
		    0;
		my_strncpy (pdsm_stats->copyfile_string,
			    my_mix_table[my_workload].copyfile_string,
			    OP_STR_LEN);
		pdsm_stats->
		    copyfile_string[strlen
				    (my_mix_table[my_workload].
				     copyfile_string)] = 0;

		pdsm_stats->rename_op_count =
		    work_obj[my_workload].rename_op_count;
		pdsm_stats->rename_op_time =
		    work_obj[my_workload].rename_op_time;

		my_mix_table[my_workload].rename_string[strlen
							(my_mix_table
							 [my_workload].rename_string)]
		    = 0;
		my_strncpy (pdsm_stats->rename_string,
			    my_mix_table[my_workload].rename_string,
			    OP_STR_LEN);
		pdsm_stats->
		    rename_string[strlen
				  (my_mix_table[my_workload].rename_string)] =
		    0;

		pdsm_stats->statfs_op_count =
		    work_obj[my_workload].statfs_op_count;
		pdsm_stats->statfs_op_time =
		    work_obj[my_workload].statfs_op_time;

		my_mix_table[my_workload].statfs_string[strlen
							(my_mix_table
							 [my_workload].statfs_string)]
		    = 0;
		my_strncpy (pdsm_stats->statfs_string,
			    my_mix_table[my_workload].statfs_string,
			    OP_STR_LEN);
		pdsm_stats->
		    statfs_string[strlen
				  (my_mix_table[my_workload].statfs_string)] =
		    0;

		pdsm_stats->pathconf_op_count =
		    work_obj[my_workload].pathconf_op_count;
		pdsm_stats->pathconf_op_time =
		    work_obj[my_workload].pathconf_op_time;

		my_mix_table[my_workload].pathconf_string[strlen
							  (my_mix_table
							   [my_workload].
							   pathconf_string)] =
		    0;
		my_strncpy (pdsm_stats->pathconf_string,
			    my_mix_table[my_workload].pathconf_string,
			    OP_STR_LEN);
		pdsm_stats->
		    pathconf_string[strlen
				    (my_mix_table[my_workload].
				     pathconf_string)] = 0;

		pdsm_stats->trunc_op_count =
		    work_obj[my_workload].trunc_op_count;
		pdsm_stats->trunc_op_time =
		    work_obj[my_workload].trunc_op_time;
		my_mix_table[my_workload].trunc_string[strlen
						       (my_mix_table
							[my_workload].
							trunc_string)] = 0;
		my_strncpy (pdsm_stats->trunc_string,
			    my_mix_table[my_workload].trunc_string,
			    OP_STR_LEN);
		pdsm_stats->trunc_string[strlen
					 (my_mix_table[my_workload].
					  trunc_string)] = 0;

		pdsm_stats->custom1_op_count =
		    work_obj[my_workload].custom1_op_count;
		pdsm_stats->custom1_op_time =
		    work_obj[my_workload].custom1_op_time;
		my_mix_table[my_workload].custom1_string[strlen
							 (my_mix_table
							  [my_workload].custom1_string)]
		    = 0;
		my_strncpy (pdsm_stats->custom1_string,
			    my_mix_table[my_workload].custom1_string,
			    OP_STR_LEN);
		pdsm_stats->custom1_string[strlen
					   (my_mix_table
					    [my_workload].custom1_string)] =
		    0;

		pdsm_stats->custom2_op_count =
		    work_obj[my_workload].custom2_op_count;
		pdsm_stats->custom2_op_time =
		    work_obj[my_workload].custom2_op_time;

		my_mix_table[my_workload].custom2_string[strlen
							 (my_mix_table
							  [my_workload].
							  custom2_string)] =
		    0;
		my_strncpy (pdsm_stats->custom2_string,
			    my_mix_table[my_workload].custom2_string,
			    OP_STR_LEN);
		pdsm_stats->
		    custom2_string[strlen
				   (my_mix_table[my_workload].
				    custom2_string)] = 0;
	    }
	    if (pdsm_file_fd)
	    {
		my_pdsm_time = (int) (cur_time - last_pdsm_beat);
		if (my_pdsm_time >= pdsm_interval)
		{
		    if (pdsm_mode == 0)
		    {
#if defined(WIN32)
			largeoffset.QuadPart =
			    sizeof (struct pdsm_remote_stats) *
			    cmdline.client_id;
			SetFilePointerEx (pdsm_file_fd, largeoffset, NULL,
					  FILE_BEGIN);
#else
			I_LSEEK (pdsm_file_fd,
				 sizeof (struct pdsm_remote_stats) *
				 cmdline.client_id, SEEK_SET);
#endif
		    }
#if defined(WIN32)
		    WriteFile (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
		    ret =
			write (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats));
#endif
#if defined(FSYNC_PDSM)
		    if (my_pdsm_time >= (2 * pdsm_interval))
			fsync (pdsm_file_fd);
#endif
		    last_pdsm_beat = cur_time;
		}
	    }
#endif
#if defined(PDSM_CO_ACTIVE)
	    if (pdsm_control)
	    {
		set_pdsm_attributes ();
	    }
#endif
	}
	if (cmdline.op_rate > 0.0)
	{
	    if (inject % i_scale == 0)
	    {
		if ((double) total_file_ops / cmdline.op_rate > my_run_time)
		{
		    seconds_to_nap =
			(double) total_file_ops / cmdline.op_rate -
			my_run_time;

		    log_file (LOG_DEBUG,
			      "Nap for %f ms %f %d %.2f %f %d Run\n",
			      seconds_to_nap * 1000, total_file_ops,
			      i_scale, cmdline.op_rate, my_run_time, my_ops);

		    nap (seconds_to_nap * 1000);
		    /* 
		     * Need to count naps and accumulated nap time to 
		     * report at end 
		     */
		    total_seconds_napped += seconds_to_nap;
		    naps_taken++;
		}
		inject = 1;
	    }
	    inject++;
	}
	loops++;
    }
    if ((pct_complete != 100) && (prev_pct_complete != 100)
	&& cmdline.heartbeat_flag)
    {
	tell_prime_pct_complete (100, cmdline.client_id, R_PERCENT_COMPLETE);
    }
#if defined(PDSM_RO_ACTIVE)
    if (pdsm_stats)
    {
	pdsm_stats->cur_state = STOP_BEAT;
	pdsm_stats->client_id = cmdline.client_id;
	pdsm_stats->mode = pdsm_mode;
	pdsm_stats->epoch_time = time (NULL);
    }
    if (pdsm_file_fd)
    {
	if (pdsm_mode == 0)
	{
#if defined(WIN32)
	    largeoffset.QuadPart =
		sizeof (struct pdsm_remote_stats) * cmdline.client_id;
	    SetFilePointerEx (pdsm_file_fd, largeoffset, NULL, FILE_BEGIN);
#else
	    I_LSEEK (pdsm_file_fd,
		     sizeof (struct pdsm_remote_stats) * cmdline.client_id,
		     SEEK_SET);
#endif
	}
#if defined(WIN32)
	WriteFile (pdsm_file_fd, pdsm_stats,
		   sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
	ret =
	    write (pdsm_file_fd, pdsm_stats,
		   sizeof (struct pdsm_remote_stats));
#endif
	if (ret <= 0)
	    log_file (LOG_ERROR, "Writing of pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
	fsync (pdsm_file_fd);
#endif
    }
#endif
    log_file (LOG_EXEC,
	      "Napped %d times for a total of %f seconds in measurement interval \n",
	      naps_taken, total_seconds_napped);
    if (lru_on)
    {
	log_file (LOG_EXEC,
		  "Post_run: Client file_accesses %lld, fd_cache_accesses %lld, fd_cache_misses %lld \n",
		  file_accesses, fd_accesses, fd_cache_misses);
	log_file (LOG_EXEC,
		  "Percent fd_cache misses %lld, Percent misses of all file accesses %lld\n",
		  (fd_cache_misses * 100LL) / (fd_accesses ? fd_accesses : 1),
		  (fd_cache_misses * 100LL) /
		  (file_accesses ? file_accesses : 1));
	log_file (LOG_EXEC,
		  "fd cache list length %d LRU promote count %lld\n",
		  fd_list_length (), lru_promote_count);
	log_file (LOG_EXEC, "LRU reuse count %lld \n", lru_reuse_count);
    }

    fd_cache_misses = fd_accesses = file_accesses = lru_promote_count = 0;

    (*Netmist_Ops[my_workload].teardown) ();

    /* 
     * Set the globals so that they can be sent back to the prime to log it 
     * with print_results 
     *
     */

    /*
     * In SP1 workloads the write_throughput is a combination of write(), 
     * meta-data writes, and meta-data reads.  This gives the vendor credit 
     * for throughput when the workload has meta-data that is causing writes.  
     * Without giving any credit a pure meta-data workload could give the 
     * vendor no credit for writes, when in fact there are many.
     * In SP1 workloads, the vendor is given credit for the meta-data write, 
     * and meta-data read, when any meta-data id done.  The thinking is that 
     * to to a meta-data write there musht have been a meta-data read first.
     */
    if (IS_SP1_WL (my_workload))
    {
	Nread_throughput = total_read_bytes / my_run_time;
	Nread_kbytes = total_read_bytes;
	read_throughput =
	    (total_read_bytes +
	     ((total_meta_r_ops + total_meta_w_ops) * META_B)) / my_run_time;
	read_kbytes =
	    total_read_bytes +
	    ((total_meta_r_ops + total_meta_w_ops) * META_B);
	Nwrite_throughput = total_write_bytes / my_run_time;
	Nwrite_kbytes = total_write_bytes;
	write_throughput =
	    (total_write_bytes +
	     ((total_meta_w_ops + total_meta_r_ops) * META_B)) / my_run_time;
	write_kbytes =
	    total_write_bytes +
	    ((total_meta_w_ops + total_meta_r_ops) * META_B);
	meta_r_kbytes = (total_meta_r_ops * META_B);
	meta_w_kbytes = (total_meta_w_ops * META_B);
    }
    /*
     * In SP2 workloads or later, the vendor is given credit for the meta-data write, 
     * and/or meta-data read, when a specific  meta-data op done.  
     */
    else
    {
	Nread_throughput = total_read_bytes / my_run_time;
	Nread_kbytes = total_read_bytes;
	read_throughput =
	    (total_read_bytes + (total_meta_r_ops * META_B)) / my_run_time;
	read_kbytes = (total_read_bytes + (total_meta_r_ops * META_B));
	Nwrite_throughput = total_write_bytes / my_run_time;
	Nwrite_kbytes = total_write_bytes;
	write_throughput =
	    (total_write_bytes + (total_meta_w_ops * META_B)) / my_run_time;
	write_kbytes = total_write_bytes + (total_meta_w_ops * META_B);
	meta_r_kbytes = (total_meta_r_ops * META_B);
	meta_w_kbytes = (total_meta_w_ops * META_B);
    }

    /* Teardown the workdir handle */
    VFS (dfree, &netmist_vfs_workdir);
}

/**
 * @brief Set up netmist_vfs_workdir by either creating or finding it.
 */
static void
setup_workdir (int client_id, int create)
{
    char buf[1024];
    int ret;
    struct netmist_vfs_dir *procdir;

    if (cmdline.sharing_flag)
    {
	snprintf (buf, sizeof (buf), "SM_%s",
		  work_obj[my_workload].work_obj_name);
    }
    else
    {
	snprintf (buf, sizeof (buf), "CL%d_%s", client_id,
		  work_obj[my_workload].work_obj_name);
    }
    procdir = netmist_vfs_root ();
    ret = netmist_vfs_walkme (&procdir, buf);
    if (ret)
    {
	log_file (LOG_ERROR, "client_init_files VFS walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    /* Use virtualized interface to create the directory. */
    log_file (LOG_DEBUG, "Mkdir %s \n", VFS (pathptr, procdir));
    ret = (*Netmist_Ops[my_workload].stat_workdir) (procdir);
    if (ret != 0)
    {
	ret = (*Netmist_Ops[my_workload].init_dir) (procdir);
	if (ret != 0)
	{
	    log_file (LOG_ERROR, "client_init_files cannot mkdir %s: %d\n",
		      VFS (pathptr, procdir), ret);
	    R_exit (LOAD_GEN_ERR, ret);
	}
    }

    slinky (0, my_mix_table[my_workload].extra_dir_levels,
	    my_mix_table[my_workload].chaff_count, &procdir,
	    create ? SLINKY_CREATE : SLINKY_NOOP);

    /* Publish reference */
    netmist_vfs_workdir = procdir;
}

/**
 * @brief Initializes the data set for the benchmark.
 */
void
complete_client_init_work (void)
{
#if defined(EIN32)
    DWORD ret = 0;
#endif
#if defined(PDSM_RO_ACTIVE)
    int flags = 0;

    if ((cmdline.skip_init == 1) && (dir_exist () == 0))
    {
	log_file (LOG_EXEC, "**** Using abbreviated INIT phase ****\n");
	log_file (LOG_EXEC,
		  "Run may fail if dataset was not created before\n");
    }

    /*
     * If the PDSM mechansim is active....
     * then update the stats.
     */

#ifdef WIN32
    if ((strcmp (cmdline.windows_pdsm_file, "_") != 0) && (pdsm_file_fd == 0))
    {
#else
    if ((strcmp (cmdline.unix_pdsm_file, "_") != 0) && (pdsm_file_fd == 0))
    {
#endif
	pdsm_mode = cmdline.pdsm_mode;
	pdsm_interval = cmdline.pdsm_interval;
#ifdef WIN32
	strcpy (my_pdsm_file, cmdline.windows_pdsm_file);
#else
	strcpy (my_pdsm_file, cmdline.unix_pdsm_file);
#endif
	log_file (LOG_EXEC, "PDSM filename: %s Entry size %d\n", my_pdsm_file,
		  sizeof (struct pdsm_remote_stats));
	if (pdsm_mode == 0)	/* Over-write mode */
	{
#if defined(WIN32)
	    flags |= FILE_FLAG_POSIX_SEMANTICS;
#else
	    flags = O_CREAT | O_RDWR;
#endif
	}
	else			/* Append mode */
	{
#if defined(WIN32)
	    flags |= FILE_FLAG_POSIX_SEMANTICS | FILE_APPEND_DATA;
#else
	    flags = O_CREAT | O_RDWR | O_APPEND;
#endif
	}
#if defined(WIN32)
	pdsm_file_fd = CreateFile (my_pdsm_file,
				   GENERIC_READ | GENERIC_WRITE,
				   FILE_SHARE_READ | FILE_SHARE_WRITE |
				   FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,
				   flags, NULL);
#else
	pdsm_file_fd = I_OPEN (my_pdsm_file, flags, 0666);
#endif

	if (pdsm_file_fd < 0)
	    pdsm_file_fd = 0;
	else
	    guard_band++;

	if (pdsm_file_fd && (pdsm_stats == NULL))
	    pdsm_stats = (struct pdsm_remote_stats *)
		my_malloc (sizeof (struct pdsm_remote_stats));
    }
#endif
#if defined(PDSM_CO_ACTIVE)
#ifdef WIN32
    if ((strcmp (cmdline.windows_pdsm_control_file, "_") != 0)
	&& (pdsm_control_file_fd == 0))
#else
    if ((strcmp (cmdline.unix_pdsm_control_file, "_") != 0)
	&& (pdsm_control_file_fd == 0))
#endif
    {
	pdsm_interval = cmdline.pdsm_interval;
#ifdef WIN32
	strcpy (my_pdsm_control_file, cmdline.windows_pdsm_control_file);
#else
	strcpy (my_pdsm_control_file, cmdline.unix_pdsm_control_file);
#endif
	log_file (LOG_EXEC, "PDSM Control filename: %s Entry size %d\n",
		  my_pdsm_control_file, sizeof (struct pdsm_remote_control));
#if defined(WIN32)
	flags |= FILE_FLAG_POSIX_SEMANTICS;
#else
	flags = O_CREAT | O_RDWR;
#endif
#if defined(WIN32)
	pdsm_control_file_fd = CreateFile (my_pdsm_control_file,
					   GENERIC_READ | GENERIC_WRITE,
					   FILE_SHARE_READ | FILE_SHARE_WRITE
					   | FILE_SHARE_DELETE, NULL,
					   OPEN_ALWAYS, flags, NULL);
#else
	pdsm_control_file_fd = I_OPEN (my_pdsm_control_file, flags, 0666);
#endif

	if (pdsm_control_file_fd < 0)
	    pdsm_control_file_fd = 0;
	else
	    guard_band++;

	if (pdsm_control_file_fd && (pdsm_control == NULL))
	    pdsm_control = (struct pdsm_remote_control *)
		my_malloc (sizeof (struct pdsm_remote_control));
    }
#endif
    if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "BLOCK")) == 0)
    {
	client_init_block (cmdline.workdir);	/* Initialize all blocks of cmdline.workdir/dev */
	log_file (LOG_EXEC_VERBOSE, "Finished client init device.\n");
    }
    else
    {
	if ((cmdline.skip_init == 1) && (dir_exist () == 0))
	{

	    /*
	     * If we're skipping initialization, we still need to walk down to
	     * get our working directory, which would ordinarily be a
	     * side-effect of client_init_files.  Instead, do just that piece here.
	     */

	    setup_workdir (cmdline.client_id, 0);
	}
	else
	{
	    if (cmdline.sharing_flag)
	    {
		client_init_files (0);
	    }
	    else
	    {
		client_init_files (cmdline.client_id);
	    }
	}
	log_file (LOG_EXEC, "Finished client init files.\n");
	if (lru_on)
	{
	    log_file (LOG_EXEC,
		      "Client file_accesses %lld fd_cache_accesses %lld fd_cache_misses %lld \n",
		      file_accesses, fd_accesses, fd_cache_misses);
	    log_file (LOG_EXEC,
		      "Percent fd_cache misses %lld%% Percent misses of all accesses %lld %%\n",
		      (fd_cache_misses * 100LL) /
		      (fd_accesses ? fd_accesses : 1),
		      (fd_cache_misses * 100LL) /
		      (file_accesses ? file_accesses : 1));
	    log_file (LOG_EXEC,
		      "fd cache list length %d Cached_open_count %d LRU promote count %lld \n",
		      fd_list_length (), cached_open_count,
		      lru_promote_count);
	    log_file (LOG_EXEC, "LRU reuse count %lld \n", lru_reuse_count);
	}
	fd_accesses = fd_cache_misses = file_accesses = cached_open_count =
	    lru_promote_count = lru_reuse_count = 0;
    }

    clear_stats ();
}

/**
 * @brief Complete the work associated with receipt of a Epoch message from
 * the nodeManager.
 */
void
complete_client_epoch_work (void)
{
    double my_eclock_time, prime_eclock_time, time_skew;

    internal_cfg.u.client_cfg.start_epoch = g_cc->c_start_epoch;
    internal_cfg.u.client_cfg.prime_eclock_time = g_cc->c_prime_eclock_time;
    internal_cfg.u.client_cfg.my_eclock_time = getepochtime ();
    internal_cfg.u.client_cfg.curr_epoch = getepochtime ();

    my_eclock_time = internal_cfg.u.client_cfg.my_eclock_time;
    prime_eclock_time = internal_cfg.u.client_cfg.prime_eclock_time;
    time_skew = prime_eclock_time - my_eclock_time;

    /*
     * Calculate the difference in clocks between mine, and the prime's 
     * clock. Then use the difference to calculate the prime's clock 
     * given mine.
     * By doing this, the clocks do not need to be synchronized across 
     * all of the clients, and the benchmark can have clients in different 
     * time zones.
     * Note: Won't work if clients are in different gravatation field 
     * densities, or if either of the clients is in motion, relative 
     * to the Prime.
     */

    /* Storing clock adjustment.                   */
    internal_cfg.u.client_cfg.time_skew = time_skew;
}

/**
 * @brief Complete the work associated with receipt of a GO message from
 * the nodeManager.
 */
void
complete_client_go_work (void)
{
    extern int min_pre_name_length;
    extern int max_pre_name_length;
    extern int min_post_name_length;
    extern int max_post_name_length;
    int workload_id;
    int y;

    int ret, j;
    long patt;
    int chosen_fs = 0;
    double res;
    long u, delay_count;

    int b, maxiosize;
    int retry_count = 0;
    double op_time_ms = 0.0;
    unsigned int delay = 0;
    void *context = NULL;

    struct netmist_vfs_dir *rootdir;

    /* Client:
     * Need to have num_work_obj set before calling import_dist_table.
     */
    num_work_obj = g_cc->c_workload_count;

    log_file (LOG_DEBUG, "Client has %d possible workloads\n", num_work_obj);
    /* 
     * Client_localname is already set in do_all_client_work()
     */
    log_file (LOG_EXEC, "Client name %s \n", client_localname);
    if (import_dist_table (g_cc, my_mix_table, num_work_obj) != 0)
    {
	log_file (LOG_ERROR, "Failed to import distribution table!!\n");
	R_exit (BAD_MIX, 0);
    }


    /* Currently the version is coming from netmist_if.h. */
    if (strcmp (g_cc->c_client_version, git_version))
    {
	R_exit (WRONG_VERS, 0);
    }

/*    cmdline.unlink2_no_recreate = g_cc->c_unlink2_no_recreate;*/
/*    cmdline.desired_mb_per_sec = g_cc->c_desired_mb_per_sec; */

    cmdline.do_validate = g_cc->c_do_validate;

    cmdline.flush_flag = g_cc->c_flush_flag;
    cmdline.cleanup_flag = g_cc->c_cleanup;
    cmdline.dump_files_flag = g_cc->c_dump_files_flag;
    cmdline.ipv6_enable = g_cc->c_ipv6_enable;
    cmdline.heartbeat_flag = g_cc->c_heartbeat;

    log_file (LOG_DEBUG,
	      "Values passed via the client_command came from the XML file.\n");
    log_file (LOG_DEBUG,
	      "These values have been replaced by the YAML values, passed via the standard import_dist_tables function.\n");
    log_file (LOG_DEBUG,
	      "Ignoring client_dirs, and client_files, from the client command.\n");

    num_clients = g_cc->c_num_clients;
    cmdline.fd_caching_limit = g_cc->c_fd_caching_limit;
    cmdline.Op_lat_flag = g_cc->c_op_lat_flag;
    cmdline.gg_offset = g_cc->c_gg_offset;
    cmdline.gg_flag = g_cc->c_gg_flag;
    cmdline.pdsm_mode = g_cc->c_pdsm_mode;
    cmdline.pdsm_interval = g_cc->c_pdsm_interval;
    cmdline.workload_count = g_cc->c_workload_count;
    strcpy (cmdline.unix_pdsm_file, g_cc->c_unix_pdsm_file);
    strcpy (cmdline.unix_pdsm_control_file, g_cc->c_unix_pdsm_control_file);
    strcpy (cmdline.windows_pdsm_file, g_cc->c_windows_pdsm_file);
    strcpy (cmdline.windows_pdsm_control_file,
	    g_cc->c_windows_pdsm_control_file);

    Netmist_Ops =
	(struct method_table *) my_malloc (sizeof (struct method_table) *
					   num_work_obj);

    /* allocate for client */
    work_obj =
	(struct work_object *) my_malloc (sizeof (struct work_object) *
					  num_work_obj);

    log_file (LOG_DEBUG, "Client has %d possible workloads\n",
	      cmdline.workload_count);
    log_file (LOG_DEBUG, "Client has pdsm_mode %d\n", cmdline.pdsm_mode);
    log_file (LOG_DEBUG, "Client has pdsm_interval %d\n",
	      cmdline.pdsm_interval);

    if (cmdline.heartbeat_flag)
    {
	log_file (LOG_EXEC, "Client will send heartbeat\n");
    }

    if (cmdline.cleanup_flag)
    {
	log_file (LOG_EXEC, "Client will do cleanup\n");
    }

    internal_cfg.u.client_cfg.workload_id = -1;

    /*
     * cmdline.my_tmp_workload_name was set by the -W command line option. This is the 
     * workload component that the SfsManager told this client to run.
     */
    trim_leading_spaces (cmdline.my_tmp_workload_name);

    /* Setup the workload name to my_workload relationship */
    for (y = 0; y < num_work_obj; y++)
    {
	log_file (LOG_EXEC_VERBOSE, "Comparing to %d - %s\n", y,
		  my_mix_table[y].workload_name);

	if (my_strcasecmp (cmdline.my_tmp_workload_name,
			   my_mix_table[y].workload_name) == 0)
	{
	    internal_cfg.u.client_cfg.workload_id = y;
	    break;
	}
    }

    /* 
     * The value of 'y' is the workload component index for the desired workload to run.
     */
    if (internal_cfg.u.client_cfg.workload_id == -1)
    {
	log_file (LOG_ERROR, "Child got invalid workload name: %s\n",
		  cmdline.my_tmp_workload_name);

	R_exit (BAD_WORKLOAD, 0);
    }

    if (my_mix_table[y].rel_version != netmist_engine_major_version)
    {
	log_file (LOG_ERROR,
		  "Error: Child mismatch workload version = %d Netmist Engine verison = %d.\n",
		  my_mix_table[y].rel_version, netmist_engine_major_version);
	R_exit (BAD_WORKLOAD, 0);
    }
    log_file (LOG_EXEC, "Workload definition's platform type = %s\n",
	      my_mix_table[y].platform_type);
#ifdef WIN32
    log_file (LOG_EXEC, "Client OS type = %s \n", "Windows variant");
#else
    log_file (LOG_EXEC, "Client OS type = %s \n", "Unix variant");
#endif
    min_pre_name_length = my_mix_table[y].min_pre_name_length;
    max_pre_name_length = my_mix_table[y].max_pre_name_length;
    min_post_name_length = my_mix_table[y].min_post_name_length;
    max_post_name_length = my_mix_table[y].max_post_name_length;

    /* 
     * Ignore the benchmarks.xml values for dir_count, files_per_dir, file_size, op_rate ...
     * Ignore command line options for these.
     * Use the values from the yaml file and not the benchmarks.xml file, or command line options.
     * These attributes are now associated with the workload component.
     * These are no longer passed via a command line argument, but instead uses the normal 
     * export/import method for the workload attributes.
     */
    log_file (LOG_DEBUG, "YAML:Workload: %s\n",
	      my_mix_table[y].workload_name);
    log_file (LOG_DEBUG,
	      "YAML:Override: dir_count %d, now from mix_table instead of client_command.\n",
	      my_mix_table[y].dir_count);
    log_file (LOG_DEBUG,
	      "YAML:Override: files_per_dir %d, now from mix_table instead of client_command.\n",
	      my_mix_table[y].files_per_dir);
    log_file (LOG_DEBUG,
	      "YAML:Override: file_size %d, now from mix_table instead of command line.\n",
	      my_mix_table[y].file_size);
    log_file (LOG_DEBUG,
	      "YAML:Override: op_rate %5.1f, now from mix_table instead of command line.\n",
	      my_mix_table[y].op_rate);
    log_file (LOG_DEBUG,
	      "YAML:Override: run_time %d, now from mix_table instead of command line.\n",
	      cmdline.run_time);
    log_file (LOG_DEBUG,
	      "YAML:Override: warm_time %d, now from mix_table instead of command line.\n",
	      my_mix_table[y].warm_time);
    log_file (LOG_DEBUG,
	      "YAML:Override: sharemode %d, now from mix_table instead of command line.\n",
	      my_mix_table[y].sharemode);
    cmdline.client_files = my_mix_table[y].files_per_dir;
    cmdline.client_dirs = my_mix_table[y].dir_count;
    cmdline.fsize = my_mix_table[y].file_size;

    cmdline.op_rate = my_mix_table[y].op_rate;
    cmdline.warm_time = (int) my_mix_table[y].warm_time;
    cmdline.sharing_flag = (int) my_mix_table[y].sharemode;

    /* MUST be called before calling init_space */
    workload_id = internal_cfg.u.client_cfg.workload_id;
    my_workload = workload_id;

#if defined(WIN32)
    /* WINDOWS HERE: 
     * There is no hard-set limit for max open local files, and no max 
     * processes on Windows. MSDN blog indicates a limit on network open 
     * files to be 16384 per session.  We will default to WIN32_MAX_OPEN
     * and let the user over-ride by using cache limit option (-J).
     *
     * Re-initialize here, after my_workload has been established.
     * Only needed for Windws as it changed based on version of benchmark 
     */
    if ((IS_SP1_WL (my_workload)) || IS_SP2_WL (my_workload))
	max_open = WIN32_MAX_OPEN;

    if (IS_SFS2020_WL (my_workload))
	max_open = WIN32_MAX_OPEN_SFS2020;
#endif

    /* Re-initialize here, after my_workload has been established. */
    cmdline.fsize = my_mix_table[my_workload].file_size;

    init_space ();

    my_strncpy (work_obj[workload_id].work_obj_name,
		cmdline.my_tmp_workload_name, MAXWLNAME);

    /* This is where we load up the indirect function table. These functions 
     * implement all of the POSIX-ish, or other, types of calls that
     * may be filesystem type dependent.
     *
     * If the fs_type is POSIX, then use the POSIX methods.
     * If it is some other type, please add it here.
     * If the type is garbage, then this will default to POSIX.
     */

    if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "POSIX")) == 0)
    {
	/* The POSIX versions of these virtual functions */
	Netmist_Ops[my_workload].read = generic_read;
	Netmist_Ops[my_workload].read_file = generic_read_file;
	Netmist_Ops[my_workload].mmap_read = generic_mmap_read;
	Netmist_Ops[my_workload].read_rand = generic_read_rand;
	Netmist_Ops[my_workload].write = generic_write;
	Netmist_Ops[my_workload].write_file = generic_write_file;
	Netmist_Ops[my_workload].mmap_write = generic_mmap_write;
	Netmist_Ops[my_workload].write_rand = generic_write_rand;
	Netmist_Ops[my_workload].rmw = generic_rmw;
	Netmist_Ops[my_workload].mkdir = generic_mkdir;
	Netmist_Ops[my_workload].rmdir = generic_rmdir;
	Netmist_Ops[my_workload].unlink = generic_unlink;
	Netmist_Ops[my_workload].unlink2 = generic_unlink2;
	Netmist_Ops[my_workload].create = generic_create;
	Netmist_Ops[my_workload].append = generic_append;
	Netmist_Ops[my_workload].locking = generic_locking;
	Netmist_Ops[my_workload].access = generic_access;
	Netmist_Ops[my_workload].stat = generic_stat;
	Netmist_Ops[my_workload].neg_stat = generic_neg_stat;
	Netmist_Ops[my_workload].chmod = generic_chmod;
	Netmist_Ops[my_workload].readdir = generic_readdir;
	Netmist_Ops[my_workload].copyfile = generic_copyfile;
	Netmist_Ops[my_workload].rename = generic_rename;
	Netmist_Ops[my_workload].statfs = generic_statfs;
	Netmist_Ops[my_workload].pathconf = generic_pathconf;
	Netmist_Ops[my_workload].trunc = generic_trunc;
	Netmist_Ops[my_workload].custom1 = generic_custom1;
	Netmist_Ops[my_workload].custom2 = generic_custom2;
	Netmist_Ops[my_workload].makepath = generic_make_path;
	Netmist_Ops[my_workload].prefix_op = generic_prefix_op;
	Netmist_Ops[my_workload].postfix_op = generic_postfix_op;
	Netmist_Ops[my_workload].fill_buf = generic_fill_buf;
	Netmist_Ops[my_workload].init_dir = generic_init_dir;
	Netmist_Ops[my_workload].init_file = generic_init_file;
	Netmist_Ops[my_workload].init_empty_file = generic_init_empty_file;
	Netmist_Ops[my_workload].stat_workdir = generic_stat_workdir;
	Netmist_Ops[my_workload].remove_file = generic_remove_file;
	Netmist_Ops[my_workload].remove_dir = generic_remove_dir;
	Netmist_Ops[my_workload].setup = generic_setup;
	Netmist_Ops[my_workload].teardown = generic_teardown;
	Netmist_Ops[my_workload].impersonate = generic_impersonate;
	Netmist_Ops[my_workload].make_impersonate_context =
	    generic_make_impersonate_context;
	chosen_fs = 1;
    }
    if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "BLOCK")) == 0)
    {
	/* The POSIX versions of these virtual functions */
	Netmist_Ops[my_workload].read = generic_bread;
	Netmist_Ops[my_workload].read_file = generic_empty_fileop;
	Netmist_Ops[my_workload].mmap_read = generic_empty_fileop;
	Netmist_Ops[my_workload].read_rand = generic_bread_rand;
	Netmist_Ops[my_workload].write = generic_bwrite;
	Netmist_Ops[my_workload].write_file = generic_empty_fileop;
	Netmist_Ops[my_workload].mmap_write = generic_empty_fileop;
	Netmist_Ops[my_workload].write_rand = generic_bwrite_rand;
	Netmist_Ops[my_workload].rmw = generic_empty_fileop;
	Netmist_Ops[my_workload].mkdir = generic_empty_fileop;
	Netmist_Ops[my_workload].rmdir = generic_empty_fileop;
	Netmist_Ops[my_workload].unlink = generic_empty_fileop_int;
	Netmist_Ops[my_workload].unlink2 = generic_empty_fileop_int;
	Netmist_Ops[my_workload].create = generic_empty_fileop_int;
	Netmist_Ops[my_workload].append = generic_empty_fileop;
	Netmist_Ops[my_workload].locking = generic_empty_fileop;
	Netmist_Ops[my_workload].access = generic_empty_fileop;
	Netmist_Ops[my_workload].stat = generic_empty_fileop;
	Netmist_Ops[my_workload].chmod = generic_empty_fileop;
	Netmist_Ops[my_workload].readdir = generic_empty_fileop;
	Netmist_Ops[my_workload].copyfile = generic_empty_fileop;
	Netmist_Ops[my_workload].rename = generic_empty_fileop;
	Netmist_Ops[my_workload].statfs = generic_empty_fileop;
	Netmist_Ops[my_workload].pathconf = generic_empty_fileop;
	Netmist_Ops[my_workload].trunc = generic_empty_fileop_int;
	Netmist_Ops[my_workload].custom1 = generic_empty_fileop;
	Netmist_Ops[my_workload].custom2 = generic_empty_fileop;
	Netmist_Ops[my_workload].makepath = generic_bmake_path;
	Netmist_Ops[my_workload].prefix_op = generic_empty_int_fileop;
	Netmist_Ops[my_workload].postfix_op = generic_empty_int_fileop;
	Netmist_Ops[my_workload].fill_buf = generic_fill_buf;
	Netmist_Ops[my_workload].init_dir = generic_iempty_dir;
	Netmist_Ops[my_workload].init_file = generic_binit_file;
	Netmist_Ops[my_workload].init_empty_file = generic_iempty_dir_s;
	Netmist_Ops[my_workload].stat_workdir = generic_stat_workdir;
	Netmist_Ops[my_workload].remove_file = generic_iempty_dir_s;
	Netmist_Ops[my_workload].remove_dir = generic_iempty_dir;
	Netmist_Ops[my_workload].setup = generic_bsetup;
	Netmist_Ops[my_workload].teardown = generic_bteardown;
	Netmist_Ops[my_workload].impersonate = generic_impersonate;
	Netmist_Ops[my_workload].make_impersonate_context =
	    generic_make_impersonate_context;
	chosen_fs = 1;
    }
    else if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "WINDOWS")) ==
	     0)
    {
	chosen_fs = 1;
    }
    /* Then default to POSIX */
    if (chosen_fs == 0)
    {
	/* The POSIX versions of these virtual functions */
	Netmist_Ops[my_workload].read = generic_read;
	Netmist_Ops[my_workload].read_file = generic_read_file;
	Netmist_Ops[my_workload].mmap_read = generic_mmap_read;
	Netmist_Ops[my_workload].read_rand = generic_read_rand;
	Netmist_Ops[my_workload].write = generic_write;
	Netmist_Ops[my_workload].write_file = generic_write_file;
	Netmist_Ops[my_workload].mmap_write = generic_mmap_write;
	Netmist_Ops[my_workload].write_rand = generic_write_rand;
	Netmist_Ops[my_workload].rmw = generic_rmw;
	Netmist_Ops[my_workload].mkdir = generic_mkdir;
	Netmist_Ops[my_workload].rmdir = generic_rmdir;
	Netmist_Ops[my_workload].unlink = generic_unlink;
	Netmist_Ops[my_workload].unlink2 = generic_unlink2;
	Netmist_Ops[my_workload].create = generic_create;
	Netmist_Ops[my_workload].append = generic_append;
	Netmist_Ops[my_workload].locking = generic_locking;
	Netmist_Ops[my_workload].access = generic_access;
	Netmist_Ops[my_workload].stat = generic_stat;
	Netmist_Ops[my_workload].neg_stat = generic_neg_stat;
	Netmist_Ops[my_workload].chmod = generic_chmod;
	Netmist_Ops[my_workload].readdir = generic_readdir;
	Netmist_Ops[my_workload].copyfile = generic_copyfile;
	Netmist_Ops[my_workload].rename = generic_rename;
	Netmist_Ops[my_workload].statfs = generic_statfs;
	Netmist_Ops[my_workload].pathconf = generic_pathconf;
	Netmist_Ops[my_workload].trunc = generic_trunc;
	Netmist_Ops[my_workload].custom1 = generic_custom1;
	Netmist_Ops[my_workload].custom2 = generic_custom2;
	Netmist_Ops[my_workload].makepath = generic_make_path;
	Netmist_Ops[my_workload].prefix_op = generic_prefix_op;
	Netmist_Ops[my_workload].postfix_op = generic_postfix_op;
	Netmist_Ops[my_workload].fill_buf = generic_fill_buf;
	Netmist_Ops[my_workload].init_dir = generic_init_dir;
	Netmist_Ops[my_workload].init_file = generic_init_file;
	Netmist_Ops[my_workload].init_empty_file = generic_init_empty_file;
	Netmist_Ops[my_workload].stat_workdir = generic_stat_workdir;
	Netmist_Ops[my_workload].remove_file = generic_remove_file;
	Netmist_Ops[my_workload].remove_dir = generic_remove_dir;
	Netmist_Ops[my_workload].setup = generic_setup;
	Netmist_Ops[my_workload].teardown = generic_teardown;
	Netmist_Ops[my_workload].impersonate = generic_impersonate;
	Netmist_Ops[my_workload].make_impersonate_context =
	    generic_make_impersonate_context;
	chosen_fs = 1;
	log_file (LOG_EXEC_VERBOSE,
		  "Filesystem type %s not supported. Defaulting to POSIX\n",
		  my_mix_table[my_workload].fs_type);
    }

    /* we impersonate user correctly before accessing files */
    log_file (LOG_DEBUG, "Child impersonate.!!\n");

    context = (*Netmist_Ops[my_workload].make_impersonate_context) ();
    ret = (*Netmist_Ops[my_workload].impersonate) (context);
    if (ret != 0)
    {
	R_exit (LOGON_FAILURE, ret);
    }

    trim_leading_spaces (cmdline.workdir);

    {
	struct netmist_vfs_init_ret vfsret = {
	    .vermagic = NETMIST_VFS_VERMAGIC_0,
	    .rock = NULL,
	    .vtable_v0 = NULL
	};
	netmist_vfs_init_t initfun = NULL;
	char *vfs_argv[] = { "VFS", "-R", cmdline.workdir, NULL, NULL };
	int vfs_argc = 3;

	/* Pass through a positional argument for VFS's use */
	if (cmdline.vfs_arg[0] != '\0')
	{
	    vfs_argv[vfs_argc++] = cmdline.vfs_arg;
	}

	if (cmdline.vfs_so[0] == '\0')
	{
#if defined(WIN32)
	    initfun = netmist_win32_vfs_init;
#else
	    initfun = netmist_posix_vfs_init;
#endif
	}
	else
	{
	    log_file (LOG_EXEC, "Loading dynamic shared lib: %s\n",
		      cmdline.vfs_so);

#if !defined(WIN32)
	    vfs_so_handle = dlopen (cmdline.vfs_so, RTLD_LOCAL | RTLD_NOW);
	    if (vfs_so_handle == NULL)
	    {
		log_file (LOG_ERROR,
			  "Cannot find VFS shared object '%s': %s\n",
			  cmdline.vfs_so, dlerror ());
		R_exit (NETMIST_NO_VFS, 0);
	    }

	    initfun =
		(netmist_vfs_init_t) dlsym (vfs_so_handle,
					    "netmist_vfs_init");
#endif
	    if (initfun == NULL)
	    {
		log_file (LOG_ERROR, "Cannot call VFS shared object %s\n",
			  cmdline.vfs_so);
		R_exit (NETMIST_NO_VFS, 1);
	    }
	}
	ret = initfun (vfs_argc, vfs_argv, &vfsret);
	if (ret != 0)
	{
	    log_file (LOG_ERROR,
		      "Unable to initialize VFS layer (error %d; workdir '%s')\n",
		      ret, cmdline.workdir);
	    R_exit (NETMIST_VFS_ERR, ret);
	}
	vfsr = vfsret.rock;
	switch (vfsret.vermagic)
	{
	case NETMIST_VFS_VERMAGIC_0:
	    vfst = vfsret.vtable_v0;
	    break;
	default:
	    if (vfsret.vtable_v0 == NULL)
	    {
		log_file (LOG_ERROR,
			  "VFS layer reports unknown version magic"
			  " (netmist: %" PRIxPTR " != plugin: %" PRIxPTR ")"
			  " and has no interface I understand.\n",
			  NETMIST_VFS_VERMAGIC_0, vfsret.vermagic);
		R_exit (NETMIST_VFS_ERR, 0);
	    }
	    log_file (LOG_ERROR,
		      "VFS layer reports unknown version magic"
		      " (netmist: %" PRIxPTR " != plugin: %" PRIxPTR ")"
		      " but still speaks in ways I understand.\n",
		      NETMIST_VFS_VERMAGIC_0, vfsret.vermagic);
	    vfst = vfsret.vtable_v0;
	}
    }
    if (vfst == NULL)
    {
	log_file (LOG_ERROR,
		  "Treachery!  VFS plugin initialized successfully, but did not"
		  " provide a virtual dispatch table!\n");
	R_exit (NETMIST_VFS_ERR, 0);
    }

    log_file (LOG_DEBUG, "Child statdir.!!\n");

    /*
     * Stat the cmdline.workdir. If it doesn't exist... exit.
     * Use virtualized API for portability across file system types.
     * Try a few times, as some systems stumble at first. 
     */
  tryit:
#if defined(WIN32)
    /* 
     * The \\.\PhysicalDriveX does not actually live in the namespace 
     * such that "stat" can examine it on Windows.
     */
    if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "BLOCK")) == 0)
    {
	block_mode = 1;
	goto jumpout;
    }

#endif

    rootdir = netmist_vfs_root ();

    ret = (*Netmist_Ops[my_workload].stat_workdir) (rootdir);
    if (ret != 0)
    {
	if (retry_count++ < 10)
	{
	    sleep (1);
	    /* Try to create it */
	    (*Netmist_Ops[my_workload].init_dir) (rootdir);
	    goto tryit;
	}
	R_exit (NO_WORKDIR, ret);
    }

    VFS (dfree, &rootdir);

    if (bad_clock)
    {
	R_exit (BAD_CLOCK, 0);
    }
#if defined(WIN32)
  jumpout:
#endif

    if (cmdline.fd_caching_limit != 0)
    {
#if defined(WIN32)
	/* Windows doesn't have a real limit, so let folks set one */
	if (cmdline.fd_caching_limit < WIN32_MAX_SESS_OPEN)
	    max_open = cmdline.fd_caching_limit;
#else
	if (cmdline.fd_caching_limit < max_open)
	    max_open = cmdline.fd_caching_limit;
#endif

    }

    if (cmdline.op_rate != 0.0)
    {
	/*calculate average time for each op */
	op_time_ms = 1000 / cmdline.op_rate;
	if (op_time_ms >= 2.0)
	{
	    /*seed by moving client id to most signaficate bits */
	    netmist_srand (reverse_bits (cmdline.client_id) ^ (int)
			   gettime ());
	    delay = netmist_rand () % (int) op_time_ms;

	    log_file (LOG_EXEC_VERBOSE,
		      "Before test starts, the client will sleep for %d ms (max %.2f ms)\n",
		      delay, op_time_ms);
	}

	log_file (LOG_EXEC,
		  "Client %d: My_workload id %d Name: %s Op_rate = %.2f SP_version = %d\n",
		  cmdline.client_id, my_workload,
		  my_mix_table[my_workload].workload_name, cmdline.op_rate,
		  my_mix_table[my_workload].rel_version);
    }
    else
    {
	log_file (LOG_EXEC, "Client %d: My_workload id %d Name: %s\n",
		  cmdline.client_id, my_workload,
		  my_mix_table[my_workload].workload_name);
    }

    /*
     * Set the LRU policy from the workload.
     */
    lru_on = my_mix_table[my_workload].lru_on;

    log_file (LOG_DEBUG, "File descriptor policy: LRU recycle %d.\n", lru_on);

    /*
     * Impersonate is used to have all files be shared by 
     * all procs, on all clients. By using impersonate all 
     * of the files appear to be created, and accessed, by
     * a single client ID. (client id = 0)
     */
    if ((!cmdline.sharing_flag) && my_mix_table[my_workload].sharemode)
	cmdline.fsize = cmdline.fsize * (num_clients);	/* Re-scale */

    if (my_mix_table[my_workload].sharemode)
    {
	log_file (LOG_EXEC, "Using share mode \n");
	cmdline.sharing_flag = 1;
    }

    if (cmdline.sharing_flag)
	impersonate = 0;
    else
	impersonate = cmdline.client_id;
    /*
     * Get buffers ready to go;
     */
    netmist_srand (INIT_SEED + impersonate);
#ifdef RANDOMIZE_ACROSS_LOAD_POINTS
    init_genrand64 ((unsigned long long) time (NULL) +
		    (unsigned long long) impersonate);
#else
    init_genrand64 ((unsigned long long) (INIT_SEED + impersonate));
#endif

    patt = netmist_rand () & 0x7fffffff;

    maxiosize = rsize;		/* Default 4 K buffer */

    /*
     * find the largest xfer size needed 
     */
    for (b = 0; b < MAX_DIST_ELEM; b++)
    {
	if (my_mix_table[my_workload].read_dist[b].percent > 0)
	{
	    if (my_mix_table[my_workload].read_dist[b].size_max >
		(unsigned int) maxiosize)
		maxiosize = my_mix_table[my_workload].read_dist[b].size_max;
	}
	if (my_mix_table[my_workload].write_dist[b].percent > 0)
	{
	    if (my_mix_table[my_workload].write_dist[b].size_max >
		(unsigned int) maxiosize)
		maxiosize = my_mix_table[my_workload].write_dist[b].size_max;
	}
    }
    max_xfer_size = maxiosize;	/* Push to global here */
    /* Allocate buffer, and align for O_DIRECT */
    main_buf = (char *) MALLOC (maxiosize + 4096);
    memset ((void *) main_buf, 0, (size_t) (maxiosize + 4096));
    main_buf = (char *) (((intptr_t) main_buf + 4096) & ~4095);
    log_file (LOG_EXEC, "Max io size is %d\n", maxiosize);


    (*Netmist_Ops[my_workload].fill_buf) (impersonate, patt, maxiosize,
					  main_buf);

    /*
     * Determine the timer resolution.
     */
    my_time_res = 999999.99;
    for (j = 0; j < 2; j++)
    {
	my_start_time = gettime ();	/* Prime path */
	sleep (1);
    }
    delay_count = 1;

    for (j = 0; j < 10; j++)
    {
      over:
	my_start_time = gettime ();	/* Prime path */
	for (u = 0; u < delay_count; u++)
	    ;
	my_start_time2 = gettime ();	/* Prime path */
	if (my_start_time2 - my_start_time <= 0.0)
	{
	    delay_count++;
	    goto over;
	}
	res = my_start_time2 - my_start_time;
	if (res < 0.0)
	    goto over;		/* Yeah, in VMs */
	if (res < my_time_res)
	    my_time_res = res;
    }
    log_file (LOG_EXEC, "Max open files per process %d\n", max_open);
    log_file (LOG_EXEC, "Max PROCs %d\n", max_proc);
    log_file (LOG_EXEC, "Timer resolution %12.9f seconds.\n", my_time_res);

    nap (1);			/* Prime instruction and data caches, and clear page faults */
    my_start_time = my_start_time2;
    nap (1);			/* Now take the meausrement */
    my_start_time2 = gettime ();
    log_file (LOG_EXEC, "Nap resolution %12.9f seconds.\n",
	      my_start_time2 - my_start_time);


    /*
     * Randomize the seed so that each client will perform slightly 
     * different selections of file ops.
     */
    netmist_srand (impersonate + 1);
    /*
     * Initialize internal data structures..
     */
    init_tests (test_ops);

    /*
     * Initialize internal data structures..
     */
    init_dist_tables ();

    log_file (LOG_EXEC_VERBOSE,
	      "Client %d performing %d ops. File size %lld kbytes, "
	      "Record size %ld kbytes.\n",
	      cmdline.client_id, number_ops, cmdline.fsize, avg_read_size);


    /*
     * Validate that all of the Op types, will actually work on the client.
     */
    if ((my_strcasecmp (my_mix_table[my_workload].fs_type, "BLOCK")) == 0)
	cmdline.do_validate = 0;

    if (cmdline.do_validate)
    {
	log_file (LOG_EXEC, "Starting op validation\n");
	client_validate_ops ();
	log_file (LOG_EXEC, "Finished op validation\n");
    }
    else
    {
	log_file (LOG_EXEC, "op validation skipped!\n");
    }

    clear_stats ();
}


/**
 * @brief Allocate the space for the internal test array, 
 *        then fill it in according to the op distribution mix.
 *
 * @param num_test_ops : Number of test ops
 */
/*
 * __doc__
 * __doc__  Function : void init_tests(int number_of_test_ops)
 * __doc__  Arguments: int number_of_test_ops
 * __doc__  Performs : Allocate the space for the internal test array, 
 * __doc__             then fill it in according to the op distribution mix.
 * __doc__
 */
void
init_tests (int num_test_ops)
{
    tests = (struct test *) MALLOC (sizeof (struct test) * num_test_ops);
    if (tests == 0)
    {
	log_file (LOG_ERROR, "Malloc failed.\n");
	exit (1);
    }
    memset ((void *) tests, 0,
	    (size_t) (sizeof (struct test) * num_test_ops));
    create_tests (num_test_ops);
}

/**
 * @brief Initialize the test array. There are "num_test_ops" 
 *        entries in the array and this code distributes the 
 *        percentages of each type of test across the array. 
 */
/*
 * __doc__
 * __doc__  Function : void create_tests(int num_test_ops)
 * __doc__  Arguments: int number of test ops.
 * __doc__  Performs : Initialize the test array. There are "num_test_ops" 
 * __doc__             entries in the array and this code distributes the 
 * __doc__             percentages of each type of test across the array. 
 * __note__          : Note: The value of my_workload is already set by now !.
 * __doc__
 */
void
create_tests (int num_test_ops)
{
    int x, j;
    int k;
    struct test *tp;

    log_file (LOG_DEBUG, "Init %d test ops\n", num_test_ops);

    k = 0;
    /*
     * Initialize "writes" in the test array.
     * x=(num_test_ops*PERCENT_WRITE)/100;  
     */
    x = (num_test_ops * my_mix_table[my_workload].percent_write) / 100;

    log_file (LOG_DEBUG, "Init %d write test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_WRITE;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "write file ops" in the test array.
     */
    x = (num_test_ops * my_mix_table[my_workload].percent_write_file) / 100;

    log_file (LOG_DEBUG, "Init %d write file test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_WRITE_FILE;
	tp->status_flags = NONE;
    }

    /*
     * Initialize "mmap writes" in the test array.
     * x=(num_test_ops*PERCENT_MM_WRITE)/100;       
     */
    x = (num_test_ops * my_mix_table[my_workload].percent_mmap_write) / 100;

    log_file (LOG_DEBUG, "Init %d mmap write test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_MM_WRITE;
	tp->status_flags = NONE;
    }

    /*
     * Initialize "write rand" in the test array.
     * x=(num_test_ops*PERCENT_WRITE_RAND)/100;     
     */
    x = (num_test_ops * my_mix_table[my_workload].percent_write_rand) / 100;

    log_file (LOG_DEBUG, "Init %d write rand test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_WRITE_RAND;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "reads" in the test array.
     */
    x = (num_test_ops * PERCENT_READ) / 100;

    log_file (LOG_DEBUG, "Init %d read test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_READ;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "read file ops" in the test array.
     */
    x = (num_test_ops * PERCENT_READ_FILE) / 100;

    log_file (LOG_DEBUG, "Init %d read file test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_READ_FILE;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "mmap reads" in the test array.
     * x=(num_test_ops*PERCENT_MM_READ)/100;        
     */
    x = (num_test_ops * my_mix_table[my_workload].percent_mmap_read) / 100;

    log_file (LOG_DEBUG, "Init %d mmap read test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_MM_READ;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "read rand" in the test array.
     */
    x = (num_test_ops * PERCENT_READ_RAND) / 100;

    log_file (LOG_DEBUG, "Init %d read rand test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_READ_RAND;
	tp->status_flags = NONE;
    }

    /*
     * Initialize "rmw" in the test array.
     */
    x = (num_test_ops * PERCENT_RMW) / 100;

    log_file (LOG_DEBUG, "Init %d rmw test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_RMW;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "mkdir" in the test array.
     */
    x = (num_test_ops * PERCENT_MKDIR) / 100;

    log_file (LOG_DEBUG, "Init %d mkdir test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_MKDIR;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "rmdir" in the test array.
     */
    x = (num_test_ops * PERCENT_RMDIR) / 100;

    log_file (LOG_DEBUG, "Init %d rmdir test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_RMDIR;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "unlink" in the test array.
     */
    x = (num_test_ops * PERCENT_UNLINK) / 100;

    log_file (LOG_DEBUG, "Init %d unlink test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_UNLINK;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "unlink2" in the test array.
     */
    x = (num_test_ops * PERCENT_UNLINK2) / 100;

    log_file (LOG_DEBUG, "Init %d unlink2 test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_UNLINK2;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "Create" in the test array.
     */
    x = (num_test_ops * PERCENT_CREATE) / 100;

    log_file (LOG_DEBUG, "Init %d create test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_CREATE;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "stat" in the test array.
     */
    x = (num_test_ops * PERCENT_STAT) / 100;

    log_file (LOG_DEBUG, "Init %d stat test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_STAT;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "neg_stat" in the test array.
     */
    x = (num_test_ops * PERCENT_NEG_STAT) / 100;

    log_file (LOG_DEBUG, "Init %d neg_stat test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_NEG_STAT;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "append" in the test array.
     */
    x = (num_test_ops * PERCENT_APPEND) / 100;

    log_file (LOG_DEBUG, "Init %d append test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_APPEND;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "locking" in the test array.
     */
    x = (num_test_ops * PERCENT_LOCKING) / 100;

    log_file (LOG_DEBUG, "Init %d locking test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_LOCKING;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "access" in the test array.
     */
    x = (num_test_ops * PERCENT_ACCESS) / 100;

    log_file (LOG_DEBUG, "Init %d access test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_ACCESS;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "chmod" in the test array.
     */
    x = (num_test_ops * PERCENT_CHMOD) / 100;

    log_file (LOG_DEBUG, "Init %d chmod test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_CHMOD;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "readdir" in the test array.
     */
    x = (num_test_ops * PERCENT_READDIR) / 100;

    log_file (LOG_DEBUG, "Init %d readdir test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_READDIR;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "copyfile" in the test array.
     */
    x = (num_test_ops * PERCENT_COPYFILE) / 100;

    log_file (LOG_DEBUG, "Init %d copyfile test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_COPYFILE;
	tp->status_flags = NONE;
    }

    /*
     * Initialize "Rename" in the test array.
     */
    x = (num_test_ops * PERCENT_RENAME) / 100;

    log_file (LOG_DEBUG, "Init %d rename test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_RENAME;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "statfs" in the test array.
     */
    x = (num_test_ops * PERCENT_STATFS) / 100;

    log_file (LOG_DEBUG, "Init %d statfs test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_STATFS;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "pathconf" in the test array.
     */
    x = (num_test_ops * PERCENT_PATHCONF) / 100;

    log_file (LOG_DEBUG, "Init %d pathconf test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_PATHCONF;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "trunc" in the test array.
     */
    x = (num_test_ops * PERCENT_TRUNC) / 100;

    log_file (LOG_DEBUG, "Init %d trunc test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_TRUNC;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "custom1" in the test array.
     */
    x = (num_test_ops * PERCENT_CUSTOM1) / 100;

    log_file (LOG_DEBUG, "Init %d custom1 test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_CUSTOM1;
	tp->status_flags = NONE;
    }
    /*
     * Initialize "custom2" in the test array.
     */
    x = (num_test_ops * PERCENT_CUSTOM2) / 100;

    log_file (LOG_DEBUG, "Init %d custom2 test ops\n", x);

    for (j = 0; (j < x && k < num_test_ops); j++)
    {
	tp = &tests[k++];
	tp->op_type = OP_CUSTOM2;
	tp->status_flags = NONE;
    }

    log_file (LOG_DEBUG, "Initialized %d tests\n", k);
}

/**
 * @brief This is the call that the client makes to pick its 
 *        next operation to perform. It will pick a random 
 *        entry from the test array and perform that operation.
 *
 * @param num_test_ops : Is the size of the test array.
 * @param client_id : Client ID
 */
/*
 * __doc__
 * __doc__  Function : int gen_op(int num_test_ops, int client_id)
 * __doc__  Arguments: int num_test_ops, int client_id
 * __doc__             num_test_ops: is the size of the array.
 * __doc__             
 * __doc__  Returns  : Int.  Success or failure.
 * __doc__  Performs : This is the call that the client makes to pick its 
 * __doc__             next operation to perform. It will pick a random 
 * __doc__             entry from the test array and perform that operation.
 * __note__          : Some operations have work that needs to be done before
 * __note__            the operation can be performed, others need some work
 * __note__            to be done after an operation has been performed. 
 * __note__            The generic_prefix_op() and generic_postfix_op() handle
 * __note__            this work.
 * __doc__
 */
int
gen_op (int num_test_ops, int client_id)
{
    int x;
    struct test *tp;
    struct file_object *fp;

    /* 
     * Pick a random operation to perform from the test array.
     */
    x = ((unsigned int) netmist_rand ()) % num_test_ops;
    tp = &tests[x];

    log_file (LOG_DEBUG, "Client picks test number %d OP = %d\n", x,
	      tp->op_type);

    switch (tp->op_type)
    {
    case OP_WRITE:
	/* Remove the file first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_WRITE_FILE:
	/* Remove the file first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write_file) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_MM_WRITE:
	/* Remove the file first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mmap_write) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_MM_READ:
	/* Remove the file first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mmap_read) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_WRITE_RAND:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write_rand) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_READ:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_READ_FILE:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read_file) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_READ_RAND:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read_rand) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_RMW:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].rmw) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_MKDIR:
	/* Remove the directory first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mkdir) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_RMDIR:
	/* Create the directory first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].rmdir) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_UNLINK:
	/* Put back after unlink ??? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].unlink) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_UNLINK2:
	/* Put back after unlink ??? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].unlink2) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_CREATE:
	/* Put back after unlink ??? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].create) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_STAT:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].stat) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_NEG_STAT:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].neg_stat) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_CHMOD:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].chmod) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_READDIR:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].readdir) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_ACCESS:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].access) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_LOCKING:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].locking) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_APPEND:
	/* Remove the file first ? */
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].append) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_COPYFILE:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].copyfile) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_RENAME:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].rename) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_STATFS:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].statfs) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_PATHCONF:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].pathconf) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_TRUNC:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].trunc) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_CUSTOM1:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].custom1) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    case OP_CUSTOM2:
	fp = (*Netmist_Ops[my_workload].makepath) (client_id, x);
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].custom2) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	break;

    default:
	log_file (LOG_ERROR, "Unknown Op type \n");

	break;
    }
    return (x);			/* Return index in tests array */
}

/**
 * @brief Create the directory path and file path for this test 
 *        entry for this client, or child of client. Store the 
 *        path in the test array entry associated with this 
 *        operation.
 *
 * @param  client_id : Used for sharing.
 * @param  test_num  : Used to select which type of test.
 */
/*
 * __doc__
 * __doc__  Function : struct file_object * 
 * __doc__             generic_make_path(int client_id, int test_num)
 * __doc__  Arguments: int client_id: Used for sharing.
 * __doc__             int test_num: Used to select which type of test.
 * __doc__  Returns  : Returns a pointer to a file_object.
 * __doc__  Performs : Create the directory path and file path for this test 
 * __doc__             entry for this client, or child of client. Store the 
 * __doc__             path in the test array entry associated with this 
 * __doc__             operation.
 * __doc__
 */
struct file_object *
generic_make_path (int client_id, int test_num)
{
    int dir, filenum, op_type = 0;
    int sel;
    int err;
    struct test *tp;
    char leaf_name[MAXNAME];
    struct file_object *file_ptr = NULL;
    struct file_object *file_array_ptr = NULL;
    int jj = 0;

    /*
     * Pick a type of OP
     */
    tp = &tests[test_num];
    /*
     * Pick a random directory
     */
    dir = netmist_rand () % cmdline.client_dirs;
    /*
     * Pick a random file within this directory.
     */
    /* filenum=netmist_rand()%cmdline.client_files; */

    jj = (int) ((netmist_rand () % 100) + 1);
    if ((my_mix_table[my_workload].percent_geometric) &&
	(jj <= my_mix_table[my_workload].percent_geometric))
	filenum = geometric_rand (cmdline.client_files);	/* jj % of the time */
    else
	filenum = netmist_rand () % cmdline.client_files;	/* remaining % of time */

    /*
     * Pick a file for op type
     */
    if (my_mix_table[my_workload].shared_buckets == 1)
    {
	switch (tp->op_type)
	{
	case OP_WRITE:
	    /* Pick seq read or write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    }
	    break;
	case OP_WRITE_FILE:
	    /* Pick seq read or write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    }
	    break;
	case OP_MM_WRITE:
	    /* Pick seq mmap read or mmap write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    }
	    break;
	case OP_MM_READ:
	    /* Pick seq mmap read or seq mmap write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    }
	    break;
	case OP_READ:
	    /* Pick seq read or seq write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    }
	    break;
	case OP_READ_FILE:
	    /* Pick seq read or seq write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    }
	    break;
	case OP_WRITE_RAND:
	    /* Pick rand read or rand write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    }
	    break;
	case OP_READ_RAND:
	    /* Pick rand read or rand write */
	    sel = netmist_rand () % 2;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    }
	    break;
	case OP_RMW:
	    /* Pick rand read or rand write, or rmw  */
	    sel = netmist_rand () % 3;
	    if (sel == 0)
	    {
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
	    }
	    if (sel == 1)
	    {
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
	    }
	    if (sel == 2)
	    {
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
	    }
	    break;
	case OP_MKDIR:
	    file_array_ptr = op_X_array[OP_MKDIR];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_RMDIR:
	    file_array_ptr = op_X_array[OP_RMDIR];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_UNLINK:
	    file_array_ptr = op_X_array[OP_UNLINK];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_TRUNC:
	    file_array_ptr = op_X_array[OP_TRUNC];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_UNLINK2:
	    /* Over-ride filenum to pick next file in dir */
	    if (my_mix_table[my_workload].unlink2_no_recreate
		&& (phase == RUN_PHASE))
	    {
		if (!one_time_unlink2)
		{
		    unlink2_dir_array =
			(int *) malloc (sizeof (int) * cmdline.client_dirs);
		    log_file (LOG_EXEC, "Number of cmdline.client_dirs %d\n",
			      cmdline.client_dirs);

		    memset (unlink2_dir_array, 0,
			    (sizeof (int) * cmdline.client_dirs));
		    one_time_unlink2++;
		}
		if (unlink2_dir_array[dir] != cmdline.client_files)
		{
		    unlink2_dir_array[dir]++;
		    filenum = unlink2_dir_array[dir];
		}
	    }
	    file_array_ptr = op_X_array[OP_UNLINK2];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_CREATE:
	    file_array_ptr = op_X_array[OP_CREATE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_STAT:
	    /* 
	     * Pick from: write, read, append, rand write, rand read, access, 
	     * append, or lock. Don't share with unlink or rename, as things 
	     * might disappear, in the shared files mode. 
	     */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_NEG_STAT:
	    /* 
	     * Pick from: write, read, append, rand write, rand read, access, 
	     * append, or lock. Don't share with unlink or rename, as things 
	     * might disappear, in the shared files mode. 
	     */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_APPEND:
	    file_array_ptr = op_X_array[OP_APPEND];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_LOCKING:
	    file_array_ptr = op_X_array[OP_LOCKING];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_ACCESS:
	    /* Pick from: write, read, append, rand write, 
	       rand read, access, append, copyfile */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_CHMOD:
	    /* Pick from: write, read, append, rand write, 
	       rand read, access, append, or lock */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_READDIR:
	    /* Pick from: write, read, append, rand write, 
	       rand read, access, append, or lock */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_COPYFILE:
	    file_array_ptr = op_X_array[OP_COPYFILE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_RENAME:
	    file_array_ptr = op_X_array[OP_RENAME];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_STATFS:
	    /* Pick from: write, read, append, rand write, 
	       rand read, access, append, or lock */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_PATHCONF:
	    /* Pick from: write, read, append, rand write, 
	       rand read, access, append, or lock */
	    sel = netmist_rand () % 19;
	    switch (sel)
	    {
	    case 0:
		file_array_ptr = op_X_array[OP_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE;
		break;
	    case 1:
		file_array_ptr = op_X_array[OP_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ;
		break;
	    case 2:
		file_array_ptr = op_X_array[OP_APPEND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_APPEND;
		break;
	    case 3:
		file_array_ptr = op_X_array[OP_WRITE_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_RAND;
		break;
	    case 4:
		file_array_ptr = op_X_array[OP_READ_RAND];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_RAND;
		break;
	    case 5:
		file_array_ptr = op_X_array[OP_ACCESS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_ACCESS;
		break;
	    case 6:
		file_array_ptr = op_X_array[OP_RMW];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_RMW;
		break;
	    case 7:
		file_array_ptr = op_X_array[OP_COPYFILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_COPYFILE;
		break;
	    case 8:
		file_array_ptr = op_X_array[OP_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STAT;
		break;
	    case 9:
		file_array_ptr = op_X_array[OP_CHMOD];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_CHMOD;
		break;
	    case 10:
		file_array_ptr = op_X_array[OP_MM_READ];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_READ;
		break;
	    case 11:
		file_array_ptr = op_X_array[OP_MM_WRITE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_MM_WRITE;
		break;
	    case 12:
		file_array_ptr = op_X_array[OP_LOCKING];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_LOCKING;
		break;
	    case 13:
		file_array_ptr = op_X_array[OP_PATHCONF];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_PATHCONF;
		break;
	    case 14:
		file_array_ptr = op_X_array[OP_STATFS];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_STATFS;
		break;
	    case 15:
		file_array_ptr = op_X_array[OP_READDIR];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READDIR;
		break;
	    case 16:
		file_array_ptr = op_X_array[OP_WRITE_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_WRITE_FILE;
		break;
	    case 17:
		file_array_ptr = op_X_array[OP_READ_FILE];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_READ_FILE;
		break;
	    case 18:
		file_array_ptr = op_X_array[OP_NEG_STAT];
		file_ptr =
		    &file_array_ptr[(dir * cmdline.client_files) + filenum];
		op_type = OP_NEG_STAT;
		break;
	    }
	    break;
	case OP_CUSTOM1:
	    file_array_ptr = op_X_array[OP_CUSTOM1];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_CUSTOM2:
	    file_array_ptr = op_X_array[OP_CUSTOM2];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	}
    }
    else			/* my_mix_table[my_workload].shared_buckets == 0 */
    {
	switch (tp->op_type)
	{
	case OP_WRITE:
	    file_array_ptr = op_X_array[OP_WRITE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_WRITE;
	    break;
	case OP_WRITE_FILE:
	    file_array_ptr = op_X_array[OP_WRITE_FILE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_WRITE_FILE;
	    break;
	case OP_MM_WRITE:
	    file_array_ptr = op_X_array[OP_MM_WRITE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_MM_WRITE;
	    break;
	case OP_MM_READ:
	    file_array_ptr = op_X_array[OP_MM_READ];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_MM_READ;
	    break;
	case OP_READ:
	    file_array_ptr = op_X_array[OP_READ];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_READ;
	    break;
	case OP_READ_FILE:
	    file_array_ptr = op_X_array[OP_READ_FILE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_READ_FILE;
	    break;
	case OP_WRITE_RAND:
	    file_array_ptr = op_X_array[OP_WRITE_RAND];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_WRITE_RAND;
	    break;
	case OP_READ_RAND:
	    file_array_ptr = op_X_array[OP_READ_RAND];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_READ_RAND;
	    break;
	case OP_RMW:
	    file_array_ptr = op_X_array[OP_RMW];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_RMW;
	    break;
	case OP_MKDIR:
	    file_array_ptr = op_X_array[OP_MKDIR];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_RMDIR:
	    file_array_ptr = op_X_array[OP_RMDIR];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_UNLINK:
	    file_array_ptr = op_X_array[OP_UNLINK];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_TRUNC:
	    file_array_ptr = op_X_array[OP_TRUNC];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_UNLINK2:
	    /* Over-ride filenum to pick next file in dir */
	    if (my_mix_table[my_workload].unlink2_no_recreate
		&& (phase == RUN_PHASE))
	    {
		if (!one_time_unlink2)
		{
		    unlink2_dir_array =
			(int *) malloc (sizeof (int) * cmdline.client_dirs);
		    log_file (LOG_EXEC, "Number of cmdline.client_dirs %d\n",
			      cmdline.client_dirs);

		    memset ((void *) unlink2_dir_array, 0,
			    (size_t) (sizeof (int) * cmdline.client_dirs));
		    one_time_unlink2++;
		}
		if (unlink2_dir_array[dir] != cmdline.client_files)
		{
		    unlink2_dir_array[dir]++;
		    filenum = unlink2_dir_array[dir];
		}
	    }
	    file_array_ptr = op_X_array[OP_UNLINK2];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_CREATE:
	    file_array_ptr = op_X_array[OP_CREATE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_STAT:
	    file_array_ptr = op_X_array[OP_STAT];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_STAT;
	    break;
	case OP_NEG_STAT:
	    file_array_ptr = op_X_array[OP_NEG_STAT];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_NEG_STAT;
	    break;
	case OP_APPEND:
	    file_array_ptr = op_X_array[OP_APPEND];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_LOCKING:
	    file_array_ptr = op_X_array[OP_LOCKING];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_ACCESS:
	    file_array_ptr = op_X_array[OP_ACCESS];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_ACCESS;
	    break;
	case OP_CHMOD:
	    file_array_ptr = op_X_array[OP_CHMOD];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_CHMOD;
	    break;
	case OP_READDIR:
	    file_array_ptr = op_X_array[OP_READDIR];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_READDIR;
	    break;
	case OP_COPYFILE:
	    file_array_ptr = op_X_array[OP_COPYFILE];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_RENAME:
	    file_array_ptr = op_X_array[OP_RENAME];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_STATFS:
	    file_array_ptr = op_X_array[OP_STATFS];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_STATFS;
	    break;
	case OP_PATHCONF:
	    file_array_ptr = op_X_array[OP_PATHCONF];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = OP_PATHCONF;
	    break;
	case OP_CUSTOM1:
	    file_array_ptr = op_X_array[OP_CUSTOM1];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	case OP_CUSTOM2:
	    file_array_ptr = op_X_array[OP_CUSTOM2];
	    file_ptr =
		&file_array_ptr[(dir * cmdline.client_files) + filenum];
	    op_type = tp->op_type;
	    break;
	}
    }

    /* Walk into the target directory */
    {
	char buf[MAXNAME2];
	struct netmist_vfs_dir *vfsdir = NULL;

	snprintf (buf, MAXNAME2, "Dir%d", dir);
	err = VFS (walk, netmist_vfs_workdir, buf, &vfsdir);
	if (err)
	    goto out_vfs_err;

	snprintf (buf, MAXNAME2, "bucket%d", op_type);
	err = VFS (walk, vfsdir, buf, &file_ptr->dir);
	if (err)
	    goto out_vfs_err;

	VFS (dfree, &vfsdir);
    }

    /* Create the file name */

    memset ((void *) leaf_name, 0, MAXNAME);
    make_name (leaf_name,
	       (client_id * cmdline.client_dirs * NUM_OP_TYPES *
		cmdline.client_files) +
	       (dir * NUM_OP_TYPES * cmdline.client_files) +
	       (op_type * cmdline.client_files) + filenum, TYPE_FILE,
	       filenum);

    log_file (LOG_DEBUG, "Client got dir %s file %s\n",
	      VFS (pathptr, file_ptr->dir), leaf_name);

    free (file_ptr->relfilename);
    file_ptr->relfilename = strdup (leaf_name);
    if (file_ptr->relfilename == NULL)
	goto out_nomem;

    file_ptr->access_count++;	/* Bump access count for this file */

    return ((void *) file_ptr);

  out_vfs_err:
    log_file (LOG_ERROR, "make_path VFS walk error %d (%s)\n", err,
	      netmist_vfs_errstr (err));
    R_exit (NETMIST_VFS_ERR, err);

  out_nomem:
    log_file (LOG_ERROR, "Out of memory while making path");
    R_exit (NETMIST_OOM, 0);

    /* NOTREACHED ; placate control-reaches-end-of-function warning */
    return NULL;
}

/**
 * @brief Create the path for this block based test entry
 *        for this client, or child of client. Store the path 
 *        in the test array entry associated with this block 
 *        operation.
 *
 * @param  client_id : Used for sharing
 * @param  test_num  : Used to select test type.
 */
/*
 * __doc__
 * __doc__  Function : struct file_object * 
 * __doc__             generic_bmake_path(int client_id, int test_num)
 * __doc__  Arguments: int client_id: used for sharing
 * __doc__             int test_num: used to select test type.
 * __doc__  Returns  : Returns a pointer to a file_object.
 * __doc__  Performs : Create the path for this block based test entry
 * __doc__             for this client, or child of client. Store the path 
 * __doc__             in the test array entry associated with this block 
 * __doc__             operation.
 * __doc__
 */
struct file_object *
generic_bmake_path (int client_id, int test_num)
{
    struct file_object *file_ptr = NULL;
    struct file_object *file_array_ptr = NULL;
    struct test *tp;

    log_file (LOG_DEBUG, "Entering generic_bmake_path. %d\n", client_id);

    tp = &tests[test_num];

    file_array_ptr = op_X_array[tp->op_type];
    file_ptr = &file_array_ptr[0];

    file_ptr->dir = netmist_vfs_root ();

    free (file_ptr->relfilename);
    file_ptr->relfilename = strdup ("");
    if (file_ptr->relfilename == NULL)
	goto out_nomem;

    file_ptr->access_count++;	/* Bump access count for this file */

    return ((void *) file_ptr);

  out_nomem:
    log_file (LOG_ERROR, "Out of memory while making path");
    R_exit (NETMIST_OOM, 0);

    /* NOTREACHED ; placate control-reaches-end-of-function warning */
    return NULL;
}

/**
 * @brief If it is the case that some pre-op work needs to be 
 *        done for some operations. This function would get called 
 *        just before the work was to start. No time would be 
 *        accumlated for this action.
 *
 * @param  test_num : Which test type.
 * @param  fp       : Used to point at a particular file
 */
/*
 * __doc__
 * __doc__  Function : void 
 * __doc__             generic_prefix_op(int test_num, struct file_object *fp)
 * __doc__  Arguments: int test_num: Which test type.
 * __doc__             struct file_object *fp: Used to point at a particular 
 * __doc__             file.
 * __doc__  Returns  : void
 * __doc__  Performs : It is the case that some pre-op work needs to be 
 * __doc__             done for some operations. This function would get called 
 * __doc__             just before the work was to start. No time would be 
 * __doc__             accumlated for this action.
 * __doc__
 */
void
generic_prefix_op (int test_num, struct file_object *fp)
{
    struct test *tp;
    int ret;
    struct netmist_vfs_stat stbuf;
    struct netmist_vfs_object *fd = NULL;

    tp = &tests[test_num];
    switch (tp->op_type)
    {
    case OP_WRITE:
	if (((netmist_rand () % 100) < my_mix_table[my_workload].notify)
	    && !in_validate)
	    generic_arm_notify (fp);
	break;
    case OP_WRITE_FILE:
	if (((netmist_rand () % 100) < my_mix_table[my_workload].notify)
	    && !in_validate)
	    generic_arm_notify (fp);
	break;
    case OP_READ:
	break;
    case OP_READ_FILE:
	break;
    case OP_RMW:
	if (((netmist_rand () % 100) < my_mix_table[my_workload].notify)
	    && !in_validate)
	    generic_arm_notify (fp);
	break;
    case OP_LOCKING:
	break;
    case OP_ACCESS:
	break;
    case OP_STAT:
	break;
    case OP_NEG_STAT:
	break;
    case OP_CHMOD:
	break;
    case OP_WRITE_RAND:	/* Need to unlink the file before writing it. ??? */
	if (((netmist_rand () % 100) < my_mix_table[my_workload].notify)
	    && !in_validate)
	    generic_arm_notify (fp);
	break;
    case OP_READ_RAND:
	break;
    case OP_APPEND:
	/* Truncate back to original size */
	if (fp->file_desc == NULL)
	{
	    /* If we don't have it open and can't open it... that's too bad */
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_WRITE, &fd);
	}
	else
	{
	    fd = fp->file_desc;
	}
	if (fd != NULL)
	{
	    ubuntu = VFS (trunc, fd, fp->Original_fsize * (long long) 1024);
	    if (ubuntu)
	    {
		;
	    }
	    if (fp->file_desc == NULL)
	    {
		VFS (close, &fd);
	    }
	    else
	    {
		fd = NULL;
	    }
	}

	total_file_ops += 1;
	work_obj[my_workload].total_file_ops += 1;
	work_obj[my_workload].open_op_count++;	/* Count somewhere */
	work_obj[my_workload].close_op_count++;	/* Count somewhere */
	break;
    case OP_MKDIR:
	/* Need to remove the directory before creating a new one */

	log_file (LOG_DEBUG, "Prefix rmdir before mkdir \n");

	ret = VFS (stat, fp->dir, NULL, &stbuf);
	if (ret == 0)
	{
	    ret = VFS (rmdir, fp->dir);
	    if (ret != 0)
	    {
		log_file (LOG_ERROR, "generic_prefix_op rmdir failed\n");
	    }
	}

	/*
	 * It is normal for this to fail. The directory should not
	 * exist, but could have, if things were left behind from
	 * a previous run. 
	 */

	total_file_ops += 1;
	work_obj[my_workload].total_file_ops += 1;
	work_obj[my_workload].unlink_op_count++;	/* Count somewhere ? */
	break;

    case OP_RMDIR:
	break;
    case OP_UNLINK:
	break;
    case OP_TRUNC:
	break;
    case OP_UNLINK2:
	break;
    case OP_CREATE:
	/* Prefix is to remove any from previous create */
	VFS (unlink, fp->dir, fp->relfilename);
	break;
    case OP_READDIR:
	break;
    case OP_COPYFILE:
	break;
    case OP_RENAME:
	break;
    case OP_STATFS:
	break;
    case OP_PATHCONF:
	break;
    case OP_CUSTOM1:
	break;
    case OP_CUSTOM2:
	break;
    }
}

/**
 * @brief It may be the case that some post-op work needs to 
 *        be done after some operations. This function 
 *        is for that purpose. No time will be accumlated for this
 *        action.
 *
 * @param  test_num : Which test type.
 * @param  fp       : Used to point at a particular file
 */
/*
 * __doc__
 * __doc__  Function : void 
 * __doc__             generic_postfix_op(int test_num, struct file_object *fp)
 * __doc__  Arguments: Int test_num: Used to select a particular test.
 * __doc__             struct file_object *fp: Used to point at a particular
 * __doc__             file.
 * __doc__  Returns  : void
 * __doc__  Performs : It may be the case that some post-op work needs to 
 * __doc__             be done after some operations. This function 
 * __doc__             is for that purpose. No time will be accumlated for this
 * __doc__             action.
 * __doc__
 */
void
generic_postfix_op (int test_num, struct file_object *fp)
{
    struct test *tp;
    char altname[MAXNAME];	/* for Rename */

    tp = &tests[test_num];
    switch (tp->op_type)
    {
    case OP_WRITE:
	if ((my_mix_table[my_workload].notify) && !in_validate)
	    generic_arm_cancel (fp);
	break;
    case OP_WRITE_FILE:
	if ((my_mix_table[my_workload].notify) && !in_validate)
	    generic_arm_cancel (fp);
	break;
    case OP_READ:
	break;
    case OP_READ_FILE:
	break;
    case OP_RMW:
	if ((my_mix_table[my_workload].notify) && !in_validate)
	    generic_arm_cancel (fp);
	break;
    case OP_READDIR:
	break;
    case OP_LOCKING:
	/* Lock is already released */
	break;
    case OP_STAT:
	break;
    case OP_NEG_STAT:
	break;
    case OP_CHMOD:
	break;
    case OP_ACCESS:
	break;
    case OP_APPEND:
	break;
    case OP_MKDIR:
	break;
    case OP_RMDIR:
	/* Need to put the directory back after the rmdir */

	log_file (LOG_DEBUG, "Postfix re-create directory after rmdir \n");

	(*Netmist_Ops[my_workload].mkdir) (fp);
	break;
    case OP_WRITE_RAND:
	if ((my_mix_table[my_workload].notify) && !in_validate)
	    generic_arm_cancel (fp);
	break;
    case OP_READ_RAND:
	break;
    case OP_UNLINK:
	/* Need to put the file back after the unlink */

	log_file (LOG_DEBUG, "Postfix re-create after unlink \n");

	(*Netmist_Ops[my_workload].init_empty_file) (fp->dir,
						     fp->relfilename);
	break;
    case OP_TRUNC:
	/* Need to put the file back to its original size after the truncate */
	log_file (LOG_DEBUG, "Postfix re-fill after trunc \n");

	/* Put file back */
	(*Netmist_Ops[my_workload].init_file) (0, fp);
	break;
    case OP_UNLINK2:
	if (my_mix_table[my_workload].unlink2_no_recreate && (phase == RUN_PHASE))	/* Special case, do 
											   not put file back 
											 */
	    break;
	/* Need to put the file back after the unlink2 */
	log_file (LOG_DEBUG, "Postfix re-create full after unlink2 \n");

	/* Create a non-empty file */
	/* Put file back */
	(*Netmist_Ops[my_workload].init_file) (0, fp);
	break;
    case OP_CREATE:
	/* Post-fix is to remove any from previous create */
	VFS (unlink, fp->dir, fp->relfilename);
	break;
    case OP_COPYFILE:
	/* The old way, we deleted after every op. The new
	   way, we delete at the end of the run.
	   my_strncpy(altname,fp->filename, MAXNAME);
	   (void)my_strcat(altname,"_2");
	   unlink(altname);
	 */
	break;
    case OP_RENAME:
	snprintf (altname, sizeof (altname), "%s_2", fp->relfilename);
	/* Put it back */
	(void) VFS (rename, fp->dir, altname, fp->dir, fp->relfilename);
	break;
    case OP_STATFS:
	break;
    case OP_PATHCONF:
	break;
    case OP_CUSTOM1:
	break;
    case OP_CUSTOM2:
	break;
    }
}


/**
 * @brief This is the code the client will call to initialize
 *        the directory structure and files.
 *
 * @param client_id : Used for sharing.
 */
/*
 * __doc__
 * __doc__  Function : void client_init_files(int client_id)
 * __doc__  Arguments: int client_id: Used for sharing.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code the client will call to initialize
 * __doc__             the directory structure and files.
 * __doc__
 */
void
client_init_files (int client_id)
{
#if defined(WIN32)
    DWORD ret = 0;
    time_t ltime;
    uint64_t file_time_in_unix;
#else
    int ret;
#endif
    int i, j, k, my_delta_time;
#if defined(PDSM_RO_ACTIVE)
    int my_pdsm_time;
#endif
    int dir_count = 0;
    int file_count = 0;
    int empty_count = 0;
    int tot, pct_complete = 0, prev_pct_complete = 0;
    int locked_dir = 0;
    int usedBuckets[NUM_OP_TYPES];
    char buf[1024];
    char leaf_name[MAXNAME];
    struct file_object *file_ptr;
    struct file_object *file_array_ptr;

    struct netmist_vfs_dir *idir = NULL;
    struct netmist_vfs_dir *kdir = NULL;
    struct netmist_vfs_stat stbuf;

    for (k = 0; k < NUM_OP_TYPES; k++)
    {
	usedBuckets[k] = 1;
    }
    if (my_mix_table[my_workload].shared_buckets == 0)
    {
	if (PERCENT_WRITE == 0)
	    usedBuckets[OP_WRITE] = 0;
	if (PERCENT_WRITE_FILE == 0)
	    usedBuckets[OP_WRITE_FILE] = 0;
	if (PERCENT_READ == 0)
	    usedBuckets[OP_READ] = 0;
	if (PERCENT_READ_FILE == 0)
	    usedBuckets[OP_READ_FILE] = 0;
	if (PERCENT_WRITE_RAND == 0)
	    usedBuckets[OP_WRITE_RAND] = 0;
	if (PERCENT_READ_RAND == 0)
	    usedBuckets[OP_READ_RAND] = 0;
	if (PERCENT_RMW == 0)
	    usedBuckets[OP_RMW] = 0;
	if (PERCENT_MKDIR == 0)
	    usedBuckets[OP_MKDIR] = 0;
	if (PERCENT_RMDIR == 0)
	    usedBuckets[OP_RMDIR] = 0;
	if (PERCENT_UNLINK == 0)
	    usedBuckets[OP_UNLINK] = 0;
	/* not sure how much impact this will have, other than preserving 
	 * the consistency of the usedBuckets indicator, since there are
	 * other mechanisms to not create UNLINK2 dataset if it isn't used.
	 * however, given that we may remove that mechanism in the future, 
	 * and usedBuckets shouldn't have a hole in it, this is fine.
	 */
	if (PERCENT_UNLINK2 == 0)
	    usedBuckets[OP_UNLINK2] = 0;
	if (PERCENT_STAT == 0)
	    usedBuckets[OP_STAT] = 0;
	if (PERCENT_NEG_STAT == 0)
	    usedBuckets[OP_NEG_STAT] = 0;
	if (PERCENT_APPEND == 0)
	    usedBuckets[OP_APPEND] = 0;
	if (PERCENT_CREATE == 0)
	    usedBuckets[OP_CREATE] = 0;
	if (PERCENT_LOCKING == 0)
	    usedBuckets[OP_LOCKING] = 0;
	if (PERCENT_ACCESS == 0)
	    usedBuckets[OP_ACCESS] = 0;
	if (PERCENT_CHMOD == 0)
	    usedBuckets[OP_CHMOD] = 0;
	if (PERCENT_READDIR == 0)
	    usedBuckets[OP_READDIR] = 0;
	if (PERCENT_MM_WRITE == 0)
	    usedBuckets[OP_MM_WRITE] = 0;
	if (PERCENT_MM_READ == 0)
	    usedBuckets[OP_MM_READ] = 0;
	if (PERCENT_COPYFILE == 0)
	    usedBuckets[OP_COPYFILE] = 0;
	if (PERCENT_RENAME == 0)
	    usedBuckets[OP_RENAME] = 0;
	if (PERCENT_STATFS == 0)
	    usedBuckets[OP_STATFS] = 0;
	if (PERCENT_PATHCONF == 0)
	    usedBuckets[OP_PATHCONF] = 0;
	if (PERCENT_TRUNC == 0)
	    usedBuckets[OP_TRUNC] = 0;
	if (PERCENT_CUSTOM1 == 0)
	    usedBuckets[OP_CUSTOM1] = 0;
	if (PERCENT_CUSTOM2 == 0)
	    usedBuckets[OP_CUSTOM2] = 0;
    }

    log_file (LOG_EXEC, "Client starting init\n");

#if defined(PDSM_RO_ACTIVE)
    if (pdsm_stats)
    {
	pdsm_stats->cur_state = (int) INIT_BEAT;
	pdsm_stats->client_id = cmdline.client_id;
	pdsm_stats->mode = (int) pdsm_mode;
	pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
	pdsm_stats->achieved_op_rate = (double) 0.0;
	pdsm_stats->run_time = (double) 0.0;
	my_strncpy (pdsm_stats->client_name, client_localname, MAXHNAME);
	pdsm_stats->client_name[strlen (client_localname)] = 0;
	my_strncpy (pdsm_stats->workload_name,
		    work_obj[my_workload].work_obj_name, MAXWLNAME);
	pdsm_stats->workload_name[strlen
				  (work_obj[my_workload].work_obj_name)] = 0;
	pdsm_stats->epoch_time = time (NULL);
    }
    if (pdsm_file_fd)
    {
	if (pdsm_mode == 0)	/* Over-write mode */
	{
#if defined(WIN32)
	    largeoffset.QuadPart =
		sizeof (struct pdsm_remote_stats) * cmdline.client_id;
	    SetFilePointerEx (pdsm_file_fd, largeoffset, NULL, FILE_BEGIN);
#else
	    I_LSEEK (pdsm_file_fd,
		     sizeof (struct pdsm_remote_stats) * cmdline.client_id,
		     SEEK_SET);
#endif
	}
#if defined(WIN32)
	WriteFile (pdsm_file_fd, pdsm_stats,
		   sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
	ret =
	    write (pdsm_file_fd, pdsm_stats,
		   sizeof (struct pdsm_remote_stats));
#endif
#if defined(FSYNC_PDSM)
	fsync (pdsm_file_fd);
#endif
    }
#endif
    init_phase = 1;

    phase = INIT_PHASE;

    my_start_time = gettime ();

    log_file (LOG_EXEC, "Client %d: client_files: %d\n",
	      client_id, cmdline.client_files);

    log_file (LOG_EXEC, "Client %d: client_dirs: %d\n",
	      client_id, cmdline.client_dirs);

    setup_workdir (client_id, 1);

    for (k = 0; k < NUM_OP_TYPES; k++)
    {
	/* In order to preserve previous results, the addition of UNLINK2 */
	/* needs to not layout space unless it is used by a new workload */
	if ((k == OP_UNLINK2) && (PERCENT_UNLINK2 == 0))
	    continue;

	log_file (LOG_DEBUG, "Initializing files for op type %d \n", k);

	if (k != OP_MKDIR && usedBuckets[k] == 1)
	{
	    /* Create all directories below the top level, per proc, directory */
	    for (i = 0; i < cmdline.client_dirs; i++)
	    {
		snprintf (buf, 1024, "Dir%d", i);
		ret = VFS (walk, netmist_vfs_workdir, buf, &idir);
		if (ret)
		{
		    log_file (LOG_ERROR,
			      "client_init_files VFS walk error %d\n", ret);
		    R_exit (NETMIST_VFS_ERR, ret);
		}

		log_file (LOG_DEBUG, "Mkdir %s \n", VFS (pathptr, idir));

		/* Use virtualized, in case this function is workload 
		 * specific 
		 */
		ret = (*Netmist_Ops[my_workload].init_dir) (idir);
		if (ret != 0)
		{
		    log_file (LOG_ERROR, "Mkdir failed. Error %d on %s \n",
			      ret, VFS (pathptr, idir));
		    R_exit (LOAD_GEN_ERR, ret);
		}

		snprintf (buf, 1024, "bucket%d", k);
		ret = VFS (walk, idir, buf, &kdir);
		if (ret)
		{
		    log_file (LOG_ERROR,
			      "client_init_files VFS walk error %d\n", ret);
		    R_exit (NETMIST_VFS_ERR, ret);
		}

		log_file (LOG_DEBUG, "Mkdir %s \n", VFS (pathptr, kdir));

		/* Use virtualized interface to create the directory. */
		ret = (*Netmist_Ops[my_workload].init_dir) (kdir);
		if (ret != 0)
		{
		    log_file (LOG_EXEC, "Mkdir failed. Error %d on %s \n",
			      ret, VFS (pathptr, kdir));
		    R_exit (LOAD_GEN_ERR, ret);
		}
		dir_count++;
		locked_dir = 0;
		/*
		 * Logic to reduce STAT calls while performing a parallel 
		 * initialization of files within buckets. There arre 
		 * potential race conditions, but its ok if one hits
		 * the race. It would only cause a few extra calls to 
		 * stat().
		 */
		if (cmdline.sharing_flag)
		{
		    /* Create a lock file for the directory level */
		  too_old:
		    ret = VFS (stat, kdir, "dir_lock", &stbuf);
		    if (ret == 0)
		    {
			/* In case someone killed the test, or it died, there
			 * might be some very old dir_lock files around. We
			 * need to be sure that none exist for the first proc
			 * to work on the files in this directory. If we don't
			 * clean this up, then the files may not get created,
			 * or set back to their original sizes for the next
			 * load point. If dir_lock is older than 2 minutes, 
			 * lets unlink and try again. 
			 */
#if defined(WIN32)
			time (&ltime);
			/* HEAVY 369 year magic here !!!   Windows file_times are in 100s of nanoseconds
			 * based on an EPOCH of Jan 1st 1601.  Yeah, when Queen Elizibeth was challeged by a revolution !! 
			 * SO... we need to make DARN sure that we don't compare Unix EPOCH based times (Jan 1st 1970) with 
			 * Gregorian 400 year cycle times from Windows !!!   Thus, we get everthing in one EPOCH base.
			 */
			file_time_in_unix =
			    ((stbuf.netmist_st_ctime / 10LL) -
			     DELTA_EPOCH_IN_MICROSECS) / 1000000LL;
			if ((((uint64_t) ltime - file_time_in_unix) > 3600)
			    ||
			    (((long long) ltime -
			      (long long) file_time_in_unix) < 0LL))

#else
			if ((time (NULL) - stbuf.netmist_st_ctime > 3600) ||
			    ((long long) time (NULL) -
			     (long long) stbuf.netmist_st_ctime < 0))
#endif
			{
#if defined(WIN32)
			    log_file (LOG_EXEC,
				      "Resetting dir_lock. Delta_t = %lld\n",
				      ((uint64_t) ltime - file_time_in_unix));
#else
			    log_file (LOG_EXEC,
				      "Resetting dir_lock. Delta_t = %lld\n",
				      (long long) (time (NULL) -
						   stbuf.netmist_st_ctime));
#endif
			    VFS (unlink, kdir, "dir_lock");
			    goto too_old;
			}
			locked_dir = 0;
			/* Percent done */
			tot =
			    NUM_OP_TYPES * cmdline.client_dirs *
			    cmdline.client_files;
			pct_complete = (file_count * 100) / tot;
			if (pct_complete > 100)
			    pct_complete = 100;
			if (pct_complete
			    && (pct_complete != prev_pct_complete)
			    && ((pct_complete % 10) == 0)
			    && cmdline.heartbeat_flag)
			{
			    tell_prime_pct_complete (pct_complete,
						     cmdline.real_client_id,
						     R_PERCENT_I_COMPLETE);
			    prev_pct_complete = pct_complete;
			}
		    }
		    else
		    {
			/* Create a file, to indicate that this dir is being 
			 * worked 
			 */
			ret =
			    (*Netmist_Ops[my_workload].init_empty_file) (kdir,
									 "dir_lock");
			if (ret == 0)
			{
			    push_name (kdir, "dir_lock");	/* save name for delayed unlink */
			    locked_dir = 1;	/* Locked by me */
			}
			else
			    locked_dir = 0;	/* Locked by some other */

			/* Percent done */
			tot =
			    NUM_OP_TYPES * cmdline.client_dirs *
			    cmdline.client_files;
			pct_complete = (file_count * 100) / tot;
			if (pct_complete > 100)
			    pct_complete = 100;
			if (pct_complete
			    && (pct_complete != prev_pct_complete)
			    && ((pct_complete % 10) == 0)
			    && cmdline.heartbeat_flag)
			{
			    tell_prime_pct_complete (pct_complete,
						     cmdline.real_client_id,
						     R_PERCENT_I_COMPLETE);
			    prev_pct_complete = pct_complete;
			}
		    }
		}

		/* Create all files */
		for (j = 0; j < cmdline.client_files; j++)
		{
		     /*-----------------*/
		    /* heartbeat       */
		     /*-----------------*/
		    cur_time = gettime ();

		    /* 
		     * We need all heartbeats from all procs, let the Prime 
		     * do the filtering. 
		     */
		    my_delta_time = (int) (cur_time - last_heart_beat);

		    if (cmdline.heartbeat_flag && init_phase &&
			(my_delta_time >= HEARTBEAT_TICK))
		    {

			tell_prime_heartbeat (cmdline.real_client_id,
					      INIT_BEAT, (double) 0.0);
			last_heart_beat = cur_time;
#if defined(PDSM_RO_ACTIVE)
			if (pdsm_stats)
			{
			    pdsm_stats->epoch_time = time (NULL);
			    pdsm_stats->cur_state = (int) INIT_BEAT;
			    pdsm_stats->client_id = cmdline.client_id;
			    pdsm_stats->mode = pdsm_mode;
			    pdsm_stats->requested_op_rate =
				(double) cmdline.op_rate;
			    pdsm_stats->achieved_op_rate = (double) 0.0;
			    pdsm_stats->run_time =
				(double) (last_heart_beat - my_start_time);
			    my_strncpy (pdsm_stats->client_name,
					client_localname, MAXHNAME);
			    pdsm_stats->client_name[strlen (client_localname)]
				= 0;
			    my_strncpy (pdsm_stats->workload_name,
					work_obj[my_workload].work_obj_name,
					MAXWLNAME);
			    pdsm_stats->workload_name[strlen
						      (work_obj
						       [my_workload].
						       work_obj_name)] = 0;
			}

			my_pdsm_time = (int) (cur_time - last_pdsm_beat);
			if (pdsm_file_fd && (my_pdsm_time >= pdsm_interval))
			{
			    if (pdsm_mode == 0)	/* Over-write mode */
			    {
#if defined(WIN32)
				largeoffset.QuadPart =
				    sizeof (struct pdsm_remote_stats) *
				    cmdline.client_id;
				SetFilePointerEx (pdsm_file_fd, largeoffset,
						  NULL, FILE_BEGIN);
#else
				I_LSEEK (pdsm_file_fd,
					 sizeof (struct pdsm_remote_stats) *
					 cmdline.client_id, SEEK_SET);
#endif
			    }
#if defined(WIN32)
			    WriteFile (pdsm_file_fd, pdsm_stats,
				       sizeof (struct pdsm_remote_stats),
				       &ret, NULL);
#else
			    ret =
				write (pdsm_file_fd, pdsm_stats,
				       sizeof (struct pdsm_remote_stats));
#endif
			    if (ret <= 0)
				log_file (LOG_ERROR,
					  "writing pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
			    if (my_pdsm_time >= (2 * pdsm_interval))
				fsync (pdsm_file_fd);
#endif
			    last_pdsm_beat = cur_time;
			}
#endif
		    }
		    /*-----------------*/

		    file_count++;

		    /* Percent done */
		    tot =
			NUM_OP_TYPES * cmdline.client_dirs *
			cmdline.client_files;
		    pct_complete = (file_count * 100) / tot;
		    if (pct_complete > 100)
			pct_complete = 100;
		    if (pct_complete && (pct_complete != prev_pct_complete)
			&& ((pct_complete % 10) == 0)
			&& cmdline.heartbeat_flag)
		    {
			tell_prime_pct_complete (pct_complete,
						 cmdline.real_client_id,
						 R_PERCENT_I_COMPLETE);
			prev_pct_complete = pct_complete;
			retry1_counter = 0;
			retry2_counter = 0;
		    }

		    memset ((void *) leaf_name, 0, MAXNAME);
		    make_name (leaf_name,
			       (client_id * cmdline.client_dirs *
				NUM_OP_TYPES * cmdline.client_files) +
			       (i * NUM_OP_TYPES * cmdline.client_files) +
			       (k * cmdline.client_files) + j, TYPE_FILE, j);

		    /* 
		     * Save the Original file size information 
		     * k = to the type of op.
		     * i = which directory, cmdline.client_files is the number of files
		     * per directory, and j is which file in the lowest level 
		     * dir.
		     */
		    file_array_ptr = op_X_array[k];	/* op type */
		    file_ptr =
			&file_array_ptr[(i * cmdline.client_files) + j];
		    /*
		     * The cmdline.fsize slots will create a range of file sizes that
		     * will average out to be that of the calculated cmdline.fsize.
		     */

		    if ((my_mix_table[my_workload].uniform_file_size_dist ==
			 NO_UNIFORM_DIST)
			&& ((cmdline.client_files > FSIZE_MIN_SLOTS) ||
			    my_mix_table[my_workload].use_file_size_dist))
		    {
			file_ptr->Original_fsize =
			    get_next_fsize (cmdline.fsize * 1024) / 1024;
			file_ptr->Original_fsize =
			    (file_ptr->Original_fsize +
			     ((long long) rsize - 1)) & ~((long long) rsize -
							  1);
			file_ptr->Current_fsize = file_ptr->Original_fsize;
		    }
		    else
		    {
			file_ptr->Original_fsize =
			    ((cmdline.fsize +
			      ((long long) rsize - 1)) & ~((long long) rsize -
							   1));
			file_ptr->Current_fsize =
			    ((cmdline.fsize +
			      ((long long) rsize - 1)) & ~((long long) rsize -
							   1));
		    }

		    ret = VFS (walk, kdir, "", &file_ptr->dir);
		    if (ret != 0)
		    {
			log_file (LOG_ERROR,
				  "client_init_files VFS walk error %d\n",
				  ret);
			R_exit (NETMIST_VFS_ERR, ret);
		    }
		    file_ptr->relfilename = strdup (leaf_name);
		    if (file_ptr->relfilename == NULL)
		    {
			log_file (LOG_ERROR,
				  "Out of memory while making path");
			R_exit (NETMIST_OOM, 0);
		    }

		    if (k == OP_UNLINK || k == OP_STAT || k == OP_LOCKING ||
			k == OP_ACCESS || k == OP_CHMOD || k == OP_READDIR ||
			k == OP_STATFS || k == OP_CUSTOM1 || k == OP_CUSTOM2
			|| k == OP_PATHCONF || k == OP_RENAME
			|| k == OP_NEG_STAT || k == OP_CREATE)
		    {
			/* 
			 * Jump out to next directory, some other thread is 
			 * currently doing this directory.
			 */
			if (cmdline.sharing_flag && (locked_dir == 0))
			{
			    empty_count++;
			    continue;
			}
			/* No files under this one */
			if (k == OP_RMDIR)
			    continue;
			/* Create an empty file. Use virtualized interface  */
			(*Netmist_Ops[my_workload].init_empty_file) (kdir,
								     leaf_name);
			empty_count++;
		    }
		    else
		    {
			/* 
			 * Jump out to next directory, some other thread is 
			 * currently doing this directory.
			 */
			if (cmdline.sharing_flag && (locked_dir == 0))
			    continue;
			/* Create a file and fill it up with data. Use 
			 * virtualized interface 
			 * No files under this one.
			 */
			if (k == OP_RMDIR)
			    continue;
			(*Netmist_Ops[my_workload].init_file) (k, file_ptr);
		    }
		}
	    }
	}
    }
    if ((pct_complete != 100) && (prev_pct_complete != 100)
	&& cmdline.heartbeat_flag)
    {
	tell_prime_pct_complete (100, cmdline.real_client_id,
				 R_PERCENT_I_COMPLETE);
    }
    gdir_count = dir_count;	/* Total directory count */
    gfile_count = file_count;	/* Total file count of files that may have 
				 * data 
				 */
    gempty_count = empty_count;	/* Total file count of empty files */
    init_phase = 0;

    VFS (dfree, &kdir);
    VFS (dfree, &idir);

    /*
     * The avg_file_size was already calculated for this workload component.
     */
    file_space_mb =
	(double) (((avg_file_size * init_new_file) / 1024LL)/1024LL);
    log_file (LOG_EXEC, "Initialized file space: %.0f MiB\n", file_space_mb);
}

/**
 * @brief At the end of the test it is a good idea to cleanup the 
 *        huge collection of files that this client created.
 *        Also a good idea to call this from a signal handler for 
 *        sigint.
 *
 * @param client_id : Used for sharing.
 */
/*
 * __doc__
 * __doc__  Function : void client_remove_files(int client_id)
 * __doc__  Arguments: int client_id: Used for sharing.
 * __doc__  Returns  : void
 * __doc__  Performs : At the end of the test it is a good idea to cleanup the 
 * __doc__             huge collection of files that this client created.
 * __doc__             Also a good idea to call this from a signal handler for 
 * __doc__             sigint.
 * __doc__
 */
void
client_remove_files (int client_id)
{
    int i, j, k, ret;
    char buf[1024];
    char t_buf[MAXNAME];
    char leaf_name[MAXNAME];
    char alt_name[MAXNAME];
#if defined(PDSM_RO_ACTIVE)
    int my_pdsm_time;
#endif

    struct netmist_vfs_dir *procdir;
    struct netmist_vfs_dir *idir = NULL;
    struct netmist_vfs_dir *kdir = NULL;

    close_cached_file_desc ();

    /* Create top level directory for each proc */
    if (cmdline.sharing_flag)
    {
	snprintf (t_buf, sizeof (t_buf), "SM_%s",
		  work_obj[my_workload].work_obj_name);
    }
    else
    {
	snprintf (t_buf, sizeof (t_buf), "CL%d_%s",
		  client_id, work_obj[my_workload].work_obj_name);
    }
    procdir = netmist_vfs_root ();
    ret = netmist_vfs_walkme (&procdir, t_buf);
    if (ret)
    {
	log_file (LOG_ERROR, "client_remove_files VFS walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    slinky (0, my_mix_table[my_workload].extra_dir_levels,
	    my_mix_table[my_workload].chaff_count, &procdir, SLINKY_REMOVE);

    /*
     * At this point, procdir is just another handle to the same thing as
     * netmist_vfs_workdir.  We could close procdir and use the other one in
     * the following, but don't, because there's no real need.
     */

    /*for(k=0;k<NUM_OP_TYPES;k++) for all op types */
    for (i = 0; i < cmdline.client_dirs; i++)
    {
	snprintf (t_buf, sizeof (t_buf), "Dir%d", i);
	ret = VFS (walk, procdir, t_buf, &idir);
	if (ret)
	{
	    log_file (LOG_ERROR, "client_remove_files VFS walk error %d\n",
		      ret);
	    R_exit (NETMIST_VFS_ERR, ret);
	}

	/* for all directories of this child */
	for (k = 0; k < NUM_OP_TYPES; k++)
	{
	    snprintf (t_buf, sizeof (t_buf), "bucket%d", k);
	    ret = VFS (walk, idir, t_buf, &kdir);
	    if (ret)
	    {
		log_file (LOG_ERROR,
			  "client_remove_files VFS walk error %d\n", ret);
		R_exit (NETMIST_VFS_ERR, ret);
	    }

	    if (k == OP_MKDIR)
		goto rem_dir;	/* There are no files */

	    /* For all files in each directory */
	    for (j = 0; j < cmdline.client_files; j++)
	    {
		cur_time = gettime ();
		if (cmdline.heartbeat_flag && !cmdline.kflag &&
		    ((int) (cur_time - last_heart_beat) >= HEARTBEAT_TICK))
		{
		    tell_prime_heartbeat (cmdline.real_client_id, CLEAN_BEAT,
					  (double) 0.0);
		    last_heart_beat = cur_time;
#if defined(PDSM_RO_ACTIVE)
		    if (pdsm_stats)
		    {
			pdsm_stats->epoch_time = time (NULL);
			pdsm_stats->cur_state = (int) CLEAN_BEAT;
			pdsm_stats->mode = pdsm_mode;
			pdsm_stats->client_id = cmdline.client_id;
			pdsm_stats->requested_op_rate = (double) 0.0;
			pdsm_stats->achieved_op_rate = (double) 0.0;
			pdsm_stats->run_time =
			    (double) (my_start_time - last_heart_beat);
			my_strncpy (pdsm_stats->client_name, client_localname,
				    MAXHNAME);
			pdsm_stats->client_name[strlen (client_localname)] =
			    0;
			my_strncpy (pdsm_stats->workload_name,
				    work_obj[my_workload].work_obj_name,
				    MAXWLNAME);
			pdsm_stats->workload_name[strlen
						  (work_obj
						   [my_workload].
						   work_obj_name)] = 0;
		    }
		    my_pdsm_time = (int) (cur_time - last_pdsm_beat);
		    if (pdsm_file_fd && (my_pdsm_time >= pdsm_interval))
		    {
			if (pdsm_mode == 0)	/* Over-write mode */
			{
#if defined(WIN32)
			    largeoffset.QuadPart =
				sizeof (struct pdsm_remote_stats) *
				cmdline.client_id;
			    SetFilePointerEx (pdsm_file_fd, largeoffset, NULL,
					      FILE_BEGIN);
#else
			    I_LSEEK (pdsm_file_fd,
				     sizeof (struct pdsm_remote_stats) *
				     cmdline.client_id, SEEK_SET);
#endif
			}
#if defined(WIN32)
			WriteFile (pdsm_file_fd, pdsm_stats,
				   sizeof (struct pdsm_remote_stats), &ret,
				   NULL);
#else
			ret =
			    write (pdsm_file_fd, pdsm_stats,
				   sizeof (struct pdsm_remote_stats));
#endif
			if (ret <= 0)
			    log_file (LOG_ERROR,
				      "Writing pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
			if (my_pdsm_time >= (2 * pdsm_interval))
			    fsync (pdsm_file_fd);
#endif
			last_pdsm_beat = cur_time;
		    }
#endif
		}

		memset ((void *) leaf_name, 0, MAXNAME);
		memset ((void *) t_buf, 0, MAXNAME);
		make_name (leaf_name,
			   (client_id * cmdline.client_dirs * NUM_OP_TYPES *
			    cmdline.client_files) +
			   (i * NUM_OP_TYPES * cmdline.client_files) +
			   (k * cmdline.client_files) + j, TYPE_FILE, j);
		if (cmdline.sharing_flag)
		{
#if defined(WIN32)
		    snprintf (buf, sizeof buf, "%s\\SM_%s", cmdline.workdir,
			      work_obj[my_workload].work_obj_name);
		    snprintf (t_buf, sizeof t_buf, "\\Dir%d\\bucket%d\\%s", i,
			      k, leaf_name);
#else
		    snprintf (buf, sizeof buf, "%s/SM_%s", cmdline.workdir,
			      work_obj[my_workload].work_obj_name);
		    snprintf (t_buf, sizeof t_buf, "/Dir%d/bucket%d/%s", i, k,
			      leaf_name);
#endif
		    strncat (buf, t_buf, MAXNAME - strlen (buf));
		}
		else
		{
#if defined(WIN32)
		    snprintf (buf, sizeof buf, "%s\\CL%d_%s", cmdline.workdir,
			      client_id, work_obj[my_workload].work_obj_name);
		    snprintf (t_buf, sizeof t_buf, "\\Dir%d\\bucket%d\\%s", i,
			      k, leaf_name);
#else
		    snprintf (buf, sizeof buf, "%s/CL%d_%s", cmdline.workdir,
			      client_id, work_obj[my_workload].work_obj_name);
		    snprintf (t_buf, sizeof t_buf, "/Dir%d/bucket%d/%s", i, k,
			      leaf_name);
#endif
		    strncat (buf, t_buf, MAXNAME - strlen (buf));
		}

		/* Special cleanup for copyfile,or Rename destination files */
		if ((k == OP_COPYFILE) || (k == OP_RENAME))
		{
		    snprintf (alt_name, sizeof (alt_name), "%s_2", leaf_name);
		    (*Netmist_Ops[my_workload].remove_file) (kdir, alt_name);
		}
		if (k == OP_RMDIR)
		{
		    struct netmist_vfs_dir *sdir = NULL;
		    ret = VFS (walk, kdir, leaf_name, &sdir);
		    if (ret)
		    {
			log_file (LOG_ERROR,
				  "client_remove_files VFS walk error %d\n",
				  ret);
			R_exit (NETMIST_VFS_ERR, ret);
		    }
		    (*Netmist_Ops[my_workload].remove_dir) (sdir);
		    VFS (dfree, &sdir);
		    continue;
		}
		/* 
		 * First unlink the files and then remove the directories 
		 */
		(*Netmist_Ops[my_workload].remove_file) (kdir, leaf_name);

		log_file (LOG_DEBUG, "Remove file %s\n", buf);

	    }
	  rem_dir:
	    /* Remove all directories */
	    ret = (*Netmist_Ops[my_workload].remove_dir) (kdir);
	    if (ret != 0)
	    {
		log_file (LOG_ERROR,
			  "remove_client_files: remove_dir (bucket) failed: %d Name %s\n",
			  ret, buf);

	    }
	}
	ret = (*Netmist_Ops[my_workload].remove_dir) (idir);
	if (ret != 0)
	{
	    log_file (LOG_ERROR,
		      "remove_client_files: remove_dir (Dir) failed: %d Name %s\n",
		      ret, buf);

	}
	/* Remove all directories */
    }

    log_file (LOG_DEBUG, "rmdir %s\n", VFS (pathptr, procdir));

    ret = (*Netmist_Ops[my_workload].remove_dir) (procdir);
    if (ret != 0)
    {
	log_file (LOG_ERROR,
		  "client_remove_files: remove_dir (CL) failed: %d Name %s\n",
		  ret, buf);
    }

    VFS (dfree, &kdir);
    VFS (dfree, &idir);
    VFS (dfree, &procdir);
}

static inline int
adjust_size_direct (struct netmist_vfs_object *fd, int nozero, uint64_t * ds)
{
    int od = 0;
    uint64_t vds;

    VFS (direct, fd, -1, &od);
    if (od)
    {
	vds = vfst->netmist_vfs_direct_size (vfsr);
	*ds = (*ds / vds) * vds;
	if (nozero && (*ds == 0))
	    *ds = vds;
    }
    return od;
}

static void
netmist_directio (struct netmist_vfs_object *fd, int d)
{
    int od;
    VFS (direct, fd, d, &od);
}

/*
 * Try a read and, if it doesn't work, try again with increased alignment.  If
 * that works, the VFS will have cached this result so we avoid backing off again.
 * This is responsive to the VFS's demands; if they increase more than once,
 * we might retry more than once.
 *
 * For the moment, this takes too many parameters.  We will pare it down in
 * subsequent VFS plugin work.
 */
static int
netmist_try_large_read (struct netmist_vfs_object *nvo,
			char *buf, uint64_t len, uint64_t * alen)
{
    int d, err;
    uint64_t direct_size;

  retry:
    err = VFS (read, nvo, buf, len, alen);
    if (err == 0)
	return 0;

    direct_size = vfst->netmist_vfs_direct_size (vfsr);
    VFS (direct, nvo, -1, &d);
    if (d && ((len % direct_size) != 0))
    {
	log_file (LOG_EXEC, "Retrying with larger block size\n");

	len = (len / direct_size) * direct_size;
	if (len == 0)
	    len = direct_size;
	goto retry;
    }

    return err;
}

/*
 * Try a write and, if it doesn't work, try again with increased alignment.  If
 * that works, we will have cached this result so we avoid backing off again.
 *
 * For the moment, this takes too many parameters.  We will pare it down in
 * subsequent VFS plugin work.
 */
static int
netmist_try_large_write (struct netmist_vfs_object *nvo,
			 char *buf, uint64_t len, uint64_t * alen)
{
    int d, err;
    uint64_t direct_size;

  retry:
    err = VFS (write, nvo, buf, len, alen);
    if (err == 0)
	return 0;

    direct_size = vfst->netmist_vfs_direct_size (vfsr);
    VFS (direct, nvo, -1, &d);
    if (d && ((len % direct_size) != 0))
    {
	log_file (LOG_EXEC, "Retrying with larger block size\n");

	len = (len / direct_size) * direct_size;
	if (len == 0)
	    len = direct_size;
	goto retry;
    }

    return err;
}

/**
 * @brief This function reads a file so that cache space is 
 *        consumed. Once a sufficient amount of files has been 
 *        fed through this, the caches no longer contain cached 
 *        file data that will be used. The amount of data pumped
 *        through here is automatically scaled so that it will 
 *        break the caches.
 *
 * @param file_ptr : Used to point at a particular file object.
 */
/*
 * __doc__
 * __doc__  Function : void purge_file(struct file_object *fp, char *fn)
 * __doc__  Arguments: struct file_object *fp: Used to point at a particular 
 * __doc__             file.
 * __doc__  Returns  : void
 * __doc__  Performs : This function reads a file so that cache space is 
 * __doc__             consumed. Once a sufficient amount of files has been 
 * __doc__             fed through this, the caches no longer contain cached 
 * __doc__             file data that will be used. The amount of data pumped
 * __doc__             through here is automatically scaled so that it will 
 * __doc__             break the caches.
 * __doc__
 */
void
purge_file (struct file_object *file_ptr)
{
    int my_delta_time;
    struct netmist_vfs_object *fd = NULL;
    int err;
    char *buf;
    unsigned long long i;
    unsigned int trans;
    uint64_t tlen = 0;
    double pre_time = 0;
    double post_time = 0;
    int nap_time = 0;

    trans = max_xfer_size;
    buf = main_buf;

    err = VFS (open, file_ptr->dir, file_ptr->relfilename,
	       NETMIST_OPEN_CREATE, &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR, "Open for read failed. File %s in %s : %s\n",
		  file_ptr->relfilename, VFS (pathptr, file_ptr->dir), nes);
	R_exit (NETMIST_OP_VFS_ERR + 6, err);
	return;
    }

    for (i = 0; i < (file_ptr->Original_fsize * 1024); i += (trans))
    {
	if (init_phase)
	    pre_time = gettime ();

	err = netmist_try_large_read (fd, buf, trans, &tlen);
	if (err)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_EXEC,
		      "Purge_file read op failed file %s in %s: "
		      " Error %s, "
		      "Error value %d Ret %" PRIu64 "\n",
		      file_ptr->relfilename,
		      VFS (pathptr, file_ptr->dir), nes, err, tlen);
	    VFS (close, &fd);
	    R_exit (NETMIST_OP_VFS_ERR + 7, err);
	}

	/*-----------------*/
	/* heartbeat       */
	/*-----------------*/
	cur_time = gettime ();
	/* 
	 * Need to deliver heartbeats from every proc and filter 
	 * at the Prime.
	 */
	my_delta_time = (int) (cur_time - last_heart_beat);

	if (in_validate)
	{
	    if (cmdline.heartbeat_flag && (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }
	}
	if (init_phase)
	{
	    if (cmdline.heartbeat_flag && (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }
	}
	/*-----------------*/
	if (init_phase)
	{
	    post_time = gettime ();
	    total_init_time += (post_time - pre_time);
	    total_init_kbytes += (max_xfer_size / 1024);
	    avg_init_mb_per_sec =
		(total_init_kbytes / 1024) / total_init_time;
	}

	if (init_phase)
	{
	    if (my_mix_table[my_workload].init_rate_enable == 1)
	    {
		/*
		 * Throttle back on I/O rate, if needed. Don't swamp the filer 
		 */
		global_reset_counter++;
		/* Every 1000 I/O ops, lets restart the average  */
		/* calculation. We need to use a large sample because */
		/* the nap() resolution is in milliseconds.      */
		if (global_reset_counter == 1000)
		{
		    global_reset_counter = 0;
		    total_init_time = 0.0;
		    total_init_kbytes = 0;
		    avg_init_mb_per_sec = 0.0;
		}
		total_init_time += (post_time - pre_time);
		total_init_kbytes += (max_xfer_size / 1024);
		avg_init_mb_per_sec =
		    (total_init_kbytes / 1024) / total_init_time;

		if ((my_mix_table[my_workload].init_rate_speed != 0.0)
		    && ((avg_init_mb_per_sec) >
			my_mix_table[my_workload].init_rate_speed))
		{
		    if (nap_time == 0)
			nap_time += 1;
		    nap (nap_time);
		    total_init_time += (float) nap_time / (float) 1000;
		}
		avg_init_mb_per_sec =
		    (total_init_kbytes / 1024) / total_init_time;
		if ((my_mix_table[my_workload].init_rate_speed != 0.0)
		    && ((avg_init_mb_per_sec) >
			my_mix_table[my_workload].init_rate_speed))
		    nap_time = nap_time * 2;
		if ((my_mix_table[my_workload].init_rate_speed != 0.0)
		    && ((avg_init_mb_per_sec) <
			my_mix_table[my_workload].init_rate_speed)
		    && (nap_time != 0))
		    nap_time = nap_time / 2;
	    }
	}
    }
    if (init_phase)
    {
	global_reset_counter = 0;
	total_init_time = 0.0;
	total_init_kbytes = 0;
	avg_init_mb_per_sec = 0.0;
    }
    VFS (close, &fd);
    return;
}

/* 
 * __doc__
 * __doc__  Function : void generic_arm_notify(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : With Linux, we use inotify_init(), inotify_add_watch(),
 * __doc__             and read from the event queue.
 * __doc__
 */

void
generic_arm_notify (struct file_object *fp)
{
    int res = 0;
    log_file (LOG_DEBUG, "Doing notify op\n");

    res = VFS (wwatch, fp->dir, fp->relfilename, &fp->vfs_notify);
    if (res == 0)
	guard_band++;
}

/*
 * __doc__
 * __doc__  Function : void generic_arm_cancel(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : Disable the watch. Linux version.
 * __note__          : For Linux, we just read() from the event queue.
 * __doc__
 */
void
generic_arm_cancel (struct file_object *fp)
{
    int res;

    if (fp->vfs_notify == NULL)
    {
	log_file (LOG_DEBUG, "generic_arm_cancel NULL wait object\n");
	return;
    }

    log_file (LOG_DEBUG, "Doing read_and_cancel_notification op\n");
    res = VFS (wwait, &fp->vfs_notify);
    if (res == 0)
    {
	log_file (LOG_DEBUG, "Got event\n");
    }
    else
    {
	log_file (LOG_DEBUG, "Got event failed; return is %d\n", res);
    }

    guard_band--;		/* We just released a file descriptor */
}

/**
 * @brief This is the code that implements the "write" op. It will
 *          write a file with the name that is in the test entry and 
 *          collect statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_write(struct file_object *fp)
 * __doc__  Arguments: struct file_object *fp: Points at a particular file.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "write" op. It will
 * __doc__             write a file with the name that is in the test entry and 
 * __doc__             collect statistics for the operation.
 * __doc__
 */
void
generic_write (struct file_object *fp)
{
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
    uint64_t tlen = 0;
    uint64_t trans;
    enum netmist_vfs_open_flags flags = 0;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing write op\n");

    entry_time = gettime ();
    buf = main_buf;

    set_direct_flag (work_obj[my_workload].write_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].write_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    file_accesses++;
    fd_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags) && (fp->fd_link)
	&& (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1;	/* open */
	work_obj[my_workload].total_file_ops += 1;	/* The open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for generic write op failed. Filename = %s in %s Error: %s Error Value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}

	if (fp->fd_link != NULL)
	{
	    log_file (LOG_EXEC, "generic_write: Internal error: fd_link.\n");
	}
	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest */
	    if (fd_cache_reuse () == 1)
	    {
		work_obj[my_workload].close_op_count++;
		total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].write_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);
    adjust_size_direct (fp->file_desc, 0, &fp->seqwrite_offset);
    if (my_mix_table[my_workload].align)
    {
	fp->seqwrite_offset =
	    fp->seqwrite_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->seqwrite_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->seqwrite_offset = cmdline.gg_offset;
	else
	    fp->seqwrite_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->seqwrite_offset, NETMIST_SEEK_SET, NULL);

    /* Select write size to use */
    trans = get_next_write_size ();

    adjust_size_direct (fp->file_desc, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->seqwrite_offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = netmist_try_large_write (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Write failed to file %s in %s Error: %s Error Eval %d Ret 0x%"
		  PRIx64 " Flags 0x%x Trans %x Offset 0x%llx\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err, tlen,
		  flags, trans, (unsigned long long) (fp->seqwrite_offset));

	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 10, err);
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->seqwrite_offset);
    if ((fp->seqwrite_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->seqwrite_offset = 0;
    fp->op_count++;

    /* global (across all files in this work object) write count */
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;	/* Write */

    work_obj[my_workload].write_op_count++;

    if (cmdline.flush_flag &&
	((work_obj[my_workload].write_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }

    total_write_bytes += (double) (tlen);
    /* 
     * After writes are done then flush to make sure the 
     * client cache is not creating an effect.
     */
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fp->file_desc);	/* Also helps make local filesystem testing act
					 * like NFS */
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].write_op_time += lat;
	if ((lat < work_obj[my_workload].min_write_op_latency)
	    || (work_obj[my_workload].min_write_op_latency == 0.0))
	    work_obj[my_workload].min_write_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_op_latency)
	    || (work_obj[my_workload].max_write_op_latency == 0.0))
	    work_obj[my_workload].max_write_op_latency = lat;
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
}

/**
 * @brief This is the code that implements the "write" op for 
 *        Block devices.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_bwrite(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular device.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "write" op for 
 * __doc__             Block devices.
 * __doc__
 */
void
generic_bwrite (struct file_object *fp)
{
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
    uint64_t tlen = 0;
    uint64_t trans;
    enum netmist_vfs_open_flags flags = 0;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing write op for block\n");

    entry_time = gettime ();
    buf = main_buf;

    set_direct_flag (work_obj[my_workload].write_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].write_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    file_accesses++;
    fd_accesses++;

    if ((fp->file_desc != 0) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_WRITE | flags,
		   &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for generic bwrite op failed. Filename = %s in %s Error: %s Error Value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		work_obj[my_workload].close_op_count++;
		total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }				/* thus ends the opening of the file */

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].write_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    adjust_size_direct (fp->file_desc, 0, &fp->seqwrite_offset);
    if (my_mix_table[my_workload].align)
    {
	fp->seqwrite_offset =
	    fp->seqwrite_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->seqwrite_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->seqwrite_offset = cmdline.gg_offset;
	else
	    fp->seqwrite_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->seqwrite_offset, NETMIST_SEEK_SET, NULL);

    /* Select write size to use */
    trans = get_next_write_size ();

    adjust_size_direct (fp->file_desc, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->seqwrite_offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = netmist_try_large_write (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Write failed to file %s in %s Error: %s Error Eval %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, tlen);
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 10, err);
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->seqwrite_offset);
    if ((fp->seqwrite_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->seqwrite_offset = 0;
    fp->op_count++;

    /* global (across all files in this work object) write count */
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;	/* Write */

    work_obj[my_workload].write_op_count++;



/*
	if ( cmdline.flush_flag && ((fp->op_count % 3) == 0)) 
*/

    if (cmdline.flush_flag &&
	((work_obj[my_workload].write_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }

    total_write_bytes += (double) (tlen);
    /* 
     * After writes are done then flush to make sure the 
     * client cache is not creating an effect.
     */
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fp->file_desc);	/* Also helps make local filesystem testing act
					 * like NFS */
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].write_op_time += lat;
	if ((lat < work_obj[my_workload].min_write_op_latency)
	    || (work_obj[my_workload].min_write_op_latency == 0.0))
	    work_obj[my_workload].min_write_op_latency = lat;
	if ((lat > work_obj[my_workload].max_write_op_latency)
	    || (work_obj[my_workload].max_write_op_latency == 0.0))
	    work_obj[my_workload].max_write_op_latency = lat;
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }

    /*free(buf); */
}

/**
 * @brief  This is the code that implements the "write file" op.
 *         It will write a whole file with the name that is in
 *         the test entry and collect statistics for the operation.
 *
 * @param fp : potiner to the file_object for a particular file. 
 *
 */
/*
 * __doc__
 * __doc__  Function : void generic_write_file(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "write file" op. 
 * __doc__             It will write a whole file with the name that is in 
 * __doc__             the test entry and collect statistics for the operation.
 * __doc__
 */
void
generic_write_file (struct file_object *fp)
{
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
    uint64_t tlen = 0;
    uint64_t trans;
    unsigned long long transferred = 0;
    enum netmist_vfs_open_flags flags = 0;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing write file op\n");

    entry_time = gettime ();
    if (big_buf == 0)
    {
	big_buf = (char *) MALLOC ((1024 * 1024) + 8192);
	big_buf = (char *) (((intptr_t) big_buf + 4096) & ~4095);
    }
    buf = big_buf;

    set_direct_flag (work_obj[my_workload].write_file_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].write_file_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;
    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for generic write file op failed. Filename = %s in %s Error: %s Error Value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);

	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].write_file_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select write size to use */
    trans = max_xfer_size;	/* Trans and max_xfer_size are in bytes */
    if ((unsigned long long) (max_xfer_size / 1024) > fp->Original_fsize)
	trans = fp->Original_fsize * 1024;

    if ((unsigned long long) (fp->wholefilewrite_offset + trans) >=
	(fp->Original_fsize * 1024LL))
	fp->wholefilewrite_offset = 0;
    adjust_size_direct (fp->file_desc, 0, &fp->wholefilewrite_offset);
    if (my_mix_table[my_workload].align)
    {
	fp->wholefilewrite_offset =
	    fp->wholefilewrite_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->wholefilewrite_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->wholefilewrite_offset = cmdline.gg_offset;
	else
	    fp->wholefilewrite_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->wholefilewrite_offset, NETMIST_SEEK_SET,
	 NULL);

    adjust_size_direct (fp->file_desc, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }
    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    while (transferred < trans)
    {
	if (my_mix_table[my_workload].patt_vers == 1)
	{
	    make_non_dedup ((long *) buf, (int) trans);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) trans);
	}
	else
	{
	    make_data_layout ((long *) buf, trans, fp,
			      fp->wholefilewrite_offset, this_dedup_group,
			      dedup_across);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) trans);
	}

	err = netmist_try_large_write (fp->file_desc, buf, trans, &tlen);
	if (err)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Write file: Write failed to file %s in %s Error: %s Eval %d Ret %"
		      PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir),
		      nes, err, tlen);
	    fd_drop_from_cache (fp);	/* includes close */
	    R_exit (NETMIST_OP_VFS_ERR + 10, err);
	}

	VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR,
	     &fp->wholefilewrite_offset);
	if ((fp->wholefilewrite_offset / (unsigned long long) 1024) >=
	    (fp->Original_fsize))
	{
	    fp->wholefilewrite_offset = 0;
	    VFS (seek, fp->file_desc, fp->wholefilewrite_offset,
		 NETMIST_SEEK_SET, NULL);
	}

	transferred += tlen;
    }
    fp->op_count++;		/* for this file's personal op count */
    /* global (across all files in this work object) write count */
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;	/* Total op count */
    work_obj[my_workload].write_file_op_count++;	/* Write file op count */

    if (cmdline.flush_flag &&
	((work_obj[my_workload].write_file_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }

    total_write_bytes += (double) (transferred);
    /* 
     * After writes are done then flush to make sure the 
     * client cache is not creating an effect.
     */
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fp->file_desc);	/* Also helps make local filesystem testing act
					 * like NFS */
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].write_file_op_time += lat;
	if ((lat < work_obj[my_workload].min_write_file_op_latency)
	    || (work_obj[my_workload].min_write_file_op_latency == 0.0))
	    work_obj[my_workload].min_write_file_op_latency = lat;
	if ((lat > work_obj[my_workload].max_write_file_op_latency)
	    || (work_obj[my_workload].max_write_file_op_latency == 0.0))
	    work_obj[my_workload].max_write_file_op_latency = lat;
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
    /*free(buf); */
}



/**
 * @brief This is the code that implements the 
 *        "write rand file" op. It will write a file with 
 *        the name that is in the test entry and collect 
 *        statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_arm_cancel(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the 
 * __doc__             "write rand file" op. It will write a file with 
 * __doc__             the name that is in the test entry and collect 
 * __doc__             statistics for the operation.
 * __doc__
 */
void
generic_write_rand (struct file_object *fp)
{
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
#if defined(_LARGEFILE64_SOURCE) || defined(WIN32)
    off64_t where;
#else
    off_t where;
#endif
    enum netmist_vfs_open_flags flags = 0;
    uint64_t tlen = 0;
    uint64_t trans;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing rand write op\n");

    entry_time = gettime ();
    min_acc_per_spot = my_mix_table[my_workload].min_acc_per_spot;
    if (min_acc_per_spot <= 0)
	min_acc_per_spot = 1;

    spot_pct = my_mix_table[my_workload].percent_per_spot;
    if (spot_pct <= 0)
	spot_pct = 1;
    nspots = (100 / spot_pct);
    if (nspots < 1)
	nspots = 1;

    buf = main_buf;
    set_direct_flag (work_obj[my_workload].write_rand_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].write_rand_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for rand write op failed. Error value = %d, Filename = %s in %s Error: %s\n",
		      err, fp->relfilename, VFS (pathptr, fp->dir), nes);
	    R_exit (NETMIST_OP_VFS_ERR + 11, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].write_rand_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select write size to use */
    trans = get_next_write_size ();

    adjust_size_direct (fp->file_desc, 1, &trans);

    /* Default to uniform random */
    if (fp->Original_fsize == 1)
	where = 0;
    else
    {
	where = (netmist_llrand () % (fp->Original_fsize - 1));
	where = where & ~7;	/* Where is in Kbytes */
    }

    /* Uniform Random access */
    if (my_mix_table[my_workload].rand_dist_behavior == UNIFORM_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where =
		geometric_rand (fp->Original_fsize -
				1) % (fp->Original_fsize - 1);
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_SPOT_RAND)
    {
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
	/* Pick random offset within the file on 8k boundary */

	spot_size =
	    (fp->Original_fsize * (long long) spot_pct) / (long long) 100;
	if (spot_size <= 0)
	    spot_size = (long long) 1;

	/* Default 5 times as many 8k records as fits in a spot */
	acc_mult_spot = my_mix_table[my_workload].acc_mult_spot;
	if (acc_mult_spot <= 0)
	    acc_mult_spot = 1;
	hit_in_spot = (spot_size / 8) * acc_mult_spot;

	if ((hit_in_spot < min_acc_per_spot) || (hit_in_spot == 0))
	    hit_in_spot = min_acc_per_spot;
	if (hit_in_spot == 0)
	    hit_in_spot = 1;

	/* 
	 * Special logic for workloads that use "affinity", dividing into 
	 * spots for geometric, and jumping out of geometric PCT percent 
	 * of the time.
	 */
	if (my_mix_table[my_workload].percent_affinity != 0)
	{
	    if (((fp->db_access_count) % hit_in_spot) == 0)
		fp->which_spot =
		    (long long) (geometric_rand (nspots) % nspots);
	    /* a bit of jitter */
	    fp->which_spot = picker (fp->which_spot, nspots);
	    fp->db_access_count++;
	}
	else
	{
	    fp->which_spot = (long long) (geometric_rand (nspots) % nspots);
	}

	/* Geometric offsets within spot */
	if (my_mix_table[my_workload].spot_shape == GEOMETRIC_RAND_SPOT)
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = geometric_rand (spot_size) % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	    where = where & ~7;	/* Where is in Kbytes */
	}

	/* Uniform random offsets within spot */
	if ((my_mix_table[my_workload].spot_shape == UNIFORM_RAND_SPOT) ||
	    (my_mix_table[my_workload].spot_shape != GEOMETRIC_RAND_SPOT))
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = netmist_llrand () % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	}

	where = where & ~7;	/* Where is in Kbytes */

	log_file (LOG_DEBUG,
		  "%lld Offset %lld Spsize %lld Whichsp %lld fCount \n",
		  (long long) where, spot_size,
		  (long long) (fp->which_spot),
		  (long long) (fp->db_access_count));

    }
    /* If it is beyond eof, start over */
    if (where + (trans / (unsigned long long) 1024) > (fp->Original_fsize))
	where = 0;
    fp->offset = (where * 1024LL);

    adjust_size_direct (fp->file_desc, 0, &fp->offset);
    if (my_mix_table[my_workload].align)
    {
	fp->offset =
	    fp->offset & (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->offset = cmdline.gg_offset;
	else
	    fp->offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->offset, NETMIST_SEEK_SET, NULL);

    adjust_size_direct (fp->file_desc, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = netmist_try_large_write (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Write rand failed to file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 " Trans %d Flags %x Offset %lld\n", fp->relfilename,
		  VFS (pathptr, fp->dir), nes, err, tlen, trans, flags,
		  (unsigned long long) (fp->offset));
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 12, err);
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->offset);
    fp->op_count++;		/* write in file counter */

    /* global (across all files in this work object) write count */
    work_obj[my_workload].write_rand_op_count++;

    if (cmdline.flush_flag &&
	((work_obj[my_workload].write_rand_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }
    total_write_rand_bytes += (double) (tlen);

    total_write_bytes += (double) (tlen);

    total_file_ops += 1.0;	/* Write */
    work_obj[my_workload].total_file_ops += 1.0;	/* Write */
    /* 
     * After writes are done then flush to make sure the 
     * client cache is not creating an effect.
     */
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fp->file_desc);	/* Also helps make local filesystem testing act
					 * like NFS */
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].write_rand_op_time += lat;
	if ((lat < work_obj[my_workload].min_write_rand_op_latency)
	    || (work_obj[my_workload].min_write_rand_op_latency == 0.0))
	    work_obj[my_workload].min_write_rand_op_latency = lat;
	if ((lat > work_obj[my_workload].max_write_rand_op_latency)
	    || (work_obj[my_workload].max_write_rand_op_latency == 0.0))
	    work_obj[my_workload].max_write_rand_op_latency = lat;
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }

    /*free(buf); */
}

/**
 * @brief This is the code that implements the "write brand dev" 
 *        op. Used for block devices only.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_bwrite_rand(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "write brand dev" 
 * __doc__             op. Used for block devices only.
 * __doc__
 */
void
generic_bwrite_rand (struct file_object *fp)
{
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
#if defined(_LARGEFILE64_SOURCE) || defined(WIN32)
    off64_t where;
#else
    off_t where;
#endif
    enum netmist_vfs_open_flags flags = 0;
    uint64_t tlen = 0;
    uint64_t trans;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing brand write op\n");

    entry_time = gettime ();
    min_acc_per_spot = my_mix_table[my_workload].min_acc_per_spot;
    if (min_acc_per_spot <= 0)
	min_acc_per_spot = 1;
    spot_pct = my_mix_table[my_workload].percent_per_spot;
    if (spot_pct <= 0)
	spot_pct = 1;
    nspots = (100 / spot_pct);
    if (nspots < 1)
	nspots = 1;

    buf = main_buf;
    set_direct_flag (work_obj[my_workload].write_rand_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].write_rand_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_WRITE | flags,
		   &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for rand write op failed. Error value = %d, Filename = %s in %s Error: %s\n",
		      err, fp->relfilename, VFS (pathptr, fp->dir), nes);
	    R_exit (NETMIST_OP_VFS_ERR + 11, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].write_rand_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select write size to use */
    trans = get_next_write_size ();

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    /* Default to uniform random */
    if (fp->Original_fsize == 1)
	where = 0;
    else
    {
	where = (netmist_llrand () % (fp->Original_fsize - 1));
	where = where & ~7;	/* Where is in Kbytes */
    }

    /* Uniform Random access */
    if (my_mix_table[my_workload].rand_dist_behavior == UNIFORM_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where =
		geometric_rand (fp->Original_fsize -
				1) % (fp->Original_fsize - 1);
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_SPOT_RAND)
    {
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
	/* Pick random offset within the file on 8k boundary */

	spot_size =
	    (fp->Original_fsize * (long long) spot_pct) / (long long) 100;
	if (spot_size <= 0)
	    spot_size = (long long) 1;

	/* Default 5 times as many 8k records as fits in a spot */
	acc_mult_spot = my_mix_table[my_workload].acc_mult_spot;
	if (acc_mult_spot <= 0)
	    acc_mult_spot = 1;
	hit_in_spot = (spot_size / 8) * acc_mult_spot;

	if ((hit_in_spot < min_acc_per_spot) || (hit_in_spot == 0))
	    hit_in_spot = min_acc_per_spot;
	if (hit_in_spot == 0)
	    hit_in_spot = 1;

	/* 
	 * Special logic for workloads that use "affinity", dividing into 
	 * spots for geometric, and jumping out of geometric PCT percent of 
	 * the time.
	 */
	if (my_mix_table[my_workload].percent_affinity != 0)
	{
	    if (((fp->db_access_count) % hit_in_spot) == 0)
		fp->which_spot =
		    (long long) (geometric_rand (nspots) % nspots);
	    /* a bit of jitter */
	    fp->which_spot = picker (fp->which_spot, nspots);
	    fp->db_access_count++;
	}
	else
	{
	    fp->which_spot = (long long) (geometric_rand (nspots) % nspots);
	}

	/* Geometric offsets within spot */
	if (my_mix_table[my_workload].spot_shape == GEOMETRIC_RAND_SPOT)
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = geometric_rand (spot_size) % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	    where = where & ~7;	/* Where is in Kbytes */
	}

	/* Uniform random offsets within spot */
	if ((my_mix_table[my_workload].spot_shape == UNIFORM_RAND_SPOT) ||
	    (my_mix_table[my_workload].spot_shape != GEOMETRIC_RAND_SPOT))
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = netmist_llrand () % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	}
	where = where & ~7;	/* Where is in Kbytes */

	log_file (LOG_DEBUG,
		  "%lld Offset %lld Spsize %lld Whichsp %lld fCount \n",
		  (long long) where, spot_size,
		  (long long) (fp->which_spot),
		  (long long) (fp->db_access_count));

    }

    /* If it is beyond eof, start over */
    if (where + (trans / (unsigned long long) 1024) > (fp->Original_fsize))
	where = 0;
    fp->offset = (where * 1024);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->offset);
    if (my_mix_table[my_workload].align)
    {
	fp->offset =
	    fp->offset & (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->offset = cmdline.gg_offset;
	else
	    fp->offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->offset, NETMIST_SEEK_SET, NULL);

    /* align min size for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = netmist_try_large_write (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Write rand failed to file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, tlen);
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 12, err);
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->offset);
    fp->op_count++;		/* write in file counter */

    /* global (across all files in this work object) write count */
    work_obj[my_workload].write_rand_op_count++;

    if (cmdline.flush_flag &&
	((work_obj[my_workload].write_rand_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }
    total_write_rand_bytes += (double) (tlen);

    total_write_bytes += (double) (tlen);

    total_file_ops += 1.0;	/* Write */
    work_obj[my_workload].total_file_ops += 1.0;	/* Write */
    /* 
     * After writes are done then flush to make sure the 
     * client cache is not creating an effect.
     */
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fp->file_desc);	/* Also helps make local filesystem testing act
					 * like NFS */
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].write_rand_op_time += lat;
	if ((lat < work_obj[my_workload].min_write_rand_op_latency)
	    || (work_obj[my_workload].min_write_rand_op_latency == 0.0))
	    work_obj[my_workload].min_write_rand_op_latency = lat;
	if ((lat > work_obj[my_workload].max_write_rand_op_latency)
	    || (work_obj[my_workload].max_write_rand_op_latency == 0.0))
	    work_obj[my_workload].max_write_rand_op_latency = lat;
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }

    /*free(buf); */
}

/**
 * @brief This is the code that implements the "read" op. It will
 *        read a file with the name that is in the test entry and 
 *        collect statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_read(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "read" op. It will
 * __doc__             read a file with the name that is in the test entry and 
 * __doc__             collect statistics for the operation.
 * __doc__
 */
void
generic_read (struct file_object *fp)
{
    int err;
    char *buf;
    uint64_t trans;
    uint64_t tlen = 0;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing read op\n");

    entry_time = gettime ();
    buf = main_buf;
    purge_cache ();

    set_direct_flag (work_obj[my_workload].read_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq (probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand (probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need (probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].read_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags) && (fp->fd_link)
	&& (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for read op failed. Filename = %s in %s Error: %s Error value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 13, err);
	}

	if (fp->fd_link != NULL)
	{
	    log_file (LOG_EXEC, "generic_read: Internal error: fd_link.\n");

	}
	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].read_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select write size to use */
    trans = get_next_read_size ();

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->seqread_offset);

    if ((fp->seqread_offset / 1024LL) + (trans / (unsigned long long) 1024) >
	(fp->Original_fsize))
	fp->seqread_offset = 0;

    if (my_mix_table[my_workload].align)
    {
	fp->seqread_offset =
	    fp->seqread_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }

    if (cmdline.gg_flag)
    {
	if ((fp->seqread_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->seqread_offset = cmdline.gg_offset;
	else
	    fp->seqread_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->seqread_offset, NETMIST_SEEK_SET, NULL);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    err = netmist_try_large_read (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_EXEC,
		  "Generic read op failed file %s in %s: Error %s, Error value %d Ret %d Flags 0x%x Trans 0x%x Offset 0x%llx\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err, flags,
		  trans, fp->seqread_offset);

	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 14, err);
    }

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].read_op_time += lat;
	if ((lat < work_obj[my_workload].min_read_op_latency)
	    || (work_obj[my_workload].min_read_op_latency == 0.0))
	    work_obj[my_workload].min_read_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_op_latency)
	    || (work_obj[my_workload].max_read_op_latency == 0.0))
	    work_obj[my_workload].max_read_op_latency = lat;
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->seqread_offset);
    if ((fp->seqread_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->seqread_offset = 0;
    total_read_bytes += (double) (tlen);
    total_file_ops += 1.0;	/* Read */
    work_obj[my_workload].total_file_ops += 1.0;	/* Read */
    fp->op_count++;
    /* global (across all files in this work object) read count */
    work_obj[my_workload].read_op_count++;
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
    /*free(buf); */
}

/**
 * @brief This is the code that implements the "read" op for 
 *        Block devices. 
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_bread(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a device
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "read" op for 
 * __doc__             Block devices. 
 * __doc__
 */
void
generic_bread (struct file_object *fp)
{
    int err;
    char *buf;
    uint64_t trans;
    uint64_t tlen = 0;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing read op for Block\n");

    entry_time = gettime ();
    buf = main_buf;
    purge_cache ();
    set_direct_flag (work_obj[my_workload].read_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].read_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_WRITE | flags,
		   &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for bread op failed. Filename = %s in %s Error: %s Error value: %d\n",
		      VFS (pathptr, fp->dir), fp->relfilename, nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 13, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].read_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select write size to use */
    trans = get_next_read_size ();

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->seqread_offset);

    if ((fp->seqread_offset / 1024LL) + (trans / (unsigned long long) 1024) >
	(fp->Original_fsize))
	fp->seqread_offset = 0;

    if (my_mix_table[my_workload].align)
    {
	fp->seqread_offset =
	    fp->seqread_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }

    if (cmdline.gg_flag)
    {
	if ((fp->seqread_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->seqread_offset = cmdline.gg_offset;
	else
	    fp->seqread_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->seqread_offset, NETMIST_SEEK_SET, NULL);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    err = netmist_try_large_read (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_EXEC,
		  "Generic read op failed file %s in %s: Error %s,"
		  " Error value %d Ret %" PRIu64 "\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err, tlen);
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 14, err);
    }

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].read_op_time += lat;
	if ((lat < work_obj[my_workload].min_read_op_latency)
	    || (work_obj[my_workload].min_read_op_latency == 0.0))
	    work_obj[my_workload].min_read_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_op_latency)
	    || (work_obj[my_workload].max_read_op_latency == 0.0))
	    work_obj[my_workload].max_read_op_latency = lat;
    }

    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->seqread_offset);
    if ((fp->seqread_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->seqread_offset = 0;
    total_read_bytes += (double) (tlen);
    total_file_ops += 1.0;	/* Read */
    work_obj[my_workload].total_file_ops += 1.0;	/* Read */
    fp->op_count++;
    /* global (across all files in this work object) read count */
    work_obj[my_workload].read_op_count++;
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
    /*free(buf); */
}

/**
 * @brief This is the code that implements the "read whole file" 
 *        op. It will read a file with the name that is in the 
 *        test entry and collect statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_read_file(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "read whole file" 
 * __doc__             op. It will read a file with the name that is in the 
 * __doc__             test entry and collect statistics for the operation.
 * __doc__
 */
void
generic_read_file (struct file_object *fp)
{
    int err;
    char *buf;
    uint64_t trans;
    unsigned long long transferred = 0;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing read file op\n");

    entry_time = gettime ();
    if (big_buf == 0)
    {
	big_buf = (char *) MALLOC ((1024 * 1024) + 4096);
	big_buf = (char *) (((intptr_t) big_buf + 4096) & ~4095);
    }
    buf = big_buf;
    purge_cache ();
    set_direct_flag (work_obj[my_workload].read_file_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].read_file_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);


    trans = 1024 * 1024;	/* One Meg transfers */

    if ((unsigned long long) (fp->wholefileread_offset / 1024LL) >=
	fp->Original_fsize)
	fp->wholefileread_offset = 0;
    /*
     * If trans is larger than remaining data in the file.
     **/
    if (trans >= ((fp->Original_fsize * 1024) - fp->wholefileread_offset))
	trans =
	    (fp->Original_fsize -
	     (fp->wholefileread_offset / 1024LL)) * 1024LL;

    fd_accesses++;
    file_accesses++;
    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for read file op failed. Filename = %s in %s Error: %s Error value: %d \n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 13, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].read_file_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Seek to last known wholefileread_offset */
    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->wholefileread_offset);

    if ((fp->wholefileread_offset / 1024LL) +
	(trans / (unsigned long long) 1024) > (fp->Original_fsize))
	fp->wholefileread_offset = 0;

    if (my_mix_table[my_workload].align)
    {
	fp->wholefileread_offset =
	    fp->wholefileread_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }

    if (cmdline.gg_flag)
    {
	if ((fp->wholefileread_offset +
	     cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->wholefileread_offset = cmdline.gg_offset;
	else
	    fp->wholefileread_offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->wholefileread_offset, NETMIST_SEEK_SET,
	 NULL);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    while (transferred < trans)
    {
	uint64_t tlen = 0;

	err = netmist_try_large_read (fp->file_desc, buf, trans, &tlen);
	if (err)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Read file op failed file %s in %s Error: %s Error value %d Ret %"
		      PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir),
		      nes, err, tlen);
	    fd_drop_from_cache (fp);	/* includes close */
	    R_exit (NETMIST_OP_VFS_ERR + 14, err);
	}

	if (tlen < (uint64_t) trans)
	{
	    fp->wholefileread_offset = 0LL;
	    VFS (seek, fp->file_desc, fp->wholefileread_offset,
		 NETMIST_SEEK_SET, NULL);
	    total_read_bytes += (double) (tlen);
	    transferred += tlen;
	    break;
	}
	/*
	 *  Save the concurency.
	 */
	VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR,
	     &fp->wholefileread_offset);
	total_read_bytes += (double) (tlen);
	transferred += tlen;
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].read_file_op_time += lat;
	if ((lat < work_obj[my_workload].min_read_file_op_latency)
	    || (work_obj[my_workload].min_read_file_op_latency == 0.0))
	    work_obj[my_workload].min_read_file_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_file_op_latency)
	    || (work_obj[my_workload].max_read_file_op_latency == 0.0))
	    work_obj[my_workload].max_read_file_op_latency = lat;
    }
    total_file_ops += 1.0;	/* Read file op */
    work_obj[my_workload].total_file_ops += 1.0;	/* total ops */
    fp->op_count++;		/* Read file op count for this file */
    work_obj[my_workload].read_file_op_count++;	/* Global read_file op count */
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
    /* free(buf); */
}


/**
 * @brief This is the code that implements the "read rand file" 
 *        op. It will read a file with the name that is in the 
 *        test entry and collect statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_read_rand(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "read rand file" 
 * __doc__             op. It will read a file with the name that is in the 
 * __doc__             test entry and collect statistics for the operation.
 * __doc__
 */
void
generic_read_rand (struct file_object *fp)
{
    char *buf;
    int err;
#if defined(_LARGEFILE64_SOURCE) || defined(WIN32)
    off64_t where;
#else
    off_t where;
#endif
    uint64_t trans;
    uint64_t tlen = 0;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing read rand op\n");

    entry_time = gettime ();
    min_acc_per_spot = my_mix_table[my_workload].min_acc_per_spot;
    if (min_acc_per_spot <= 0)
	min_acc_per_spot = 1;
    spot_pct = my_mix_table[my_workload].percent_per_spot;
    if (spot_pct <= 0)
	spot_pct = 1;
    nspots = (100 / spot_pct);
    if (nspots < 1)
	nspots = 1;
    buf = main_buf;
    purge_cache ();
    set_direct_flag (work_obj[my_workload].read_rand_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].read_rand_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for read rand op failed. Error value = %d, Filename = %s in %s Error: %s\n",
		      err, fp->relfilename, VFS (pathptr, fp->dir), nes);
	    R_exit (NETMIST_OP_VFS_ERR + 15, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].read_rand_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select read size to use */
    trans = get_next_read_size ();

    /* Default to uniform random */
    if (fp->Original_fsize == 1)
	where = 0;
    else
    {
	where = (netmist_llrand () % (fp->Original_fsize - 1));
	where = where & ~7;	/* Where is in Kbytes */
    }

    /* Uniform Random access */
    if (my_mix_table[my_workload].rand_dist_behavior == UNIFORM_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where =
		geometric_rand (fp->Original_fsize -
				1) % (fp->Original_fsize - 1);
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_SPOT_RAND)
    {
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
	/* Pick random offset within the file on 8k boundary */

	spot_size =
	    (fp->Original_fsize * (long long) spot_pct) / (long long) 100;
	if (spot_size <= 0)
	    spot_size = (long long) 1;

	/* Default 5 times as many 8k records as fits in a spot */
	acc_mult_spot = my_mix_table[my_workload].acc_mult_spot;
	if (acc_mult_spot <= 0)
	    acc_mult_spot = 1;
	hit_in_spot = (spot_size / 8) * acc_mult_spot;

	if ((hit_in_spot < min_acc_per_spot) || (hit_in_spot == 0))
	    hit_in_spot = min_acc_per_spot;
	if (hit_in_spot == 0)
	    hit_in_spot = 1;

	/* 
	 * Special logic for workloads that use "affinity", dividing into 
	 * spots for geometric, and jumping out of geometric PCT percent 
	 * of the time.
	 */
	if (my_mix_table[my_workload].percent_affinity != 0)
	{
	    if (((fp->db_access_count) % hit_in_spot) == 0)
		fp->which_spot =
		    (long long) (geometric_rand (nspots) % nspots);
	    /* a bit of jitter */
	    fp->which_spot = picker (fp->which_spot, nspots);
	    fp->db_access_count++;
	}
	else
	{
	    fp->which_spot = (long long) (geometric_rand (nspots) % nspots);
	}

	/* Geometric offsets within spot */
	if (my_mix_table[my_workload].spot_shape == GEOMETRIC_RAND_SPOT)
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = geometric_rand (spot_size) % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	    where = where & ~7;	/* Where is in Kbytes */
	}

	/* Uniform random offsets within spot */
	if ((my_mix_table[my_workload].spot_shape == UNIFORM_RAND_SPOT) ||
	    (my_mix_table[my_workload].spot_shape != GEOMETRIC_RAND_SPOT))
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = netmist_llrand () % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	}
	where = where & ~7;	/* Where is in Kbytes */

	log_file (LOG_DEBUG,
		  "%lld Offset %lld Spsize %lld Whichsp %lld fCount \n",
		  (long long) where, spot_size,
		  (long long) (fp->which_spot),
		  (long long) (fp->db_access_count));

    }

    /* If it is beyond eof, start over */
    if (where + (trans / (unsigned long long) 1024) > (fp->Original_fsize))
	where = 0;
    fp->offset = (where * 1024);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->offset);
    if (my_mix_table[my_workload].align)
    {
	fp->offset =
	    fp->offset & (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->offset = cmdline.gg_offset;
	else
	    fp->offset += cmdline.gg_offset;
    }
    VFS (seek, fp->file_desc, fp->offset, NETMIST_SEEK_SET, NULL);
    /* align min size for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    err = netmist_try_large_read (fp->file_desc, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Read file rand op failed file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, tlen);
	/*free(buf); */
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 16, err);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].read_rand_op_time += lat;
	if ((lat < work_obj[my_workload].min_read_rand_op_latency)
	    || (work_obj[my_workload].min_read_rand_op_latency == 0.0))
	    work_obj[my_workload].min_read_rand_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_rand_op_latency)
	    || (work_obj[my_workload].max_read_rand_op_latency == 0.0))
	    work_obj[my_workload].max_read_rand_op_latency = lat;
    }

    total_read_bytes += (double) (tlen);
    total_read_rand_bytes += (double) (tlen);
    total_file_ops += 1.0;	/*Read */
    work_obj[my_workload].total_file_ops += 1.0;	/*Read */
    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->offset);

    fp->op_count++;
    work_obj[my_workload].read_rand_op_count++;
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
    /*free(buf); */
}

/**
 * @brief This is the code that implements the 
 *        "read modify write file" op. It will read a file with 
 *        the name that is in the test entry and collect statistics
 *        for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_rmw(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the 
 * __doc__             "read modify write file" op. It will read a file with 
 * __doc__             the name that is in the test entry and collect statistics
 * __doc__             for the operation.
 * __doc__
 */
void
generic_rmw (struct file_object *fp)
{
    int err;
    char *buf;
#if defined(_LARGEFILE64_SOURCE) || defined(WIN32)
    off64_t where;
#else
    off_t where;
#endif
    uint64_t read_ret = 0, write_ret = 0;
    uint64_t trans;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing rmw op\n");

    entry_time = gettime ();
    buf = main_buf;
    purge_cache ();
    set_direct_flag (work_obj[my_workload].rmw_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].rmw_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fp->file_desc);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for rmw op failed. Filename = %s in %s Error: %s Error Value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 15, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].rmw_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select read size to use */
    trans = get_next_read_size ();

    /* Pick random offset within the file on 8k boundary */
    if (fp->Original_fsize == 1)
	where = 0;
    else
	where = (netmist_llrand () % (fp->Original_fsize - 1));

    /* If it is beyond eof, start over */
    if ((unsigned long long) ((where * 1024) + trans) >
	(fp->Original_fsize * (unsigned long long) 1024))
	where = 0;

    /* align things for O_DIRECT */
    if (adjust_size_direct (fp->file_desc, 0, &fp->rmw_offset))
    {
	where = (fp->rmw_offset / (long long) 1024);	/* Save where for later 
							 * use by write. */
    }
    if (my_mix_table[my_workload].align)
    {
	fp->rmw_offset =
	    fp->rmw_offset &
	    (off64_t) (~(my_mix_table[my_workload].align - 1));
    }

    if (cmdline.gg_flag)
    {
	if ((fp->rmw_offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	{
	    fp->rmw_offset = cmdline.gg_offset;
	    /* Save where for later use by write. */
	    where = (fp->rmw_offset / (unsigned long long) 1024);
	}
	else
	{
	    fp->rmw_offset += cmdline.gg_offset;
	    /* Save where for later use by write. */
	    where = (fp->rmw_offset / (unsigned long long) 1024);
	}
    }
    VFS (seek, fp->file_desc, fp->rmw_offset, NETMIST_SEEK_SET, NULL);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    err = netmist_try_large_read (fp->file_desc, buf, trans, &read_ret);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Rmw file op failed file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, read_ret);
	/*free(buf); */
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 16, err);
    }

    total_rmw_bytes += (double) (read_ret);
    total_file_ops += 1.0;	/* Read */
    work_obj[my_workload].total_file_ops += 1.0;	/* Read */
    /*work_obj[my_workload].rmw_op_count++; */
    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->rmw_offset);

    /* Move back to previous location */
    fp->rmw_offset -= (long long) trans;
    /* Go back for the write cycle */
    VFS (seek, fp->file_desc, where * 1024, NETMIST_SEEK_SET, NULL);
    /*is this statement needed? ==> fp->rmw_offset = where*1024; */
    if (cmdline.gg_flag)
    {
	if ((fp->rmw_offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	{
	    fp->rmw_offset = cmdline.gg_offset;
	    /* Save where for later use by write. */
	    where = (fp->rmw_offset / (unsigned long long) 1024);
	}
	else
	{
	    fp->rmw_offset += cmdline.gg_offset;
	    /* Save where for later use by write. */
	    where = (fp->rmw_offset / (unsigned long long) 1024);
	}
	VFS (seek, fp->file_desc, fp->rmw_offset, NETMIST_SEEK_SET, NULL);
    }

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->rmw_offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = VFS (write, fp->file_desc, buf, trans, &write_ret);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_EXEC,
		  "Rmw file op failed file %s in %s Error: %s Error value %d Ret %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err,
		  write_ret);
	/*free(buf); */
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 16, err);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].rmw_op_time += lat;
	if ((lat < work_obj[my_workload].min_rmw_op_latency)
	    || (work_obj[my_workload].min_rmw_op_latency == 0.0))
	    work_obj[my_workload].min_rmw_op_latency = lat;
	if ((lat > work_obj[my_workload].max_rmw_op_latency)
	    || (work_obj[my_workload].max_rmw_op_latency == 0.0))
	    work_obj[my_workload].max_rmw_op_latency = lat;
    }

    fp->op_count++;		/* Read */
    total_file_ops += 1.0;	/* Write */
    work_obj[my_workload].total_file_ops += 1.0;	/* Write */
    /*work_obj[my_workload].write_op_count++; */
    fp->op_count++;		/* write */

    total_rmw_bytes += (double) (write_ret);
    VFS (seek, fp->file_desc, 0, NETMIST_SEEK_CUR, &fp->rmw_offset);

    work_obj[my_workload].rmw_op_count++;
	/*********************************************************/
    /* Increment so that the totals of Op counts will add up */
    total_file_ops += 1.0;	/* RMW */
    work_obj[my_workload].total_file_ops += 1.0;	/* RMW */
	/*********************************************************/
    total_rmw_ops += 1.0;
    if (cmdline.flush_flag &&
	((work_obj[my_workload].rmw_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fp->file_desc);
    }
    if (in_validate)
    {
	fd_drop_from_cache (fp);	/* includes close */
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
}

/**
 * @brief This is the code that implements the "stat" op. It will
 *        perform a "stat" call on a file with the name that is 
 *        in the test entry and collect statistics for the 
 *        operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_stat(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "stat" op. It will
 * __doc__             perform a "stat" call on a file with the name that is 
 * __doc__             in the test entry and collect statistics for the 
 * __doc__             operation.
 * __doc__
 */
void
generic_stat (struct file_object *fp)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;
    struct netmist_vfs_stat stbuf;

    log_file (LOG_DEBUG, "Doing stat op\n");

    entry_time = gettime ();
    ret = VFS (stat, fp->dir, fp->relfilename, &stbuf);
    if (ret != 0)
    {
	const char *nes = netmist_vfs_errstr (ret);
	log_file (LOG_ERROR,
		  "Stat failed on file %s in %s Error: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, ret);
	R_exit (NETMIST_OP_VFS_ERR + 17, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].stat_op_time += lat;
	if ((lat < work_obj[my_workload].min_stat_op_latency)
	    || (work_obj[my_workload].min_stat_op_latency == 0.0))
	    work_obj[my_workload].min_stat_op_latency = lat;
	if ((lat > work_obj[my_workload].max_stat_op_latency)
	    || (work_obj[my_workload].max_stat_op_latency == 0.0))
	    work_obj[my_workload].max_stat_op_latency = lat;
    }
    file_accesses++;
    work_obj[my_workload].stat_op_count++;
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_r_ops += 1.0;
}

/**
 * @brief This is the code that implements the "neg_stat" op. It will
 *        perform a "stat" call on a file with the name that does not exist. 
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
void
generic_neg_stat (struct file_object *fp)
{
    char mynewname[MAXNAME];
    struct netmist_vfs_stat stbuf;
    double lat = 0.0;
    double entry_time = 0.0;
    int ret;

    log_file (LOG_DEBUG, "Doing Neg_stat.\n");

    entry_time = gettime ();
    /* Modify the real name, so it will be a miss */
    strncpy (mynewname, fp->relfilename, MAXNAME - 1);
    if (strlen (mynewname) > 2)
    {
	mynewname[strlen (mynewname) - 2] = '#';
	mynewname[strlen (mynewname) - 1] = '#';
    }
    else
    {
	return;
    }
    log_file (LOG_DEBUG, "Doing Neg_stat on file %s.\n", mynewname);
    ret = VFS (stat, fp->dir, &(mynewname[0]), &stbuf);
    if (ret != 0)
    {
	;
    }
    else
    {
	log_file (LOG_ERROR, "Neg-Stat failed on file %s File Found !\n",
		  mynewname);
	R_exit (NETMIST_OP_VFS_ERR + 17, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].neg_stat_op_time += lat;
	if ((lat < work_obj[my_workload].min_neg_stat_op_latency)
	    || (work_obj[my_workload].min_neg_stat_op_latency == 0.0))
	    work_obj[my_workload].min_neg_stat_op_latency = lat;
	if ((lat > work_obj[my_workload].max_neg_stat_op_latency)
	    || (work_obj[my_workload].max_neg_stat_op_latency == 0.0))
	    work_obj[my_workload].max_neg_stat_op_latency = lat;
    }
    file_accesses++;
    work_obj[my_workload].neg_stat_op_count++;
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_r_ops += 1.0;
}

/**
 * @brief This is the code that implements the "chmod" op. It will
 *        perform a "chmod" call on a file with the name that is 
 *        in the test entry and collect statistics for the 
 *        operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_chmod(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "chmod" op. It will
 * __doc__             perform a "chmod" call on a file with the name that is 
 * __doc__             in the test entry and collect statistics for the 
 * __doc__             operation.
 * __doc__
 */
void
generic_chmod (struct file_object *fp)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing chmod op\n");

    entry_time = gettime ();
    file_accesses++;
    ret = VFS (chmod, fp->dir, fp->relfilename);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "chmod failed on file %s in %s Error: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), ret);

	R_exit (NETMIST_OP_VFS_ERR + 18, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].chmod_op_time += lat;
	if ((lat < work_obj[my_workload].min_chmod_op_latency)
	    || (work_obj[my_workload].min_chmod_op_latency == 0.0))
	    work_obj[my_workload].min_chmod_op_latency = lat;
	if ((lat > work_obj[my_workload].max_chmod_op_latency)
	    || (work_obj[my_workload].max_chmod_op_latency == 0.0))
	    work_obj[my_workload].max_chmod_op_latency = lat;
    }

    work_obj[my_workload].chmod_op_count++;
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_w_ops += 1.0;
}

/**
 * @brief This is the code that implements the "readdir" op. It 
 *        will perform a "readdir" call on a file with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_readdir(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "readdir" op. It 
 * __doc__             will perform a "readdir" call on a file with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */
void
generic_readdir (struct file_object *fp)
{
    struct netmist_vfs_dentry ret;
    int res;
    double lat = 0.0;
    double entry_time = 0.0;
    long seek_loc = 0;
    long modvalue;

    log_file (LOG_DEBUG, "Doing readdir op\n");

    entry_time = gettime ();
    /* Seek to random location within the directory */
    modvalue = cmdline.client_files;
    if (modvalue <= 0)
	modvalue = 1;
    seek_loc = netmist_rand () % modvalue;

    /* Special case as there are not cmdline.client_files in this directory */
    if (in_validate)
	seek_loc = 0;

    res = VFS (readden, fp->dir, seek_loc, &ret);
    if (res != 0)
    {
	log_file (LOG_ERROR, "Read dir entry failed on file %s (err %d)\n",
		  VFS (pathptr, fp->dir), res);
	R_exit (NETMIST_OP_VFS_ERR + 19, res);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].readdir_op_time += lat;
	if ((lat < work_obj[my_workload].min_readdir_op_latency)
	    || (work_obj[my_workload].min_readdir_op_latency == 0.0))
	    work_obj[my_workload].min_readdir_op_latency = lat;
	if ((lat > work_obj[my_workload].max_readdir_op_latency)
	    || (work_obj[my_workload].max_readdir_op_latency == 0.0))
	    work_obj[my_workload].max_readdir_op_latency = lat;
    }

    work_obj[my_workload].readdir_op_count++;
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_r_ops += 1.0;
}

/**
 * @brief This is the code that implements the "access" op. It 
 *        will perform a "access" call on a file with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_access(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "access" op. It 
 * __doc__             will perform a "access" call on a file with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */
void
generic_access (struct file_object *fp)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing access op\n");

    entry_time = gettime ();
    file_accesses++;
    ret = VFS (access, fp->dir, fp->relfilename);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "Access failed on file %s in %s Error %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), ret);

	R_exit (NETMIST_OP_VFS_ERR + 20, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].access_op_time += lat;
	if ((lat < work_obj[my_workload].min_access_op_latency)
	    || (work_obj[my_workload].min_access_op_latency == 0.0))
	    work_obj[my_workload].min_access_op_latency = lat;
	if ((lat > work_obj[my_workload].max_access_op_latency)
	    || (work_obj[my_workload].max_access_op_latency == 0.0))
	    work_obj[my_workload].max_access_op_latency = lat;
    }

    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    work_obj[my_workload].access_op_count++;
    total_meta_r_ops += 1.0;
}

/**
 * @brief This is the code that implements the "mkdir" op. It will
 *        perform a "mkdir" call on a directory  with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_mkdir(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "mkdir" op. It will
 * __doc__             perform a "mkdir" call on a directory  with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */
void
generic_mkdir (struct file_object *fp)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing mkdir op\n");

    entry_time = gettime ();
    ret = VFS (mkdir, fp->dir, NULL);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "Mkdir failed to file %s Error %d\n",
		  VFS (pathptr, fp->dir), ret);
	R_exit (NETMIST_OP_VFS_ERR + 21, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].mkdir_op_time += lat;
	if ((lat < work_obj[my_workload].min_mkdir_op_latency)
	    || (work_obj[my_workload].min_mkdir_op_latency == 0.0))
	    work_obj[my_workload].min_mkdir_op_latency = lat;
	if ((lat > work_obj[my_workload].max_mkdir_op_latency)
	    || (work_obj[my_workload].max_mkdir_op_latency == 0.0))
	    work_obj[my_workload].max_mkdir_op_latency = lat;
    }

    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_w_ops += 1.0;
    work_obj[my_workload].mkdir_op_count++;
}

/**
 * @brief This is the code that implements the "rmdir" op. It will
 *        perform a "rmdir" call on a directory  with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_rmdir(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "rmdir" op. It will
 * __doc__             perform a "rmdir" call on a directory  with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */
void
generic_rmdir (struct file_object *fp)
{
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing rmdir op\n");

    entry_time = gettime ();
    (void) VFS (rmdir, fp->dir);

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].rmdir_op_time += lat;
	if ((lat < work_obj[my_workload].min_rmdir_op_latency)
	    || (work_obj[my_workload].min_rmdir_op_latency == 0.0))
	    work_obj[my_workload].min_rmdir_op_latency = lat;
	if ((lat > work_obj[my_workload].max_rmdir_op_latency)
	    || (work_obj[my_workload].max_rmdir_op_latency == 0.0))
	    work_obj[my_workload].max_rmdir_op_latency = lat;
    }

    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    total_meta_w_ops += 1.0;
    work_obj[my_workload].rmdir_op_count++;
}

/**
 * @brief This is used to create the directories during the 
 *        client_init_files phase.
 *
 * @param d : Pointer to the netmist_vfs_dir object.
 */
/*
 * __doc__
 * __doc__  Function : void generic_init_dir(struct netmist_vfs_dir *d)
 * __doc__  Arguments: struct netmist_vfs_dir *d: Directory to create/initialize.
 * __doc__  Returns  : VFS error code
 * __doc__  Performs : This is used to create the directories during the 
 * __doc__             client_init_files phase.
 * __doc__
 */
int
generic_init_dir (struct netmist_vfs_dir *d)
{
    log_file (LOG_DEBUG, "Doing init_dir op\n");

    return VFS (mkdir, d, NULL);
}

/**
 * @brief This is the code that implements the "append" op. It will
 *        perform an "append" on a file  with the name that is in 
 *        the test entry and collect statistics for the operation.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_append(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "append" op. It will
 * __doc__             perform an "append" on a file  with the name that is in 
 * __doc__             the test entry and collect statistics for the operation.
 * __doc__
 */
void
generic_append (struct file_object *fp)
{
    struct netmist_vfs_object *fd = NULL;
    char *buf;
    int err;
    double lat = 0.0;
    double entry_time = 0.0;
    uint64_t tlen = 0;
    uint64_t trans;
    enum netmist_vfs_open_flags flags = NETMIST_OPEN_APPEND;
    int this_dedup_group;
    int dedup_across;

    log_file (LOG_DEBUG, "Doing append op\n");

    entry_time = gettime ();
    buf = main_buf;
    file_accesses++;
/*
 * Linux systems may not permit O_DIRECT and O_APPEND over NFS, so 
 * let's not go there for now.
 */
#ifdef COMBINE_DIRECT_APPEND
    if (((work_obj[my_workload].append_op_count % 100) <
	 my_mix_table[my_workload].percent_direct))
    {
#if defined(WIN32)
	flags |= NETMIST_OPEN_DIRECT;
#else
	/* disable for now */
#endif
    }
#endif
    if (((work_obj[my_workload].append_op_count % 100) <
	 my_mix_table[my_workload].percent_osync))
    {
	flags |= NETMIST_OPEN_SYNC;
    }

    err =
	VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
	     &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open for append op failed. Filename = %s in %s Error: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 22, err);
    }

    netmist_directio (fd,
		      (work_obj[my_workload].append_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    total_file_ops += 1.0;	/* Open */
    work_obj[my_workload].total_file_ops += 1.0;	/* Open */
    work_obj[my_workload].open_op_count++;
    VFS (seek, fd, 0, NETMIST_SEEK_END, &fp->offset);
    /* align things for O_DIRECT */
    if (adjust_size_direct (fd, 0, &fp->offset))
    {
	VFS (seek, fd, fp->offset, NETMIST_SEEK_SET, NULL);
    }
    if (my_mix_table[my_workload].align)
    {
	fp->offset =
	    fp->offset & (off64_t) (~(my_mix_table[my_workload].align - 1));
	VFS (seek, fd, fp->offset, NETMIST_SEEK_SET, NULL);
    }
    /* Select write size to use */
    trans = get_next_write_size ();
    /* align things for O_DIRECT */
    adjust_size_direct (fd, 1, &trans);

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    err = netmist_try_large_write (fd, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Append failed to file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, tlen);
	VFS (close, &fd);
	R_exit (NETMIST_OP_VFS_ERR + 23, err);
    }

    total_file_ops += 1.0;	/* Write/append */
    work_obj[my_workload].total_file_ops += 1.0;	/* Write/append */
    fp->op_count++;
    VFS (seek, fd, 0, NETMIST_SEEK_CUR, &fp->offset);

    /* global (across all files in this work object) write count */
    work_obj[my_workload].append_op_count++;

    if (cmdline.flush_flag &&
	((work_obj[my_workload].append_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fd);
    }
    if (cmdline.lflag && cmdline.flush_flag)
	VFS (fsync, fd);
    VFS (close, &fd);
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].append_op_time += lat;
	if ((lat < work_obj[my_workload].min_append_op_latency)
	    || (work_obj[my_workload].min_append_op_latency == 0.0))
	    work_obj[my_workload].min_append_op_latency = lat;
	if ((lat > work_obj[my_workload].max_append_op_latency)
	    || (work_obj[my_workload].max_append_op_latency == 0.0))
	    work_obj[my_workload].max_append_op_latency = lat;
    }
    total_write_bytes += (double) (tlen);
    work_obj[my_workload].close_op_count++;
    total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    /*free(buf); */
}

/**
 * @brief This is the code that implements the "unlink" op. It will
 *        perform an "unlink" on a file  with the name that is in 
 *        the test entry and collect statistics for the operation.
 *        postfix() will put the file back.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 * @param force : Used to force the operation to happen.
 */
/*
 * __doc__
 * __doc__  Function : void generic_unlink(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "unlink" op. It will
 * __doc__             perform an "unlink" on a file  with the name that is in 
 * __doc__             the test entry and collect statistics for the operation.
 * __doc__             postfix() will put the file back.
 * __doc__
 */
void
generic_unlink (struct file_object *fp, int force)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing unlink op\n");

    entry_time = gettime ();
    /* Let's not do this while sharing all files */
    if ((!force) && (cmdline.sharing_flag))
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].unlink_op_time += lat;
	    if ((lat < work_obj[my_workload].min_unlink_op_latency)
		|| (work_obj[my_workload].min_unlink_op_latency == 0.0))
		work_obj[my_workload].min_unlink_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_unlink_op_latency)
		|| (work_obj[my_workload].max_unlink_op_latency == 0.0))
		work_obj[my_workload].max_unlink_op_latency = lat;
	}
	return;
    }
    file_accesses++;
    ret = VFS (unlink, fp->dir, fp->relfilename);
    if ((ret != 0) && !cmdline.sharing_flag)
    {
	log_file (LOG_ERROR, "Unlink failed on %s in %s Error: %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir), ret);

	R_exit (NETMIST_OP_VFS_ERR + 24, ret);
    }
    if (ret == 0)
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].unlink_op_time += lat;
	    if ((lat < work_obj[my_workload].min_unlink_op_latency)
		|| (work_obj[my_workload].min_unlink_op_latency == 0.0))
		work_obj[my_workload].min_unlink_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_unlink_op_latency)
		|| (work_obj[my_workload].max_unlink_op_latency == 0.0))
		work_obj[my_workload].max_unlink_op_latency = lat;
	}

	work_obj[my_workload].unlink_op_count++;
	total_file_ops += 1.0;
	work_obj[my_workload].total_file_ops += 1.0;
	total_meta_w_ops += 1.0;
    }
}

/**
 * @brief This is the code that implements the "unlink2" op. It 
 *        will perform an "unlink" on a file  with the name that 
 *        is in the test entry and collect statistics for the 
 *        operation. postfix() will put the file back. This 
 *        version unlinks files that are non-zero in length.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 * @param force : used to force the operation to happen.
 */
/*
 * __doc__
 * __doc__  Function : void generic_unlink2(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "unlink2" op. It 
 * __doc__             will perform an "unlink" on a file  with the name that 
 * __doc__             is in the test entry and collect statistics for the 
 * __doc__             operation. postfix() will put the file back. This 
 * __doc__             version unlinks files that are non-zero in length.
 * __doc__
 */
int msg_count;
int msg_flood = 10;

void
generic_unlink2 (struct file_object *fp, int force)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing unlink2 op\n");

    entry_time = gettime ();
    file_accesses++;
    /* Let's not do this while sharing all files */
    if ((!force) && (cmdline.sharing_flag))
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].unlink2_op_time += lat;
	    if ((lat < work_obj[my_workload].min_unlink2_op_latency)
		|| (work_obj[my_workload].min_unlink2_op_latency == 0.0))
		work_obj[my_workload].min_unlink2_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_unlink2_op_latency)
		|| (work_obj[my_workload].max_unlink2_op_latency == 0.0))
		work_obj[my_workload].max_unlink2_op_latency = lat;
	}
	return;
    }
    ret = VFS (unlink, fp->dir, fp->relfilename);
    if ((ret != 0) && !cmdline.sharing_flag)
    {
	/* Don't exit in this special case */
	if (my_mix_table[my_workload].unlink2_no_recreate
	    && (phase == RUN_PHASE))
	{
	    if (msg_count < msg_flood)
	    {
		msg_count++;
		log_file (LOG_EXEC,
			  "no_recreate set: Unlink2 failed on %s in %s Error: %d\n",
			  fp->relfilename, VFS (pathptr, fp->dir), ret);

	    }
	    return;
	}
	else
	{
	    log_file (LOG_ERROR, "Unlink2 failed on %s in %s Error: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), ret);

	    R_exit (NETMIST_OP_VFS_ERR + 24, ret);
	}
    }
    if (ret == 0)
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].unlink2_op_time += lat;
	    if ((lat < work_obj[my_workload].min_unlink2_op_latency)
		|| (work_obj[my_workload].min_unlink2_op_latency == 0.0))
		work_obj[my_workload].min_unlink2_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_unlink2_op_latency)
		|| (work_obj[my_workload].max_unlink2_op_latency == 0.0))
		work_obj[my_workload].max_unlink2_op_latency = lat;
	}

	work_obj[my_workload].unlink2_op_count++;
	total_file_ops += 1.0;
	work_obj[my_workload].total_file_ops += 1.0;
	total_meta_w_ops += 1.0;
    }
}

/**
 * @brief This is the code that implements the "create" op. It will
 *        perform an "create" of a file  with the name that is in 
 *        the test entry and collect statistics for the operation.
 *        postfix() will remove the file.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 * @param force : used to force the operation to happen.
 */
/*
 * __doc__
 * __doc__  Function : void generic_create(struct file_object *fp,int force)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__             Int force:  Only do this if force is set. Needed for 
 * __doc__             sharing.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "create" op. It will
 * __doc__             perform an "create" of a file  with the name that is in 
 * __doc__             the test entry and collect statistics for the operation.
 * __doc__             postfix() will remove the file.
 * __doc__
 */
void
generic_create (struct file_object *fp, int force)
{
    struct netmist_vfs_object *fd = NULL;
    int err = 0;

    double lat = 0.0;
    double entry_time = 0.0;

    entry_time = gettime ();

    log_file (LOG_DEBUG, "Doing create op\n");

    /* Let's not do this while sharing all files */
    if ((!force) && (cmdline.sharing_flag))
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].create_op_time += lat;
	    if ((lat < work_obj[my_workload].min_create_op_latency)
		|| (work_obj[my_workload].min_create_op_latency == 0.0))
		work_obj[my_workload].min_create_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_create_op_latency)
		|| (work_obj[my_workload].max_create_op_latency == 0.0))
		work_obj[my_workload].max_create_op_latency = lat;
	}
	return;
    }

    err = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE, &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open for create op failed. Filename = %s in %s Error: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 24, err);
    }
    else
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].create_op_time += lat;
	    if ((lat < work_obj[my_workload].min_create_op_latency)
		|| (work_obj[my_workload].min_create_op_latency == 0.0))
		work_obj[my_workload].min_create_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_create_op_latency)
		|| (work_obj[my_workload].max_create_op_latency == 0.0))
		work_obj[my_workload].max_create_op_latency = lat;
	}

	work_obj[my_workload].create_op_count++;
	total_file_ops += 1.0;
	work_obj[my_workload].total_file_ops += 1.0;
	total_meta_w_ops += 1.0;
    }
    VFS (close, &fd);
    work_obj[my_workload].close_op_count++;
    total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
}

/**
 * @brief This is the code that implements the "lock" op. It will
 *        perform a "lock" call on a file with the name that 
 *        is in the test entry and collect statistics for the 
 *        operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_locking(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "lock" op. It will
 * __doc__             perform a "lock" call on a file with the name that 
 * __doc__             is in the test entry and collect statistics for the 
 * __doc__             operation.
 * __doc__
 */
void
generic_locking (struct file_object *fp)
{
    int ret;
    struct netmist_vfs_object *fd = NULL;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing lock op\n");

    entry_time = gettime ();

    ret = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE, &fd);
    if (ret != 0)
    {
	const char *nes = netmist_vfs_errstr (ret);
	log_file (LOG_ERROR,
		  "Open for lock op failed. Filename = %s in %sError: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, ret);
	R_exit (NETMIST_OP_VFS_ERR + 26, ret);
    }
    /* Generic lock the whole file */
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    work_obj[my_workload].open_op_count++;
    total_meta_r_ops += 1.0;

    ret = VFS (lock, fd, NETMIST_VFS_LOCK_WAIT);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "Lock failed on file %s in %s Error %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir),
		  netmist_vfs_errstr (ret));
	R_exit (NETMIST_OP_VFS_ERR + 25, ret);
    }

    fp->op_count++;

    total_file_ops += 1.0;	/* fcntl */
    work_obj[my_workload].total_file_ops += 1.0;	/* fcntl */
    work_obj[my_workload].lock_op_count++;

    VFS (lock, fd, NETMIST_VFS_LOCK_UNLOCK);
    VFS (close, &fd);

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].lock_op_time += lat;
	if ((lat < work_obj[my_workload].min_lock_op_latency)
	    || (work_obj[my_workload].min_lock_op_latency == 0.0))
	    work_obj[my_workload].min_lock_op_latency = lat;
	if ((lat > work_obj[my_workload].max_lock_op_latency)
	    || (work_obj[my_workload].max_lock_op_latency == 0.0))
	    work_obj[my_workload].max_lock_op_latency = lat;
    }
    total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].close_op_count++;
}

/**
 * @brief This is the code that implements the "copyfile" op. It 
 *        will perform a "copyfile" call on a file with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_copyfile(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "copyfile" op. It 
 * __doc__             will perform a "copyfile" call on a file with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */

void
generic_copyfile (struct file_object *fp)
{
    struct netmist_vfs_stat stbuf = { 0, 0, 0 };
    struct netmist_vfs_object *fdi = NULL;
    struct netmist_vfs_object *fdo = NULL;
    uint64_t read_ret = 0, write_ret = 0;
    int read_transferred = 0;
    int write_transferred = 0;
    size_t my_size;
    int i;
    enum netmist_vfs_open_flags iflags = 0, oflags = 0;	/*initialized flags to 0 */
    double lat = 0.0;
    double entry_time = 0.0;
    char rel_alt_name[MAXNAME];
    int err;
    int probability = 0;

    entry_time = gettime ();
    /* Allocate and align buffer for possible use in O_DIRECT operatino */
    if (copy_file_buf == NULL)	/* If you have one, just use it, else get */
    {
	copy_file_buf = (char *) MALLOC (CHUNK_SIZE + 4096);
	copy_file_buf = (char *) (((intptr_t) copy_file_buf + 4096) & ~4095);
    }
    set_osync_flag (work_obj[my_workload].copyfile_op_count,
		    my_mix_table[my_workload].percent_osync, &iflags);
    set_osync_flag (work_obj[my_workload].copyfile_op_count,
		    my_mix_table[my_workload].percent_osync, &oflags);

    set_direct_flag (work_obj[my_workload].copyfile_op_count,
		     my_mix_table[my_workload].percent_direct, &iflags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &iflags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand,
		      &iflags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &iflags);
    set_direct_flag (work_obj[my_workload].copyfile_op_count,
		     my_mix_table[my_workload].percent_direct, &oflags);

    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &oflags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand,
		      &oflags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &oflags);

    log_file (LOG_DEBUG, "Doing copyfile op\n");

    snprintf (rel_alt_name, sizeof (rel_alt_name), "%s_2", fp->relfilename);

    /* Get the size and mode */
    err = VFS (stat, fp->dir, fp->relfilename, &stbuf);
    my_size = stbuf.netmist_st_size;

    /* If dest does not exist then start from beginning */
    err = VFS (stat, fp->dir, rel_alt_name, &stbuf);
    if (err != 0)
	fp->copyfile_offset = 0LL;

    /* Open the input file */
    err = VFS (open, fp->dir, fp->relfilename, iflags, &fdi);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open for copyfile op failed. Filename = %s in %s, Error: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 26, err);
    }

    /* Seek to last known copyfile_offset */
    VFS (seek, fdi, fp->copyfile_offset, NETMIST_SEEK_SET, NULL);
    /* Now let's go the destination file */
    err =
	VFS (open, fp->dir, rel_alt_name, NETMIST_OPEN_CREATE | oflags, &fdo);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open for copyfile op failed. Filename = %s in %s Error: %s Error value: %d\n",
		  rel_alt_name, VFS (pathptr, fp->dir), nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 26, err);
    }

    netmist_directio (fdo,
		      (work_obj[my_workload].copyfile_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Seek to same copyfile_offset in the output file */
    VFS (seek, fdi, fp->copyfile_offset, NETMIST_SEEK_SET, NULL);

    /* Now copy a chunk */
    for (i = 0; i < COPY_CHUNK; i++)
    {
	/* The copy should proceed if the 
	 * copyfile_offset+CHUNK_SIZE == my_size 
	 */
	if (((long long) fp->copyfile_offset + CHUNK_SIZE) <=
	    (long long) my_size)
	{
	    err = VFS (read, fdi, copy_file_buf, CHUNK_SIZE, &read_ret);
	    if (err)
	    {
		const char *nes = netmist_vfs_errstr (err);
		log_file (LOG_ERROR,
			  "Copyfile: Read failed from file %s in %s"
			  " Error: %s Error value %d Ret %" PRIu64 "\n",
			  fp->relfilename, VFS (pathptr, fp->dir), nes, err,
			  read_ret);
		VFS (close, &fdi);	/* counted for close op below */
		VFS (close, &fdo);	/* counted below */
		fp->copyfile_offset = 0LL;
		VFS (unlink, fp->dir, rel_alt_name);
		R_exit (NETMIST_OP_VFS_ERR + 10, err);
	    }
	    else
	    {
		read_transferred += read_ret;

		err = VFS (write, fdo, copy_file_buf, read_ret, &write_ret);
		if (err)
		{
		    const char *nes = netmist_vfs_errstr (err);
		    log_file (LOG_ERROR,
			      "Copyfile: Write failed to file %s in %s Error: %s Error value %d Ret %"
			      PRIu64 "\n", rel_alt_name, VFS (pathptr,
							      fp->dir), nes,
			      err, write_ret);
		    VFS (close, &fdi);
		    VFS (close, &fdo);
		    VFS (unlink, fp->dir, rel_alt_name);
		    R_exit (NETMIST_OP_VFS_ERR + 10, err);
		}

		fp->copyfile_offset += (long long) write_ret;
		write_transferred += write_ret;
	    }
	}
	else
	{
	    /* We're done, so unlink the destination and head for home */
	    fp->copyfile_offset = 0LL;
	    VFS (unlink, fp->dir, rel_alt_name);
	    /* Need a "break" statement here?? otherwise if the condition 
	     * is satisfied in the first iteration, it will do the copy again 
	     */
	    break;
	}
    }

    /* global (across all files in this work object) write count */
    work_obj[my_workload].copyfile_op_count++;

    if (cmdline.flush_flag &&
	((work_obj[my_workload].copyfile_op_count % 100) <
	 my_mix_table[my_workload].percent_commit))
    {
	VFS (fsync, fdo);
    }

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].copyfile_op_time += lat;
	if ((lat < work_obj[my_workload].min_copyfile_op_latency)
	    || (work_obj[my_workload].min_copyfile_op_latency == 0.0))
	    work_obj[my_workload].min_copyfile_op_latency = lat;
	if ((lat > work_obj[my_workload].max_copyfile_op_latency)
	    || (work_obj[my_workload].max_copyfile_op_latency == 0.0))
	    work_obj[my_workload].max_copyfile_op_latency = lat;
    }
    total_copyfile_bytes += (double) ((read_transferred + write_transferred));
    total_read_bytes += (double) (read_transferred);
    total_write_bytes += (double) (write_transferred);

    total_file_ops += 2.0;	/* Open of both files */
    work_obj[my_workload].total_file_ops += 2.0;	/* Open */
    work_obj[my_workload].open_op_count += 2;	/* Open */

    VFS (close, &fdi);
    VFS (close, &fdo);
    total_file_ops += 2.0;	/* Close */
    work_obj[my_workload].close_op_count += 2;	/* Close */
    work_obj[my_workload].total_file_ops += 2.0;	/* Close */
}

/**
 * @brief This is the code that implements the "rename" op. It will
 *        perform a "rename" call on a file with the name that 
 *        is in the test entry and collect statistics for the 
 *        operation. The post-op will rename the file back to 
 *        its original name.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_rename(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "rename" op. It will
 * __doc__             perform a "rename" call on a file with the name that 
 * __doc__             is in the test entry and collect statistics for the 
 * __doc__             operation. The post-op will rename the file back to 
 * __doc__             its original name.
 * __doc__
 */
void
generic_rename (struct file_object *fp)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;
    char alt_name[MAXNAME];

    log_file (LOG_DEBUG, "Doing rename op\n");

    entry_time = gettime ();
    if (cmdline.sharing_flag)
	return;			/* Let's not do this while sharing all files */

    snprintf (alt_name, sizeof (alt_name), "%s_2", fp->relfilename);

    ret = VFS (rename, fp->dir, fp->relfilename, fp->dir, alt_name);
    if ((ret != 0) && !cmdline.sharing_flag)
    {
	log_file (LOG_ERROR, "Rename failed to file %s in %s. Error: %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir),
		  netmist_vfs_errstr (ret));
	R_exit (NETMIST_OP_VFS_ERR + 25, ret);
    }
    if (ret == 0)
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].rename_op_time += lat;
	    if ((lat < work_obj[my_workload].min_rename_op_latency)
		|| (work_obj[my_workload].min_rename_op_latency == 0.0))
		work_obj[my_workload].min_rename_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_rename_op_latency)
		|| (work_obj[my_workload].max_rename_op_latency == 0.0))
		work_obj[my_workload].max_rename_op_latency = lat;
	}

	/* global (across all files in this work object) write count */
	work_obj[my_workload].rename_op_count++;
	work_obj[my_workload].total_file_ops += 1.0;	/* rename */
	total_file_ops += 1.0;

    }
}

/**
 * @brief This is the code that implements the "statfs" op. It will
 *        perform a "statfs" call on a file with the name that is 
 *        in the test entry and collect statistics for the 
 *        operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_statfs(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "statfs" op. It will
 * __doc__             perform a "statfs" call on a file with the name that is 
 * __doc__             in the test entry and collect statistics for the 
 * __doc__             operation.
 * __doc__
 */
#if defined(_bsd_) || defined(_macos_) || defined(_solaris_)
#include <sys/statvfs.h>
#elif !defined(WIN32)
#include <sys/vfs.h>
#endif

void
generic_statfs (struct file_object *fp)
{
    int ret;
    struct netmist_vfs_statfs st;
    double lat = 0.0;
    double entry_time = 0.0;

    log_file (LOG_DEBUG, "Doing statfs op\n");

    entry_time = gettime ();

    ret = VFS (statfs, fp->dir, fp->relfilename, &st);
    if (ret != 0)
    {
	log_file (LOG_ERROR,
		  "generic_statfs() failed: file name = %s in %s, error string: %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir),
		  netmist_vfs_errstr (ret));
	R_exit (NETMIST_OP_VFS_ERR + 26, ret);
    }

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].statfs_op_time += lat;
	if ((lat < work_obj[my_workload].min_statfs_op_latency)
	    || (work_obj[my_workload].min_statfs_op_latency == 0.0))
	    work_obj[my_workload].min_statfs_op_latency = lat;
	if ((lat > work_obj[my_workload].max_statfs_op_latency)
	    || (work_obj[my_workload].max_statfs_op_latency == 0.0))
	    work_obj[my_workload].max_statfs_op_latency = lat;
    }

    /* global (across all files in this work object) write count */
    work_obj[my_workload].statfs_op_count++;

    total_file_ops += 1.0;	/* for the statfs call */
    work_obj[my_workload].total_file_ops += 1.0;	/* Open */

}

/**
 * @brief This is the code that implements the "pathconf" op. It 
 *        will perform a "pathconf" call on a file with the name 
 *        that is in the test entry and collect statistics for 
 *        the operation.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_pathconf(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "pathconf" op. It 
 * __doc__             will perform a "pathconf" call on a file with the name 
 * __doc__             that is in the test entry and collect statistics for 
 * __doc__             the operation.
 * __doc__
 */
void
generic_pathconf (struct file_object *fp)
{

    double lat = 0.0;
    double entry_time = 0.0;
    int ret;

    log_file (LOG_DEBUG, "Doing pathconf op\n");

    entry_time = gettime ();

    ret = VFS (pathconf, fp->dir);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "pathconf op failed. Error %s\n",
		  netmist_vfs_errstr (ret));
	R_exit (LOAD_GEN_ERR, ret);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].pathconf_op_time += lat;
	if ((lat < work_obj[my_workload].min_pathconf_op_latency)
	    || (work_obj[my_workload].min_pathconf_op_latency == 0.0))
	    work_obj[my_workload].min_pathconf_op_latency = lat;
	if ((lat > work_obj[my_workload].max_pathconf_op_latency)
	    || (work_obj[my_workload].max_pathconf_op_latency == 0.0))
	    work_obj[my_workload].max_pathconf_op_latency = lat;
    }

    /* global (across all files in this work object) write count */
    work_obj[my_workload].pathconf_op_count++;

    total_file_ops += 1.0;	/* for the pathconf call */
    work_obj[my_workload].total_file_ops += 1.0;	/* Open */

}

/**
 * @brief This is a placeholder for additions.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_custom1(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is a placeholder for additions.
 * __doc__
 */
void
generic_custom1 (struct file_object *fp)
{
    struct file_object *dummy;
    dummy = fp;
    if (dummy)
    {
	;
    }
    /* Implement Custom1 op type here */
}

/**
 * @brief This is the code that implements the "trunc" op. It will
 *        perform an "trunc" on a file with the name that is in 
 *        the test entry and collect statistics for the operation.
 *        postfix() will put the file back to its original size.
 *
 * @param fp : Struct file_object points at a particular file or dir.
 * @param force : Used to force the operation to happen.
 */
/*
 * __doc__
 * __doc__  Function : void generic_unlink(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "unlink" op. It will
 * __doc__             perform an "unlink" on a file  with the name that is in 
 * __doc__             the test entry and collect statistics for the operation.
 * __doc__             postfix() will put the file back.
 * __doc__
 */
void
generic_trunc (struct file_object *fp, int force)
{
    int ret;
    double lat = 0.0;
    double entry_time = 0.0;
    struct netmist_vfs_object *fd = NULL;

    log_file (LOG_DEBUG, "Doing truncate op\n");

    entry_time = gettime ();
    /* Let's not do this while sharing all files */
    if ((!force) && (cmdline.sharing_flag))
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].trunc_op_time += lat;
	    if ((lat < work_obj[my_workload].min_trunc_op_latency)
		|| (work_obj[my_workload].min_trunc_op_latency == 0.0))
		work_obj[my_workload].min_trunc_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_trunc_op_latency)
		|| (work_obj[my_workload].max_trunc_op_latency == 0.0))
		work_obj[my_workload].max_trunc_op_latency = lat;
	}
	return;
    }
    file_accesses++;

    if (fp->file_desc != NULL)
    {
	/* Use the cached file descriptor if there is one, avoiding an open */
	fd = fp->file_desc;
    }
    else
    {
	int err;

	/*
	 * Need this open as we are no longer calling truncate but the equivilent of ftruncate.
	 * So, we need an open file descriptor.
	 */
	err = VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE, &fd);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for generic truncate op failed. Filename = %s in %s Error: %s Error Value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}
    }
    ret = VFS (trunc, fd, (off_t) 0);
    if ((ret != 0) && !cmdline.sharing_flag)
    {
	log_file (LOG_ERROR, "Truncate failed to file %s in %s Error: %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir),
		  netmist_vfs_errstr (ret));
	R_exit (NETMIST_OP_VFS_ERR + 24, ret);
    }
    if (ret == 0)
    {
	if (cmdline.Op_lat_flag)
	{
	    lat = gettime () - entry_time;
	    hist_insert (lat);
	    work_obj[my_workload].trunc_op_time += lat;
	    if ((lat < work_obj[my_workload].min_trunc_op_latency)
		|| (work_obj[my_workload].min_trunc_op_latency == 0.0))
		work_obj[my_workload].min_trunc_op_latency = lat;
	    if ((lat > work_obj[my_workload].max_trunc_op_latency)
		|| (work_obj[my_workload].max_trunc_op_latency == 0.0))
		work_obj[my_workload].max_trunc_op_latency = lat;
	}

	work_obj[my_workload].trunc_op_count++;
	total_file_ops += 1.0;
	work_obj[my_workload].total_file_ops += 1.0;
	total_meta_w_ops += 1.0;

	/* If we did not use a cached descriptor, close the one we opened */
	if (fp->file_desc == NULL)
	    VFS (close, &fd);
    }
}

/**
 * @brief This is a placeholder for additions.
 *
 * @param fp : Struct file_object *, points at a particular file or dir.
 */
/*
 * __doc__
 * __doc__  Function : void generic_custom2(struct file_object *fp)
 * __doc__  Arguments: Struct file_object points at a particular file or dir.
 * __doc__  Returns  : void
 * __doc__  Performs : This is a placeholder for additions.
 * __doc__
 */
void
generic_custom2 (struct file_object *fp)
{
    struct file_object *dummy;
    dummy = fp;
    if (dummy)
    {
	;
    }
    /* Implement Custom2 op type here */
}

/**
 * @brief This is used to reset the statistics that have been 
 *        collected.
 */
/*
 * __doc__
 * __doc__  Function : void clear_stats(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : This is used to reset the statistics that have been 
 * __doc__             collected.
 * __doc__
 */
void
clear_stats (void)
{
    int i;

    total_read_bytes = 0;
    total_write_bytes = 0;
    total_copyfile_bytes = total_file_ops = total_read_rand_bytes = 0;
    total_file_op_time = total_meta_r_ops = total_meta_w_ops = 0;
    total_rmw_ops = 0;
    for (i = 0; i < num_work_obj; i++)
    {
	/* Raw counters */
	work_obj[i].write_op_count = 0;
	work_obj[i].write_file_op_count = 0;
	work_obj[i].read_op_count = 0;
	work_obj[i].read_file_op_count = 0;
	work_obj[i].mkdir_op_count = 0;
	work_obj[i].rmdir_op_count = 0;
	work_obj[i].unlink_op_count = 0;
	work_obj[i].unlink2_op_count = 0;
	work_obj[i].create_op_count = 0;
	work_obj[i].stat_op_count = 0;
	work_obj[i].neg_stat_op_count = 0;
	work_obj[i].copyfile_op_count = 0;
	work_obj[i].rename_op_count = 0;
	work_obj[i].statfs_op_count = 0;
	work_obj[i].pathconf_op_count = 0;
	work_obj[i].trunc_op_count = 0;
	work_obj[i].custom1_op_count = 0;
	work_obj[i].custom2_op_count = 0;
	work_obj[i].append_op_count = 0;
	work_obj[i].lock_op_count = 0;
	work_obj[i].access_op_count = 0;
	work_obj[i].chmod_op_count = 0;
	work_obj[i].readdir_op_count = 0;
	work_obj[i].write_rand_op_count = 0;
	work_obj[i].read_rand_op_count = 0;
	work_obj[i].mmap_write_op_count = 0;
	work_obj[i].mmap_read_op_count = 0;
	work_obj[i].rmw_op_count = 0;
	work_obj[i].open_op_count = 0;
	work_obj[i].close_op_count = 0;

	/* Raw time accumulators */
	work_obj[i].write_op_time = 0.0;
	work_obj[i].min_write_op_latency = 0.0;
	work_obj[i].max_write_op_latency = 0.0;
	work_obj[i].write_file_op_time = 0.0;
	work_obj[i].min_write_file_op_latency = 0.0;
	work_obj[i].max_write_file_op_latency = 0.0;
	work_obj[i].read_op_time = 0.0;
	work_obj[i].min_read_op_latency = 0.0;
	work_obj[i].max_read_op_latency = 0.0;
	work_obj[i].read_file_op_time = 0.0;
	work_obj[i].min_read_file_op_latency = 0.0;
	work_obj[i].max_read_file_op_latency = 0.0;
	work_obj[i].mkdir_op_time = 0.0;
	work_obj[i].min_mkdir_op_latency = 0.0;
	work_obj[i].max_mkdir_op_latency = 0.0;
	work_obj[i].rmdir_op_time = 0.0;
	work_obj[i].min_rmdir_op_latency = 0.0;
	work_obj[i].max_rmdir_op_latency = 0.0;
	work_obj[i].unlink_op_time = 0.0;
	work_obj[i].min_unlink_op_latency = 0.0;
	work_obj[i].max_unlink_op_latency = 0.0;
	work_obj[i].unlink2_op_time = 0.0;
	work_obj[i].min_unlink2_op_latency = 0.0;
	work_obj[i].max_unlink2_op_latency = 0.0;
	work_obj[i].create_op_time = 0.0;
	work_obj[i].min_create_op_latency = 0.0;
	work_obj[i].max_create_op_latency = 0.0;
	work_obj[i].stat_op_time = 0.0;
	work_obj[i].min_stat_op_latency = 0.0;
	work_obj[i].max_stat_op_latency = 0.0;
	work_obj[i].copyfile_op_time = 0.0;
	work_obj[i].min_copyfile_op_latency = 0.0;
	work_obj[i].max_copyfile_op_latency = 0.0;
	work_obj[i].rename_op_time = 0.0;
	work_obj[i].min_rename_op_latency = 0.0;
	work_obj[i].max_rename_op_latency = 0.0;
	work_obj[i].statfs_op_time = 0.0;
	work_obj[i].min_statfs_op_latency = 0.0;
	work_obj[i].max_statfs_op_latency = 0.0;
	work_obj[i].pathconf_op_time = 0.0;
	work_obj[i].min_pathconf_op_latency = 0.0;
	work_obj[i].max_pathconf_op_latency = 0.0;
	work_obj[i].trunc_op_time = 0.0;
	work_obj[i].min_trunc_op_latency = 0.0;
	work_obj[i].max_trunc_op_latency = 0.0;
	work_obj[i].custom1_op_time = 0.0;
	work_obj[i].min_custom1_op_latency = 0.0;
	work_obj[i].max_custom1_op_latency = 0.0;
	work_obj[i].custom2_op_time = 0.0;
	work_obj[i].min_custom2_op_latency = 0.0;
	work_obj[i].max_custom2_op_latency = 0.0;
	work_obj[i].append_op_time = 0.0;
	work_obj[i].min_append_op_latency = 0.0;
	work_obj[i].max_append_op_latency = 0.0;
	work_obj[i].lock_op_time = 0.0;
	work_obj[i].min_lock_op_latency = 0.0;
	work_obj[i].max_lock_op_latency = 0.0;
	work_obj[i].access_op_time = 0.0;
	work_obj[i].min_access_op_latency = 0.0;
	work_obj[i].max_access_op_latency = 0.0;
	work_obj[i].chmod_op_time = 0.0;
	work_obj[i].min_chmod_op_latency = 0.0;
	work_obj[i].max_chmod_op_latency = 0.0;
	work_obj[i].readdir_op_time = 0.0;
	work_obj[i].min_readdir_op_latency = 0.0;
	work_obj[i].max_readdir_op_latency = 0.0;
	work_obj[i].write_rand_op_time = 0.0;
	work_obj[i].min_write_rand_op_latency = 0.0;
	work_obj[i].max_write_rand_op_latency = 0.0;
	work_obj[i].read_rand_op_time = 0.0;
	work_obj[i].min_read_rand_op_latency = 0.0;
	work_obj[i].max_read_rand_op_latency = 0.0;
	work_obj[i].rmw_op_time = 0.0;
	work_obj[i].min_rmw_op_latency = 0.0;
	work_obj[i].max_rmw_op_latency = 0.0;
	work_obj[i].mmap_write_op_time = 0.0;
	work_obj[i].min_mmap_write_op_latency = 0.0;
	work_obj[i].max_mmap_write_op_latency = 0.0;
	work_obj[i].mmap_read_op_time = 0.0;
	work_obj[i].min_mmap_read_op_latency = 0.0;
	work_obj[i].max_mmap_read_op_latency = 0.0;
	work_obj[i].total_file_op_time = 0.0;
	work_obj[i].total_file_ops = 0.0;
    }
    for (i = 0; i < BUCKETS; i++)
    {
	global_buckets[i] = 0LL;
	buckets[i] = 0LL;
    }
}

/**
 * @brief This is the code that implements the gettimeofday time 
 *        measurement.
 */
/*
 * __doc__
 * __doc__  Function : double gettime(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : Float that contains time in seconds.
 * __doc__  Performs : This is the code that implements the gettimeofday time 
 * __doc__             measurement.
 * __note__          : This can get time from gettimeofday, or clock_gettime(),
 * __note__            or even get it from a time server that is running on 
 * __note__            another host.
 * __doc__
 */
/*
 * If the platform supports the newer clock_gettime() then use this
 * instead of gettimeofday() as it is more accurate. clock_gettime()
 * has a resolution in nanoseconds.
 */
#if defined(_linux_) || defined(_bsd_) || defined(_solaris_)
#define _CLOCK_GETTIME_
#endif
/*
 * Return time in seconds. Floating point.
 */
double
gettime (void)
{
    struct timeval tp;
#if defined(_CLOCK_GETTIME_)
    struct timespec time_spec;
#endif

    /*
     * If one has selected to use a network based time from a time server.
     */
    if (cmdline.pit_hostname[0] != 0)
    {
#if defined(_solaris_)
	if (pit_gettimeofday ((struct timeval *) &tp, cmdline.pit_hostname,
			      cmdline.pit_service) == -1)
	{
#else
	if (pit_gettimeofday ((struct timeval *) &tp, cmdline.pit_hostname,
			      cmdline.pit_service) == -1)
	{
#endif
	    log_file (LOG_ERROR, "%it_gettimeofday error\n");
	    R_exit (BAD_PIT_CONN, 0);
	}
	return ((double) (tp.tv_sec)) + (((double) tp.tv_usec) * 0.000001);
    }
    else
    {
#if defined(_CLOCK_GETTIME_)
#if defined(_linux_) || defined(_bsd_) || defined(_solaris_)
      retry_clock:
#endif
	if (clock_gettime (CLOCK_REALTIME, &time_spec) < 0)
	{
	    log_file (LOG_ERROR, "clock_gettime() error: %s\n",
		      netmist_strerror (netmist_get_error ()));
#if defined(_linux_) || defined(_bsd_) || defined(_solaris_)
	    if (errno == EAGAIN)
	    {
		log_file (LOG_ERROR,
			  "clock_gettime() error: EAGAIN. Retrying call.\n");
		goto retry_clock;
	    }
#endif
	    return ((double) 0);
	}
	return ((double) time_spec.tv_sec +
		(double) time_spec.tv_nsec * 0.000000001);
#else

#if defined(_solaris_)
	return (m_gettimeofday ((struct timeval *) &tp));
#else
	return (m_gettimeofday ((struct timeval *) &tp));
#endif
#endif
    }
}


/**
 * @brief Called by signal handler and by exit paths where
 *        things are not going well and the test needs to
 *        cleanup and exit.
 *
 * @param val : Used to display cause.  Will equal the platform's notion of
 *              SIGPIPE.
 */
/*
 * __doc__
 * __doc__  Function : void pipe_elog(int val)
 * __doc__  Arguments: int val: Used to display cause.
 * __doc__  Returns  : void
 * __doc__  Performs : Called by signal handler and by exit paths where
 * __doc__             things are not going well and the test needs to
 * __doc__             cleanup and exit.
 * __doc__
 */
void
pipe_elog (int val)
{
    if (is_prime ())
    {
	log_all (LOG_ERROR, "Prime got a SIGPIPE. Value: %d\n", val);
	printf
	    ("This indicates that the operating system is unexpectedly closing sockets.\n");
	printf
	    ("This may indicate an overload of the clients in the solution under test.\n");
	prime_send_kill_procs ();
    }
    else if (is_nodeManager ())
    {
	log_file (LOG_ERROR, "nodeManager got a SIGPIPE. Value: %d\n", val);
    }
    else
    {
	log_file (LOG_ERROR, "Child got a SIGPIPE. Value: %d\n", val);
	log_file (LOG_ERROR,
		  "This indicates that the operating system is unexpectedly closing sockets.\n");
	log_file (LOG_ERROR,
		  "This may indicate an overload of the clients in the solution under test.\n");

	(*Netmist_Ops[my_workload].teardown) ();
	tell_nm_error (cmdline.client_id, PIPE_EXIT, val);
    }
    exit (val);
}

volatile int signal_flag = 0;

/**
 * @brief When you call this handler, the test will try to 
 *        cleanup and exit gracefully.
 * @param val : Used to indicate the cause.
 */
/*
 * __doc__
 * __doc__  Function : void sig_clean_exit(int val)
 * __doc__  Arguments: int val: used to indicate cause.
 * __doc__  Returns  : void
 * __doc__  Performs : When you call this handler, the test will try to 
 * __doc__             cleanup and exit gracefully.
 * __doc__
 */
void
sig_clean_exit (int val)
{
    signal_flag = 1;
    clean_exit (NETMIST_NO_CHILD_ERR, val);
}

/**
 * @brief This only happens with SIGSEGV or SIGILL.
 *        Time to die, but at least log this error with your dying breath.
 * @param val : Used to indicate the cause.
 */
#if defined(_linux_) || defined(_bsd_) || defined(_macos_) || defined(_solaris_)
#include <execinfo.h>
#endif

void
sig_outch (int val)
{
#if defined(_linux_) || defined(_bsd_) || defined(_macos_) || defined(_solaris_)
    void *array[20];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 20);
    strings = backtrace_symbols (array, size);
#endif
#if defined(WIN32)
    const ULONG framesToSkip = 0;
    const ULONG framesToCapture = 63;
    void *backTrace[63];
    ULONG backTraceHash = 0;

    const USHORT nFrame =
	CaptureStackBackTrace (framesToSkip, framesToCapture, backTrace,
			       &backTraceHash);
#endif

    log_file (LOG_ERROR, "FATAL ERROR: SIGSEGV or SIGILL Value: %d\n", val);

#if defined(_linux_) || defined(_bsd_) || defined(_macos_) || defined(_solaris_)
    log_file (LOG_ERROR, "Obtained %zd stack frames\n", size);
    for (i = 0; i < size; i++)
    {
	log_file (LOG_ERROR, "    %s\n", strings[i]);
    }
    free (strings);
#endif

#if defined(WIN32)
    for (USHORT iFrame = 0; iFrame < nFrame; ++iFrame)
    {
	log_file (LOG_ERROR, "[%3d] = %p\n", iFrame, backTrace[iFrame]);
    }
    log_file (LOG_ERROR, "backTraceHash = %08x\n", backTraceHash);
#endif
    /* With my dying breath... try to tell the user at the console what it happening */
    R_exit (NETMIST_FAULT, val);
    /* Now just die. */
    exit (5);
}


/**
 * @brief Implements the clean/graceful exit.  Performs the 
 *        cleanup and closing of files.
 *
 * @param val  : Indicates Netmist error code.
 * @param pval : Indicates platform specific error code. (e.g. errno)
 */
/*
 * __doc__
 * __doc__  Function : void clean_exit(int val, int pval)
 * __doc__  Arguments: enum netmist_child_err val: Indicates netmist error code.
 * __doc__             int pval: Platform-specific error code.
 * __doc__  Returns  : void
 * __doc__  Performs : Implements the clean/graceful exit.  Performs the 
 * __doc__             cleanup and closing of files.
 * __doc__
 */

void
clean_exit (enum netmist_child_err val, int pval)
{
    if (is_prime ())
    {
	log_all (LOG_ERROR, "Prime received SIGINT signal. Sig = %d\n", val);
	prime_send_kill_procs ();
    }
    else if (is_nodeManager ())
    {
	log_file (LOG_ERROR, "nodeManager received SIGINT signal\n");
	nodeManager_send_kill_procs ();
    }
    else
    {
	log_file (LOG_ERROR, "Client received SIGINT signal\n");
	/*  Clean up any left over dir_lock files */
	if (cmdline.sharing_flag)
	{
	    drain_list ();
	}

	/* Can't call teardown unless netmist_vfs_workdir has been initialized */
	if (netmist_vfs_workdir)
	    (*Netmist_Ops[my_workload].teardown) ();

	/* Don't send messages from a signal handler !! */
	if ((val != 0) & !signal_flag)
	    tell_nm_error (cmdline.client_id, val, pval);
    }
#if defined(WIN32)
    /* stop network */
    win32_close ();
#endif
    log_file (LOG_EXEC, "Exiting..\n");
    exit (val);
}

/**
 * @brief Called by exit paths on remote children. Indicates
 *        things are not going well and the test needs to
 *        cleanup and exit. 
 *
 * @param val    : Indicates Netmist error code / cause of exit.
 * @param subval : Indicates the platform specific error value. 
 */
/*
 * __doc__
 * __doc__  Function : void R_exit(enum netmist_child_err val, int subval)
 * __doc__  Arguments: enum netmist_child_err val: Indicates Netmist specific error value.
 * __doc__             int subval: Indicates the platform specific error value. e.g errno in Unix
 * __doc__  Returns  : void
 * __doc__  Performs : Called by exit paths on remote children. Indicates
 * __doc__             things are not going well and the test needs to
 * __doc__             cleanup and exit. 
 * __doc__
 */
void
R_exit (enum netmist_child_err val, int subval)
{
    if (is_prime ())		/* If I am prime */
    {
	log_all (LOG_ERROR,
		 "Exit with cleanup. Terminating remote processes.\n");
	prime_send_kill_procs ();
    }
    else if (is_nodeManager ())
    {
	log_file (LOG_ERROR, "Exit with cleanup. NM failure.\n");
	tell_prime_error (cmdline.client_id, val, subval);
    }
    else
    {
	signal_flag = 1;
	log_file (LOG_ERROR, "Exit with cleanup. Client failure. Error: %s\n",
		  netmist_strerror (netmist_get_error ()));
	/*  Clean up any left over dir_lock files */
	if (cmdline.sharing_flag)
	{
	    drain_list ();
	}
	/* Tell the NodeManager the cause, if you have one */
	if (val != NETMIST_NO_CHILD_ERR)
	    tell_nm_error (cmdline.client_id, val, subval);

	if (Netmist_Ops[my_workload].teardown)
	    (*Netmist_Ops[my_workload].teardown) ();

    }
    while (1)
    {
	log_file (LOG_EXEC, "Waiting to be terminated\n");
	sleep (1000);
    }
#if defined(WIN32)
    /* close network connection */
    win32_close ();
#endif

    exit (val);
}


/**
 * @brief Your generic help screen.
 */
/*
 * __doc__
 * __doc__  Function : void usage(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Your generic help screen.
 * __doc__
 */
void
usage (void)
{
    unsigned int sum_val = 0;

    reset_timestamp_in_log_stdout ();

    log_stdout (LOG_EXEC, "\n\t%s: \n\n", short_testname);
    log_stdout (LOG_EXEC, "\tUser options:\n");
    log_stdout (LOG_EXEC, "\tRequired:\n");
    log_stdout (LOG_EXEC,
		"\t[-m ]  ................. Prime flag - 1 Prime, 2 NodeManager, 3 Clients.\n");
    printf
	("\t[-b #[mMgGtT]] ......... Client memory size in [MGT]iGytes.\n");
    printf
	("\t[-B #[mMgGtT]] ......... Adjustable aggregate data set value in [MGT]iBytes.\n");
    log_stdout (LOG_EXEC,
		"\t[-d #]  ................ Number of processes/client.\n");
    printf
	("\t[-g client_token_file] . Path to file that contains token based info.\n");
    log_stdout (LOG_EXEC, "\tor\n");
#if defined(DEPRECATED)
    printf
	("\t[-f client_filename] ... Path to file that contains client names. (deprecated)\n");
#endif
    log_stdout (LOG_EXEC, "\t\n");
    log_stdout (LOG_EXEC, "\tOptional:\n");
    log_stdout (LOG_EXEC, "\t[-t #] ................. Seconds to run.\n");
    log_stdout (LOG_EXEC, "\t[-w #] ................. Seconds to warmup.\n");
    printf
	("\t[-r #[kKmMgG] .......... Force average file size to this size.\n");
    log_stdout (LOG_EXEC,
		"\t[-G]   ................. Enable heartbeat notifications.\n");
    log_stdout (LOG_EXEC,
		"\t[-N]   ................. Enable Op latency collection.\n");
    log_stdout (LOG_EXEC,
		"\t[-O #] ................. Requested Op/sec rate.\n");
    log_stdout (LOG_EXEC,
		"\t[-q License_key_file] .. Location of licent key file..\n");
    log_stdout (LOG_EXEC,
		"\t[-Q #] ................. Number of files per dir.\n");
    log_stdout (LOG_EXEC,
		"\t[-R primes dir ]  ...... Prime's result directory name.\n");
    log_stdout (LOG_EXEC,
		"\t[-T #] ................. Number of dirs per proc.\n");
    log_stdout (LOG_EXEC,
		"\t[-U log_path ] ......... Directory path for clients logs.\n");
    log_stdout (LOG_EXEC,
		"\t[-l log_filename] ...... Prime's log filename.\n");
    log_stdout (LOG_EXEC,
		"\t[-i] ................... Information without run.\n");
    log_stdout (LOG_EXEC,
		"\t[-k ]  ................. Kill the testing on all clients.\n");
    log_stdout (LOG_EXEC,
		"\t[-v ]  ................. Version information.\n");
    log_stdout (LOG_EXEC,
		"\t[-D ]  ................. Install benchmark on clients.\n");
#if defined(PRE_YAML)
    log_stdout (LOG_EXEC,
		"\t[-E ]  ................. Export mix file to stdout.\n");
    log_stdout (LOG_EXEC,
		"\t[-I filename]  ......... Import mix from file.\n");
#endif
    log_stdout (LOG_EXEC,
		"\t[-H name ]  ............ Pit_server's hostname.\n");
    log_stdout (LOG_EXEC, "\t[-S # ]  ............... Pit_server's port.\n");
    log_stdout (LOG_EXEC, "\t[-F ]  ................. Disable all fsyncs.\n");
    log_stdout (LOG_EXEC,
		"\t[-K ]  ................. Disable file unlink.\n");
    log_stdout (LOG_EXEC,
		"\t[-u ]  ................. Use rsh/rcp instead of ssh/scp.\n");
    log_stdout (LOG_EXEC, "\t[-y 6]  ................ Use IPv6.\n");
    log_stdout (LOG_EXEC, "\t[-z ]  ................. Run locally only.\n");
    log_stdout (LOG_EXEC,
		"\t[-a offset]  ........... Addional byte offset for xfers.\n");
    printf
	("\t[-Y ]  ................. Dump file access list to the client log.\n");
    printf
	("\t[-J #]  ................ Limit open file desc caching to this value.\n");
    log_stdout (LOG_EXEC, "\t[-X ]  ................. Sharing mode.\n");
    log_stdout (LOG_EXEC,
		"\t[-f name]  ............. External monitor script name.\n");
    printf
	("\t[-j args]  ............. External monitor script arguments.\n");
    log_stdout (LOG_EXEC,
		"\t[-x ]  ................. Enable socket debug.\n");
    printf
	("\t[-2 #]  ................ Enable trace debug: 0 = none, 1 = normal, 2 = init + normal.\n");
    log_stdout (LOG_EXEC,
		"\t[-3 #]  ................ Disable op validation.\n");
    log_stdout (LOG_EXEC, "\t[-h ]  ................. Help menu.\n");
    log_stdout (LOG_EXEC, "\n");
    log_stdout (LOG_EXEC, "\tInternal use only. Do not use.\n");
    log_stdout (LOG_EXEC,
		"\t[-e execdir ]  ......... Location of executable on client.\n");
    log_stdout (LOG_EXEC,
		"\t[-c #] ................. Total Number of clients.\n");
    log_stdout (LOG_EXEC,
		"\t[-n #] ................. Client identification number.\n");
    log_stdout (LOG_EXEC,
		"\t[-p directory path] .... Path to working directory.\n");
    log_stdout (LOG_EXEC, "\t[-M prime's name ] ..... Prime's name.\n");
    log_stdout (LOG_EXEC,
		"\t[-s #]  ................ File size in kbytes.\n");
    log_stdout (LOG_EXEC, "\t[-o #]  ................ Ops/sec.\n");
    log_stdout (LOG_EXEC,
		"\t[-P #]  ................ Prime's listen port.\n");
    log_stdout (LOG_EXEC, "\t[-W]  .................. Workload name.\n");
    log_stdout (LOG_EXEC,
		"\t[-L]  .................. Debug on local filesystem.\n\n");
    if (cmdline.yaml_flag == 1)
    {
	/* Need to call this, as the prime has not done this yet. */
	prime_check_license_file ();
	sum_val = check_sum (my_mix_table, lic_key);

	if (modified_run)
	    sum_val = 0;

	if (lic_key == 0)
	    log_stdout (LOG_EXEC, "\tPublc Finger Print     %u \n",
			(unsigned int) sum_val);
	else
	    log_stdout (LOG_EXEC, "\tRegistered Finger Print     %u \n",
			(unsigned int) sum_val);

	log_stdout (LOG_EXEC, "\n\n");
    }
}

/**
 * @brief This function purges the page cache on systems.  
 *        It also creates a huge memory load. So.. it is 
 *        currently disabled.
 */
/*
 * __doc__
 * __doc__  Function : void purge_cache(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : This function purges the page cache on systems.  
 * __doc__             It also creates a huge memory load. So.. it is 
 * __doc__             currently disabled.
 * __doc__
 */
void
purge_cache (void)
{
    int j;
    long long memsize;

    /* Disabled for now. It drives Linux into a VM nightmare */
    if (1)
	return;

    memsize = cmdline.fsize;

    log_file (LOG_DEBUG, "Client proc memsize %lld Kbytes\n", memsize);

    if (!mem_buf)
    {
	mem_buf = (long *) MALLOC (memsize * 1024);
	memset ((void *) mem_buf, 0, (size_t) (memsize * 1024));
    }

    for (j = 0; j < memsize * 1024; j += 1024)
    {
	mem_buf[j] = 0;
    }
}

/**
 * @brief Clients tell the prime the results for a given child.
 *
 * @param[in] chid         : child ID.
 * @param[in] client_name  : Name of the client.
 */
/*
 * __doc__
 * __doc__  Function : void tell_prime_results(int chid, char *client_name)
 * __doc__  Arguments: int child ID.
 * __doc__  Arguments: char *client_name.
 * __doc__  Returns  : void
 * __doc__  Performs : Clients tell the prime their results.
 * __doc__
 */
void
client_populate_results (int chid, char *client_name)
{
    int u;

    log_file (LOG_DEBUG, "Tell prime results\n");

    reset_all_prime_comm_buffers ();

    g_mc->m_command = R_SENDING_RESULTS;
    g_mc->m_client_number = (int) chid;
    g_mc->m_work_obj_index = (int) my_workload;
    my_strncpy (g_mc->m_client_name, client_name, MAXHNAME);
    my_strncpy (g_mc->m_workload_name, work_obj[my_workload].work_obj_name,
		MAXWLNAME);

    g_mc->m_read_count = work_obj[my_workload].read_op_count;
    my_strncpy (g_mc->m_read_string, my_mix_table[my_workload].read_string,
		OP_STR_LEN);

    g_mc->m_read_file_count = work_obj[my_workload].read_file_op_count;
    my_strncpy (g_mc->m_read_file_string,
		my_mix_table[my_workload].read_file_string, OP_STR_LEN);

    g_mc->m_read_rand_count = work_obj[my_workload].read_rand_op_count;
    my_strncpy (g_mc->m_read_rand_string,
		my_mix_table[my_workload].read_rand_string, OP_STR_LEN);

    g_mc->m_write_count = work_obj[my_workload].write_op_count;
    my_strncpy (g_mc->m_write_string, my_mix_table[my_workload].write_string,
		OP_STR_LEN);

    g_mc->m_write_file_count = work_obj[my_workload].write_file_op_count;
    my_strncpy (g_mc->m_write_file_string,
		my_mix_table[my_workload].write_file_string, OP_STR_LEN);

    g_mc->m_open_count = work_obj[my_workload].open_op_count;
    g_mc->m_close_count = work_obj[my_workload].close_op_count;

    g_mc->m_mmap_write_count = work_obj[my_workload].mmap_write_op_count;
    my_strncpy (g_mc->m_mmap_write_string,
		my_mix_table[my_workload].mmap_write_string, OP_STR_LEN);

    g_mc->m_mmap_read_count = work_obj[my_workload].mmap_read_op_count;
    my_strncpy (g_mc->m_mmap_read_string,
		my_mix_table[my_workload].mmap_read_string, OP_STR_LEN);

    g_mc->m_write_rand_count = work_obj[my_workload].write_rand_op_count;
    my_strncpy (g_mc->m_write_rand_string,
		my_mix_table[my_workload].write_rand_string, OP_STR_LEN);

    g_mc->m_rmw_count = work_obj[my_workload].rmw_op_count;
    my_strncpy (g_mc->m_rmw_string, my_mix_table[my_workload].rmw_string,
		OP_STR_LEN);

    g_mc->m_mkdir_count = work_obj[my_workload].mkdir_op_count;
    my_strncpy (g_mc->m_mkdir_string, my_mix_table[my_workload].mkdir_string,
		OP_STR_LEN);

    g_mc->m_rmdir_count = work_obj[my_workload].rmdir_op_count;
    my_strncpy (g_mc->m_rmdir_string, my_mix_table[my_workload].rmdir_string,
		OP_STR_LEN);

    g_mc->m_unlink_count = work_obj[my_workload].unlink_op_count;
    my_strncpy (g_mc->m_unlink_string,
		my_mix_table[my_workload].unlink_string, OP_STR_LEN);

    g_mc->m_unlink2_count = work_obj[my_workload].unlink2_op_count;
    my_strncpy (g_mc->m_unlink2_string,
		my_mix_table[my_workload].unlink2_string, OP_STR_LEN);

    g_mc->m_create_count = work_obj[my_workload].create_op_count;
    my_strncpy (g_mc->m_create_string,
		my_mix_table[my_workload].create_string, OP_STR_LEN);

    g_mc->m_append_count = work_obj[my_workload].append_op_count;
    my_strncpy (g_mc->m_append_string,
		my_mix_table[my_workload].append_string, OP_STR_LEN);

    g_mc->m_locking_count = work_obj[my_workload].lock_op_count;
    my_strncpy (g_mc->m_locking_string,
		my_mix_table[my_workload].locking_string, OP_STR_LEN);

    g_mc->m_access_count = work_obj[my_workload].access_op_count;
    my_strncpy (g_mc->m_access_string,
		my_mix_table[my_workload].access_string, OP_STR_LEN);

    g_mc->m_chmod_count = work_obj[my_workload].chmod_op_count;
    my_strncpy (g_mc->m_chmod_string, my_mix_table[my_workload].chmod_string,
		OP_STR_LEN);

    g_mc->m_readdir_count = work_obj[my_workload].readdir_op_count;
    my_strncpy (g_mc->m_readdir_string,
		my_mix_table[my_workload].readdir_string, OP_STR_LEN);

    g_mc->m_stat_count = work_obj[my_workload].stat_op_count;
    my_strncpy (g_mc->m_stat_string, my_mix_table[my_workload].stat_string,
		OP_STR_LEN);

    g_mc->m_neg_stat_count = work_obj[my_workload].neg_stat_op_count;
    my_strncpy (g_mc->m_neg_stat_string,
		my_mix_table[my_workload].neg_stat_string, OP_STR_LEN);

    g_mc->m_copyfile_count = work_obj[my_workload].copyfile_op_count;
    my_strncpy (g_mc->m_copyfile_string,
		my_mix_table[my_workload].copyfile_string, OP_STR_LEN);

    g_mc->m_rename_count = work_obj[my_workload].rename_op_count;
    my_strncpy (g_mc->m_rename_string,
		my_mix_table[my_workload].rename_string, OP_STR_LEN);

    g_mc->m_statfs_count = work_obj[my_workload].statfs_op_count;
    my_strncpy (g_mc->m_statfs_string,
		my_mix_table[my_workload].statfs_string, OP_STR_LEN);

    g_mc->m_pathconf_count = work_obj[my_workload].pathconf_op_count;
    my_strncpy (g_mc->m_pathconf_string,
		my_mix_table[my_workload].pathconf_string, OP_STR_LEN);

    g_mc->m_trunc_count = work_obj[my_workload].trunc_op_count;
    my_strncpy (g_mc->m_trunc_string, my_mix_table[my_workload].trunc_string,
		OP_STR_LEN);

    g_mc->m_custom1_count = work_obj[my_workload].custom1_op_count;
    my_strncpy (g_mc->m_custom1_string,
		my_mix_table[my_workload].custom1_string, OP_STR_LEN);

    g_mc->m_custom2_count = work_obj[my_workload].custom2_op_count;
    my_strncpy (g_mc->m_custom2_string,
		my_mix_table[my_workload].custom2_string, OP_STR_LEN);

/* HHHH */

    if (cmdline.Op_lat_flag)
    {
	g_mc->m_read_time = work_obj[my_workload].read_op_time;
	g_mc->m_min_read_latency = work_obj[my_workload].min_read_op_latency;
	g_mc->m_max_read_latency = work_obj[my_workload].max_read_op_latency;

	g_mc->m_read_file_time = work_obj[my_workload].read_file_op_time;
	g_mc->m_min_read_file_latency =
	    work_obj[my_workload].min_read_file_op_latency;
	g_mc->m_max_read_file_latency =
	    work_obj[my_workload].max_read_file_op_latency;

	g_mc->m_read_rand_time = work_obj[my_workload].read_rand_op_time;
	g_mc->m_min_read_rand_latency =
	    work_obj[my_workload].min_read_rand_op_latency;
	g_mc->m_max_read_rand_latency =
	    work_obj[my_workload].max_read_rand_op_latency;

	g_mc->m_rmw_time = work_obj[my_workload].rmw_op_time;
	g_mc->m_min_rmw_latency = work_obj[my_workload].min_rmw_op_latency;
	g_mc->m_max_rmw_latency = work_obj[my_workload].max_rmw_op_latency;

	g_mc->m_write_time = work_obj[my_workload].write_op_time;
	g_mc->m_min_write_latency =
	    work_obj[my_workload].min_write_op_latency;
	g_mc->m_max_write_latency =
	    work_obj[my_workload].max_write_op_latency;

	g_mc->m_write_file_time = work_obj[my_workload].write_file_op_time;
	g_mc->m_min_write_file_latency =
	    work_obj[my_workload].min_write_file_op_latency;
	g_mc->m_max_write_file_latency =
	    work_obj[my_workload].max_write_file_op_latency;

	g_mc->m_mmap_write_time = work_obj[my_workload].mmap_write_op_time;
	g_mc->m_min_mmap_write_latency =
	    work_obj[my_workload].min_mmap_write_op_latency;
	g_mc->m_max_mmap_write_latency =
	    work_obj[my_workload].max_mmap_write_op_latency;

	g_mc->m_mmap_read_time = work_obj[my_workload].mmap_read_op_time;
	g_mc->m_min_mmap_read_latency =
	    work_obj[my_workload].min_mmap_read_op_latency;
	g_mc->m_max_mmap_read_latency =
	    work_obj[my_workload].max_mmap_read_op_latency;

	g_mc->m_write_rand_time = work_obj[my_workload].write_rand_op_time;
	g_mc->m_min_write_rand_latency =
	    work_obj[my_workload].min_write_rand_op_latency;
	g_mc->m_max_write_rand_latency =
	    work_obj[my_workload].max_write_rand_op_latency;

	g_mc->m_rmw_time = work_obj[my_workload].rmw_op_time;
	g_mc->m_min_rmw_latency = work_obj[my_workload].min_rmw_op_latency;
	g_mc->m_max_rmw_latency = work_obj[my_workload].max_rmw_op_latency;

	g_mc->m_mkdir_time = work_obj[my_workload].mkdir_op_time;
	g_mc->m_min_mkdir_latency =
	    work_obj[my_workload].min_mkdir_op_latency;
	g_mc->m_max_mkdir_latency =
	    work_obj[my_workload].max_mkdir_op_latency;

	g_mc->m_rmdir_time = work_obj[my_workload].rmdir_op_time;
	g_mc->m_min_rmdir_latency =
	    work_obj[my_workload].min_rmdir_op_latency;
	g_mc->m_max_rmdir_latency =
	    work_obj[my_workload].max_rmdir_op_latency;

	g_mc->m_unlink_time = work_obj[my_workload].unlink_op_time;
	g_mc->m_min_unlink_latency =
	    work_obj[my_workload].min_unlink_op_latency;
	g_mc->m_max_unlink_latency =
	    work_obj[my_workload].max_unlink_op_latency;

	g_mc->m_unlink2_time = work_obj[my_workload].unlink2_op_time;
	g_mc->m_min_unlink2_latency =
	    work_obj[my_workload].min_unlink2_op_latency;
	g_mc->m_max_unlink2_latency =
	    work_obj[my_workload].max_unlink2_op_latency;

	g_mc->m_create_time = work_obj[my_workload].create_op_time;
	g_mc->m_min_create_latency =
	    work_obj[my_workload].min_create_op_latency;
	g_mc->m_max_create_latency =
	    work_obj[my_workload].max_create_op_latency;

	g_mc->m_append_time = work_obj[my_workload].append_op_time;
	g_mc->m_min_append_latency =
	    work_obj[my_workload].min_append_op_latency;
	g_mc->m_max_append_latency =
	    work_obj[my_workload].max_append_op_latency;

	g_mc->m_access_time = work_obj[my_workload].access_op_time;
	g_mc->m_min_access_latency =
	    work_obj[my_workload].min_access_op_latency;
	g_mc->m_max_access_latency =
	    work_obj[my_workload].max_access_op_latency;

	g_mc->m_chmod_time = work_obj[my_workload].chmod_op_time;
	g_mc->m_min_chmod_latency =
	    work_obj[my_workload].min_chmod_op_latency;
	g_mc->m_max_chmod_latency =
	    work_obj[my_workload].max_chmod_op_latency;

	g_mc->m_readdir_time = work_obj[my_workload].readdir_op_time;
	g_mc->m_min_readdir_latency =
	    work_obj[my_workload].min_readdir_op_latency;
	g_mc->m_max_readdir_latency =
	    work_obj[my_workload].max_readdir_op_latency;

	g_mc->m_stat_time = work_obj[my_workload].stat_op_time;
	g_mc->m_min_stat_latency = work_obj[my_workload].min_stat_op_latency;
	g_mc->m_max_stat_latency = work_obj[my_workload].max_stat_op_latency;

	g_mc->m_neg_stat_time = work_obj[my_workload].neg_stat_op_time;
	g_mc->m_min_neg_stat_latency =
	    work_obj[my_workload].min_neg_stat_op_latency;
	g_mc->m_max_neg_stat_latency =
	    work_obj[my_workload].max_neg_stat_op_latency;

	g_mc->m_copyfile_time = work_obj[my_workload].copyfile_op_time;
	g_mc->m_min_copyfile_latency =
	    work_obj[my_workload].min_copyfile_op_latency;
	g_mc->m_max_copyfile_latency =
	    work_obj[my_workload].max_copyfile_op_latency;

	g_mc->m_locking_time = work_obj[my_workload].lock_op_time;
	g_mc->m_min_locking_latency =
	    work_obj[my_workload].min_lock_op_latency;
	g_mc->m_max_locking_latency =
	    work_obj[my_workload].max_lock_op_latency;

	g_mc->m_rename_time = work_obj[my_workload].rename_op_time;
	g_mc->m_min_rename_latency =
	    work_obj[my_workload].min_rename_op_latency;
	g_mc->m_max_rename_latency =
	    work_obj[my_workload].max_rename_op_latency;

	g_mc->m_statfs_time = work_obj[my_workload].statfs_op_time;
	g_mc->m_min_statfs_latency =
	    work_obj[my_workload].min_statfs_op_latency;
	g_mc->m_max_statfs_latency =
	    work_obj[my_workload].max_statfs_op_latency;

	g_mc->m_pathconf_time = work_obj[my_workload].pathconf_op_time;
	g_mc->m_min_pathconf_latency =
	    work_obj[my_workload].min_pathconf_op_latency;
	g_mc->m_max_pathconf_latency =
	    work_obj[my_workload].max_pathconf_op_latency;

	g_mc->m_trunc_time = work_obj[my_workload].trunc_op_time;
	g_mc->m_min_trunc_latency =
	    work_obj[my_workload].min_trunc_op_latency;
	g_mc->m_max_trunc_latency =
	    work_obj[my_workload].max_trunc_op_latency;

	g_mc->m_custom1_time = work_obj[my_workload].custom1_op_time;
	g_mc->m_min_custom1_latency =
	    work_obj[my_workload].min_custom1_op_latency;
	g_mc->m_max_custom1_latency =
	    work_obj[my_workload].max_custom1_op_latency;

	g_mc->m_custom2_time = work_obj[my_workload].custom2_op_time;
	g_mc->m_min_custom2_latency =
	    work_obj[my_workload].min_custom2_op_latency;
	g_mc->m_max_custom2_latency =
	    work_obj[my_workload].max_custom2_op_latency;

	for (u = 0; u < BUCKETS; u++)
	{
	    g_mc->m_bands[u] = buckets[u];

	    log_file (LOG_RESULTS_VERBOSE, "band[%d] = %lld\n", u,
		      buckets[u]);
	}
    }

    g_mc->m_background = my_mix_table[my_workload].background;
    g_mc->m_sharemode = my_mix_table[my_workload].sharemode;
    g_mc->m_uniform_file_size_dist =
	my_mix_table[my_workload].uniform_file_size_dist;
    g_mc->m_rand_dist_behavior = my_mix_table[my_workload].rand_dist_behavior;
    g_mc->m_cipher = my_mix_table[my_workload].cipher;
    g_mc->m_notify = my_mix_table[my_workload].notify;
    g_mc->m_lru_on = my_mix_table[my_workload].lru_on;
    g_mc->m_total_file_ops = work_obj[my_workload].total_file_ops;
    g_mc->m_total_file_op_time = work_obj[my_workload].total_file_op_time;
    g_mc->m_run_time = my_run_time;	/* My run time */
    g_mc->m_min_direct_size = vfst->netmist_vfs_direct_size (vfsr);
    g_mc->m_ops_per_second =
	(float) work_obj[my_workload].total_file_ops / (float) my_run_time;

    g_mc->m_average_latency =
	(work_obj[my_workload].total_file_op_time /
	 work_obj[my_workload].total_file_ops) * (double) 1000;
    g_mc->m_read_throughput = read_throughput;
    g_mc->m_read_kbytes = read_kbytes;
    g_mc->m_write_throughput = write_throughput;
    g_mc->m_write_kbytes = write_kbytes;
    g_mc->m_Nread_throughput = Nread_throughput;
    g_mc->m_Nread_kbytes = Nread_kbytes;
    g_mc->m_Nwrite_throughput = Nwrite_throughput;
    g_mc->m_Nwrite_kbytes = Nwrite_kbytes;
    g_mc->m_meta_r_kbytes = meta_r_kbytes;
    g_mc->m_meta_w_kbytes = meta_w_kbytes;
    g_mc->m_file_space_mb = file_space_mb;
    g_mc->m_init_files = gfile_count;
    g_mc->m_init_files_ws = init_new_file;
    g_mc->m_init_dirs = gdir_count;
    g_mc->m_modified_run = modified_run;

    prime_int_to_ext (g_mc, g_emc, cmdline.Op_lat_flag);
}



/**
 * @brief Fill a buffer with a moderately random pattern.
 *        In the future one might wish to think about 
 *        how dedup is going to be integrated into this tool.
 *
 * @param  id   : identifier
 * @param  patt : pattern
 * @param  size : size of buffer
 * @param  buf  : pointer to buffer
 */
/* 
 * __doc__
 * __doc__  Function : void
 * __doc__             generic_fill_buf(long id, long patt, int size, char *buf)
 * __doc__  Arguments: long identifier
 * __doc__             long pattern
 * __doc__             int size of buffer
 * __doc__             char *buf: pointer to buffer
 * __doc__  Returns  : void
 * __doc__  Performs : Fill a buffer with a moderately random pattern.
 * __doc__             In the future one might wish to think about 
 * __doc__             how dedup is going to be integrated into this tool.
 * __doc__
 */
void
generic_fill_buf (long id, long patt, int size, char *buf)
{
    int i;
    long *lbuf;
    lbuf = (long *) buf;
    for (i = 0; i < (size / (int) sizeof (long)); i++)
	*lbuf++ = (id ^ patt) * i;
}

/**
 * @brief The Programmable Interdimensional Timer interface code.
 *        Imagine living in a Universe where every planet has a 
 *        situation where time itself moves at a different speed 
 *        than on the other planets.  In that situation the 
 *        only way to run a meaningful test is to have everyone 
 *        agree to use the same stopwatch that is located on a 
 *        planet that everyone can see.
 *
 * @param  tp           : struct timeval pointer.
 * @param  pit_hostname : Host that has the time server
 * @param  pit_service  : Service name on that host
 */
/*
 * __doc__
 * __doc__  Function : int pit_gettimeofday()
 * __doc__  Arguments: struct timeval pointer.
 * __doc__             char *pit_hostname 
 * __doc__             char *pit_service name
 * __doc__  Returns  : void
 * __doc__  Performs : The Programmable Interdimensional Timer interface code.
 * __doc__             Imagine living in a Universe where every planet has a 
 * __doc__             situation where time itself moves at a different speed 
 * __doc__             than on the other planets.  In that situation the 
 * __doc__             only way to run a meaningful test is to have everyone 
 * __doc__             agree to use the same stopwatch that is located on a 
 * __doc__             planet that everyone can see.
 * __doc__
 */

#if defined(_solaris_)
int
pit_gettimeofday (struct timeval *tp, char *pit_hostname, char *pit_service)
#else
int
pit_gettimeofday (struct timeval *tp, char *pit_hostname, char *pit_service)
#endif
{
    SOCKET sckt;		/* socket descriptor */
    unsigned scopeId = 0;
    double mytime;

    /* See if the interdimensional rift is active */

    if (pit_hostname[0] == 0)
    {
#if defined(_solaris_)
	mytime = m_gettimeofday ((struct timeval *) tp);
#else
	mytime = m_gettimeofday ((struct timeval *) tp);
#endif

	log_file (LOG_EXEC_VERBOSE,
		  "time returned from m_gettimeofday() = %f\n", mytime);
	return (0);
    }

    if ((sckt = openSckt (pit_hostname,
			  pit_service, scopeId)) == INVALID_DESC)
    {
	log_file (LOG_ERROR,
		  "Sorry... a connectionless socket could "
		  "not be set up.\n");

	return -1;
    }
    /*
     ** Get the remote PIT.
     */
    pit (sckt, tp);
    closesocket (sckt);
    return 0;
}

/**
 * @brief Opens a socket for the PIT to use to get the time
 *        from a remote time server ( A PIT server ) 
 * 
 * @param  host : hostname for the PIT server
 * @param  service : service name
 * @param  scopeId : scopeID
 */
/*
 * __doc__
 * __doc__  Function : SOCKET openSckt( char *host,char *serv, int scopeID)
 * __doc__  Arguments: char *: hostname
 * __doc__             char *: service name
 * __doc__             int *: scopeID
 * __doc__  Returns  : SOCKET
 * __doc__  Performs : Opens a socket for the PIT to use to get the time
 * __doc__             from a remote time server ( A PIT server ) 
 * __doc__
 */
SOCKET
openSckt (const char *host, const char *service, unsigned int scopeId)
{
    SOCKET sckt;
    /*
     * Initialize the 'hints' structure for getaddrinfo(3).
     */
    memset ((void *) (&hints), 0, (size_t) (sizeof (hints)));
    hints.ai_family = PF_UNSPEC;	/* IPv4 or IPv6 records */

    hints.ai_socktype = SOCK_STREAM;	/* Connection oriented communication. */
    hints.ai_protocol = IPPROTO_TCP;	/* TCP transport layer protocol only. */
    /*
     * Look up the host/service information.
     */
    if ((aiErr = getaddrinfo (host, service, &hints, &aiHead)) != 0)
    {
	log_file (LOG_ERROR,
		  "(line %d): host - %s, service - %s. ERROR - %s.\n",
		  __LINE__, host, service, gai_strerror (aiErr));

	return INVALID_DESC;
    }
    /*
     * Go through the list and try to open a connection.  Continue until either
     * a connection is established or the entire list is exhausted.
     */
    for (ai = aiHead, sckt = INVALID_DESC;
	 (ai != NULL) && (sckt == INVALID_DESC); ai = ai->ai_next)
    {
	/*
	 ** IPv6 kluge.  Make sure the scope ID is set.
	 */
	if (ai->ai_family == PF_INET6)
	{
	    pSadrIn6 = (sockaddr_in6_t *) ai->ai_addr;
	    if (pSadrIn6->sin6_scope_id == 0)
	    {
		pSadrIn6->sin6_scope_id = scopeId;
	    }			/* End IF the scope ID wasn't set. */
	}			/* End IPv6 kluge. */
	/*
	 * Create a socket.
	 */
	sckt = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (sckt == INVALID_DESC)
	{
	    continue;		/* Try the next address record in the list. */
	}
	/*
	 * Set the target destination for the remote host on this socket.  That
	 * is, this socket only communicates with the specified host.
	 */
	if (connect (sckt, ai->ai_addr, ai->ai_addrlen))
	{
	    (void) closesocket (sckt);	/* Could use system call again here,
					   but why? */
	    sckt = INVALID_DESC;
	    continue;		/* Try the next address record in the list. */
	}
    }				/* End FOR each address record returned by getaddrinfo(3). */
    /*
     * Clean up & return.
     */
    freeaddrinfo (aiHead);	/* Keep cached */
    return sckt;
}				/* End openSckt() */


/**
 * @brief Read the PIT, and convert this back into timeval 
 *        info, and store it in the timeval structure that was
 *        passed in.
 *
 * @param  sckt : socket for the PIT
 * @param  tp : Return time in this struct.
 */
/*
 * __doc__
 * __doc__  Function : void pit(int socket, struct timeval tp)
 * __doc__  Arguments: int: socket for the PIT
 * __doc__             struct timeval *tp: Return time in this struct.
 * __doc__  Returns  : void
 * __doc__  Performs : Read the PIT, and convert this back into timeval 
 * __doc__             info, and store it in the timeval structure that was
 * __doc__             passed in.
 * __doc__
 */
void
pit (int sckt, struct timeval *tp)
{
    char bfr[MAXBFRSIZE + 1];
    int inBytes;
    unsigned long long value = 0LL;
    /*
     * Send a datagram to the server to wake it up.  The content isn't
     * important, but something must be sent to let it know we want the TOD.
     */
    junk = write (sckt, "Author: Don Capps", 17);
    /*
     * Read the PIT from the remote host.
     */
    inBytes = read (sckt, bfr, MAXBFRSIZE);
    if (inBytes <= 0)
    {
	log_file (LOG_ERROR, "PIT read error: Errno:  %s\n",
		  netmist_strerror (netmist_get_error ()));

	inBytes = 0;
    }
    bfr[inBytes] = '\0';	/* Null-terminate the received string. */
    /* 
     * Convert result to timeval structure format 
     */
    (void) sscanf (bfr, "%llu\n", &value);
    tp->tv_sec = (long) (value / 1000000);
    tp->tv_usec = (long) (value % 1000000);
    if (tp->tv_sec < 0)
    {
	log_file (LOG_ERROR, "****** OOPS ***** tv_sec is negative \n");

    }
    if (tp->tv_usec < 0)
    {
	log_file (LOG_ERROR, "****** OOPS ***** tv_usec is negative \n");

    }
}

/**
 * @brief The power monitor external script startup routine.
 *        Builds command, starts with popen() so it can read
 *        any output from the command, and passes one option
 *        to the command.
 *
 * @param  flag : Used to indicate state.
 * @param  options : Extra options to pass along.
 */
/*
 * __doc__
 * __doc__  Function : int power_mon(int flag, char *options)
 * __doc__  Arguments: int flag: Used to indicate state.
 * __doc__             char *options: Extra options to pass along.
 * __doc__  Returns  : int: Success or fail.
 * __doc__  Performs : The power monitor external script startup routine.
 * __doc__             Builds command, starts with popen() so it can read
 * __doc__             any output from the command, and passes one option
 * __doc__             to the command.
 * __doc__
 */
int
power_mon (int flag, char *options)
{
    char command[MAXECOMMAND];
    char result[MAXECOMMAND];
    FILE *fp;

    /* Zero out buffer */
    memset ((void *) command, 0, (size_t) MAXECOMMAND);
    memset ((void *) result, 0, (size_t) MAXECOMMAND);
    /* If told to run the command */
    if (flag == SRUN)
    {
	/* Check for existance and executable */
	if ((access (power_script, F_OK) == 0) &&
	    (access (power_script, X_OK) == 0))
	{
	    /* Build command */
#if defined(WIN32)
	    snprintf (command, sizeof command, "%s %s\n", power_script,
		      options);
#else
	    snprintf (command, sizeof command, "%s %s", power_script,
		      options);
#endif
	    /* Run command */
	    fp = (FILE *) popen (command, "r");
	    if (fp == 0)
		return (1);
	    /* Read any returned output                                 */
	    /* The file position could be at EOF before we read the     */
	    /* file, this is not an error.                              */
	    if (feof (fp))
	    {
		pclose (fp);
		return (0);
	    }
	    else
	    {
		ubuntu = fread (result, MAXECOMMAND, 1, fp);
		if (ubuntu)
		{
		    ;
		}
		if (result[0] != 0)
		    log_stdout (LOG_EXEC, "\tPower returned: %s \n", result);
		pclose (fp);
		return (0);
	    }
	}
	return (1);
    }
    /* Just check to see if the command is present and executable */
    if (flag == SCHECK)
    {
	if ((access (power_script, F_OK) == 0) &&
	    (access (power_script, X_OK) == 0))
	{
	    return (0);
	}
	return (1);
    }
    return (1);
}

/**
 * @brief The external monitor script startup routine.
 *        Builds command, starts with popen() so it can read
 *        any output from the command, and passes one option
 *        to the command.
 *
 * @param  flag : Pass state information.
 * @param  options : Pass along extra options.
 */
/*
 * __doc__
 * __doc__  Function : int extern_mon(int flag, char *options)
 * __doc__  Arguments: int flag: Pass state information.
 * __doc__             char *: Pass along extra options.
 * __doc__  Returns  : void
 * __doc__  Performs : The external monitor script startup routine.
 * __doc__             Builds command, starts with popen() so it can read
 * __doc__             any output from the command, and passes one option
 * __doc__             to the command.
 * __doc__
 */
int
extern_mon (int flag, char *options)
{
    char command[MAXECOMMAND];
    char result[MAXECOMMAND];
    FILE *fp;

    /* Zero out buffer */
    memset ((void *) command, 0, (size_t) MAXECOMMAND);
    memset ((void *) result, 0, (size_t) MAXECOMMAND);

    /* If told to run the command */
    if (flag == SRUN)
    {
	/* Check for existance and executable */
	if ((access (mon_script, F_OK) == 0)
	    && (access (mon_script, X_OK) == 0))
	{
	    /* Build command */
#if defined(WIN32)
	    snprintf (command, sizeof command, "%s %s\n", mon_script,
		      options);
#else
	    snprintf (command, sizeof command, "%s %s", mon_script, options);
#endif
	    /* Run command */
	    fp = (FILE *) popen (command, "r");
	    if (fp == 0)
		return (1);
	    /* Read any returned output                                 */
	    /* The file position could be at EOF before we read the     */
	    /* file, this is not an error.                              */
	    if (feof (fp))
	    {
		pclose (fp);
		return (0);
	    }
	    else
	    {
		ubuntu = fread (result, MAXECOMMAND, 1, fp);
		if (ubuntu)
		{
		    ;
		}
		if (result[0] != 0)
		    log_stdout (LOG_EXEC,
				"\tExternal monitor returned: %s \n", result);
		pclose (fp);
		return (0);
	    }
	}
	return (1);
    }
    /* Just check to see if the command is present and executable */
    if (flag == SCHECK)
    {
	if ((access (mon_script, F_OK) == 0)
	    && (access (mon_script, X_OK) == 0))
	{
	    return (0);
	}
	return (1);
    }
    return (1);
}

/**
 * @brief The user defined external monitor script startup routine.
 *        Builds command, starts with popen() so it can read
 *        any output from the command, and passes one option
 *        to the command.
 *
 * @param  flag : Pass state info
 * @param  options : Pass along extra options to the output.
 */
/*
 * __doc__
 * __doc__  Function : int u_extern_mon(int flag, char *options)
 * __doc__  Arguments: int flag: Pass state info
 * __doc__             char *: Pass along extra options to the output.
 * __doc__  Returns  : Int: success or fail
 * __doc__  Performs : The user defined external monitor script startup routine.
 * __doc__             Builds command, starts with popen() so it can read
 * __doc__             any output from the command, and passes one option
 * __doc__             to the command.
 * __doc__
 */
int
u_extern_mon (int flag, char *options)
{
    char command[MAXECOMMAND];
    char result[MAXECOMMAND];
    FILE *fp;

    /* Zero out buffer */
    memset ((void *) command, 0, (size_t) MAXECOMMAND);
    memset ((void *) result, 0, (size_t) MAXECOMMAND);

    /* If told to run the command */
    if (flag == SRUN)
    {
	/* Check for existance and executable */
	if ((access (cmdline.u_external_script_name, F_OK) == 0)
	    && (access (cmdline.u_external_script_name, X_OK) == 0))
	{
	    /* Build command */
#if defined(WIN32)
	    snprintf (command, sizeof command, "%s %s %s\n",
		      cmdline.u_external_script_name, options,
		      cmdline.u_external_script_args);
#else
	    snprintf (command, sizeof command, "%s %s %s",
		      cmdline.u_external_script_name, options,
		      cmdline.u_external_script_args);
#endif
	    /* Run command */
	    fp = (FILE *) popen (command, "r");
	    if (fp == 0)
		return (1);
	    /* Read any returned output                                 */
	    /* The file position could be at EOF before we read the     */
	    /* file, this is not an error.                              */
	    if (feof (fp))
	    {
		pclose (fp);
		return (0);
	    }
	    else
	    {
		ubuntu = fread (result, MAXECOMMAND, 1, fp);
		if (ubuntu)
		{
		    ;
		}
		if (result[0] != 0)
		    log_stdout (LOG_EXEC,
				"\tUser defined monitor returned: %s \n",
				result);
		pclose (fp);
		return (0);
	    }
	}
	else
	    return (1);
    }
    /* Just check to see if the command is present and executable */
    if (flag == SCHECK)
    {
	if ((access (cmdline.u_external_script_name, F_OK) == 0)
	    && (access (cmdline.u_external_script_name, X_OK) == 0))
	{
	    return (0);
	}
	else
	    return (1);
    }
    return (1);
}

/**
 * @brief The external stat script startup routine.
 *        Builds command, starts with popen() so it can read
 *        any output from the command, and passes one option
 *        to the command.
 *
 * @param  flag : flag to pass state info.
 * @param  options : Pass extra options to the output.
 */
/*
 * __doc__
 * __doc__  Function : int extern_stat(int flag, char *options)
 * __doc__  Arguments: int: flag to pass state info.
 * __doc__             char *: Pass extra options to the output.
 * __doc__  Returns  : void
 * __doc__  Performs : The external stat script startup routine.
 * __doc__             Builds command, starts with popen() so it can read
 * __doc__             any output from the command, and passes one option
 * __doc__             to the command.
 * __doc__
 */
int
extern_stat (int flag, char *options)
{
    char command[MAXECOMMAND];
    char result[MAXECOMMAND];
    FILE *fp;

    /* Zero out buffer */
    memset ((void *) command, 0, (size_t) MAXECOMMAND);
    memset ((void *) result, 0, (size_t) MAXECOMMAND);

    /* If told to run the command */
    if (flag == SRUN)
    {
	/* Check for existance and executable */
	if ((access (stat_script, F_OK) == 0)
	    && (access (stat_script, X_OK) == 0))
	{
	    /* Build command */
#if defined(WIN32)
	    snprintf (command, sizeof command, "%s %s\n", stat_script,
		      options);
#else
	    snprintf (command, sizeof command, "%s %s", stat_script, options);
#endif
	    /* Run command */
	    fp = (FILE *) popen (command, "r");
	    if (fp == 0)
		return (1);
	    /* Read any returned output                               */
	    /* The file position could be at EOF before we read the   */
	    /* file, this is not an error.                            */
	    if (feof (fp))
	    {
		pclose (fp);
		return (0);
	    }
	    else
	    {
		ubuntu = fread (result, MAXECOMMAND, 1, fp);
		if (ubuntu)
		{
		    ;
		}
		if (result[0] != 0)
		    log_stdout (LOG_EXEC, "\tExternal stat returned: %s \n",
				result);
		pclose (fp);
		return (0);
	    }
	}
    }
    /* Just check to see if the command is present and executable */
    if (flag == SCHECK)
    {
	if ((access (stat_script, F_OK) == 0) &&
	    (access (stat_script, X_OK) == 0))
	{
	    return (0);
	}
	return (1);
    }
    return (1);
}

/**
 * @brief Return current time in seconds (double), and also 
 *        fill in the timeval structure, in case someone cares.
 *        We do NOT use w_gettimeofday() because its resolution is 15ms.
 *        We use gettimeofday(),in Windows, because it uses high resolution timers.
 *
 * @param tp : timeval to hold current time.
 */
/*
 * __doc__
 * __doc__  Function : double m_gettimeofday(struct timeval *tp)
 * __doc__  Arguments: struct timeval, to hole current time.
 * __doc__  Returns  : double: Current time in seconds.
 * __doc__  Performs : Return current time in seconds (double), and also 
 * __doc__             fill in the timeval structure, in case someone cares.
 * __doc__
 * __doc__            We do NOT use w_gettimeofday() because its resolution is 15ms.
 * __doc__            We use gettimeofday() in Windows because it uses high resolution timers.
 */
#if defined(_solaris_)
struct timeval Oink;
double
m_gettimeofday (struct timeval *tp)
#else
double
m_gettimeofday (struct timeval *tp)
#endif
{
#if defined(_solaris_)
    if (gettimeofday ((struct timeval *) tp, (long) NULL) == -1)
#else
    if (gettimeofday ((struct timeval *) tp, (struct timezone *) NULL) == -1)
#endif
    {

	log_file (LOG_ERROR, "gettimeofday failed.. Exiting !!");
	exit (-1);
    }
    return ((double) (tp->tv_sec)) + (((double) tp->tv_usec / 1000000));
}


#define MVFDN

#if defined(MVFDN)

/*
 * Creating filenames with a character distribution that
 * approximates English usage. 
 * From the mind of Don Capps. (Mr. Iozone)
 */

/* The maximum number of character in the prefix name. */
#define PRENAME 8
/* The minimum number of character in the prefix name. */
#define MINPRENAME 3
/* The maximum number of character in the suffix name. */
#define POSTNAME 5
/* The minimum number of character in the suffix name. */
#define MINPOSTNAME 0

int min_pre_name_length = MINPRENAME;
int max_pre_name_length = PRENAME;
int min_post_name_length = MINPOSTNAME;
int max_post_name_length = POSTNAME;
/* The size of the frequency distribution table.       */
#define TABLE_SIZE 10000


int t_size;			/* The active size of the distribution table */

/*
 * Frequency of character usage in the English language.
 * Multiplied by 10000 for frequency table initialization.
 */
 /* Concise Oxford Dictionary 9th edition 1995 */
int freq[] =
    { 849, 207, 454, 338, 1116, 181, 247, 300, 754, 19, 110, 548, 301,
    665, 716, 316, 19, 758, 573, 695, 363, 100, 129, 29, 177, 27
};

/*
 * English character set.
 */
char ch[] = "abcdefghijklmnopqrstuvwxyz";

char table[TABLE_SIZE];
int dist_active;		/* Flag, to permit only initializing the table once. */


/**
 * @brief Make a file name that is [3->8].[0->5] compliant, with 
 *        pre and post dot lengths that are random, and 
 *        consisting of characters that have a proper frequency 
 *        distribution.
 *
 * @param  namebuf : char *buf: Holds filename
 * @param  seed : Unique file number
 * @param  type : type of file.
 * @param  filenum : Unique number for this file.
 */
/*
 * __doc__
 * __doc__  Function : void make_name(char *buf, int seed, int type, int fnum)
 * __doc__  Arguments: char *buf: Holds filename
 * __doc__             int seed: Unique file number
 * __doc__             int type: type of file.
 * __doc__             int int: Unique number for this file.
 * __doc__  Returns  : void
 * __doc__  Performs : Make a file name that is [3->8].[0->5] compliant, with 
 * __doc__             pre and post dot lengths that are random, and 
 * __doc__             consisting of characters that have a proper frequency 
 * __doc__             distribution.
 * __doc__
 * __doc__             Note: This routine saves the Netmist random number 
 * __doc__             generator seed before and restores it after makeing the 
 * __doc__             name. This is critical as one MUST not disturb the 
 * __doc__             random namespace or it will cause the entire benchmark 
 * __doc__             to get very funky.
 * __doc__
 */
void
make_name (char *namebuf, int seed, int type, int filenum)
{
    int x, y, pt, save;
    int skew = 0;
    char pre[MAXNAME];		/* Prefix buffer */
    char post[MAXNAME];		/* Suffix buffer */

    char special[5];

    special[0] = 0;
    special[1] = 0;
    special[2] = 0;
    special[3] = 0;
    special[4] = 0;

    if (dist_active == 0)	/* Only once please */
    {
	init_dist ();
	dist_active = 1;
    }
    save = name_gseed ();	/* Save the seed !!! */
    switch (type)
    {
    case 0:
	skew = 101;		/* 0 = Files. Note: Prime */
	break;
    case 1:
	skew = 997;		/* 1 = Directories. Note: Prime */
	break;
    case 2:
	skew = 2003;		/* 2 = Symlinks. Note: Prime */
	break;
    case 3:
	skew = 3001;		/* 3 = testdir name. Note: Prime */
	break;
    };

    /* 
     * seed is the Unique file number.
     * +1 to avoid confusing netmist_srand with a zero.
     * +skew to move the directories, files and symlinks apart.
     * +cmdline.client_id to move the children apart.
     */
    name_srand (seed + 1 + skew + impersonate);
  tagain:
    x = prename_len ();		/* Length of prefix */
    y = postname_len ();	/* Length of suffix */
    for (pt = 0; pt < x; pt++)	/* build up name */
    {
	pre[pt] = get_ch1 ();	/* Get character */
    }
    pre[x] = 0;			/* Put a null at the end so we won't need to memset */

    /* Oh Geeze... Have to avoid these magic names in Windows, or
     * whenever the destination filesystem is SMB 
     */
    if ((x == 3) || (x == 4))
    {
	special[0] = (char) toupper ((int) pre[0]);
	special[1] = (char) toupper ((int) pre[1]);
	special[2] = (char) toupper ((int) pre[2]);
	special[3] = (char) 0;
	if ((my_strcasecmp (special, "CON") == 0))
	    goto tagain;
	if ((my_strcasecmp (special, "PRN") == 0))
	    goto tagain;
	if ((my_strcasecmp (special, "LPT") == 0))
	    goto tagain;
	if ((my_strcasecmp (special, "AUX") == 0))
	    goto tagain;
	if ((my_strcasecmp (special, "COM") == 0))
	    goto tagain;
	if ((my_strcasecmp (special, "NUL") == 0))
	    goto tagain;
    }
    /* Oh Geeze... Have to avoid these magic names in Windows */

    for (pt = 0; pt < y; pt++)	/* build up name */
    {
	post[pt] = get_ch1 ();	/* get character */
    }
    post[y] = 0;		/* Put a null at the end so we won't need to memset */
    if (y == 0)
    {
	snprintf (namebuf, MAXNAME, "%s%d", pre, filenum);
    }
    else
    {
	snprintf (namebuf, MAXNAME, "%s%d.%s", pre, filenum, post);
    }
    name_srand (save);		/* Reset seed back to its original value !!! */
}

/**
 * @brief Make a short file name that is [3->8] compliant, with 
 *        pre lengths that are random, and consisting of 
 *        characters that have a proper frequency distribution.
 *
 * @param  namebuf : Holds file name
 * @param  seed : Unique file number.
 * @param  type : File type.
 */
/*
 * __doc__
 * __doc__  Function : void make_short_name(char *buf, int seed, int type)
 * __doc__  Arguments: Char * buf: Holds file name
 * __doc__             int seed: Unique file number.
 * __doc__             int type: File type.
 * __doc__  Returns  : void
 * __doc__  Performs : Make a short file name that is [3->8] compliant, with 
 * __doc__             pre lengths that are random, and consisting of 
 * __doc__             characters that have a proper frequency distribution.
 * __doc__
 */
void
make_short_name (char *namebuf, int seed, int type)
{
    int x, pt, save;
    int skew = 0;
    char pre[MAXNAME];		/* Prefix buffer */
    if (dist_active == 0)	/* Only once please */
    {
	init_dist ();
	dist_active = 1;
    }
    save = name_gseed ();	/* Save the seed !!! */
    switch (type)
    {
    case 0:
	skew = 101;		/* 0 = Files. Note: Prime */
	break;
    case 1:
	skew = 997;		/* 1 = Directories. Note: Prime */
	break;
    case 2:
	skew = 2003;		/* 2 = Symlinks. Note: Prime */
	break;
    case 3:
	skew = 3001;		/* 3 = testdir name. Note: Prime */
	break;
    };

    /* 
     * seed is the Unique file number.
     * +1 to avoid confusing netmist_srand with a zero.
     * +skew to move the directories, files and symlinks apart.
     * +cmdline.client_id to move the children apart.
     */
    name_srand (seed + 1 + skew + impersonate);
    x = prename_len ();		/* Length of prefix */
    for (pt = 0; pt < x; pt++)	/* build up name */
    {
	pre[pt] = get_ch1 ();	/* Get character */
    }
    pre[x] = 0;			/* Put a null at the end so we won't need to memset */
    snprintf (namebuf, MAXNAME, "%s", pre);
    name_srand (save);		/* Reset seed back to its original value !!! */
}

/**
 * @brief Pick a length for the pre-dot part of the file name.
 */
/*
 * __doc__
 * __doc__  Function : int prename_len(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : length of pre part of the short filename.
 * __doc__  Performs : Pick a length for the pre-dot part of the file name.
 * __doc__
 */
static int
prename_len (void)
{
    int pre;
    if (max_pre_name_length > MAXNAME)
	max_pre_name_length = MAXNAME;
    pre = name_rand () % (max_pre_name_length + 1);	/* Pick a random length */
    if (pre < min_pre_name_length)	/* Enforce minimum */
	pre = min_pre_name_length;
    return (pre);
}

/**
 * @brief Pick a length for the post-dot part of the file name.
 */
/*
 * __doc__
 * __doc__  Function : int postname_len(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : length of the post_name suffix.
 * __doc__  Performs : Pick a length for the post-dot part of the file name.
 * __doc__
 */
static int
postname_len (void)
{
    int post;
    if (max_post_name_length > MAXNAME)
	max_post_name_length = MAXNAME;
    post = name_rand () % (max_post_name_length + 1);	/* Pick a random length */
    if (post < min_post_name_length)	/* Enforce minimum */
	post = min_post_name_length;
    return (post);
}

/**
 * @brief Initialize the distribution table for later use in the
 *        frequency distribution mechanism.
 */
/*
 * __doc__
 * __doc__  Function : void int_dist(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Initialize the distribution table for later use in the
 * __doc__             frequency distribution mechanism.
 * __doc__
 */
static void
init_dist (void)
{
    unsigned int i;
    int j, pos = 0;

    for (i = 0; i < strlen (ch); i++)	/* Letters in the alphabet */
    {
	for (j = 0; j < freq[i]; j++)	/* Frequency information */
	{
	    table[pos] = ch[i];	/* Fill in the table */
	    pos++;
	}
    }
    t_size = pos;		/* Actual size of the active table plus one. */
}

/**
 * @brief Return a character with the language character 
 *        frequency distribution.
 */
/*
 * __doc__
 * __doc__  Function : char get_chi(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : A character with the correct frequency dist.
 * __doc__  Performs : Return a character with the language character 
 * __doc__             frequency distribution.
 * __doc__
 */
static char
get_ch1 (void)
{
    char val;			/* char to return */
    int where;			/* index into table */
    where = (name_rand () % t_size);	/* Random index into generated table */
    val = table[where];		/* Pick up the character */
    return (val);		/* Return char */
}

#endif /* MVFDN */

/**
 * @brief Allocate space for file table objects 
 */
/* 
 * __doc__
 * __doc__  Function : void init_space(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Allocate space for file table objects 
 * __doc__
 */
void
init_space (void)
{
    int size, i, shared_buckets;

    size =
	sizeof (struct file_object) * cmdline.client_dirs *
	cmdline.client_files;

    shared_buckets = my_mix_table[my_workload].shared_buckets;

    for (i = 0; i < NUM_OP_TYPES; i++)
    {
/* 
 *	Needed by client_validate(). Don't skip this one :-) 
 *	   if((shared_buckets==0) && (i==OP_WRITE) && (PERCENT_WRITE == 0))
 *		continue;
 */
	if ((shared_buckets == 0) && (i == OP_WRITE_FILE)
	    && (PERCENT_WRITE_FILE == 0))
	    continue;
	if ((shared_buckets == 0) && (i == OP_WRITE_RAND)
	    && (PERCENT_WRITE_RAND == 0))
	    continue;
	if ((shared_buckets == 0) && (i == OP_READ) && (PERCENT_READ == 0))
	    continue;
	if ((shared_buckets == 0) && (i == OP_READ_FILE)
	    && (PERCENT_READ_FILE == 0))
	    continue;
	if ((shared_buckets == 0) && (i == OP_READ_RAND)
	    && (PERCENT_READ_RAND == 0))
	    continue;
	if ((shared_buckets == 0) && (i == OP_RMW) && (PERCENT_RMW == 0))
	    continue;
	if (shared_buckets == 0 && i == OP_MKDIR && PERCENT_MKDIR == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_UNLINK && PERCENT_UNLINK == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_STAT && PERCENT_STAT == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_NEG_STAT && PERCENT_NEG_STAT == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_APPEND && PERCENT_APPEND == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_CREATE && PERCENT_CREATE == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_LOCKING && PERCENT_LOCKING == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_ACCESS && PERCENT_ACCESS == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_CHMOD && PERCENT_CHMOD == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_READDIR && PERCENT_READDIR == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_MM_WRITE && PERCENT_MM_WRITE == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_MM_READ && PERCENT_MM_READ == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_MM_READ && PERCENT_MM_READ == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_COPYFILE && PERCENT_COPYFILE == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_RENAME && PERCENT_RENAME == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_STATFS && PERCENT_STATFS == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_PATHCONF && PERCENT_PATHCONF == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_UNLINK2 && PERCENT_UNLINK2 == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_RMDIR && PERCENT_RMDIR == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_TRUNC && PERCENT_TRUNC == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_CUSTOM1 && PERCENT_CUSTOM1 == 0)
	    continue;
	if (shared_buckets == 0 && i == OP_CUSTOM2 && PERCENT_CUSTOM2 == 0)
	    continue;
	op_X_array[i] = (struct file_object *) MALLOC (size);
	if (op_X_array[i])
	{
	    /* zero out the memory */
	    memset ((void *) (op_X_array[i]), 0, (size_t) size);
	}
    }
}

/**
 * @brief Dump all of the file objects to a log.
 *        This is used to analyze the access counts of all of 
 *        the files touched during the test.
 *
 * @param out : Pointer to FILE for writing.
 */
/*
 * __doc__
 * __doc__  Function : void dump_file_objects(FILE *)
 * __doc__  Arguments: Pointer to FILE for writing.
 * __doc__  Returns  : void
 * __doc__  Performs : Dump all of the file objects to a log.
 * __doc__             This is used to analyze the access counts of all of 
 * __doc__             the files touched during the test.
 * __doc__
 */
void
dump_file_objects (FILE * out)
{
    int size, i, j;
    struct file_object *file_ptr;
    size = cmdline.client_dirs * cmdline.client_files;

    for (i = 0; i < NUM_OP_TYPES; i++)	/* for all op types */
    {
	file_ptr = (struct file_object *) op_X_array[i];
	for (j = 0; j < size; j++)	/* All files in this op type */
	{
	    if (file_ptr && (file_ptr->access_count > 0) &&
		(file_ptr->relfilename[0] != 0))
	    {
		fprintf (out, "%8ld %s %s\n", file_ptr->access_count,
			 VFS (pathptr, file_ptr->dir), file_ptr->relfilename);
		fflush (out);
	    }
	    file_ptr++;
	}
    }
}




/**
 * @brief Creates a histogram:
 *         Buckets: (Based on a Netapp internal consensus)
 *             0       1       2       3      4
 *          <=20us  <=40us  <=60us  <=80us  <=100us
 * 
 *            5        6       7       8       9
 *         <=200us  <=400us <=600us <=88us  <=1ms
 * 
 *           10       11      12      13      14
 *         <=2ms    <=4ms   <=6ms   <=8ms   <=10ms
 * 
 *           15       16      17      18      19
 *         <=12ms   <=14ms  <=16ms  <=18ms  <=20ms
 * 
 *           20       21      22      23      24
 *         <=20ms   <=40ms  <=60ms  <=80ms  <=100ms
 * 
 *           25       26      27      28      29
 *         <=200ms  <=400ms <=600ms <=800ms <=1s
 * 
 *           30       31      32      33      34
 *         <=2s     <=4s    <=6s    <=8s    <=10s
 * 
 *           35       36      37      38      39
 *         <=20s    <=30s   <=60    <=90s    >90
 *
 * @param my_value : Insert this result in the histogram
 */
/*
 * __doc__
 * __doc__  Function : void
 * __doc__             hist_insert(double my_value)
 * __doc__  Arguments: double value: Insert this result in the histogram
 * __doc__  Returns  : void
 * __doc__  Performs : Creates a histogram:
 * __doc__            Buckets: (Based on a Netapp internal consensus)
 * __doc__                0       1       2       3      4
 * __doc__             <=20us  <=40us  <=60us  <=80us  <=100us
 * __doc__
 * __doc__               5        6       7       8       9
 * __doc__            <=200us  <=400us <=600us <=88us  <=1ms
 * __doc__
 * __doc__              10       11      12      13      14
 * __doc__            <=2ms    <=4ms   <=6ms   <=8ms   <=10ms
 * __doc__
 * __doc__              15       16      17      18      19
 * __doc__            <=12ms   <=14ms  <=16ms  <=18ms  <=20ms
 * __doc__
 * __doc__              20       21      22      23      24
 * __doc__            <=20ms   <=40ms  <=60ms  <=80ms  <=100ms
 * __doc__
 * __doc__              25       26      27      28      29
 * __doc__            <=200ms  <=400ms <=600ms <=800ms <=1s
 * __doc__
 * __doc__              30       31      32      33      34
 * __doc__            <=2s     <=4s    <=6s    <=8s    <=10s
 * __doc__
 * __doc__              35       36      37      38      39
 * __doc__            <=20s    <=30s   <=60    <=90s    >90
 * __doc__
 */
void
hist_insert (double my_value)
{
    int k;
    long long value;

    /* Convert to micro-seconds */
    value = (long long) (my_value * 1000000);
    for (k = 0; k < BUCKETS; k++)
    {
	if (k < (BUCKETS - 1))
	{
	    if (value <= bucket_val[k])
	    {
		buckets[k]++;
		break;
	    }
	}
	else
	{
	    if (value > bucket_val[k])
	    {
		buckets[k]++;
		break;
	    }
	}
    }
}

/**
 * @brief Dump the histograms.
 */
/*
 * __doc__
 * __doc__  Function : void dumb_hist(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Dump the histograms.
 * __doc__
 */
void
dump_hist (void)
{
#ifndef NO_PRINT_LLD
    log_all (LOG_RESULTS, "\tBand 1: ");
    log_all (LOG_RESULTS, " 20us:%-7.1lld ", global_buckets[0]);
    log_all (LOG_RESULTS, " 40us:%-7.1lld ", global_buckets[1]);
    log_all (LOG_RESULTS, " 60us:%-7.1lld ", global_buckets[2]);
    log_all (LOG_RESULTS, " 80us:%-7.1lld ", global_buckets[3]);
    log_all (LOG_RESULTS, "100us:%-7.1lld \n", global_buckets[4]);

    log_all (LOG_RESULTS, "\tBand 2: ");
    log_all (LOG_RESULTS, "200us:%-7.1lld ", global_buckets[5]);
    log_all (LOG_RESULTS, "400us:%-7.1lld ", global_buckets[6]);
    log_all (LOG_RESULTS, "600us:%-7.1lld ", global_buckets[7]);
    log_all (LOG_RESULTS, "800us:%-7.1lld ", global_buckets[8]);
    log_all (LOG_RESULTS, "  1ms:%-7.1lld \n", global_buckets[9]);

    log_all (LOG_RESULTS, "\tBand 3: ");
    log_all (LOG_RESULTS, "  2ms:%-7.1lld ", global_buckets[10]);
    log_all (LOG_RESULTS, "  4ms:%-7.1lld ", global_buckets[11]);
    log_all (LOG_RESULTS, "  6ms:%-7.1lld ", global_buckets[12]);
    log_all (LOG_RESULTS, "  8ms:%-7.1lld ", global_buckets[13]);
    log_all (LOG_RESULTS, " 10ms:%-7.1lld \n", global_buckets[14]);

    log_all (LOG_RESULTS, "\tBand 4: ");
    log_all (LOG_RESULTS, " 12ms:%-7.1lld ", global_buckets[15]);
    log_all (LOG_RESULTS, " 14ms:%-7.1lld ", global_buckets[16]);
    log_all (LOG_RESULTS, " 16ms:%-7.1lld ", global_buckets[17]);
    log_all (LOG_RESULTS, " 18ms:%-7.1lld ", global_buckets[18]);
    log_all (LOG_RESULTS, " 20ms:%-7.1lld \n", global_buckets[19]);

    log_all (LOG_RESULTS, "\tBand 5: ");
    log_all (LOG_RESULTS, " 40ms:%-7.1lld ", global_buckets[20]);
    log_all (LOG_RESULTS, " 60ms:%-7.1lld ", global_buckets[21]);
    log_all (LOG_RESULTS, " 80ms:%-7.1lld ", global_buckets[22]);
    log_all (LOG_RESULTS, "100ms:%-7.1lld \n", buckets[23]);

    log_all (LOG_RESULTS, "\tBand 6: ");
    log_all (LOG_RESULTS, "200ms:%-7.1lld ", global_buckets[24]);
    log_all (LOG_RESULTS, "400ms:%-7.1lld ", global_buckets[25]);
    log_all (LOG_RESULTS, "600ms:%-7.1lld ", global_buckets[26]);
    log_all (LOG_RESULTS, "800ms:%-7.1lld ", global_buckets[27]);
    log_all (LOG_RESULTS, "   1s:%-7.1lld \n", global_buckets[28]);

    log_all (LOG_RESULTS, "\tBand 7: ");
    log_all (LOG_RESULTS, "   2s:%-7.1lld ", global_buckets[29]);
    log_all (LOG_RESULTS, "   4s:%-7.1lld ", global_buckets[30]);
    log_all (LOG_RESULTS, "   6s:%-7.1lld ", global_buckets[31]);
    log_all (LOG_RESULTS, "   8s:%-7.1lld ", global_buckets[32]);
    log_all (LOG_RESULTS, "  10s:%-7.1lld \n", global_buckets[33]);

    log_all (LOG_RESULTS, "\tBand 8: ");
    log_all (LOG_RESULTS, "  20s:%-7.1lld ", global_buckets[34]);
    log_all (LOG_RESULTS, "  40s:%-7.1lld ", global_buckets[35]);
    log_all (LOG_RESULTS, "  60s:%-7.1lld ", global_buckets[36]);
    log_all (LOG_RESULTS, "  80s:%-7.1lld ", global_buckets[37]);
    log_all (LOG_RESULTS, " 120s:%-7.1lld \n", global_buckets[38]);

    log_all (LOG_RESULTS, "\tBand 9: ");
    log_all (LOG_RESULTS, "120+s:%-7.1lld \n\n", global_buckets[39]);
#else
    log_all (LOG_RESULTS, "\tBand 1: ");
    log_all (LOG_RESULTS, " 20us:%-7.1ld ", global_buckets[0]);
    log_all (LOG_RESULTS, " 40us:%-7.1ld ", global_buckets[1]);
    log_all (LOG_RESULTS, " 60us:%-7.1ld ", global_buckets[2]);
    log_all (LOG_RESULTS, " 80us:%-7.1ld ", global_buckets[3]);
    log_all (LOG_RESULTS, "100us:%-7.1ld \n", global_buckets[4]);

    log_all (LOG_RESULTS, "\tBand 2: ");
    log_all (LOG_RESULTS, "200us:%-7.1ld ", global_buckets[5]);
    log_all (LOG_RESULTS, "400us:%-7.1ld ", global_buckets[6]);
    log_all (LOG_RESULTS, "600us:%-7.1ld ", global_buckets[7]);
    log_all (LOG_RESULTS, "800us:%-7.1ld ", global_buckets[8]);
    log_all (LOG_RESULTS, "  1ms:%-7.1ld \n", global_buckets[9]);

    log_all (LOG_RESULTS, "\tBand 3: ");
    log_all (LOG_RESULTS, "  2ms:%-7.1ld ", global_buckets[10]);
    log_all (LOG_RESULTS, "  4ms:%-7.1ld ", global_buckets[11]);
    log_all (LOG_RESULTS, "  6ms:%-7.1ld ", global_buckets[12]);
    log_all (LOG_RESULTS, "  8ms:%-7.1ld ", global_buckets[13]);
    log_all (LOG_RESULTS, " 10ms:%-7.1ld \n", global_buckets[14]);

    log_all (LOG_RESULTS, "\tBand 4: ");
    log_all (LOG_RESULTS, " 12ms:%-7.1ld ", global_buckets[15]);
    log_all (LOG_RESULTS, " 14ms:%-7.1ld ", global_buckets[16]);
    log_all (LOG_RESULTS, " 16ms:%-7.1ld ", global_buckets[17]);
    log_all (LOG_RESULTS, " 18ms:%-7.1ld ", global_buckets[18]);
    log_all (LOG_RESULTS, " 20ms:%-7.1ld \n", global_buckets[19]);

    log_all (LOG_RESULTS, "\tBand 5: ");
    log_all (LOG_RESULTS, " 40ms:%-7.1ld ", global_buckets[20]);
    log_all (LOG_RESULTS, " 60ms:%-7.1ld ", global_buckets[21]);
    log_all (LOG_RESULTS, " 80ms:%-7.1ld ", global_buckets[22]);
    log_all (LOG_RESULTS, "100ms:%-7.1ld \n", global_buckets[23]);

    log_all (LOG_RESULTS, "\tBand 6: ");
    log_all (LOG_RESULTS, "200ms:%-7.1ld ", global_buckets[24]);
    log_all (LOG_RESULTS, "400ms:%-7.1ld ", global_buckets[25]);
    log_all (LOG_RESULTS, "600ms:%-7.1ld ", global_buckets[26]);
    log_all (LOG_RESULTS, "800ms:%-7.1ld ", global_buckets[27]);
    log_all (LOG_RESULTS, "   1s:%-7.1ld \n", global_buckets[28]);

    log_all (LOG_RESULTS, "\tBand 7: ");
    log_all (LOG_RESULTS, "   2s:%-7.1ld ", global_buckets[29]);
    log_all (LOG_RESULTS, "   4s:%-7.1ld ", global_buckets[30]);
    log_all (LOG_RESULTS, "   6s:%-7.1ld ", global_buckets[31]);
    log_all (LOG_RESULTS, "   8s:%-7.1ld ", global_buckets[32]);
    log_all (LOG_RESULTS, "  10s:%-7.1ld \n", global_buckets[33]);

    log_all (LOG_RESULTS, "\tBand 8: ");
    log_all (LOG_RESULTS, "  20s:%-7.1ld ", global_buckets[34]);
    log_all (LOG_RESULTS, "  40s:%-7.1ld ", global_buckets[35]);
    log_all (LOG_RESULTS, "  60s:%-7.1ld ", global_buckets[36]);
    log_all (LOG_RESULTS, "  80s:%-7.1ld ", global_buckets[37]);
    log_all (LOG_RESULTS, " 120s:%-7.1ld \n", global_buckets[38]);

    log_all (LOG_RESULTS, "\tBand 9: ");
    log_all (LOG_RESULTS, "120+s:%-7.1ld \n", global_buckets[39]);
#endif
}

/**
 * @brief Function that generates data layout that implements 
 *        the patterns needed for deduplication and compression.
 *
 * @param  buf : Buffer pointer
 * @param  size : Size of the buffer
 * @param  file_ptr : Object for this file.
 * @param  Offset : Offset within the file.
 * @param  dedup_group : Group for dedupe data.
 * @param  across_only : Used to place dedupe in across only reg
 */
/*
 * __doc__
 * __doc__  Function : void make_data_layout( long *buf, int size, 
 * __doc__                  struct file_object *fp, long long Offset,
 * __doc__                  int dedup_group, int across_only)
 * __doc__  Arguments: long *buf: Buffer pointer
 * __doc__             int size: Size of the buffer
 * __doc__             struct file_object: Object for this file.
 * __doc__             long long Offset: Offset within the file.
 * __doc__             int dedupe_group: Group for dedupe data.
 * __doc__             int across_only: Used to place dedupe in across only reg
 * __doc__  Returns  : void
 * __doc__  Performs : Function that generates data layout that implements 
 * __doc__             the patterns needed for deduplication and compression.
 * __doc__
 */
void
make_data_layout (long *buf, int size, struct file_object *file_ptr,
		  long long Offset, int dedup_group, int across_only)
{
    int percent_compress, percent_dedup;
    long long dedup_gran_size;
    long long comp_gran_size;

    percent_compress = my_mix_table[my_workload].percent_compress;
    percent_dedup = my_mix_table[my_workload].percent_dedup;
    dedup_gran_size =
	(long long) my_mix_table[my_workload].dedup_granule_size;
    comp_gran_size = (long long) my_mix_table[my_workload].comp_granule_size;


    make_buf (percent_dedup, percent_compress,
	      (100 - (percent_compress + percent_dedup)), (char *) buf,
	      (long long) (file_ptr->Original_fsize * 1024LL),
	      (long long) size, (long long) dedup_gran_size,
	      (long long) comp_gran_size, Offset, dedup_group, across_only);

}

/**
 * @brief There is a check_sum of the workloads. It is printed 
 *        out, at the end of a run. If one wishes to have this
 *        result submitted for publication, then one acquires a
 *        magic key from the publisher. This key is then XOR-d with
 *        the check_sum and submitted to the publisher for 
 *        validation.
 *        The magic key, from the publisher, is delivered via a 
 *        secure exchange (cypher).
 *
 * @param  my_mix_table : Pointer to the mix_table structure.
 * @param  license_key  : Int, comapny license key
 */
/*
 * __doc__
 * __doc__  Function : unsigned short
 * __doc__             check_sum( struct mix_table *my_mix_table, unsigned int license_key)
 * __doc__  Arguments: struct mix_table *: Pointer to the mix_table structure.
 * __doc__             int: License (on or off)
 * __doc__  Returns  : unsigned short: Check sum.
 * __doc__  Performs : There is a check_sum of the workloads. It is printed 
 * __doc__             out, at the end of a run. If one wishes to have this
 * __doc__             result submitted for publication, then one acquires a
 * __doc__             magic key from the publisher. This key is then XOR-d with
 * __doc__             the check_sum and submitted to the publisher for 
 * __doc__             validation.
 * __doc__             The magic key, from the publisher, is delivered via a 
 * __doc__             secure exchange (cypher).
 * __doc__
 */
unsigned int
check_sum (struct mix_table *my_mix_table, unsigned int license_key)
{
    int i;
    u_int32_t k = 0;
    unsigned char *ptr, *ptr2;
    unsigned *md_dest;

    long long count = 0;

    log_all (LOG_RESULTS, "\t%-30s%10s\n", "Workload", "MD5");
    for (i = 0; i < MAX_WORK_OBJ; i++)
    {
	if (strlen (my_mix_table[i].workload_name) == 0)
	    continue;
	ptr = (unsigned char *) &my_mix_table[i];
	ptr2 = (unsigned char *) &my_mix_table[i].warm_time;
	count = llabs ((long long)( ptr2 - ptr));
	/* The count below is for the checksum area of the mix_table. e.g. excluding warm_time and fs_type */
	k += crc32_generate ((void *) ptr, (size_t) count);
	md_dest = md5 ((char *) ptr, (int) count);
	print_md5 (my_mix_table[i].workload_name, md_dest);
/*
        for (j = 0; j < (int)count; j++)
        {
            k += (((unsigned short) *ptr) * j);
            ptr++;
        }
*/
    }

    k = k ^ license_key;

    return (k);			/* Licensed finger print */
}



#if defined(PDSM_CO_ACTIVE)
/**
 * @brief Function that imports the PDSM changes that are 
 *        being requested dynamically.
 */
/*
 * __doc__
 * __doc__  Function : void set_pdsm_attributes(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Function that imports the PDSM changes that are 
 * __doc__             being requested dynamically.
 * __doc__
 */
void
set_pdsm_attributes (void)
{
#if defined(WIN32)
    DWORD ret;
#else
    int ret;
#endif
    /* Copy inbound op_rate and mix table (workload) to current definitions. */
    ret = pdsm_get_control_data (pdsm_control);	/* get the control data */
    if (ret != 0)
    {
	log_file (LOG_ERROR, "pdsm_get_control_data returned an error\n");
	return;
    }
    modified_run = 1;
    /* If the user said to set the op_rate */
    if (pdsm_control->set_op_rate)
    {
	cmdline.op_rate = pdsm_control->op_rate;
	pdsm_control->set_op_rate = 0;
	pdsm_put_control_data (pdsm_control);
	/* Reset to look like it has been running at the new rate for the current run time */
	total_file_ops =
	    (pdsm_control->op_rate *
	     (double) (last_heart_beat - my_start_time));
	log_file (LOG_EXEC, "pdsm_set_attributes, changing op rate to %f.\n",
		  cmdline.op_rate);
    }
    /* If the user said to set the workload */
    if (pdsm_control->set_workload)
    {
	memcpy (&my_mix_table[my_workload], &pdsm_control->work_load_out,
		sizeof (struct mix_table));
	create_tests (test_ops);
	init_dist_tables ();
	pdsm_control->set_workload = 0;
	log_file (LOG_EXEC, "pdsm_set_attributes, changing workload.\n");
	pdsm_put_control_data (pdsm_control);
	/* Nuke results so that the rate adjustment will be made asap */
	/* Reset to look like it has been running at the new rate for the current run time */
	total_file_ops =
	    (pdsm_control->op_rate *
	     (double) (last_heart_beat - my_start_time));
    }
}

int
pdsm_get_control_data (struct pdsm_remote_control *pdsm_control)
{
#if defined(WIN32)
    DWORD ret, flags = 0;
#else
    int ret, flags = 0;
#endif
#ifdef _LARGEFILE64_SOURCE
    /* 
     * Heavy magic happens with the size of the stat and statfs structures
     * about the time that 10.6 came out.
     */
#if defined(_macos_)
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  > 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  < 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  == 1050
    struct stat64 stbuf;
#endif
#else
    struct stat64 stbuf;
#endif
#else
    struct stat stbuf;
#endif

    /* if it's not open, or doesn't exist, open this file */
#if defined(WIN32)
    flags |= FILE_FLAG_POSIX_SEMANTICS;
#else
    flags |= O_CREAT | O_RDWR;
#endif
    if (pdsm_control_file_fd <= 0)
    {
#if defined(WIN32)
	pdsm_control_file_fd = CreateFile (my_pdsm_control_file,
					   GENERIC_READ | GENERIC_WRITE,
					   FILE_SHARE_READ | FILE_SHARE_WRITE
					   | FILE_SHARE_DELETE, NULL,
					   OPEN_ALWAYS, flags, NULL);
#else
	pdsm_control_file_fd = I_OPEN (my_pdsm_control_file, flags, 0666);
#endif
    }

    /* If not initialized, write out this proc's mix_table entry */
    if (pdsm_control_once == 0)
    {
	pdsm_control_once++;
	pdsm_control->op_rate = cmdline.op_rate;
	pdsm_control->set_op_rate = 0;
	pdsm_control->set_workload = 0;
	my_strncpy (pdsm_control->hostname, client_localname, MAXHNAME);
	pdsm_control->client_id = cmdline.client_id;
	memcpy (&pdsm_control->work_load_out, &my_mix_table[my_workload],
		sizeof (struct mix_table));
#if defined(WIN32)
	largeoffset.QuadPart =
	    sizeof (struct pdsm_remote_control) * cmdline.client_id;
	SetFilePointerEx (pdsm_control_file_fd, largeoffset, NULL,
			  FILE_BEGIN);
#else
	I_LSEEK (pdsm_control_file_fd,
		 sizeof (struct pdsm_remote_control) * cmdline.client_id,
		 SEEK_SET);
#endif
	log_file (LOG_DEBUG, "Putting initial pdsm control data\n");
#if defined(WIN32)
	WriteFile (pdsm_control_file_fd, pdsm_control,
		   sizeof (struct pdsm_remote_control), &ret, NULL);
#else
	ret =
	    write (pdsm_control_file_fd, pdsm_control,
		   sizeof (struct pdsm_remote_control));
#endif
    }
    ret = I_STAT (my_pdsm_control_file, &stbuf);
    if (ret < 0)
    {
	log_file (LOG_ERROR, "pdsm get control file stat() failed\n");
	return (0);
    }
    if (pdsm_cur_stat_mod_time < stbuf.st_mtime)
    {
#if defined(WIN32)
	largeoffset.QuadPart =
	    sizeof (struct pdsm_remote_control) * cmdline.client_id;
	SetFilePointerEx (pdsm_control_file_fd, largeoffset, NULL,
			  FILE_BEGIN);
#else
	I_LSEEK (pdsm_control_file_fd,
		 sizeof (struct pdsm_remote_control) * cmdline.client_id,
		 SEEK_SET);
#endif
	log_file (LOG_DEBUG, "Getting pdsm control data %d %d\n",
		  pdsm_cur_stat_mod_time, stbuf.st_mtime);
#if defined(WIN32)
	ReadFile (pdsm_control_file_fd, pdsm_control,
		  sizeof (struct pdsm_remote_control), &ret, NULL);
#else
	ret =
	    read (pdsm_control_file_fd, pdsm_control,
		  sizeof (struct pdsm_remote_control));
#endif
	if (ret <= 0)
	{
	    log_file (LOG_ERROR,
		      "pdsm_get_control_data: Unable to read data from %s\n",
		      my_pdsm_control_file);
	    return (0);
	}
	pdsm_cur_stat_mod_time = stbuf.st_mtime;

    }
    return (0);
}

int
pdsm_put_control_data (struct pdsm_remote_control *pdsm_control)
{
#if defined(WIN32)
    DWORD ret, flags = 0;
#else
    int ret, flags = 0;
#endif
    /* if it's not open, or doesn't exist, open this file */
    if (pdsm_control_file_fd <= 0)
    {
#if defined(WIN32)
	flags |= FILE_FLAG_POSIX_SEMANTICS;
#else
	flags |= O_CREAT | O_RDWR;
#endif
#if defined(WIN32)
	pdsm_control_file_fd = CreateFile (my_pdsm_control_file,
					   GENERIC_READ | GENERIC_WRITE,
					   FILE_SHARE_READ | FILE_SHARE_WRITE
					   | FILE_SHARE_DELETE, NULL,
					   OPEN_ALWAYS, flags, NULL);
#else
	pdsm_control_file_fd = I_OPEN (my_pdsm_control_file, flags, 0666);
#endif

    }
    if (pdsm_control_file_fd < 0)
    {
	log_file (LOG_ERROR,
		  "pdsm_put_control_data: Unable to open control file %s\n",
		  my_pdsm_control_file);
	return (0);
    }
    /* If not initialized, write out this proc's mix_table entry */
    if (pdsm_control_once == 0)
    {
	pdsm_control->set_op_rate = cmdline.op_rate;
	memcpy (&pdsm_control->work_load_out, &my_mix_table[my_workload],
		sizeof (struct mix_table));
	my_strncpy (pdsm_control->hostname, client_localname, MAXHNAME);
	pdsm_control->client_id = cmdline.client_id;
#if defined(WIN32)
	largeoffset.QuadPart =
	    sizeof (struct pdsm_remote_control) * cmdline.client_id;
	SetFilePointerEx (pdsm_control_file_fd, largeoffset, NULL,
			  FILE_BEGIN);
#else
	I_LSEEK (pdsm_control_file_fd,
		 sizeof (struct pdsm_remote_control) * cmdline.client_id,
		 SEEK_SET);
#endif
	log_file (LOG_DEBUG, "Putting pdsm control data\n");
#if defined(WIN32)
	WriteFile (pdsm_control_file_fd, pdsm_control,
		   sizeof (struct pdsm_remote_control), &ret, NULL);
#else
	ret =
	    write (pdsm_control_file_fd, pdsm_control,
		   sizeof (struct pdsm_remote_control));
#endif
	if (ret <= 0)
	{
	    log_file (LOG_ERROR,
		      "pdsm_put_control_data: Unable to write control file %s\n",
		      my_pdsm_control_file);
	    return (0);
	}
	pdsm_control_once++;
    }
    my_strncpy (pdsm_control->hostname, client_localname, MAXHNAME);
    pdsm_control->client_id = cmdline.client_id;
#if defined(WIN32)
    largeoffset.QuadPart =
	sizeof (struct pdsm_remote_control) * cmdline.client_id;
    SetFilePointerEx (pdsm_control_file_fd, largeoffset, NULL, FILE_BEGIN);
#else
    I_LSEEK (pdsm_control_file_fd,
	     sizeof (struct pdsm_remote_control) * cmdline.client_id,
	     SEEK_SET);
#endif
    log_file (LOG_DEBUG, "Putting pdsm control data\n");
    pdsm_cur_stat_mod_time = time (NULL);
#if defined(WIN32)
    WriteFile (pdsm_control_file_fd, pdsm_control,
	       sizeof (struct pdsm_remote_control), &ret, NULL);
#else
    ret =
	write (pdsm_control_file_fd, pdsm_control,
	       sizeof (struct pdsm_remote_control));
#endif
    if (ret <= 0)
    {
	log_file (LOG_ERROR,
		  "pdsm_put_control_data: Unable to write control file %s\n",
		  my_pdsm_control_file);
	return (0);
    }
    fsync (pdsm_control_file_fd);	/* This is rare, so lets get it on disk, and pushed to any remote servers */
    return (0);
}
#endif

#if !defined(WIN32)
#include <sys/mman.h>
#endif

/**
 * @brief Function that implements the mmap_write op.
 *
 * @param fp : Holder of this file.
 */
/*
 * __doc__
 * __doc__  Function : void generic_mmap_write(struct file_object *fp)
 * __doc__  Arguments: struct file_object *: Holder of this file.
 * __doc__  Returns  : void
 * __doc__  Performs : Function that implements the mmap_write op.
 * __doc__
 */
void
generic_mmap_write (struct file_object *fp)
{
    struct netmist_vfs_object *fd = NULL;
    int err;
    char *buf = NULL, *pa = NULL;
    struct netmist_vfs_mmo *mmo = NULL;
    const char *err_str = NULL;
    double lat = 0.0;
    double entry_time = 0.0;
    int trans;
    size_t filebytes;
    enum netmist_vfs_open_flags flags = 0;
    int this_dedup_group;
    int dedup_across;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing mmap write op\n");

    entry_time = gettime ();
    buf = main_buf;
    fd_accesses++;
    file_accesses++;

    probability = (long) (netmist_rand () % 100);

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	fd = fp->file_desc;
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fd);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for mmap write op failed. Filename = %s in %s Error: %s Error value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
    }
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    VFS (seek, fd, fp->mmapwrite_offset, NETMIST_SEEK_SET, NULL);
    /* Select write size to use */
    trans = get_next_write_size ();

    if (fp->dedup_group == 0)
	fp->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = fp->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(fp->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;

    if (my_mix_table[my_workload].patt_vers == 1)
    {
	make_non_dedup ((long *) buf, (int) trans);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }
    else
    {
	make_data_layout ((long *) buf, trans, fp, fp->mmapwrite_offset,
			  this_dedup_group, dedup_across);
	if (my_mix_table[my_workload].cipher)
	    make_cypher ((char *) buf, (int) trans);
    }

    filebytes = fp->Original_fsize * (long long) 1024;

    err = VFS (mmap, fd, 0, filebytes, 1, &mmo, &pa);

    if (err != 0)
    {
	err_str = netmist_vfs_errstr (err);

	log_file (LOG_ERROR, "Mmap Write failed on file %s in %s Error: %s\n",
		  fp->relfilename, VFS (pathptr, fp->dir), err_str);

	fd_remove_from_cache (fp);	/* Does not include close */
	R_exit (NETMIST_OP_VFS_ERR + 10, err);
    }

    if (trans < 0)
	trans = 0;
    if (((long long) fp->mmapwrite_offset > (long long) filebytes)
	|| ((long long) (fp->mmapwrite_offset) < (long long) 0))
	fp->mmapwrite_offset = 0LL;
    if ((long long) (fp->mmapwrite_offset + trans) > (long long) filebytes)
	fp->mmapwrite_offset = 0LL;

    if ((long long) (fp->mmapwrite_offset + trans) > (long long) filebytes)
	/* Limit the io size, ensure the operation won't go beyond the valid 
	 * memory size */
	trans = filebytes - fp->mmapwrite_offset;

    log_file (LOG_DEBUG, "Trans %d Size %lld Offset %lld\n", trans,
	      (long long) filebytes, (long long) (fp->mmapwrite_offset));

#if !(defined(WIN32) || defined(_macos_))
    if (my_mix_table[my_workload].percent_madvise_seq
	&& (probability < my_mix_table[my_workload].percent_madvise_seq))
	posix_madvise (&pa[(long long) (fp->mmapwrite_offset)], trans,
		       POSIX_MADV_SEQUENTIAL);
    if (my_mix_table[my_workload].percent_madvise_rand
	&& (probability < my_mix_table[my_workload].percent_madvise_rand))
	posix_madvise (&pa[(long long) (fp->mmapwrite_offset)], trans,
		       POSIX_MADV_RANDOM);
    if (my_mix_table[my_workload].percent_madvise_dont_need
	&& (probability <
	    my_mix_table[my_workload].percent_madvise_dont_need))
	posix_madvise (&pa[(long long) (fp->mmapwrite_offset)], trans,
		       POSIX_MADV_DONTNEED);
#endif

    memcpy (&pa[(long long) (fp->mmapwrite_offset)], buf, (size_t) trans);
    fp->mmapwrite_offset += (long long) trans;
    if ((fp->mmapwrite_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->mmapwrite_offset = 0LL;
    fp->op_count++;

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].mmap_write_op_time += lat;
	if ((lat < work_obj[my_workload].min_mmap_write_op_latency)
	    || (work_obj[my_workload].min_mmap_write_op_latency == 0.0))
	    work_obj[my_workload].min_mmap_write_op_latency = lat;
	if ((lat > work_obj[my_workload].max_mmap_write_op_latency)
	    || (work_obj[my_workload].max_mmap_write_op_latency == 0.0))
	    work_obj[my_workload].max_mmap_write_op_latency = lat;
    }

    /* global (across all files in this work object) write count */
    work_obj[my_workload].mmap_write_op_count++;

    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;

    total_write_bytes += (double) (trans);

    work_obj[my_workload].mmap_write_op_count++;
    VFS (munmap, &mmo);
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    if (in_validate)
    {
	if (fp->fd_link)
	{
	    fd_drop_from_cache (fp);	/* includes close */
	}
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
}

/**
 * @brief Function that implements the mmap_read op.
 *
 * @param fp : Holder of this file.
 */
/*
 * __doc__
 * __doc__  Function : void generic_mmap_read(struct file_object *fp)
 * __doc__  Arguments: struct file_object *: Holder of this file.
 * __doc__  Returns  : void
 * __doc__  Performs : Function that implements the mmap_read op.
 * __doc__
 */
void
generic_mmap_read (struct file_object *fp)
{
    struct netmist_vfs_object *fd = NULL;
    int err;
    char *buf;
    const char *err_str = NULL;
    char *pa = NULL;
    struct netmist_vfs_mmo *mmo = NULL;
    double lat = 0.0;
    double entry_time = 0.0;
    int trans;
    uint64_t filebytes;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing mmap read op\n");

    entry_time = gettime ();
    buf = main_buf;
    fd_accesses++;
    file_accesses++;

    probability = (long) (netmist_rand () % 100);

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	fd = fp->file_desc;
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_CREATE | flags,
		 &fd);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for mmap read op failed. Filename = %s in %s Error: %s Error value: %d\n",
		      fp->relfilename, VFS (pathptr, fp->dir), nes, err);
	    R_exit (NETMIST_OP_VFS_ERR + 9, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
    }
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    VFS (seek, fd, fp->mmapread_offset, NETMIST_SEEK_SET, NULL);

    /* Select read size to use */
    trans = get_next_read_size ();

    filebytes = fp->Original_fsize * (long long) 1024;

    err = VFS (mmap, fd, 0, filebytes, 0, &mmo, &pa);

    if (err != 0)
    {
	err_str = netmist_vfs_errstr (err);

	log_file (LOG_ERROR,
		  "Mmap_Read MapViewOfFile failed on file %s in %s Error: %s Error value: %d\n",
		  fp->relfilename, VFS (pathptr, fp->dir), err_str, err);

	fd_remove_from_cache (fp);	/* Does not include close */
	R_exit (NETMIST_OP_VFS_ERR + 10, err);
    }
    if (trans < 0)
	trans = 0;
    if ((fp->mmapread_offset > filebytes)
	|| ((long long) (fp->mmapread_offset) < (long long) 0))
	fp->mmapread_offset = 0LL;
    if (fp->mmapread_offset + trans > filebytes)
	fp->mmapread_offset = 0LL;

    if (fp->mmapread_offset + trans > filebytes)
	/* Limit the io size, ensure the operation won't go beyond the 
	 * valid memory size */
	trans = (int) (filebytes - fp->mmapread_offset);

    log_file (LOG_DEBUG, "Trans %d Size %lld Offset %lld\n", trans,
	      (long long) filebytes, (long long) (fp->mmapread_offset));

#if !(defined(WIN32) || defined(_macos_))
    if (my_mix_table[my_workload].percent_madvise_seq
	&& (probability < my_mix_table[my_workload].percent_madvise_seq))
	posix_madvise (&pa[(long long) (fp->mmapread_offset)], trans,
		       POSIX_MADV_SEQUENTIAL);
    if (my_mix_table[my_workload].percent_madvise_rand
	&& (probability < my_mix_table[my_workload].percent_madvise_rand))
	posix_madvise (&pa[(long long) (fp->mmapread_offset)], trans,
		       POSIX_MADV_RANDOM);
    if (my_mix_table[my_workload].percent_madvise_dont_need
	&& (probability <
	    my_mix_table[my_workload].percent_madvise_dont_need))
	posix_madvise (&pa[(long long) (fp->mmapread_offset)], trans,
		       POSIX_MADV_DONTNEED);
#endif

    memcpy (buf, &pa[(long long) (fp->mmapread_offset)], (size_t) trans);

    log_file (LOG_DEBUG, "memcpy completed\n");

    fp->mmapread_offset += (long long) trans;
    if ((fp->mmapread_offset / (unsigned long long) 1024) >=
	(fp->Original_fsize))
	fp->mmapread_offset = 0LL;
    fp->op_count++;

    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].mmap_read_op_time += lat;
	if ((lat < work_obj[my_workload].min_mmap_read_op_latency)
	    || (work_obj[my_workload].min_mmap_read_op_latency == 0.0))
	    work_obj[my_workload].min_mmap_read_op_latency = lat;
	if ((lat > work_obj[my_workload].max_mmap_read_op_latency)
	    || (work_obj[my_workload].max_mmap_read_op_latency == 0.0))
	    work_obj[my_workload].max_mmap_read_op_latency = lat;
    }

    /* global (across all files in this work object) wreadcount */
    work_obj[my_workload].mmap_read_op_count++;

    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;

    total_read_bytes += (double) (trans);

    work_obj[my_workload].mmap_read_op_count++;
    VFS (munmap, &mmo);
    total_file_ops += 1.0;
    work_obj[my_workload].total_file_ops += 1.0;
    if (in_validate)
    {
	if (fp->fd_link)
	{
	    fd_drop_from_cache (fp);	/* includes close */
	}
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }
}



/**
 * @brief Giving a maximum of "val" return a random number that
 *        is less than or equal to val that follows a geometric
 *        distribution with a lambda of 4.0.
 *
 * @param  val : Sets scale for function.
 */
/*
 * __doc__
 * __doc__  Function : int geometric_rand(int val)
 * __doc__  Arguments: int val: Sets scale for function.
 * __doc__  Returns  : int: Random number following geometric distribution.
 * __doc__  Performs : Giving a maximum of "val" return a random number that
 * __doc__             is less than or equal to val that follows a geometric
 * __doc__             distribution with a lambda of 4.0.
 * __doc__             
 */

int
geometric_rand (int val)
{
    double lambda = 4.0;
    double L;
    unsigned int k = 0;
    float ra;
    float p = 1.0;
    if (val == 0)		/* Ensure no divide by zero */
	val = 1;
    L = exp (-lambda);
    do
    {
	k++;
	ra = (float) netmist_rand () / (float) INT_MAX;
	p *= ra;
    }
    while (p > L);
    return ((int) ((k - 1) % val));
}

int spot_stuff[100];
int pick_init;

/**
 * @brief Given a spot/chunk number, create a 5% jitter.
 *        So... 95 % of the time, the same value is returned, but
 *        5% of the time, it will return a different chunk number;
 *
 * @param input : Input value
 * @param max : Maximum value to ever return.
 */
/*
 * __doc__
 * __doc__  Function : int picker(int input, int max)
 * __doc__  Arguments: int input, int max value.
 * __doc__  Returns  : int chunk number.
 * __doc__  Performs : Given a spot/chunk number, create a 5% jitter.
 * __doc__             So... 95 % of the time, the same value is returned, but
 * __doc__             5% of the time, it will return a different chunk number;
 * __doc__
 */
int
picker (int input, int max)
{
    int i, ret;

    percent_affinity = my_mix_table[my_workload].percent_affinity;
    if (!pick_init)		/* Init the Xpercent with other possibilities */
    {
	for (i = 0; i < percent_affinity; i++)
	    spot_stuff[i] = (netmist_rand () % max);
    }
    for (i = percent_affinity; i < 100; i++)
	spot_stuff[i] = input;
    ret = spot_stuff[(netmist_rand () % 100)];
    return (ret);
}

/**
 * @brief validates clients can perform all op types.
 */
/*
 * __doc__
 * __doc__  Function : void client_validate_ops(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : validates clients can perform all op types.
 * __doc__
 */
void
client_validate_ops (void)
{
    int ret;
    int x = 0;
    char buf[1024];
    struct test *tp;
    char alt_name[1024];
    int saved;
    int saved_direct_percent;
    int old_max_open;
    struct file_object fo, *fp;
    struct netmist_vfs_dir *vdir;

    fp = &fo;
    memset ((void *) fp, 0, sizeof (*fp));

    /* Turn off file descriptor caching, force all open() calls to
     * be closed before returning from the virtualized call.
     */
    in_validate = 1;

    /* Limit file descriptor caching to one entry */
    old_max_open = max_open;	/* Save original maximum cache size */
    max_open = guard_band + 1;	/* Limit to one usable cache entry */
    tp = &tests[0];

    saved = tp->op_type;	/* Save original value */

    fp->Original_fsize = cmdline.fsize;	/* This is full test, but consunmes 
					   vast amounts of time for DB */
    if (fp->Original_fsize > 1024)
	fp->Original_fsize = 1024;	/* Do 1 MB files to shorten validate phase */

    /* Let's force this now, so we don't have it blow up later. */
    saved_direct_percent = my_mix_table[my_workload].percent_direct;
    if (my_mix_table[my_workload].percent_direct != 0)
    {
	/*
	   fprintf(newstdout,"Set Direct \n");
	   fflush(newstdout);
	 */
	my_mix_table[my_workload].percent_direct = 100;
    }
    /* Create validate directory */

    snprintf (buf, sizeof buf, "CL%d_%s_val", cmdline.client_id,
	      work_obj[my_workload].work_obj_name);
    vdir = netmist_vfs_root ();
    ret = netmist_vfs_walkme (&vdir, buf);
    if (ret)
    {
	log_file (LOG_ERROR, "client_validate_ops VFS walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    fp->dir = vdir;
    fp->relfilename = "validate_file";

    /* Use virtualized, in case this function is workload specific */
    ret = (*Netmist_Ops[my_workload].init_dir) (vdir);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "Mkdir failed. Error: %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    /* Create file to validate op */
    log_file (LOG_DEBUG, "validate: Create\n");

    /* Create a file and fill it up with data */
    (*Netmist_Ops[my_workload].init_file) (0, fp);

    if (my_mix_table[my_workload].percent_write != 0)
    {
	/* Write */
	tp->op_type = OP_WRITE;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;

	log_file (LOG_DEBUG, "validate: Write\n");

	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* Write file op */
    if (my_mix_table[my_workload].percent_write_file != 0)
    {
	tp->op_type = OP_WRITE_FILE;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;

	log_file (LOG_DEBUG, "validate: Write File\n");

	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write_file) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* Write_rand */
    if (my_mix_table[my_workload].percent_write_rand != 0)
    {

	log_file (LOG_DEBUG, "validate: Write rand\n");

	tp->op_type = OP_WRITE_RAND;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].write_rand) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* Read */
    if (my_mix_table[my_workload].percent_read != 0)
    {
	log_file (LOG_DEBUG, "validate: Read\n");

	tp->op_type = OP_READ;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* Read file op */
    if (my_mix_table[my_workload].percent_read_file != 0)
    {
	log_file (LOG_DEBUG, "validate: Read file\n");

	tp->op_type = OP_READ_FILE;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read_file) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* Read rand */
    if (my_mix_table[my_workload].percent_read_rand != 0)
    {
	log_file (LOG_DEBUG, "validate: Read Rand\n");

	tp->op_type = OP_READ_RAND;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].read_rand) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* mmap read */
    if (my_mix_table[my_workload].percent_mmap_read != 0)
    {
	log_file (LOG_DEBUG, "validate: Mmap Read\n");

	tp->op_type = OP_MM_READ;
	fp->mmapread_offset = 0LL;
	fp->offset = 0;
#if defined(WIN32)
	fp->file_desc = (HANDLE) 0;
	fp->file_open_flags = 0;
#else
	fp->file_desc = 0;
	fp->file_open_flags = (unsigned int) 0;
#endif
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mmap_read) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* mmap write */
    if (my_mix_table[my_workload].percent_mmap_write != 0)
    {
	log_file (LOG_DEBUG, "validate: Mmap write\n");

	fp->access_count = 0;
	tp->op_type = OP_MM_WRITE;
	fp->offset = 0;
	fp->mmapwrite_offset = 0LL;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mmap_write) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* rmw  */
    if (my_mix_table[my_workload].percent_rmw != 0)
    {
	log_file (LOG_DEBUG, "validate: RMW\n");

	tp->op_type = OP_RMW;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].rmw) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }
    /* Create an mkdir directory */
    if (my_mix_table[my_workload].percent_mkdir != 0)
    {
	char mkdirbuf[MAXNAME];
	struct netmist_vfs_dir *mkdirdir;
	struct netmist_vfs_dir *olddir = fp->dir;

	log_file (LOG_DEBUG, "validate: Mkdir\n");

	snprintf (mkdirbuf, sizeof (mkdirbuf), "CL%d_%s_mkdir",
		  cmdline.client_id, work_obj[my_workload].work_obj_name);
	mkdirdir = netmist_vfs_root ();
	ret = netmist_vfs_walkme (&mkdirdir, mkdirbuf);
	if (ret)
	{
	    log_file (LOG_ERROR, "client_validate_ops VFS walk error %d\n",
		      ret);
	    R_exit (NETMIST_VFS_ERR, ret);
	}

	/* Use virtualized, in case this function is workload specific */
	ret = (*Netmist_Ops[my_workload].init_dir) (mkdirdir);
	if (ret != 0)
	{
	    log_file (LOG_ERROR, "Mkdir failed of %s Error: %d\n", mkdirbuf,
		      ret);
	    R_exit (NETMIST_VFS_ERR, ret);
	}
	fp->dir = mkdirdir;
	fp->relfilename = "";
	/* mkdir  */
	tp->op_type = OP_MKDIR;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].mkdir) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);

	/* Now remove it */
	ret = (*Netmist_Ops[my_workload].remove_dir) (fp->dir);
	if (ret < 0)
	{
	    log_file (LOG_ERROR,
		      "client_validate_ops: remove_dir op failed: %s\n",
		      netmist_strerror (netmist_get_error ()));
	}

	fp->dir = olddir;
	VFS (dfree, &mkdirdir);
    }

    /* unlink  */
    if (my_mix_table[my_workload].percent_unlink != 0)
    {
	log_file (LOG_DEBUG, "validate: Unlink\n");

	tp->op_type = OP_UNLINK;
	fp->relfilename = "unlinkfile";
	/* Create an empty file */
	log_file (LOG_DEBUG, "validate: Init empty file\n");

	(*Netmist_Ops[my_workload].init_empty_file) (vdir, "unlinkfile");
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	fp->relfilename = "unlinkfile";
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].unlink) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);

	/* The postfix_op routine put it back */
	(*Netmist_Ops[my_workload].remove_file) (fp->dir, "unlinkfile");
	fp->relfilename = "validate_file";
    }

    /* truncate  */
    if (my_mix_table[my_workload].percent_trunc != 0)
    {
	log_file (LOG_DEBUG, "validate: Truncate\n");

	tp->op_type = OP_TRUNC;
	fp->relfilename = "trunc_file";

	(*Netmist_Ops[my_workload].init_empty_file) (vdir, "trunc_file");
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].trunc) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);

	/* Now remove it */
	(*Netmist_Ops[my_workload].remove_file) (fp->dir, "trunc_file");
    }

    /* create  */
    if (my_mix_table[my_workload].percent_create != 0)
    {
	log_file (LOG_DEBUG, "validate: Create\n");

	tp->op_type = OP_CREATE;
	fp->relfilename = "createfile";
	/* Create an empty file */
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].create) (fp, 0);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);

	fp->relfilename = "validate_file";
    }

    /* append  */
    if (my_mix_table[my_workload].percent_append != 0)
    {
	log_file (LOG_DEBUG, "validate: Append\n");

	tp->op_type = OP_APPEND;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].append) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }
    /* locking  */
    if (my_mix_table[my_workload].percent_locking != 0)
    {
	log_file (LOG_DEBUG, "validate: Lock\n");

	tp->op_type = OP_LOCKING;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].locking) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }
    /* access  */
    if (my_mix_table[my_workload].percent_access != 0)
    {
	log_file (LOG_DEBUG, "validate: Access\n");

	tp->op_type = OP_ACCESS;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].access) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }
    /* stat  */
    if (my_mix_table[my_workload].percent_stat != 0)
    {
	log_file (LOG_DEBUG, "validate: Stat\n");

	tp->op_type = OP_STAT;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].stat) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* chmod  */
    if (my_mix_table[my_workload].percent_chmod != 0)
    {
	log_file (LOG_DEBUG, "validate: Chmod\n");

	tp->op_type = OP_CHMOD;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].chmod) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* readdir  */
    if (my_mix_table[my_workload].percent_readdir != 0)
    {
	log_file (LOG_DEBUG, "validate: Readdir\n");

	tp->op_type = OP_READDIR;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].readdir) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* rename  */
    if (my_mix_table[my_workload].percent_rename != 0)
    {
	log_file (LOG_DEBUG, "validate: Rename\n");

	tp->op_type = OP_RENAME;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].rename) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* statfs  */
    if (my_mix_table[my_workload].percent_statfs != 0)
    {
	log_file (LOG_DEBUG, "validate: Statfs\n");

	tp->op_type = OP_STATFS;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].statfs) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* pathconf  */
    if (my_mix_table[my_workload].percent_pathconf != 0)
    {
	log_file (LOG_DEBUG, "validate: Pathconf\n");

	tp->op_type = OP_PATHCONF;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].pathconf) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
    }

    /* copyfile  */
    if (my_mix_table[my_workload].percent_copyfile != 0)
    {
	log_file (LOG_DEBUG, "validate: Copyfile\n");

	tp->op_type = OP_COPYFILE;
	fp->offset = 0;
	fp->access_count = 0;
	fp->op_count = 0;
	fp->db_access_count = 0;
	(*Netmist_Ops[my_workload].prefix_op) (x, fp);
	(*Netmist_Ops[my_workload].copyfile) (fp);
	(*Netmist_Ops[my_workload].postfix_op) (x, fp);
	(*Netmist_Ops[my_workload].remove_file) (fp->dir, fp->relfilename);
	snprintf (alt_name, sizeof (alt_name), "%s_2", fp->relfilename);
	(*Netmist_Ops[my_workload].remove_file) (fp->dir, alt_name);
    }
    else
    {
	(*Netmist_Ops[my_workload].remove_file) (fp->dir, fp->relfilename);
    }

    /* Turn file descriptor caching back on */
    in_validate = 0;
    special = 0;
    /* Restore maximum cache size to what was found by the sysconf call. */
    max_open = old_max_open;
    ret = (*Netmist_Ops[my_workload].remove_dir) (vdir);
    if (ret != 0)
    {
	log_file (LOG_ERROR,
		  "client_validate_ops: remove_dir (_val) failed: %d\n", ret);

    }
    VFS (dfree, &fp->dir);
    fp->relfilename = NULL;	/* This was in static storage */
    fp->Original_fsize = cmdline.fsize;	/* set it back to default */

    /* free (fo.relfilename); This is on the stack !! */

    fp = NULL;
    /* Restore this now */
    my_mix_table[my_workload].percent_direct = saved_direct_percent;
    tp->op_type = saved;	/* Restore original value */
}


/**
 * @brief This creates a file with the path that is provided.
 *        It is used in the INIT phase before the actual
 *        testing starts, and by postfix() to put files back 
 *        after an unlink.
 *        When this is called during the INIT phase, it can 
 *        create a new file and fill it, or.. it can truncate 
 *        it back to the write size, or it can grow it to the 
 *        right size, or it can just read it so that the client's 
 *        caches are defeated.
 *
 * @param  op_type  : Op type for this operation.
 * @param  file_ptr : Pointer to file object for this file.
 */
/*
 * __doc__
 * __doc__  Function : void generic_init_file(char *filename, int op_type, 
 * __doc__                  int which_dir, int filenum, struct file_object *fp)
 * __doc__  Arguments: op_type: Op type for this operation.
 * __doc__             file_ptr: Pointer to the file object for this file.
 * __doc__  Returns  : void
 * __doc__  Performs : This creates a file with the path that is provided in the file object.
 * __doc__             It is used in the INIT phase before the actual
 * __doc__             testing starts, and by postfix() to put files back 
 * __doc__             after an unlink.
 * __doc__             When this is called during the INIT phase, it can 
 * __doc__             create a new file and fill it, or.. it can truncate 
 * __doc__             it back to the write size, or it can grow it to the 
 * __doc__             right size, or it can just read it so that the client's 
 * __doc__             caches are defeated.
 * __doc__
 *
 */
void
generic_init_file (int op_type, struct file_object *file_ptr)
{
    char *buf;
    int err;
    uint64_t tlen;
    int fringe = 0;
    long long xfer_count = 0;	/* In bytes */
    long long jj;
    double pre_time = 0;
    double pre_stat_time = 0;
    double post_time = 0;
    double post_stat_time = 0;
    int lat_delay = 0;
    int this_dedup_group;
    int nap_time = 0;
    int dedup_across, my_delta_time;

    struct netmist_vfs_object *fd = NULL;
    long long i;
    struct netmist_vfs_stat stbuf;

    log_file (LOG_DEBUG, "Enter generic_init_file\n");

    buf = main_buf;
    if (init_phase)		/* Just get the length set correctly */
    {
	/* If the file doesn't exist, then skip to the create region. */
	log_file (LOG_DEBUG, "Enter generic_init_file #3\n");

	pre_stat_time = gettime ();
	err = VFS (stat, file_ptr->dir, file_ptr->relfilename, &stbuf);
	if (err != 0)
	{
	    log_file (LOG_DEBUG, "stat error %d: goto Jump1\n", err);
	    goto jump1;
	}
	post_stat_time = gettime ();

	   /*-----------------------------------
	    * Not right size ? Try to grow it 
	    *-----------------------------------
	    */
	if (((unsigned long long) (stbuf.netmist_st_size)) <
	    (file_ptr->Original_fsize * 1024))
	{
	    log_file (LOG_DEBUG, "Grow Was %lld  is %lld\n",
		      (long long) (stbuf.netmist_st_size),
		      file_ptr->Original_fsize);


	    err = VFS (open, file_ptr->dir, file_ptr->relfilename,
		       NETMIST_OPEN_WRITE | NETMIST_OPEN_APPEND, &fd);
	    if (err != 0)
	    {
		const char *nes = netmist_vfs_errstr (err);
		log_file (LOG_ERROR,
			  "Open existing file for write failed. Filename = %s in %s Error: %s Error value: %d\n",
			  file_ptr->relfilename, VFS (pathptr, file_ptr->dir),
			  nes, err);
		R_exit (NETMIST_OP_VFS_ERR + 6, err);
	    }
#if defined(WIN32)
	    /* XXX Why is this windows only? */
	    VFS (seek, fd, 0, NETMIST_SEEK_END, NULL);
#endif
	    if (cmdline.sharing_flag)	/* In sharing mode, prevent dual writers */
	    {
		err = VFS (lock, fd, NETMIST_VFS_LOCK_TRY);
		if (err == 0)	/* Got lock ? */
		{
		    pre_stat_time = gettime ();
		    err =
			VFS (stat, file_ptr->dir, file_ptr->relfilename,
			     &stbuf);
		    post_stat_time = gettime ();

		    if (((long long) (stbuf.netmist_st_size) /
			 (unsigned long long) 1024) ==
			(file_ptr->Original_fsize))
			goto out;
		}
		else
		    goto out;	/* Let the other proc do it */
	    }
	    if (file_ptr->dedup_group == 0)
		file_ptr->dedup_group = netmist_rand ();

	    if (my_mix_table[my_workload].dedup_group_count == 0)
		my_mix_table[my_workload].dedup_group_count = 1;

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_within)
		this_dedup_group = file_ptr->dedup_group;
	    else
	    {
		if (my_mix_table[my_workload].dedup_group_count == 1)
		    this_dedup_group = 1;
		else
		    this_dedup_group =
			(file_ptr->dedup_group %
			 (my_mix_table[my_workload].dedup_group_count));
	    }

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_across)
		dedup_across = 1;
	    else
		dedup_across = 0;
	    if ((file_ptr->Original_fsize * 1024) - (stbuf.netmist_st_size) >=
		(unsigned long long) max_xfer_size)
	    {
		for (i = ((unsigned long long) (stbuf.netmist_st_size));
		     (unsigned long long) (i + max_xfer_size) <
		     (file_ptr->Original_fsize * 1024);
		     i += (unsigned long long) max_xfer_size)
		{
		    if (my_mix_table[my_workload].patt_vers == 1)
		    {
			make_non_dedup ((long *) buf, (int) max_xfer_size);
			if (my_mix_table[my_workload].cipher)
			    make_cypher ((char *) buf, (int) max_xfer_size);
		    }
		    else
		    {
			make_data_layout ((long *) buf, max_xfer_size,
					  file_ptr, (long long) i,
					  this_dedup_group, dedup_across);
			if (my_mix_table[my_workload].cipher)
			    make_cypher ((char *) buf, (int) max_xfer_size);
		    }
		    xfer_count += max_xfer_size;

		    log_file (LOG_DEBUG,
			      "Writing chunk, non-fringe. Size %d b. File size %lld bytes\n",
			      max_xfer_size, file_ptr->Original_fsize * 1024);

		    if (init_phase)
			pre_time = gettime ();

		    err = VFS (write, fd, buf, max_xfer_size, &tlen);
		    if (err)
		    {
			const char *nes = netmist_vfs_errstr (err);
			log_file (LOG_ERROR,
				  "Write file failed (1) to file %s in %s Error: %s Error value %d Ret %"
				  PRIu64 "\n", file_ptr->relfilename,
				  VFS (pathptr, file_ptr->dir), nes, err,
				  tlen);
			VFS (close, &fd);
			R_exit (NETMIST_OP_VFS_ERR + 7, err);
		    }

		    if (init_phase)
		    {
			post_time = gettime ();
			/*
			 * Throttle back on I/O rate, if needed. Don't swamp the filer 
			 */
			if (my_mix_table[my_workload].init_rate_enable == 1)
			{
			    global_reset_counter++;
			    /* Every 1000 I/O ops, lets restart the average  
			     * calculation. We need to use a large sample 
			     * because the nap() resolution is in 
			     * milliseconds.   
			     */
			    if (global_reset_counter == 1000)
			    {
				global_reset_counter = 0;
				total_init_time = 0.0;
				total_init_kbytes = 0;
				avg_init_mb_per_sec = 0.0;
			    }
			    total_init_time += (post_time - pre_time);
			    total_init_kbytes += (max_xfer_size / 1024);
			    avg_init_mb_per_sec =
				(total_init_kbytes / 1024) / total_init_time;

			    if ((my_mix_table[my_workload].init_rate_speed !=
				 0.0)
				&& ((avg_init_mb_per_sec) >
				    my_mix_table[my_workload].
				    init_rate_speed))
			    {
				if (nap_time == 0)
				    nap_time += 1;
				nap (nap_time);
				total_init_time +=
				    (float) nap_time / (float) 1000;
			    }
			    avg_init_mb_per_sec =
				(total_init_kbytes / 1024) / total_init_time;
			    if ((my_mix_table[my_workload].init_rate_speed !=
				 0.0)
				&& ((avg_init_mb_per_sec) >
				    my_mix_table[my_workload].
				    init_rate_speed))
				nap_time = nap_time * 2;
			    if ((my_mix_table[my_workload].init_rate_speed !=
				 0.0)
				&& ((avg_init_mb_per_sec) <
				    my_mix_table[my_workload].init_rate_speed)
				&& (nap_time != 0))
				nap_time = nap_time / 2;
			}

			/*
			 * Emergency throttle back on I/O rate, if needed. 
			 * Don't swamp the filer
			 */
			if ((post_time - pre_time) > MAX_LATENCY)
			{
			    log_file (LOG_DEBUG, "Slowing init\n");

			    lat_delay += 1000;
			    if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
				lat_delay = (MAX_SLOW_LATENCY * 1000);
			}
			if (((post_time - pre_time) < MAX_LATENCY)
			    && (lat_delay > 0))
			{
			    log_file (LOG_DEBUG, "Accelerating init\n");
			    lat_delay = lat_delay / 2;
			}
			if (lat_delay < 0)
			    lat_delay = 0;
			if (lat_delay > 0)
			    nap (lat_delay);
		    }
		}
	    }

	    if (file_ptr->dedup_group == 0)
		file_ptr->dedup_group = netmist_rand ();

	    if (my_mix_table[my_workload].dedup_group_count == 0)
		my_mix_table[my_workload].dedup_group_count = 1;

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_within)
		this_dedup_group = file_ptr->dedup_group;
	    else
	    {
		if (my_mix_table[my_workload].dedup_group_count == 1)
		    this_dedup_group = 1;
		else
		    this_dedup_group =
			(file_ptr->dedup_group %
			 (my_mix_table[my_workload].dedup_group_count));
	    }

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_across)
		dedup_across = 1;
	    else
		dedup_across = 0;
	    if ((unsigned long long) (stbuf.netmist_st_size + xfer_count) <
		(file_ptr->Original_fsize * 1024))
	    {
		/* Fix up the fringe at the end */
		fringe =
		    (file_ptr->Original_fsize * 1024 -
		     (stbuf.netmist_st_size + xfer_count));
		jj = stbuf.netmist_st_size + xfer_count;

		if (my_mix_table[my_workload].patt_vers == 1)
		{
		    make_non_dedup ((long *) buf, (int) fringe);
		    if (my_mix_table[my_workload].cipher)
			make_cypher ((char *) buf, (int) fringe);
		}
		else
		{
		    make_data_layout ((long *) buf, fringe, file_ptr,
				      (long long) jj, this_dedup_group,
				      dedup_across);
		    if (my_mix_table[my_workload].cipher)
			make_cypher ((char *) buf, (int) fringe);
		}

		log_file (LOG_DEBUG,
			  "Writing fringe of %d bytes. xfer %lld Original %lld st_size %lld\n",
			  fringe, xfer_count,
			  (long long) (file_ptr->Original_fsize),
			  (long long) (stbuf.netmist_st_size / 1024));


		if (my_mix_table[my_workload].patt_vers == 1)
		{
		    make_non_dedup ((long *) buf, (int) fringe);
		    if (my_mix_table[my_workload].cipher)
			make_cypher ((char *) buf, (int) fringe);
		}
		else
		{
		    make_data_layout ((long *) buf, fringe, file_ptr,
				      ((file_ptr->Original_fsize * 1024LL) -
				       fringe), this_dedup_group,
				      dedup_across);
		    if (my_mix_table[my_workload].cipher)
			make_cypher ((char *) buf, (int) fringe);
		}

		err = VFS (write, fd, buf, fringe, &tlen);
		if (err)
		{
		    const char *nes = netmist_vfs_errstr (err);
		    log_file (LOG_ERROR,
			      "Write file failed (2) to file %s in %s Error: %s Error value %d Ret %"
			      PRIu64 "\n", file_ptr->relfilename,
			      VFS (pathptr, file_ptr->dir), nes, err, tlen);
		    VFS (close, &fd);
		    R_exit (NETMIST_OP_VFS_ERR + 7, err);
		}
	    }
	    VFS (close, &fd);	/* counted below */
	    if (my_mix_table[my_workload].init_read == 1)
		purge_file (file_ptr);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	    return;
	}
	/*
	 *----------------------------------------------------------------
	 * Case of the file is too big. So we need to then trim it back.
	 *----------------------------------------------------------------
	 */
	if (((unsigned long long) (stbuf.netmist_st_size)) >
	    (file_ptr->Original_fsize * 1024))
	{

	    log_file (LOG_DEBUG, "Shrink Was %lld  is %lld\n",
		      (long long) (stbuf.netmist_st_size),
		      file_ptr->Original_fsize);

	    err = VFS (open, file_ptr->dir, file_ptr->relfilename,
		       NETMIST_OPEN_WRITE | NETMIST_OPEN_APPEND, &fd);
	    if (err != 0)
	    {
		const char *nes = netmist_vfs_errstr (err);
		log_file (LOG_ERROR,
			  "Open existing file for trunc failed. Filename = %s in %s Error: %s Error value: %d\n",
			  file_ptr->relfilename, VFS (pathptr, file_ptr->dir),
			  nes, err);
		R_exit (NETMIST_OP_VFS_ERR + 6, err);
	    }

	    ubuntu = VFS (trunc, fd,
			  file_ptr->Original_fsize * (long long) 1024);
	    if (ubuntu)
	    {
		;
	    }
	    VFS (close, &fd);
	    /*  Just enough to break the caches. One op type has sufficient 
	     *  files and space allocated to break the client's caches.
	     *  Reading more than this just consumes wall clock with no
	     *  benefit to the results.
	     */
	    if ((op_type == OP_READ) || (op_type == OP_READ_FILE))
	    {
		if (my_mix_table[my_workload].init_read == 1)
		    purge_file (file_ptr);
		return;
	    }
	    else
		return;
	}
	/* File the file is already the right size */
	if (((unsigned long long) (stbuf.netmist_st_size)) ==
	    (file_ptr->Original_fsize * 1024))
	{
	    /*  Just enough to break the caches. One op type has sufficient 
	     *  files and space allocated to break the client's caches.
	     *  Reading more than this just consumes wall clock with no
	     *  benefit to the results.
	     */
	    if ((op_type == OP_READ) || (op_type == OP_READ_FILE))
	    {

		log_file (LOG_DEBUG, "Purge file\n");

		if (my_mix_table[my_workload].init_read == 1)
		    purge_file (file_ptr);
		/*
		 * Emergency throttle back on stat() rate, if needed. 
		 * Don't swamp the filer
		 */
		if ((post_stat_time - pre_stat_time) > MAX_STAT_LATENCY)
		{
		    log_file (LOG_DEBUG, "Slowing stat rate during init\n");
		    lat_delay += 1000;
		    if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
			lat_delay = (MAX_SLOW_LATENCY * 1000);
		}
		if (((post_stat_time - pre_stat_time) < MAX_STAT_LATENCY)
		    && (lat_delay > 0))
		{
		    log_file (LOG_DEBUG, "Accelerating init\n");
		    lat_delay = lat_delay / 2;
		}
		if (lat_delay < 0)
		    lat_delay = 0;
		if (lat_delay > 0)
		    nap (lat_delay);
		/* End of throttle stat() calls */
		return;
	    }
	    else
	    {
		log_file (LOG_DEBUG, "Skip file\n");
		/*
		 * Emergency throttle back on stat() rate, if needed. 
		 * Don't swamp the filer
		 */
		if ((post_stat_time - pre_stat_time) > MAX_STAT_LATENCY)
		{
		    log_file (LOG_DEBUG, "Slowing stat rate during init\n");
		    lat_delay += 1000;
		    if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
			lat_delay = (MAX_SLOW_LATENCY * 1000);
		}
		if (((post_stat_time - pre_stat_time) < MAX_STAT_LATENCY)
		    && (lat_delay > 0))
		{
		    log_file (LOG_DEBUG, "Accelerating init\n");
		    lat_delay = lat_delay / 2;
		}
		if (lat_delay < 0)
		    lat_delay = 0;
		if (lat_delay > 0)
		    nap (lat_delay);
		/* End of throttle stat() calls */
		return;
	    }
	}
    }

  jump1:
    /*
     *----------------------------------------------------------------
     * Case of the file doesn't exist... So we create and fill here.
     *----------------------------------------------------------------
     */
    log_file (LOG_DEBUG, "New Size %lld \n", file_ptr->Original_fsize);

    err =
	VFS (open, file_ptr->dir, file_ptr->relfilename, NETMIST_OPEN_CREATE,
	     &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open new file for write failed. Filename = %s in %s Error: %s Error value: %d\n",
		  file_ptr->relfilename, VFS (pathptr, file_ptr->dir),
		  nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 6, err);
    }
    if (cmdline.sharing_flag)	/* In sharing mode, prevent dual writers */
    {
	err = VFS (lock, fd, NETMIST_VFS_LOCK_TRY);
	if (err == 0)		/* Got lock ? */
	{
	    /* Check to see if we have a collision in sharemode */
	    pre_stat_time = gettime ();
	    err = VFS (stat, file_ptr->dir, file_ptr->relfilename, &stbuf);
	    post_stat_time = gettime ();

	    if (((unsigned long long) (stbuf.netmist_st_size) /
		 (long long) 1024) == (file_ptr->Original_fsize))
		goto out;
	}
	else
	    goto out;
    }
    if (init_phase)
	init_new_file++;
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    work_obj[my_workload].open_op_count += 1;
    xfer_count = 0;
    if ((file_ptr->Original_fsize * 1024) >=
	(unsigned long long) max_xfer_size)
    {
	for (i = 0;
	     (unsigned long long) (i + max_xfer_size) <
	     (file_ptr->Original_fsize * 1024);
	     i += (unsigned long long) (max_xfer_size))
	{
	    /*-----------------*/
	    /* heartbeat       */
	    /*-----------------*/
	    cur_time = gettime ();
	    /* 
	     * Need to deliver heartbeats from every proc and filter 
	     * at the Prime.
	     */
	    my_delta_time = (int) (cur_time - last_heart_beat);

	    if (cmdline.heartbeat_flag && init_phase
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }
	    if (cmdline.heartbeat_flag && in_validate
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }
	    /*-----------------*/
	    if (file_ptr->dedup_group == 0)
		file_ptr->dedup_group = netmist_rand ();

	    if (my_mix_table[my_workload].dedup_group_count == 0)
		my_mix_table[my_workload].dedup_group_count = 1;

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_within)
		this_dedup_group = file_ptr->dedup_group;
	    else
	    {
		if (my_mix_table[my_workload].dedup_group_count == 1)
		    this_dedup_group = 1;
		else
		    this_dedup_group =
			(file_ptr->dedup_group %
			 (my_mix_table[my_workload].dedup_group_count));
	    }

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_across)
		dedup_across = 1;
	    else
		dedup_across = 0;
	    if (my_mix_table[my_workload].patt_vers == 1)
	    {
		make_non_dedup ((long *) buf, (int) max_xfer_size);
		if (my_mix_table[my_workload].cipher)
		    make_cypher ((char *) buf, (int) max_xfer_size);
	    }
	    else
	    {
		make_data_layout ((long *) buf, max_xfer_size, file_ptr,
				  (long long) xfer_count, this_dedup_group,
				  dedup_across);
		if (my_mix_table[my_workload].cipher)
		    make_cypher ((char *) buf, (int) max_xfer_size);
	    }
	    xfer_count += max_xfer_size;

	    log_file (LOG_DEBUG,
		      "Writing new chunk, non-fringe. Size %d bytes. File size %lld bytes\n",
		      max_xfer_size, file_ptr->Original_fsize * 1024);

	    if (init_phase)
		pre_time = gettime ();
	    cur_time = gettime ();
	    my_delta_time = (int) (cur_time - last_heart_beat);

	    if (cmdline.heartbeat_flag && init_phase
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }
	    if (cmdline.heartbeat_flag && in_validate
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }

	    err = VFS (write, fd, buf, max_xfer_size, &tlen);
	    if (err)
	    {
		const char *nes = netmist_vfs_errstr (err);
		log_file (LOG_ERROR,
			  "Write file failed (3) to file %s in %s Error: %s Error value %d Ret %"
			  PRIu64 "\n", file_ptr->relfilename, VFS (pathptr,
								   file_ptr->dir),
			  nes, err, tlen);
#if defined(WIN32)
		/*explicitly unlock file */
		UnlockFile (fd, 0, 0, (DWORD) - 1, (DWORD) - 1);
#endif
		VFS (close, &fd);
		R_exit (NETMIST_OP_VFS_ERR + 7, err);
	    }

	    if (init_phase)
	    {
		post_time = gettime ();
		if (my_mix_table[my_workload].init_rate_enable == 1)
		{
		    global_reset_counter++;
		    /* Every 1000 I/O ops, lets restart the average         */
		    /* calculation. We need to use a large sample because */
		    /* the nap() resolution is in milliseconds.     */
		    if (global_reset_counter == 1000)
		    {
			global_reset_counter = 0;
			total_init_time = 0.0;
			total_init_kbytes = 0;
			avg_init_mb_per_sec = 0.0;
		    }
		    total_init_time += (post_time - pre_time);
		    total_init_kbytes += (max_xfer_size / 1024);
		    avg_init_mb_per_sec =
			(total_init_kbytes / 1024) / total_init_time;

		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) >
			    my_mix_table[my_workload].init_rate_speed))
		    {
			if (nap_time == 0)
			    nap_time += 1;
			nap (nap_time);
			total_init_time += (float) nap_time / (float) 1000;
		    }
		    avg_init_mb_per_sec =
			(total_init_kbytes / 1024) / total_init_time;

		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) >
			    my_mix_table[my_workload].init_rate_speed)
			&& (nap_time != 0))
			nap_time = nap_time * 2;
		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) <
			    my_mix_table[my_workload].init_rate_speed)
			&& (nap_time != 0))
			nap_time = nap_time / 2;
		}

		/*
		 * Emergency throttle back on I/O rate, if needed. Don't swamp the filer
		 */
		if ((post_time - pre_time) > MAX_LATENCY)
		{

		    log_file (LOG_DEBUG, "Slowing init\n");

		    lat_delay += 1000;
		    if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
			lat_delay = (MAX_SLOW_LATENCY * 1000);
		}
		if (((post_time - pre_time) < MAX_LATENCY) && (lat_delay > 0))
		{
		    log_file (LOG_DEBUG, "Accerating init\n");

		    lat_delay = lat_delay / 2;
		}
		if (lat_delay < 0)
		    lat_delay = 0;
		if (lat_delay > 0)
		    nap (lat_delay);
	    }
	    total_file_ops += 1;
	    work_obj[my_workload].total_file_ops += 1;
	    work_obj[my_workload].write_op_count += 1;
	    /*
	     * Help keep the client's page cache from getting too dirty.
	     */
	    if ((work_obj[my_workload].write_op_count % 10 == 0)
		&& cmdline.flush_flag)
		VFS (fsync, fd);
	}
    }
    if (file_ptr->dedup_group == 0)
	file_ptr->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = file_ptr->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(file_ptr->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;
    /* Fix up the fringe at the end */
    if ((unsigned long long) xfer_count < file_ptr->Original_fsize * 1024)
    {
	cur_time = gettime ();
	my_delta_time = (int) (cur_time - last_heart_beat);

	if (cmdline.heartbeat_flag && init_phase
	    && (my_delta_time >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				  (double) 0.0);
	    last_heart_beat = cur_time;
	}
	if (cmdline.heartbeat_flag && in_validate
	    && (my_delta_time >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT,
				  (double) 0.0);
	    last_heart_beat = cur_time;
	}
	fringe = file_ptr->Original_fsize * 1024 - xfer_count;
	if (my_mix_table[my_workload].patt_vers == 1)
	{
	    make_non_dedup ((long *) buf, (int) fringe);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) fringe);
	}
	else
	{
	    make_data_layout ((long *) buf, fringe, file_ptr,
			      (long long) ((file_ptr->Original_fsize * 1024LL)
					   - fringe), this_dedup_group,
			      dedup_across);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) fringe);
	}

	log_file (LOG_DEBUG,
		  "Writing fringe of %d bytes. xfercount %lld\n", fringe,
		  xfer_count);
	log_file (LOG_DEBUG, "Original cmdline.fsize %lld\n",
		  file_ptr->Original_fsize * 1024);

	cur_time = gettime ();
	my_delta_time = (int) (cur_time - last_heart_beat);

	if (cmdline.heartbeat_flag && init_phase
	    && (my_delta_time >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				  (double) 0.0);
	    last_heart_beat = cur_time;
	}
	if (cmdline.heartbeat_flag && in_validate
	    && (my_delta_time >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT,
				  (double) 0.0);
	    last_heart_beat = cur_time;
	}

	err = VFS (write, fd, buf, fringe, &tlen);
	if (err)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Write file failed (4) to file %s in %s Error: %s Error value %d Ret %"
		      PRIu64 "\n", file_ptr->relfilename, VFS (pathptr,
							       file_ptr->dir),
		      nes, err, tlen);
	    VFS (close, &fd);
	    R_exit (NETMIST_OP_VFS_ERR + 7, err);
	}
    }
/****/

    pre_stat_time = gettime ();
    err = VFS (stat, file_ptr->dir, file_ptr->relfilename, &stbuf);
    log_file (LOG_DEBUG, "New actual size %lld\n",
	      (long long) (stbuf.netmist_st_size));
    post_stat_time = gettime ();

/****/
    if (cmdline.lflag && cmdline.flush_flag)
    {
	VFS (fsync, fd);	/* Not needed as it is not measured */
    }
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    work_obj[my_workload].close_op_count += 1;
  out:
    /*
     * Emergency throttle back on stat() rate, if needed. 
     * Don't swamp the filer
     */
    if ((post_stat_time - pre_stat_time) > MAX_LATENCY)
    {
	log_file (LOG_DEBUG, "Slowing init\n");
	lat_delay += 1000;
	if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
	    lat_delay = (MAX_SLOW_LATENCY * 1000);
    }
    if (((post_stat_time - pre_stat_time) < MAX_LATENCY) && (lat_delay > 0))
    {
	log_file (LOG_DEBUG, "Accelerating init\n");
	lat_delay = lat_delay / 2;
    }
    if (lat_delay < 0)
	lat_delay = 0;
    if (lat_delay > 0)
	nap (lat_delay);
    /* End of throttle stat() calls */

    cur_time = gettime ();
    my_delta_time = (int) (cur_time - last_heart_beat);

    if (cmdline.heartbeat_flag && init_phase
	&& (my_delta_time >= HEARTBEAT_TICK))
    {
	tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
			      (double) 0.0);
	last_heart_beat = cur_time;
    }
    if (cmdline.heartbeat_flag && in_validate
	&& (my_delta_time >= HEARTBEAT_TICK))
    {
	tell_prime_heartbeat (cmdline.real_client_id, VAL_BEAT, (double) 0.0);
	last_heart_beat = cur_time;
    }
#if defined(WIN32)
    /*explicitly unlock file */
    UnlockFile (fd, 0, 0, (DWORD) - 1, (DWORD) - 1);
#endif
    VFS (close, &fd);
    work_obj[my_workload].close_op_count++;
    total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
}

/**
 * @brief This creates an empty file with the path that is 
 *        provided. It is used in the INIT phase before the 
 *        actual testing starts, and by postfix() to put files 
 *        back after an unlink.
 *
 * @param d : Pointer to the directory object
 * @param relfilename : Pointer to relative file name. Relative to this directory object.
 */
/*
 * __doc__
 * __doc__  Function : void generic_init_empty_file(struct netmist_vfs_dir *d, char *relfilename)
 * __doc__  Arguments: d : Pointer to the directory object
 * __doc__  Arguments: relfilename : String/name of file that is relative to this directory object.
 * __doc__  Returns  : error codes or 0
 * __doc__  Performs : This creates an empty file with the path that is 
 * __doc__             provided. It is used in the INIT phase before the 
 * __doc__             actual testing starts, and by postfix() to put files 
 * __doc__             back after an unlink.
 * __doc__
 */
int
generic_init_empty_file (struct netmist_vfs_dir *d, char *relfilename)
{
    int err;
    int retry_unlink = 0;
    struct netmist_vfs_object *fd = NULL;

  again:
    err = VFS (open, d, relfilename, NETMIST_OPEN_CREATE, &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);

#if defined(WIN32)
	if (err == ERROR_FILE_EXISTS)	/* If exists, just return */
	{
	    return err;
	}
	/* 
	 * Windows unique behavior here !!!! 
	 * If any process anywhere on any node has an open handle
	 * or an open handle in progress, or a delete that is pending,
	 * then this create can fail with ERROR_ACCESS_DENIED.
	 *
	 * In our case, someone else has initialized the dir, so just
	 * skip this dir and move on.
	 */
	if (err == ERROR_ACCESS_DENIED)
	{
	    return err;
	}
#endif

	log_file (LOG_ERROR,
		  "Write empty file failed %s in %s Error Value %d  Error %s\n",
		  relfilename, VFS (pathptr, d), err, nes);
#if !defined(WIN32)
	/* Need to return the error so that the caller can take appropriate steps */
	if (init_phase)
	{
	    if (err == EEXIST)	/* If exists, just return */
	    {
		return err;
	    }
	}
#endif

	if (retry_unlink++ < 10)
	{
	    log_file (LOG_ERROR, "try again\n");
	    goto again;
	}
	R_exit (NETMIST_OP_VFS_ERR + 8, err);
    }
    VFS (close, &fd);
    work_obj[my_workload].close_op_count++;
    total_file_ops += 1.0;	/* Close */
    work_obj[my_workload].total_file_ops += 1.0;	/* Close */

    return err;
}

/**
 * @brief Initialize the device specified, following the desired 
 *        pattern fill and layout.
 *
 * @param op : directory object for the block dev
 * @param file_ptr : Pointer to the file object for this block device.
 */
/*
 * __doc__
 * __doc__  Function : void generic_binit_file(int op, struct file_object *file_ptr)
 * __doc__  Arguments: int op:  Object handle for this block device.
 * __doc__  Arguments: struct file_objec *file_ptr. Pointer to the file object for this dev.
 * __doc__  Returns  : void
 * __doc__  Performs : Initialize the device specified, following the desired 
 * __doc__             pattern fill and layout.
 * __doc__
 */
void
generic_binit_file (int op, struct file_object *file_ptr)
{
    char *buf;
    long long xfer_count = 0;	/* In bytes */
    int fringe = 0;
    int locked_dir = 0;
    double pre_time = 0;
    double post_time = 0;
    int lat_delay = 0;
    int this_dedup_group;
    int dedup_across, my_delta_time;
#if defined(PDSM_RO_ACTIVE)
    int my_pdsm_time;
#endif
    int pct_complete = 0, prev_pct_complete = 0;
    int nap_time = 0;

    struct netmist_vfs_object *fd = NULL;
    int err;
    int j;
    long long i;
    uint64_t tlen;
    char block_lock_file[MAXNAME];
    char dir_buf[MAXNAME2];

#ifdef _LARGEFILE64_SOURCE
    /* 
     * Heavy magic happens with the size of the stat and statfs structures
     * about the time that 10.6 came out.
     */
#if defined(_macos_)
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  > 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  < 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  == 1050
    struct stat64 stbuf;
#endif
#else
    struct stat64 stbuf;
#endif
#else
#if defined(WIN32)
    struct _stati64 stbuf;
    time_t ltime;
#else
    struct stat stbuf;
#endif
#endif

    (void) op;

    memset ((void *) block_lock_file, 0, (size_t) MAXNAME);
    memset ((void *) dir_buf, 0, (size_t) MAXNAME2);

    log_file (LOG_DEBUG, "Entering binit file\n");

    i = snprintf (block_lock_file, sizeof (block_lock_file), "%s/%s",
		  VFS (pathptr, file_ptr->dir), file_ptr->relfilename);
    if (i < 0 || (size_t) i >= sizeof (block_lock_file))
    {
	log_file (LOG_ERROR, "Cannot create block lock file name\n");
	R_exit (PATHNAME_TOO_LONG, 0);
    }
    for (j = 0; j < i; j++)
    {
	if ((block_lock_file[j] == '/') || (block_lock_file[j] == '\\'))
	    block_lock_file[j] = '_';
    }

    if (cmdline.sharing_flag)
    {
	/* Create a lock file for the block device level 
	 * The lock file will live in the client's log directory and have 
	 * a name like /tmp/dev_sda1  
	 * The slashes are converted to underbars, and the child's log 
	 * directory name is prepended. 
	 * The existance of this file means that someone else is doing the init.
	 * If the file is missing, they this thread will try to do the init.
	 * If the file is there, but very old, then blow it away and try again.
	 * Cleanup is done by drain_list() at the end of a run.
	 *
	 * Note: This file is only creating a barrier for threads/procs on this
	 * client node.  It is not a global barrier. The idea is that we can 
	 * reduce the repetition of work by procs within a node, and not worry
	 * about procs on other nodes going after the same raw block device. 
	 * And if that happens, just let it.
	 *
	 * Emphatically, the path given here, while derived from paths given
	 * to the VFS layer, is not, itself, accessed through the VFS layer.
	 * In fact, it may not even *be* accessible through the VFS layer!
	 * As a result, this code looks a little more raw.
	 */
#if defined(WIN32)
	snprintf (dir_buf, sizeof dir_buf, "%s\\%s",
		  &cmdline.client_windows_log_dir[0], block_lock_file);
#else
	snprintf (dir_buf, sizeof dir_buf, "%s/%s",
		  &cmdline.client_log_dir[0], block_lock_file);
#endif
	log_file (LOG_DEBUG, "Entering binit file. Lock file %s\n", dir_buf);


      too_old:
#if defined(WIN32)
	err = GetFileAttributes (dir_buf);
	if (err != INVALID_FILE_ATTRIBUTES)
	{
	    err = I_STAT (dir_buf, &stbuf);
	    time (&ltime);
	    /* In case someone killed the test, or it died, there
	     * might be some very old dir_lock files around. We
	     * need to be sure that none exist for the first proc
	     * to work on the files in this directory. If we don't
	     * clean this up, then the files may not get created,
	     * or set back to their original sizes for the next
	     * load point. If dir_lock is older than 2 minutes, 
	     * lets unlink and try again. 
	     */
	    if (ltime - stbuf.st_ctime > 3600)
	    {
		log_file (LOG_EXEC, "Resetting dir_lock. Delta_t = %lld\n",
			  (long long) (ltime - stbuf.st_ctime));
		unlink (dir_buf);
		goto too_old;
	    }
	    locked_dir = 0;
	    goto jump_out;
	}			/* If found, then do nothing */
#else
	err = I_STAT (dir_buf, &stbuf);
	if (err == 0)
	{
	    /* In case someone killed the test, or it died, there
	     * might be some very old dir_lock files around. We
	     * need to be sure that none exist for the first proc
	     * to work on the files in this directory. If we don't
	     * clean this up, then the files may not get created,
	     * or set back to their original sizes for the next
	     * load point. If dir_lock is older than 2 minutes, 
	     * lets unlink and try again. 
	     */
	    if (time (NULL) - stbuf.st_ctime > 3600)
	    {
		log_file (LOG_EXEC, "Resetting dir_lock. Delta_t = %lld\n",
			  (long long) (time (NULL) - stbuf.st_ctime));
		unlink (dir_buf);
		goto too_old;
	    }
	    locked_dir = 0;
	    goto jump_out;
	}			/* If found, then do nothing */
#endif
	else
	{
#if defined(WIN32)
	    HANDLE h;
#endif
	    int ret;


#if defined(WIN32)
	    /* Create a file, to indicate that this dir is being worked */
	    h = CreateFile (dir_buf,
			    GENERIC_READ | GENERIC_WRITE,
			    FILE_SHARE_READ | FILE_SHARE_WRITE |
			    FILE_SHARE_DELETE, NULL, CREATE_NEW,
			    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS,
			    NULL);
	    if (h == INVALID_HANDLE_VALUE)
	    {
		ret = 1;
	    }
	    else
	    {
		ret = 0;
		CloseHandle (h);
	    }
#else
	    /* Create a file, to indicate that this dir is being worked */
	    ret = I_OPEN (dir_buf, O_CREAT | O_EXCL | O_RDWR, 0666);
	    if (ret >= 0)
	    {
		close (ret);
	    }
#endif
	    if (ret == 0)
	    {
		push_name (NULL, dir_buf);	/* save name for delayed unlink */
		locked_dir = 1;	/* Locked by me */
	    }
	    else
	    {
		locked_dir = 0;	/* Locked by some other */
		goto jump_out;
	    }
	}
    }
    if ((locked_dir == 0) && cmdline.sharing_flag)
	goto jump_out;

    /*--------------------------------------------------------------*/

    buf = main_buf;
    if (init_phase)		/* Just get the length set correctly */
    {
	/* If the file doesn't exist, then skip to the create region. */
    }
    /*
     *--------------------------------------------------------------
     * Case of the file doesn't exist... So we create and fill here.
     *--------------------------------------------------------------
     */

    log_file (LOG_DEBUG, "Active set size %lld \n", file_ptr->Original_fsize);

    err =
	VFS (open, file_ptr->dir, file_ptr->relfilename, NETMIST_OPEN_CREATE,
	     &fd);
    if (err != 0)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Open new file for write failed. Filename = %s in %s Error: %s Error value: %d\n",
		  file_ptr->relfilename, VFS (pathptr, file_ptr->dir),
		  nes, err);
	R_exit (NETMIST_OP_VFS_ERR + 6, err);
    }

    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    work_obj[my_workload].open_op_count += 1;
    xfer_count = 0;
    if ((file_ptr->Original_fsize * 1024) >=
	(unsigned long long) max_xfer_size)
    {
	for (i = 0;
	     (unsigned long long) (i + max_xfer_size) <
	     (file_ptr->Original_fsize * 1024);
	     i += (unsigned long long) (max_xfer_size))
	{
		/*-----------------*/
	    /* heartbeat       */
		/*-----------------*/
	    cur_time = gettime ();
	    my_delta_time = (int) (cur_time - last_heart_beat);

	    if (cmdline.heartbeat_flag && init_phase
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
#if defined(PDSM_RO_ACTIVE)
		if (pdsm_stats)
		{
		    pdsm_stats->epoch_time = time (NULL);
		    pdsm_stats->cur_state = (int) INIT_BEAT;
		    pdsm_stats->client_id = cmdline.client_id;
		    pdsm_stats->mode = pdsm_mode;
		    pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
		    pdsm_stats->achieved_op_rate = (double) 0.0;
		    pdsm_stats->run_time =
			(double) (last_heart_beat - my_start_time);
		    my_strncpy (pdsm_stats->client_name, client_localname,
				MAXHNAME);
		    my_strncpy (pdsm_stats->workload_name,
				work_obj[my_workload].work_obj_name,
				MAXWLNAME);
		}
		my_pdsm_time = (int) (cur_time - last_pdsm_beat);
		if (pdsm_file_fd && (my_pdsm_time >= pdsm_interval))
		{
		    if (pdsm_mode == 0)	/* Over-write mode */
		    {
#if defined(WIN32)
			largeoffset.QuadPart =
			    sizeof (struct pdsm_remote_stats) *
			    cmdline.client_id;
			SetFilePointerEx (pdsm_file_fd, largeoffset, NULL,
					  FILE_BEGIN);
#else
			I_LSEEK (pdsm_file_fd,
				 sizeof (struct pdsm_remote_stats) *
				 cmdline.client_id, SEEK_SET);
#endif
		    }
#if defined(WIN32)
		    err =
			WriteFile (pdsm_file_fd, pdsm_stats,
				   sizeof (struct pdsm_remote_stats), &err,
				   NULL);
#else
		    err =
			write (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats));
#endif
#if defined(FSYNC_PDSM)
		    if (my_pdsm_time >= (2 * pdsm_interval))
			fsync (pdsm_file_fd);
#endif
		    last_pdsm_beat = cur_time;
		}
#endif
	    }
	    pct_complete =
		(int) (((i +
			 max_xfer_size) * 100LL) / (file_ptr->Original_fsize *
						    1024LL));
	    if (pct_complete > 100)
		pct_complete = 100;
	    if (pct_complete && (pct_complete != prev_pct_complete)
		&& ((pct_complete % 10) == 0) && cmdline.heartbeat_flag)
	    {
		tell_prime_pct_complete (pct_complete, cmdline.real_client_id,
					 R_PERCENT_I_COMPLETE);
		prev_pct_complete = pct_complete;
	    }

	    if (file_ptr->dedup_group == 0)
		file_ptr->dedup_group = netmist_rand ();

	    if (my_mix_table[my_workload].dedup_group_count == 0)
		my_mix_table[my_workload].dedup_group_count = 1;

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_within)
		this_dedup_group = file_ptr->dedup_group;
	    else
	    {
		if (my_mix_table[my_workload].dedup_group_count == 1)
		    this_dedup_group = 1;
		else
		    this_dedup_group =
			(file_ptr->dedup_group %
			 (my_mix_table[my_workload].dedup_group_count));
	    }

	    if ((netmist_rand () % 100) <
		my_mix_table[my_workload].percent_dedup_across)
		dedup_across = 1;
	    else
		dedup_across = 0;
	    if (my_mix_table[my_workload].patt_vers == 1)
	    {
		make_non_dedup ((long *) buf, (int) max_xfer_size);
		if (my_mix_table[my_workload].cipher)
		    make_cypher ((char *) buf, (int) max_xfer_size);
	    }
	    else
	    {
		make_data_layout ((long *) buf, max_xfer_size, file_ptr,
				  (long long) xfer_count, this_dedup_group,
				  dedup_across);
		if (my_mix_table[my_workload].cipher)
		    make_cypher ((char *) buf, (int) max_xfer_size);
	    }
	    xfer_count += max_xfer_size;

	    log_file (LOG_DEBUG,
		      "Writing new chunk, non-fringe. Size %d bytes. File size %lld bytes\n",
		      max_xfer_size, file_ptr->Original_fsize * 1024);

	    if (init_phase)
		pre_time = gettime ();
	    cur_time = gettime ();
	    my_delta_time = (int) (cur_time - last_heart_beat);

	    if (cmdline.heartbeat_flag && init_phase
		&& (my_delta_time >= HEARTBEAT_TICK))
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
		last_heart_beat = cur_time;
	    }

	    err = VFS (write, fd, buf, max_xfer_size, &tlen);
	    if (err)
	    {
		const char *nes = netmist_vfs_errstr (err);
		log_file (LOG_ERROR,
			  "Write file failed (3) to file %s in %s Error: %s Error value %d Ret %"
			  PRIu64 "\n", file_ptr->relfilename, VFS (pathptr,
								   file_ptr->dir),
			  nes, err, tlen);
		VFS (close, &fd);
		R_exit (NETMIST_OP_VFS_ERR + 7, err);
	    }

	    if (init_phase)
	    {
		post_time = gettime ();
		if (my_mix_table[my_workload].init_rate_enable == 1)
		{
		    /*
		     * Throttle back on I/O rate, if needed. Don't swamp 
		     * the filer
		     */
		    global_reset_counter++;
		    /* Every 1000 I/O ops, lets restart the average        */
		    /* calculation. We need to use a large sample because */
		    /* the nap() resolution is in milliseconds.    */
		    if (global_reset_counter == 1000)
		    {
			global_reset_counter = 0;
			total_init_time = 0.0;
			total_init_kbytes = 0;
			avg_init_mb_per_sec = 0.0;
		    }
		    total_init_time += (post_time - pre_time);
		    total_init_kbytes += (max_xfer_size / 1024);
		    avg_init_mb_per_sec =
			(total_init_kbytes / 1024) / total_init_time;

		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) >
			    my_mix_table[my_workload].init_rate_speed))
		    {
			if (nap_time == 0)
			    nap_time += 1;
			nap (nap_time);
			total_init_time += (float) nap_time / (float) 1000;
		    }
		    avg_init_mb_per_sec =
			(total_init_kbytes / 1024) / total_init_time;
		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) >
			    my_mix_table[my_workload].init_rate_speed))
			nap_time = nap_time * 2;
		    if ((my_mix_table[my_workload].init_rate_speed != 0.0)
			&& ((avg_init_mb_per_sec) <
			    my_mix_table[my_workload].init_rate_speed)
			&& (nap_time != 0))
			nap_time = nap_time / 2;
		}
		/*
		 * Emergency throttle back on I/O rate, if needed. Don't 
		 * swamp the filer
		 */
		if ((post_time - pre_time) > MAX_LATENCY)
		{
		    log_file (LOG_DEBUG, "Slowing init\n");

		    lat_delay += 1000;
		    if (lat_delay >= (MAX_SLOW_LATENCY * 1000))
			lat_delay = (MAX_SLOW_LATENCY * 1000);
		}
		if (((post_time - pre_time) < MAX_LATENCY) && (lat_delay > 0))
		{
		    log_file (LOG_DEBUG, "Accerating init\n");

		    lat_delay = lat_delay / 2;
		}
		if (lat_delay < 0)
		    lat_delay = 0;
		if (lat_delay > 0)
		    nap (lat_delay);
	    }
	    total_file_ops += 1;
	    work_obj[my_workload].total_file_ops += 1;
	    work_obj[my_workload].write_op_count += 1;
	    /*
	     * Help keep the client's page cache from getting too dirty.
	     */
	    if ((work_obj[my_workload].write_op_count % 10 == 0)
		&& cmdline.flush_flag)
		VFS (fsync, fd);
	}
    }

    if (file_ptr->dedup_group == 0)
	file_ptr->dedup_group = netmist_rand ();

    if (my_mix_table[my_workload].dedup_group_count == 0)
	my_mix_table[my_workload].dedup_group_count = 1;

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_within)
	this_dedup_group = file_ptr->dedup_group;
    else
    {
	if (my_mix_table[my_workload].dedup_group_count == 1)
	    this_dedup_group = 1;
	else
	    this_dedup_group =
		(file_ptr->dedup_group %
		 (my_mix_table[my_workload].dedup_group_count));
    }

    if ((netmist_rand () % 100) <
	my_mix_table[my_workload].percent_dedup_across)
	dedup_across = 1;
    else
	dedup_across = 0;
    /* Fix up the fringe at the end */
    if ((unsigned long long) xfer_count < file_ptr->Original_fsize * 1024)
    {
	fringe = file_ptr->Original_fsize * 1024 - xfer_count;
	if (my_mix_table[my_workload].patt_vers == 1)
	{
	    make_non_dedup ((long *) buf, (int) fringe);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) fringe);
	}
	else
	{
	    make_data_layout ((long *) buf, fringe, file_ptr,
			      (long long) ((file_ptr->Original_fsize * 1024LL)
					   - fringe), this_dedup_group,
			      dedup_across);
	    if (my_mix_table[my_workload].cipher)
		make_cypher ((char *) buf, (int) fringe);
	}

	log_file (LOG_DEBUG,
		  "Writing fringe of %d bytes. xfercount %lld\n", fringe,
		  xfer_count);
	log_file (LOG_DEBUG, "Original cmdline.fsize %lld\n",
		  file_ptr->Original_fsize * 1024);

	cur_time = gettime ();
	my_delta_time = (int) (cur_time - last_heart_beat);

	if (cmdline.heartbeat_flag && init_phase
	    && (my_delta_time >= HEARTBEAT_TICK))
	{
	    tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				  (double) 0.0);
	    last_heart_beat = cur_time;
	}

	err = VFS (write, fd, buf, fringe, &tlen);
	if (err)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Write file failed (4) to file %s in %s Error: %s Error value %d Ret %"
		      PRIu64 "\n", file_ptr->relfilename, VFS (pathptr,
							       file_ptr->dir),
		      nes, err, tlen);
	    VFS (close, &fd);
	    R_exit (NETMIST_OP_VFS_ERR + 7, err);
	}
    }
    if (cmdline.lflag && cmdline.flush_flag)
    {
	VFS (fsync, fd);	/* Not needed as it is not measured */
    }

    if (pct_complete != 100)
	pct_complete = 100;
    if (pct_complete && (pct_complete != prev_pct_complete)
	&& ((pct_complete % 10) == 0) && cmdline.heartbeat_flag)
    {
	tell_prime_pct_complete (pct_complete, cmdline.real_client_id,
				 R_PERCENT_I_COMPLETE);
	prev_pct_complete = pct_complete;
    }
    VFS (close, &fd);		/* close counted below */
    total_file_ops += 1;
    work_obj[my_workload].total_file_ops += 1;
    work_obj[my_workload].close_op_count += 1;

    if (cmdline.sharing_flag)
    {
	drain_list ();
    }
  jump_out:
    return;
}

/**
 * @brief Generic function to check to see if the directory exists,
 *        and is usable as a work directory for testing.
 *
 * @param dir : Pointer to the directory object
 */
/*
 * __doc__
 * __doc__  Function : int generic_stat_workdir(struct netmist_vfs_dir *dir)
 * __doc__  Arguments: struct netmist_vfs_dir *dir
 * __doc__  Returns  : success or failure.
 * __doc__  Performs : Generic function to check to see if the directory exists,
 * __doc__             and is usable as a work directory for testing.
 * __doc__
 */

int
generic_stat_workdir (struct netmist_vfs_dir *dir)
{
    int ret;
    struct netmist_vfs_stat stbuf;
    const char *dirname = VFS (pathptr, dir);

    log_file (LOG_DEBUG, "Doing stat_workdir op. %s\n", dirname);

    ret = VFS (stat, dir, NULL, &stbuf);
    if (ret != 0)
    {
	const char *nes = netmist_vfs_errstr (ret);
	if (!quiet_now)
	    log_file (LOG_ERROR,
		      "Cannot get stats for dirname > %s < Error: %s Error Value: %d\n",
		      dirname, nes, ret);
    }
    return (ret);
}

/**
 * @brief Generic_remove_file() Virtualized interface so that 
 *        this functionality can be used on workload dependent 
 *        filesystems that might have a different type of unlink.
 *
 * @param d : Pointer to the directory object.
 * @param rfn : Relative filename, relative to this directory object.
 */
/* 
 * __doc__
 * __doc__  Function : int generic_remove_file(char *filename)
 * __doc__  Arguments: char *file name
 * __doc__  Returns  : success or failure
 * __doc__  Performs : Generic_remove_file() Virtualized interface so that 
 * __doc__             this functionality can be used on workload dependent 
 * __doc__             filesystems that might have a different type of unlink.
 * __doc__             
 */
int
generic_remove_file (struct netmist_vfs_dir *d, char *rfn)
{
    log_file (LOG_DEBUG, "Doing generic_remove_file\n");

    return VFS (unlink, d, rfn);
}

/**
 * @brief Generic_remove_dir() Virtualized interface so that 
 *        this functionality can be used on workload dependent 
 *        filesystems that might have a different type of rmdir.
 *
 * @param d : Pointer to the directory object.
 */
/* 
 * __doc__
 * __doc__  Function : int generic_remove_dir(char *dirname)
 * __doc__  Arguments: char *directory name.
 * __doc__  Returns  : int success or failure.
 * __doc__  Performs : Generic_remove_dir() Virtualized interface so that 
 * __doc__             this functionality can be used on workload dependent 
 * __doc__             filesystems that might have a different type of rmdir.
 * __doc__             
 */
int
generic_remove_dir (struct netmist_vfs_dir *d)
{
    int ret;

    log_file (LOG_DEBUG, "Doing generic_remove_dir\n");

    ret = VFS (rmdir, d);
    if (ret != 0)
    {
	log_file (LOG_ERROR, "generic_remove_dir failed: %d Name %s\n",
		  ret, VFS (pathptr, d));

    }
    return (ret);
}

/**
 * @brief Common function to perform op validation, and 
 *        initialize files.
 */
/*
 * __doc__
 * __doc__  Function : void generic_setup(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Common function to perform op validation, and 
 * __doc__             initialize files.
 * __doc__            
 */
void
generic_setup (void)
{
    client_validate_ops ();
    if ((cmdline.skip_init == 1) && (dir_exist () == 0))
    {
	return;
    }
    if (cmdline.sharing_flag)
    {
	client_init_files (0);
    }
    else
    {
	client_init_files (cmdline.client_id);
    }
}

static void
client_init_block_fd (struct file_object *fp)
{
    fp->dir = netmist_vfs_root ();
    /* Block-devices don't use relative paths; they just use the dir handle */
    fp->relfilename = NULL;

    fp->Original_fsize =
	((cmdline.fsize + ((long long) rsize - 1)) & ~((long long) rsize -
						       1));
    fp->Current_fsize =
	((cmdline.fsize + ((long long) rsize - 1)) & ~((long long) rsize -
						       1));

}

/**
 * @brief Function that initializes block devices.
 *
 * @param workdir : device name
 */
/*
 * __doc__
 * __doc__  Function : void client_init_block(char *workdir)
 * __doc__  Arguments: char * device name
 * __doc__  Returns  : void
 * __doc__  Performs : Function that initializes block devices.
 * __doc__             
 */
void
client_init_block (char *workdir)	/* Initialize all blocks of cmdline.workdir/dev */
{
    struct file_object *file_array_ptr;
#if defined(WIN32)
    DWORD ret = 0;
    BOOL err = 0;
#else
    int ret;
#endif

#if defined(PDSM_RO_ACTIVE)
    if (pdsm_stats)
    {
	pdsm_stats->cur_state = (int) INIT_BEAT;
	pdsm_stats->epoch_time = time (NULL);
	pdsm_stats->client_id = cmdline.client_id;
	pdsm_stats->mode = pdsm_mode;
	pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
	pdsm_stats->achieved_op_rate = (double) 0.0;
	pdsm_stats->run_time = (double) 0.0;
	my_strncpy (pdsm_stats->client_name, client_localname, MAXHNAME);
	my_strncpy (pdsm_stats->workload_name,
		    work_obj[my_workload].work_obj_name, MAXWLNAME);
    }
    if (pdsm_file_fd)
    {
	if (pdsm_mode == 0)	/* Over-write mode */
	{
#if defined(WIN32)
	    largeoffset.QuadPart =
		sizeof (struct pdsm_remote_stats) * cmdline.client_id;
	    SetFilePointerEx (pdsm_file_fd, largeoffset, NULL, FILE_BEGIN);
#else
	    I_LSEEK (pdsm_file_fd,
		     sizeof (struct pdsm_remote_stats) * cmdline.client_id,
		     SEEK_SET);
#endif
	}
#if defined(WIN32)
	err =
	    WriteFile (pdsm_file_fd, pdsm_stats,
		       sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
	ret =
	    write (pdsm_file_fd, pdsm_stats,
		   sizeof (struct pdsm_remote_stats));
#endif
	if (ret >= 0)
	    log_file (LOG_DEBUG, "writing pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
	fsync (pdsm_file_fd);
#endif
    }
#endif
    log_file (LOG_EXEC, "Client starting block init \n");

    log_file (LOG_EXEC, "Workload = %14s Location = %s\n",
	      my_mix_table[my_workload].workload_name, workdir);


    /* 
     * Initialize the op type associated file sizes 
     * BLOCK only uses 4 OP types. Write, Write_rand, Read, and
     * Read rand. 
     */
    file_array_ptr = op_X_array[OP_READ_RAND];	/* op type */
    client_init_block_fd (&file_array_ptr[0]);

    file_array_ptr = op_X_array[OP_WRITE_RAND];	/* op type */
    client_init_block_fd (&file_array_ptr[0]);

    file_array_ptr = op_X_array[OP_READ];	/* op type */
    client_init_block_fd (&file_array_ptr[0]);

    file_array_ptr = op_X_array[OP_WRITE];	/* op type */
    client_init_block_fd (&file_array_ptr[0]);

    init_phase = 1;

    (*Netmist_Ops[my_workload].init_file) (OP_WRITE,
					   &op_X_array[OP_WRITE][0]);
    log_file (LOG_EXEC, "Client returning block init \n");

    init_phase = 0;
}

/**
 * @brief Function to initialize the specified block device.
 */
/* 
 * __doc__
 * __doc__  Function : void generic_bsetup(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Function to initialize the specified block device.
 * __doc__
 */
void
generic_bsetup (void)
{
    client_init_block (cmdline.workdir);	/* Initialize all blocks of cmdline.workdir/dev */
}

/**
 * @brief Function that cleans up at the end of a test.
 */
/*
 * __doc__
 * __doc__  Function : void generic_teardown(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Function that cleans up at the end of a test.
 * __doc__
 */
void
generic_teardown (void)
{
    if (cmdline.sharing_flag)
    {
	if (cmdline.cleanup_flag)
	    client_remove_files (0);
    }
    else
    {
	if (cmdline.cleanup_flag)
	    client_remove_files (cmdline.client_id);
    }
}

/**
 * @brief Function that cleans up the block devices at the end 
 *        of a test.
 */
/*
 * __doc__
 * __doc__  Function : void generic_bteardown(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Function that cleans up the block devices at the end 
 * __doc__             of a test.
 * __doc__
 */
void
generic_bteardown (void)
{
}

int init_sizes;
long long size_table[136];

/**
 * @brief This creates a bell curve of read sizes. The center of 
 *        the curve is the natural read size. The read size steps 
 *        are:
 *        (in percentage of the natural file size)
 *        10%, 20%, 40%, 60%, 80%, 100%, 100%, 120%, 140%, 
 *        160%, 180%, 190%
 *
 */
/*
 * __doc__
 * __doc__  Function : uint64_t  get_next_read_size( void )
 * __doc__  Arguments: void
 * __doc__  Returns  : next read size.
 * __doc__  Performs : This creates a bell curve of file sizes. The center of 
 * __doc__             the curve is the natural read size. The read size steps 
 * __doc__             are:
 * __doc__             (in percentage of the natural file size)
 * __doc__             10%, 20%, 40%, 60%, 80%, 100%, 100%, 120%, 140%, 
 * __doc__             160%, 180%, 190%
 * __doc__
 * __doc__  Curve approximation has steps: 1,3,7,14,20,23,23,20,14,7,3,1
 * __doc__  There is   1/136 slots has a read size that is  10% of natural size.
 * __doc__  There are  3/136 slots has a read size that is  20% of natural size.
 * __doc__  There are  7/136 slots has a read size that is  40% of natural size.
 * __doc__  There are 14/136 slots has a read size that is  60% of natural size.
 * __doc__  There are 20/136 slots has a read size that is  80% of natural size.
 * __doc__  There are 46/136 slots has a read size that is 100% of natural size.
 * __doc__  There are 20/136 slots has a read size that is 120% of natural size.
 * __doc__  There are 14/136 slots has a read size that is 140% of natural size.
 * __doc__  There are  7/136 slots has a read size that is 160% of natural size.
 * __doc__  There are  3/136 slots has a read size that is 180% of natural size.
 * __doc__  There are  1/136 slots has a read size that is 190% of natural size.
 * __doc__             
 */

uint64_t
get_next_read_size (void)
{
    uint64_t minv, maxv, val, nval, ret_val;
    int slot, m;

    m = netmist_rand () % RW_ARRAY;	/* pick percent slot */
    slot = read_dist_array[m];	/* Extract read_size slot number */
    minv = my_mix_table[my_workload].read_dist[slot].size_min;
    maxv = my_mix_table[my_workload].read_dist[slot].size_max;
    val = (maxv - minv + 1);
    nval = ((uint64_t) netmist_rand () % val) + minv;
    ret_val = nval;		/* Random value within the range for this slot */
    return (ret_val);
}

/**
 * @brief This creates a bell curve of write sizes. The center of 
 *        the curve is the natural write size. The write size steps 
 *        are:
 *        (in percentage of the natural file size)
 *        10%, 20%, 40%, 60%, 80%, 100%, 100%, 120%, 140%, 
 *        160%, 180%, 190%
 *
 */
uint64_t
get_next_write_size (void)
{
    uint64_t minv, maxv, val, nval, ret_val;
    int slot, m;

    m = netmist_rand () % RW_ARRAY;	/* pick percent slot */
    slot = write_dist_array[m];	/* Extract write_size slot number */
    minv = my_mix_table[my_workload].write_dist[slot].size_min;
    maxv = my_mix_table[my_workload].write_dist[slot].size_max;
    val = (maxv - minv + 1);
    nval = ((unsigned int) netmist_rand () % val) + minv;
    ret_val = nval;		/* Random value within the range for this slot */
    return (ret_val);
}

/**
 * @brief This creates a bell curve of file sizes. The center of 
 *        the curve is the natural file size. The file size steps 
 *        are:
 *        (in percentage of the natural file size)
 *        10%, 20%, 40%, 60%, 80%, 100%, 100%, 120%, 140%, 
 *        160%, 180%, 190%
 *
 * @param size : Size of the next file 
 */
unsigned long long
get_next_fsize (long long size)
{
    int j, k, m, slot;
    unsigned long long minv, maxv, nval, val;
    unsigned long long ret_val;

    log_file (LOG_DEBUG, "next size input %lld\n", size);

    if (init_sizes == 0)
    {
	k = 0;
	size_table[k++] = (size * 10LL) / 100.0;
	for (j = 0; j < 3; j++)
	    size_table[k++] = (size * 20LL) / 100LL;
	for (j = 0; j < 7; j++)
	    size_table[k++] = (size * 40LL) / 100LL;
	for (j = 0; j < 14; j++)
	    size_table[k++] = (size * 60LL) / 100LL;
	for (j = 0; j < 20; j++)
	    size_table[k++] = (size * 80LL) / 100LL;
	for (j = 0; j < 46; j++)
	    size_table[k++] = (size * 100LL) / 100LL;
	for (j = 0; j < 20; j++)
	    size_table[k++] = (size * 120LL) / 100LL;
	for (j = 0; j < 14; j++)
	    size_table[k++] = (size * 140LL) / 100LL;
	for (j = 0; j < 7; j++)
	    size_table[k++] = (size * 160LL) / 100LL;
	for (j = 0; j < 3; j++)
	    size_table[k++] = (size * 180LL) / 100LL;
	for (j = 0; j < 1; j++)
	    size_table[k++] = (size * 190LL) / 100LL;
	init_sizes = 1;
    }
    if (my_mix_table[my_workload].use_file_size_dist)
    {
	m = netmist_rand () % RW_ARRAY;	/* pick percent slot */
	slot = file_size_dist_array[m];	/* Extract file_size slot number */
	minv = my_mix_table[my_workload].file_size_dist[slot].size_min;
	maxv = my_mix_table[my_workload].file_size_dist[slot].size_max;
	val = (maxv - minv + 1);
	nval = ((unsigned long long) netmist_rand () % val) + minv;
	ret_val = nval;		/* Random value within the range for this slot */
    }
    else
    {
	ret_val = size_table[((name_rand ()) % 136)];
    }
    if (ret_val == 0)		/* Set minimum size */
	ret_val = rsize * 1024;
    ret_val =
	(ret_val +
	 ((long long) (rsize * 1024) - 1)) & ~((long long) (rsize * 1024) -
					       1);
    log_file (LOG_DEBUG, "next size %lld\n", ret_val);

    return (ret_val);
}

/**
 * @brief Dump the child histograms.
 *
 * @param out : Pointer for writing the file.
 */
/*
 * __doc__
 * __doc__  Function : void child_dump_hist(FILE *out)
 * __doc__  Arguments: FILE *: Pointer for writing the file.
 * __doc__  Returns  : void
 * __doc__  Performs : Dump the child histograms.
 * __doc__             
 */
void
child_dump_hist (FILE * out)
{

#ifndef NO_PRINT_LLD
    fprintf (out, "\tBand 1: ");
    fprintf (out, " 20us:%-7.1lld ", buckets[0]);
    fprintf (out, " 40us:%-7.1lld ", buckets[1]);
    fprintf (out, " 60us:%-7.1lld ", buckets[2]);
    fprintf (out, " 80us:%-7.1lld ", buckets[3]);
    fprintf (out, "100us:%-7.1lld \n", buckets[4]);

    fprintf (out, "\tBand 2: ");
    fprintf (out, "200us:%-7.1lld ", buckets[5]);
    fprintf (out, "400us:%-7.1lld ", buckets[6]);
    fprintf (out, "600us:%-7.1lld ", buckets[7]);
    fprintf (out, "800us:%-7.1lld ", buckets[8]);
    fprintf (out, "  1ms:%-7.1lld \n", buckets[9]);

    fprintf (out, "\tBand 3: ");
    fprintf (out, "  2ms:%-7.1lld ", buckets[10]);
    fprintf (out, "  4ms:%-7.1lld ", buckets[11]);
    fprintf (out, "  6ms:%-7.1lld ", buckets[12]);
    fprintf (out, "  8ms:%-7.1lld ", buckets[13]);
    fprintf (out, " 10ms:%-7.1lld \n", buckets[14]);

    fprintf (out, "\tBand 4: ");
    fprintf (out, " 12ms:%-7.1lld ", buckets[15]);
    fprintf (out, " 14ms:%-7.1lld ", buckets[16]);
    fprintf (out, " 16ms:%-7.1lld ", buckets[17]);
    fprintf (out, " 18ms:%-7.1lld ", buckets[18]);
    fprintf (out, " 20ms:%-7.1lld \n", buckets[19]);

    fprintf (out, "\tBand 5: ");
    fprintf (out, " 40ms:%-7.1lld ", buckets[20]);
    fprintf (out, " 60ms:%-7.1lld ", buckets[21]);
    fprintf (out, " 80ms:%-7.1lld ", buckets[22]);
    fprintf (out, "100ms:%-7.1lld \n", buckets[23]);

    fprintf (out, "\tBand 6: ");
    fprintf (out, "200ms:%-7.1lld ", buckets[24]);
    fprintf (out, "400ms:%-7.1lld ", buckets[25]);
    fprintf (out, "600ms:%-7.1lld ", buckets[26]);
    fprintf (out, "800ms:%-7.1lld ", buckets[27]);
    fprintf (out, "   1s:%-7.1lld \n", buckets[28]);

    fprintf (out, "\tBand 7: ");
    fprintf (out, "   2s:%-7.1lld ", buckets[29]);
    fprintf (out, "   4s:%-7.1lld ", buckets[30]);
    fprintf (out, "   6s:%-7.1lld ", buckets[31]);
    fprintf (out, "   8s:%-7.1lld ", buckets[32]);
    fprintf (out, "  10s:%-7.1lld \n", buckets[33]);

    fprintf (out, "\tBand 8: ");
    fprintf (out, "  20s:%-7.1lld ", buckets[34]);
    fprintf (out, "  40s:%-7.1lld ", buckets[35]);
    fprintf (out, "  60s:%-7.1lld ", buckets[36]);
    fprintf (out, "  80s:%-7.1lld ", buckets[37]);
    fprintf (out, " 120s:%-7.1lld \n", buckets[38]);

    fprintf (out, "\tBand 9: ");
    fprintf (out, "120+s:%-7.1lld \n\n", buckets[39]);
#else
    fprintf (out, "\tBand 1: ");
    fprintf (out, " 20us:%-7.1ld ", buckets[0]);
    fprintf (out, " 40us:%-7.1ld ", buckets[1]);
    fprintf (out, " 60us:%-7.1ld ", buckets[2]);
    fprintf (out, " 80us:%-7.1ld ", buckets[3]);
    fprintf (out, "100us:%-7.1ld \n", buckets[4]);

    fprintf (out, "\tBand 2: ");
    fprintf (out, "200us:%-7.1ld ", buckets[5]);
    fprintf (out, "400us:%-7.1ld ", buckets[6]);
    fprintf (out, "600us:%-7.1ld ", buckets[7]);
    fprintf (out, "800us:%-7.1ld ", buckets[8]);
    fprintf (out, "  1ms:%-7.1ld \n", buckets[9]);

    fprintf (out, "\tBand 3: ");
    fprintf (out, "  2ms:%-7.1ld ", buckets[10]);
    fprintf (out, "  4ms:%-7.1ld ", buckets[11]);
    fprintf (out, "  6ms:%-7.1ld ", buckets[12]);
    fprintf (out, "  8ms:%-7.1ld ", buckets[13]);
    fprintf (out, " 10ms:%-7.1ld \n", buckets[14]);

    fprintf (out, "\tBand 4: ");
    fprintf (out, " 12ms:%-7.1ld ", buckets[15]);
    fprintf (out, " 14ms:%-7.1ld ", buckets[16]);
    fprintf (out, " 16ms:%-7.1ld ", buckets[17]);
    fprintf (out, " 18ms:%-7.1ld ", buckets[18]);
    fprintf (out, " 20ms:%-7.1ld \n", buckets[19]);

    fprintf (out, "\tBand 5: ");
    fprintf (out, " 40ms:%-7.1ld ", buckets[20]);
    fprintf (out, " 60ms:%-7.1ld ", buckets[21]);
    fprintf (out, " 80ms:%-7.1ld ", buckets[22]);
    fprintf (out, "100ms:%-7.1ld \n", buckets[23]);

    fprintf (out, "\tBand 6: ");
    fprintf (out, "200ms:%-7.1ld ", buckets[24]);
    fprintf (out, "400ms:%-7.1ld ", buckets[25]);
    fprintf (out, "600ms:%-7.1ld ", buckets[26]);
    fprintf (out, "800ms:%-7.1ld ", buckets[27]);
    fprintf (out, "   1s:%-7.1ld \n", buckets[28]);

    fprintf (out, "\tBand 7: ");
    fprintf (out, "   2s:%-7.1ld ", buckets[29]);
    fprintf (out, "   4s:%-7.1ld ", buckets[30]);
    fprintf (out, "   6s:%-7.1ld ", buckets[31]);
    fprintf (out, "   8s:%-7.1ld ", buckets[32]);
    fprintf (out, "  10s:%-7.1ld \n", buckets[33]);

    fprintf (out, "\tBand 8: ");
    fprintf (out, "  20s:%-7.1ld ", buckets[34]);
    fprintf (out, "  40s:%-7.1ld ", buckets[35]);
    fprintf (out, "  60s:%-7.1ld ", buckets[36]);
    fprintf (out, "  80s:%-7.1ld ", buckets[37]);
    fprintf (out, " 120s:%-7.1ld \n", buckets[38]);

    fprintf (out, "\tBand 9: ");
    fprintf (out, "120+s:%-7.1ld \n", buckets[39]);
    fflush (out);
#endif
}

/*
 * This is a list of file names for the "locking" directory
 * files, used in the parallelized initialization path for 
 * ShareMode.
 */
struct name_list
{
    struct netmist_vfs_dir *vfsd;
    char name[MAXNAME2];
    struct name_list *next;
};

struct name_list *gn_head;

/**
 * @brief Save the names of the directory locking files to a list. 
 *        This list will be processed later, to unlink all of the 
 *        directory locking files.
 *
 * @param vfsd : Pointer to the directory object that contains this file.
 * @param name : name to push
 *
 * If vfsd is NULL, name is interpreted locally; otherwise, name should
 * be the name of a VFS object within the directory vfsd (with no attempt
 * for additional directory traversals!).
 */
/* 
 * __doc__
 * __doc__  Function : void push_name(struct netmist_vfs_dir *dir, char *name)
 * __doc__  Arguments: struct netmist_vfs_dir *dir  (pointer to the directory object)
 * __doc__  Arguments: char *name to push
 * __doc__  Returns  : void
 * __doc__  Performs : Save the names of the directory locking files to a list. 
 * __doc__             This list will be processed later, to unlink all of the 
 * __doc__             directory locking files.
 * __doc__             
 *
 */
void
push_name (struct netmist_vfs_dir *vfsd, char *name)
{
    struct name_list *buffer;

#if defined(_P1_DEBUG_)
    log_file (LOG_DEBUG, "Adding %s to list \n", name);

#endif
    buffer = (struct name_list *) my_malloc (sizeof (struct name_list));
    if (buffer == 0)
	exit (99);		/* Can't get here ever. malloc checks for null. 
				   only need this to get static code analysis to stop
				   falsely complaining about NULL dereference */
    buffer->next = 0;

    buffer->vfsd = NULL;
    if (vfsd != NULL)
    {
	VFS (walk, vfsd, "", &buffer->vfsd);
	if (buffer->vfsd == NULL)
	{
	    /* This should not happen, since our VFS layer prefers
	     * to refcount directory structs rather than create new
	     * ones when not walking to a relative directory, as
	     * above.  It's possible some alternate plugin might,
	     * in which case, well, OOM it is.
	     */
	    free (buffer);
	    R_exit (NETMIST_OOM, 0);
	}
    }

    my_strncpy (buffer->name, name, sizeof (buffer->name));

    if (gn_head)
    {
	buffer->next = gn_head;
	gn_head = buffer;
    }
    else
    {
	gn_head = buffer;
    }
}

/**
 * @brief This will unlink the list of directory locking files 
 *        and free the memory that was used to keep track of these.
 */
/*
 * __doc__
 * __doc__  Function : void drain_list(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : This will unlink the list of directory locking files 
 * __doc__             and free the memory that was used to keep track of these.
 * __doc__
 */
void
drain_list (void)
{
    struct name_list *buffer, *old;
    buffer = gn_head;
    if (buffer == 0)
    {
	return;			/* nothing to do */
    }
    while (buffer)
    {
#if defined(_P1_DEBUG_)
	log_file (LOG_DEBUG, "Unlinking %s from list \n", buffer->name);

#endif
	if (buffer->vfsd == NULL)
	{
	    unlink (buffer->name);
	}
	else
	{
	    VFS (unlink, buffer->vfsd, buffer->name);
	    VFS (dfree, &(buffer->vfsd));
	}
	old = buffer;
	buffer = buffer->next;
	gn_head = buffer;
	free (old);
    }
}

/**
 * @brief On Unix boxes, one can delete files, and their parent
 *        directoires, even if there is an open file descriptor to
 *        one of those files. BUT !!! on Windows, the rmdir will
 *        FAIL if there are any open file descriptors, to files 
 *        that are open, and unlinked.  So.. we MUST close ALL 
 *        open file descriptors before we remove the parent 
 *        directories.
 */
/*
 * __doc__
 * __doc__  Function : void close_cached_file_desc(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : On Unix boxes, one can delete files, and their parent
 * __doc__             directoires, even if there is an open file descriptor to
 * __doc__             one of those files. BUT !!! on Windows, the rmdir will
 * __doc__             FAIL if there are any open file descriptors, to files 
 * __doc__             that are open, and unlinked.  So.. we MUST close ALL 
 * __doc__             open file descriptors before we remove the parent 
 * __doc__             directories.
 * __doc__             
 */
void
close_cached_file_desc (void)
{
    int i, j, k;
    int close_count = 0;
    struct file_object *array, *fp;
    /*size = sizeof(struct file_object )* cmdline.client_dirs * cmdline.client_files; */
    for (i = 0; i < NUM_OP_TYPES; i++)
    {
	array = (struct file_object *) op_X_array[i];
	for (j = 0; j < cmdline.client_dirs; j++)
	{
	    for (k = 0; k < cmdline.client_files; k++)
	    {
		if (op_X_array[i])
		{
		    fp = (struct file_object *)
			&array[(j * cmdline.client_files) + k];
		    if (fp && (fp->file_desc != 0))
		    {
			close_count++;
			VFS (close, &fp->file_desc);
		    }
		}
	    }
	}
    }

    log_file (LOG_DEBUG, "Closed %d cached file descriptors\n", close_count);

}

/**
 * @brief Used by Windows platforms to impersonate another user 
 *        account. Needed to run within an active directory 
 *        domain.  Since Vista, this new security (&*&%^ has
 *        been a HUGE pain in the butt. !!!! 
 *
 * @param context : Pointer to a context region. (WIN32)
 */
/*
 * __doc__
 * __doc__  Function : int generic_impersonate(void* context)
 * __doc__  Arguments: pointer to a context region.
 * __doc__  Returns  : int: Success or failure
 * __doc__  Performs : Used by Windows platforms to impersonate another user 
 * __doc__             account. Needed to run within an active directory 
 * __doc__             domain.  Since Vista, this new security (&*&%^ has
 * __doc__             been a HUGE pain in the butt. !!!! 
 * __doc__             
 */
int
generic_impersonate (void *context)
{
    int result = 0;
    void *dummy;

    dummy = context;
    if (dummy)
    {
	;
    }
    if (cmdline.localflag)
	return result;

#if defined(WIN32)
    if (ERROR_SUCCESS !=
	(result = LogonAndImpersonateUser (wmiUser, wmiPassword, wmiDomain)))
	log_file (LOG_ERROR, "Failed to impersonate as WMI user >%s\\%s<\n",
		  wmiDomain, wmiUser);
#endif
    return result;
}

/**
 * @brief Function for not implemented.
 */
/*
 * __doc__
 * __doc__  Function : void* generic_make_impersonate_context(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Function for not implemented.
*/
void *
generic_make_impersonate_context (void)
{

    return (void *) NULL;
}

/**
 * @brief The prime dumping its histograms from all of the 
 *        children.
 *
 * @param here : Result object for results.
 * @param out : Output file 
 */
/*
 * __doc__
 * __doc__  Function : void prime_dump_hist(struct result_object *here, 
 * __doc__                 FILE *out)
 * __doc__  Arguments: Result object for results.
 * __doc__             FILE *: Output file 
 * __doc__  Returns  : void
 * __doc__  Performs : The prime dumping its histograms from all of the 
 * __doc__             children.
 * __doc__
 */
void
prime_dump_hist (struct result_object *here, FILE * out)
{

#ifndef NO_PRINT_LLD
    fprintf (out, "\tBand 1: ");
    fprintf (out, " 20us:%-7.1lld ", here->bands[0]);
    fprintf (out, " 40us:%-7.1lld ", here->bands[1]);
    fprintf (out, " 60us:%-7.1lld ", here->bands[2]);
    fprintf (out, " 80us:%-7.1lld ", here->bands[3]);
    fprintf (out, "100us:%-7.1lld \n", here->bands[4]);

    fprintf (out, "\tBand 2: ");
    fprintf (out, "200us:%-7.1lld ", here->bands[5]);
    fprintf (out, "400us:%-7.1lld ", here->bands[6]);
    fprintf (out, "600us:%-7.1lld ", here->bands[7]);
    fprintf (out, "800us:%-7.1lld ", here->bands[8]);
    fprintf (out, "  1ms:%-7.1lld \n", here->bands[9]);

    fprintf (out, "\tBand 3: ");
    fprintf (out, "  2ms:%-7.1lld ", here->bands[10]);
    fprintf (out, "  4ms:%-7.1lld ", here->bands[11]);
    fprintf (out, "  6ms:%-7.1lld ", here->bands[12]);
    fprintf (out, "  8ms:%-7.1lld ", here->bands[13]);
    fprintf (out, " 10ms:%-7.1lld \n", here->bands[14]);

    fprintf (out, "\tBand 4: ");
    fprintf (out, " 12ms:%-7.1lld ", here->bands[15]);
    fprintf (out, " 14ms:%-7.1lld ", here->bands[16]);
    fprintf (out, " 16ms:%-7.1lld ", here->bands[17]);
    fprintf (out, " 18ms:%-7.1lld ", here->bands[18]);
    fprintf (out, " 20ms:%-7.1lld \n", here->bands[19]);

    fprintf (out, "\tBand 5: ");
    fprintf (out, " 40ms:%-7.1lld ", here->bands[20]);
    fprintf (out, " 60ms:%-7.1lld ", here->bands[21]);
    fprintf (out, " 80ms:%-7.1lld ", here->bands[22]);
    fprintf (out, "100ms:%-7.1lld \n", here->bands[23]);

    fprintf (out, "\tBand 6: ");
    fprintf (out, "200ms:%-7.1lld ", here->bands[24]);
    fprintf (out, "400ms:%-7.1lld ", here->bands[25]);
    fprintf (out, "600ms:%-7.1lld ", here->bands[26]);
    fprintf (out, "800ms:%-7.1lld ", here->bands[27]);
    fprintf (out, "   1s:%-7.1lld \n", here->bands[28]);

    fprintf (out, "\tBand 7: ");
    fprintf (out, "   2s:%-7.1lld ", here->bands[29]);
    fprintf (out, "   4s:%-7.1lld ", here->bands[30]);
    fprintf (out, "   6s:%-7.1lld ", here->bands[31]);
    fprintf (out, "   8s:%-7.1lld ", here->bands[32]);
    fprintf (out, "  10s:%-7.1lld \n", here->bands[33]);

    fprintf (out, "\tBand 8: ");
    fprintf (out, "  20s:%-7.1lld ", here->bands[34]);
    fprintf (out, "  40s:%-7.1lld ", here->bands[35]);
    fprintf (out, "  60s:%-7.1lld ", here->bands[36]);
    fprintf (out, "  80s:%-7.1lld ", here->bands[37]);
    fprintf (out, " 120s:%-7.1lld \n", here->bands[38]);

    fprintf (out, "\tBand 9: ");
    fprintf (out, "120+s:%-7.1lld \n\n", here->bands[39]);
#else
    fprintf (out, "\tBand 1: ");
    fprintf (out, " 20us:%-7.1ld ", here->bands[0]);
    fprintf (out, " 40us:%-7.1ld ", here->bands[1]);
    fprintf (out, " 60us:%-7.1ld ", here->bands[2]);
    fprintf (out, " 80us:%-7.1ld ", here->bands[3]);
    fprintf (out, "100us:%-7.1ld \n", here->bands[4]);

    fprintf (out, "\tBand 2: ");
    fprintf (out, "200us:%-7.1ld ", here->bands[5]);
    fprintf (out, "400us:%-7.1ld ", here->bands[6]);
    fprintf (out, "600us:%-7.1ld ", here->bands[7]);
    fprintf (out, "800us:%-7.1ld ", here->bands[8]);
    fprintf (out, "  1ms:%-7.1ld \n", here->bands[9]);

    fprintf (out, "\tBand 3: ");
    fprintf (out, "  2ms:%-7.1ld ", here->bands[10]);
    fprintf (out, "  4ms:%-7.1ld ", here->bands[11]);
    fprintf (out, "  6ms:%-7.1ld ", here->bands[12]);
    fprintf (out, "  8ms:%-7.1ld ", here->bands[13]);
    fprintf (out, " 10ms:%-7.1ld \n", here->bands[14]);

    fprintf (out, "\tBand 4: ");
    fprintf (out, " 12ms:%-7.1ld ", here->bands[15]);
    fprintf (out, " 14ms:%-7.1ld ", here->bands[16]);
    fprintf (out, " 16ms:%-7.1ld ", here->bands[17]);
    fprintf (out, " 18ms:%-7.1ld ", here->bands[18]);
    fprintf (out, " 20ms:%-7.1ld \n", here->bands[19]);

    fprintf (out, "\tBand 5: ");
    fprintf (out, " 40ms:%-7.1ld ", here->bands[20]);
    fprintf (out, " 60ms:%-7.1ld ", here->bands[21]);
    fprintf (out, " 80ms:%-7.1ld ", here->bands[22]);
    fprintf (out, "100ms:%-7.1ld \n", here->bands[23]);

    fprintf (out, "\tBand 6: ");
    fprintf (out, "200ms:%-7.1ld ", here->bands[24]);
    fprintf (out, "400ms:%-7.1ld ", here->bands[25]);
    fprintf (out, "600ms:%-7.1ld ", here->bands[26]);
    fprintf (out, "800ms:%-7.1ld ", here->bands[27]);
    fprintf (out, "   1s:%-7.1ld \n", here->bands[28]);

    fprintf (out, "\tBand 7: ");
    fprintf (out, "   2s:%-7.1ld ", here->bands[29]);
    fprintf (out, "   4s:%-7.1ld ", here->bands[30]);
    fprintf (out, "   6s:%-7.1ld ", here->bands[31]);
    fprintf (out, "   8s:%-7.1ld ", here->bands[32]);
    fprintf (out, "  10s:%-7.1ld \n", here->bands[33]);

    fprintf (out, "\tBand 8: ");
    fprintf (out, "  20s:%-7.1ld ", here->bands[34]);
    fprintf (out, "  40s:%-7.1ld ", here->bands[35]);
    fprintf (out, "  60s:%-7.1ld ", here->bands[36]);
    fprintf (out, "  80s:%-7.1ld ", here->bands[37]);
    fprintf (out, " 120s:%-7.1ld \n", here->bands[38]);

    fprintf (out, "\tBand 9: ");
    fprintf (out, "120+s:%-7.1ld \n", here->bands[39]);
    fflush (out);
#endif
}

/**
 * @brief The prime dumping its histograms from all of the 
 *        children into csv.
 *
 * @param here : Result object for results.
 * @param out : Output file 
 */
/*
 * __doc__
 * __doc__  Function : void prime_dump_hist_in_csv (struct result_object *here, 
 * __doc__                 FILE *out)
 * __doc__  Arguments: Result object for results.
 * __doc__             FILE *: Output file 
 * __doc__  Returns  : void
 * __doc__  Performs : The prime dumping its histograms from all of the 
 * __doc__             children.
 * __doc__
 */
void
prime_dump_hist_in_csv (struct result_object *here, FILE * out)
{

#ifndef NO_PRINT_LLD
    fprintf (out, "%lld,", here->bands[0]);
    fprintf (out, "%lld,", here->bands[1]);
    fprintf (out, "%lld,", here->bands[2]);
    fprintf (out, "%lld,", here->bands[3]);
    fprintf (out, "%lld,", here->bands[4]);

    fprintf (out, "%lld,", here->bands[5]);
    fprintf (out, "%lld,", here->bands[6]);
    fprintf (out, "%lld,", here->bands[7]);
    fprintf (out, "%lld,", here->bands[8]);
    fprintf (out, "%lld,", here->bands[9]);

    fprintf (out, "%lld,", here->bands[10]);
    fprintf (out, "%lld,", here->bands[11]);
    fprintf (out, "%lld,", here->bands[12]);
    fprintf (out, "%lld,", here->bands[13]);
    fprintf (out, "%lld,", here->bands[14]);

    fprintf (out, "%lld,", here->bands[15]);
    fprintf (out, "%lld,", here->bands[16]);
    fprintf (out, "%lld,", here->bands[17]);
    fprintf (out, "%lld,", here->bands[18]);
    fprintf (out, "%lld,", here->bands[19]);

    fprintf (out, "%lld,", here->bands[20]);
    fprintf (out, "%lld,", here->bands[21]);
    fprintf (out, "%lld,", here->bands[22]);
    fprintf (out, "%lld,", here->bands[23]);

    fprintf (out, "%lld,", here->bands[24]);
    fprintf (out, "%lld,", here->bands[25]);
    fprintf (out, "%lld,", here->bands[26]);
    fprintf (out, "%lld,", here->bands[27]);
    fprintf (out, "%lld,", here->bands[28]);

    fprintf (out, "%lld,", here->bands[29]);
    fprintf (out, "%lld,", here->bands[30]);
    fprintf (out, "%lld,", here->bands[31]);
    fprintf (out, "%lld,", here->bands[32]);
    fprintf (out, "%lld,", here->bands[33]);

    fprintf (out, "%lld,", here->bands[34]);
    fprintf (out, "%lld,", here->bands[35]);
    fprintf (out, "%lld,", here->bands[36]);
    fprintf (out, "%lld,", here->bands[37]);
    fprintf (out, "%lld", here->bands[38]);

    fprintf (out, "%lld", here->bands[39]);
#else
    fprintf (out, "%ld,", here->bands[0]);
    fprintf (out, "%ld,", here->bands[1]);
    fprintf (out, "%ld,", here->bands[2]);
    fprintf (out, "%ld,", here->bands[3]);
    fprintf (out, "%ld,", here->bands[4]);

    fprintf (out, "%ld,", here->bands[5]);
    fprintf (out, "%ld,", here->bands[6]);
    fprintf (out, "%ld,", here->bands[7]);
    fprintf (out, "%ld,", here->bands[8]);
    fprintf (out, "%ld,", here->bands[9]);

    fprintf (out, "%ld,", here->bands[10]);
    fprintf (out, "%ld,", here->bands[11]);
    fprintf (out, "%ld,", here->bands[12]);
    fprintf (out, "%ld,", here->bands[13]);
    fprintf (out, "%ld,", here->bands[14]);

    fprintf (out, "%ld,", here->bands[15]);
    fprintf (out, "%ld,", here->bands[16]);
    fprintf (out, "%ld,", here->bands[17]);
    fprintf (out, "%ld,", here->bands[18]);
    fprintf (out, "%ld,", here->bands[19]);

    fprintf (out, "%ld,", here->bands[20]);
    fprintf (out, "%ld,", here->bands[21]);
    fprintf (out, "%ld,", here->bands[22]);
    fprintf (out, "%ld,", here->bands[23]);

    fprintf (out, "%ld,", here->bands[24]);
    fprintf (out, "%ld,", here->bands[25]);
    fprintf (out, "%ld,", here->bands[26]);
    fprintf (out, "%ld,", here->bands[27]);
    fprintf (out, "%ld,", here->bands[28]);

    fprintf (out, "%ld,", here->bands[29]);
    fprintf (out, "%ld,", here->bands[30]);
    fprintf (out, "%ld,", here->bands[31]);
    fprintf (out, "%ld,", here->bands[32]);
    fprintf (out, "%ld,", here->bands[33]);

    fprintf (out, "%ld,", here->bands[34]);
    fprintf (out, "%ld,", here->bands[35]);
    fprintf (out, "%ld,", here->bands[36]);
    fprintf (out, "%ld,", here->bands[37]);
    fprintf (out, "%ld,", here->bands[38]);

    fprintf (out, "%ld", here->bands[39]);
#endif
}




/**
 * @brief Generates pattern layouts for dedup and compression.
 *
 * @param  p_dedup 		Percent Dedupable
 * @param  p_comp 		Percent Compressible.
 * @param  p_rand		Percent Un-compressible.
 * @param  bufp			Pointer to buffer to fill.
 * @param  tot_size		Total size of the file.
 * @param  xfer_size		Transfer size in bytes.
 * @param  dedup_gran_size	Dedup Granule size.
 * @param  comp_gran_size	Comp Granule size.
 * @param  offset		Offset in the file for this xfer.
 * @param  dedup_group		Which dedup set to use for this xfer.
 * @param  across_only	        Dedup across but not within flag.
 */
/*
 * __doc__
 * __doc__  Function : void make_buf(int p_dedup, int p_comp, int p_rand, 
 * __doc__                    char *bufp, long long tot_size, 
 * __doc__                    long long xfer_size, long long dedup_gran_size, 
 * __doc__ 		      long long comp_gran_size,long long offset, 
 * __doc__                    int dedup_group, int across_only)
 * __doc__  Arguments: 
 * __doc__  p_dedup 		.. 	Percent Dedupable
 * __doc__  p_comp 		..	Percent Compressible.
 * __doc__  p_rand		..	Percent Un-compressible.
 * __doc__  bufp			..	Pointer to buffer to fill.
 * __doc__  tot_size		..	Total size of the file.
 * __doc__  xfer_size		..	Transfer size in bytes.
 * __doc__  dedup_gran_size	..	Dedup Granule size.
 * __doc__  comp_granule_size	..	Comp Granule size.
 * __doc__  offset		..	Offset in the file for this xfer.
 * __doc__  dedup_group		..	Which dedup set to use for this xfer.
 * __doc__  across_only	..	Dedup across but not within flag.
 * __doc__             
 * __doc__  Returns  : void
 * __doc__  Performs : make_buf()   Generates pattern layouts for dedup and 
 * __doc__             compression.
 * __doc__             
 */
double global_counter;
int gbase_time;

void
make_buf (int p_dedup, int p_comp, int p_rand, char *bufp,
	  long long tot_size, long long xfer_size, long long dedup_gran_size,
	  long long comp_gran_size, long long offset, int dedup_group,
	  int across_only)
{

    long long dedup_regsz, comp_regsz, rand_regsz, dedup_start;
    long long dedup_end, comp_start, comp_end, rand_start, rand_end;
    long long dedup_offset, rand_offset, comp_offset, dedup_gran_rep_limit;
    long long dedup_xfer_size, comp_xfer_size, rand_xfer_size;
    int st_reg, end_reg, i;
    long long remaining, j = 0;
    int *dedup_buf;
    int *comp_buf;
    int *rand_buf;
    int save_seed, rel_time;
    int int_patt, base_time = 0;
    long long inbuf_offset = 0LL;	/* Offset in the xfer BUFFER */
    long long rand_gran_size;
    double local_time, msec_time;
    int int_local_time;
    unsigned int tmp_seed;

    if (gbase_time == 0)
    {
	base_time = (int) gettime ();
	gbase_time = 1;
    }
    global_counter += 1.0;
    dedup_gran_rep_limit =
	(long long) my_mix_table[my_workload].dedup_gran_rep_limit;

    if (dedup_group == 0)
	dedup_group = 1;
    st_reg = 0;

    rand_gran_size = dedup_gran_size;

    /* Round to nearest granule offset */
    dedup_offset = offset & ~(dedup_gran_size - 1);
    comp_offset = offset & ~(comp_gran_size - 1);
    rand_offset = offset & ~(rand_gran_size - 1);

    /* Round to nearest granula offset */
    dedup_xfer_size = xfer_size & ~(dedup_gran_size - 1);
    comp_xfer_size = xfer_size & ~(comp_gran_size - 1);
    rand_xfer_size = xfer_size & ~(rand_gran_size - 1);

    /* 
     * Find region sizes. (for the entire file )
     */
    dedup_regsz = (p_dedup * tot_size) / 100LL;
    comp_regsz = (p_comp * tot_size) / 100LL;
    rand_regsz = (p_rand * tot_size) / 100LL;

    dedup_regsz = (dedup_regsz / dedup_gran_size) * dedup_gran_size;
    comp_regsz = (comp_regsz / comp_gran_size) * comp_gran_size;
    rand_regsz = (rand_regsz / rand_gran_size) * rand_gran_size;

    /* 
     * Find where each region starts and ends 
     */
    dedup_start = 0LL;
    dedup_end =
	((dedup_start + dedup_regsz) / dedup_gran_size) * dedup_gran_size;
    if (dedup_regsz / dedup_gran_size != 0)
	dedup_end -= 1;

    comp_start = dedup_end;
    if (dedup_regsz / dedup_gran_size != 0)
	comp_start += 1;

    comp_end = ((comp_regsz / comp_gran_size) * comp_gran_size) + comp_start;
    if (comp_end != 0)
	comp_end -= 1;
#if defined(DEDUP_DEBUG)
    log_file (LOG_DEBUG,
	      "Comp_start  = %9lld  Comp_regsz %lld Comp_end  = %9lld\n",
	      comp_start, comp_regsz, comp_end);
#endif

    rand_start = comp_end;

    if ((comp_regsz / comp_gran_size != 0) || (comp_regsz == 0))
	rand_start += 1;
    if (rand_start == 1)	/* fix up in case comp size and dedup size are zero */
	rand_start = 0;

    rand_end = ((rand_regsz / rand_gran_size) * rand_gran_size) + rand_start;
    if (rand_regsz / rand_gran_size != 0)
	rand_end -= 1;

#if defined(DEDUP_DEBUG)
    log_file (LOG_EXEC, "Dedup_group    \t= %9d\n", dedup_group);
    log_file (LOG_EXEC,
	      "Dedup_size     \t= %9lld (rounded to Granule size)\n",
	      dedup_regsz);
    log_file (LOG_EXEC,
	      "Comp_size      \t= %9lld (rounded to Granule size)\n",
	      comp_regsz);
    log_file (LOG_EXEC,
	      "Rand_size      \t= %9lld (rounded to Granule size)\n",
	      rand_regsz);
    log_file (LOG_EXEC, "\n");
    if (dedup_regsz > 0)
	log_file (LOG_EXEC, "Dedup_start = %9lld  Dedup_end = %9lld\n",
		  dedup_start, dedup_end);
    if (comp_regsz > 0)
	log_file (LOG_EXEC, "Comp_start  = %9lld  Comp_end  = %9lld\n",
		  comp_start, comp_end);
    if (rand_regsz > 0)
	log_file (LOG_EXEC, "Rand_start  = %9lld  Rand_end  = %9lld\n",
		  rand_start, rand_end);
#endif

    /* 
     * Find the transfer's start region 
     */
    if ((dedup_offset >= dedup_start) && (dedup_offset <= dedup_end) &&
	(dedup_regsz != 0))
	st_reg = DEDUP;
    if ((comp_offset >= comp_start) && (comp_regsz != 0))
	st_reg = COMP;
    if ((rand_offset >= rand_start) && (rand_regsz != 0))
	st_reg = RAND;

    end_reg = 0;
    if (end_reg != 0)
	end_reg = 0;
    /* 
     * Find the transfer's end region 
     */
    if (dedup_regsz && (dedup_offset + dedup_xfer_size - 1) <= dedup_end)
	end_reg = DEDUP;
    if (comp_regsz && ((comp_offset + comp_xfer_size - 1) <= comp_end) &&
	((comp_offset + comp_xfer_size) > (comp_start)))
	end_reg = COMP;
    if (rand_regsz && ((rand_offset + rand_xfer_size - 1) <= rand_end) &&
	((rand_offset + rand_xfer_size) > (rand_start)))
	end_reg = RAND;

#if defined(DEDUP_DEBUG)
    log_file (LOG_EXEC,
	      "\nTransfer unit starts in region %d, ends in region %d\n\n",
	      st_reg, end_reg);
    log_file (LOG_EXEC, "\n");

#endif
    /*
     * Set the initial remaining number of bytes to the total xfer.
     */
    remaining = xfer_size;

    if (xfer_size == 0LL)
	return;

    /*
     * If the start region begins on the DEDUP region, then do all the regions.
     */
    if (st_reg == DEDUP)
    {
#if defined(DEDUP_DEBUG)
	log_file (LOG_EXEC, "Writing Dedup section.\n");

#endif
	/* 
	 * Make a dedupable Granule 
	 */
	dedup_buf = (int *) MALLOC ((size_t) dedup_gran_size);
	save_seed = netmist_gseed ();
	netmist_srand (save_seed ^
		       (dedup_group * (dedup_offset / dedup_gran_size)));
	for (i = 0; i < (int) (dedup_gran_size / sizeof (int)); i++)
	    dedup_buf[i] = (int) netmist_rand ();
	netmist_srand (save_seed);
	inbuf_offset = 0LL;
#if defined(DEDUP_DEBUG)
	log_file (LOG_EXEC,
		  "\tOffset %lld Dedup_end %lld Gran_size %lld Remaining %lld Xfer_size %lld Dedup Group %d\n",
		  dedup_offset, dedup_end, dedup_gran_size, remaining,
		  dedup_xfer_size, dedup_group);

#endif
	if (remaining < dedup_gran_size)
	{
	    if (across_only)
		dedup_buf[0] = (int) (dedup_offset / dedup_gran_size);
	    memcpy (&bufp[inbuf_offset], (char *) dedup_buf, remaining);
	    free (dedup_buf);
	    return;
	}

	for (j = dedup_offset; (j + dedup_gran_size <= dedup_end + 1) && (remaining >= dedup_gran_size) && (inbuf_offset + dedup_gran_size < dedup_xfer_size); j += dedup_gran_size)	/* file offsets */
	{
	    /* Limit the number of replicates of the exact same granule */
	    if ((j / dedup_gran_size != 0)
		&& (((j / dedup_gran_size) % dedup_gran_rep_limit) == 0))
	    {
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tDedup replicate limit\n");

#endif
		save_seed = netmist_gseed ();
		jitter = (unsigned long) netmist_rand ();
		netmist_srand (save_seed ^ ((dedup_group *
					     (j /
					      dedup_gran_size)) ^ jitter));
		for (i = 0; i < (int) (dedup_gran_size / sizeof (int)); i++)
		    dedup_buf[i] = (int) netmist_rand ();
		netmist_srand (save_seed);
	    }
	    if (across_only)
		dedup_buf[0] = (int) (j / dedup_gran_size);
	    memcpy (&bufp[inbuf_offset], (char *) dedup_buf, dedup_gran_size);
	    remaining = remaining - dedup_gran_size;
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Dedup granule %d\n",
		      (int) (j / dedup_gran_size));

#endif
	    inbuf_offset += dedup_gran_size;	/* buffer offsets */
	}
	free (dedup_buf);
	dedup_offset = j;
	comp_offset = dedup_offset & ~(comp_gran_size - 1);
	comp_offset = j;
#if defined(DEDUP_DEBUG)
	log_file (LOG_EXEC, "\tFilling Dedup done \n");

#endif

	/*
	 * If there is more, then move on to the compressible region.
	 */
	if (remaining > 0LL)
	{

#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "Writing Comp section. \n");

#endif
	    comp_buf = (int *) MALLOC ((size_t) comp_gran_size);
	    /* file offsets */

	    if (remaining < comp_gran_size)
	    {
		int_patt = (int) netmist_rand ();
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "Filling fragment Comp with %x \n",
			  int_patt);

#endif
		for (i = 0; i < (int) (comp_gran_size / sizeof (int)); i++)
		    comp_buf[i] = int_patt;
		/* Make random data even more unique. Scattered over time. */
		save_seed = netmist_gseed ();
		local_time = gettime ();
		rel_time = (int) local_time - base_time;
		msec_time = local_time - (int) local_time;
		int_local_time =
		    (int) ((cmdline.real_client_id) << 15) +
		    (int) global_counter + (int) rel_time +
		    (int) (msec_time * 1000000.0);
		global_counter += 1.0;
		tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
		tmp_seed ^= int_local_time;
		netmist_srand ((int) tmp_seed);
		/* Zap on dedup boundaries */
		for (i = 0; i < (int) (comp_gran_size / dedup_gran_size) * 2;
		     i++)
		{
		    if (i == (comp_gran_size / dedup_gran_size))
		    {
			local_time = gettime ();
			rel_time = (int) local_time - base_time;
			msec_time = local_time - (int) local_time;
			int_local_time =
			    (int) ((cmdline.real_client_id) << 15) +
			    (int) global_counter + (int) rel_time +
			    (int) (msec_time * 1000000.0);
			global_counter += 1.0;
			tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
			tmp_seed ^= (int) int_local_time;
			netmist_srand ((int) tmp_seed);
		    }
		    comp_buf[((i * dedup_gran_size) / 2) / sizeof (int)] =
			(int) netmist_rand ();
		}
		netmist_srand (save_seed);
		memcpy (&bufp[inbuf_offset], (char *) comp_buf, remaining);
		free (comp_buf);
		return;
	    }

	    int_patt = (int) netmist_rand ();
	    for (j = comp_offset;
		 (j + comp_gran_size <= comp_end + 1)
		 && (remaining >= comp_gran_size); j += comp_gran_size)
	    {
		/* Make compressible region, but not dedupable. */
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "Filling granule Comp with %x \n",
			  int_patt);

#endif
		for (i = 0; i < (int) (comp_gran_size / sizeof (int)); i++)
		    comp_buf[i] = int_patt;
		/* Make random data even more unique. Scattered over time. */
		save_seed = netmist_gseed ();
		local_time = gettime ();
		rel_time = (int) local_time - base_time;
		msec_time = local_time - (int) local_time;
		int_local_time =
		    ((int) (cmdline.real_client_id) << 15) +
		    (int) global_counter + (int) rel_time +
		    (int) (msec_time * 1000000.0);
		global_counter += 1.0;
		tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
		tmp_seed ^= (int) int_local_time;
		netmist_srand ((int) tmp_seed);
		/* Zap on dedup boundaries */
		for (i = 0; i < (int) (comp_gran_size / dedup_gran_size) * 2;
		     i++)
		{
		    if (i == (comp_gran_size / dedup_gran_size))
		    {
			local_time = gettime ();
			rel_time = (int) local_time - base_time;
			msec_time = local_time - (int) local_time;
			int_local_time =
			    ((int) (cmdline.real_client_id) << 15) +
			    (int) global_counter + (int) rel_time +
			    (int) (msec_time * 1000000.0);
			global_counter += 1.0;
			tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
			tmp_seed ^= (int) int_local_time;
			netmist_srand ((int) tmp_seed);
		    }
		    comp_buf[((i * dedup_gran_size) / 2) / sizeof (int)] =
			(int) netmist_rand ();
		}
		netmist_srand (save_seed);
		memcpy (&bufp[inbuf_offset], (char *) comp_buf,
			comp_gran_size);
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tFilling Comp granule  %d\n",
			  (int) (j / comp_gran_size));

#endif
		remaining = remaining - comp_gran_size;
		inbuf_offset += comp_gran_size;	/* Offsets in buffer */
	    }
	    free (comp_buf);
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Comp done \n");

#endif
	}
	dedup_offset = j;
	comp_offset = j;
	rand_offset = j;
	/*
	 * If there is more, then move on to the uncompressible, and 
	 * un-dedupable region.
	 */
	if (remaining > 0LL)
	{
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "Writing Rand section 1.\n");

#endif
	    rand_buf = (int *) MALLOC ((size_t) rand_gran_size);

	    save_seed = netmist_gseed ();
	    /* Make random data even more unique. Scattered over time. */
	    local_time = gettime ();
	    rel_time = (int) local_time - base_time;
	    msec_time = local_time - (int) local_time;
	    int_local_time =
		((int) (cmdline.real_client_id) << 15) +
		(int) global_counter + (int) rel_time +
		(int) (msec_time * 1000000.0);
	    global_counter += 1.0;
	    tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
	    tmp_seed ^= (int) int_local_time;
	    netmist_srand ((int) tmp_seed);
	    if (remaining < rand_gran_size)
	    {
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf, remaining);
		free (rand_buf);
		save_seed = netmist_gseed ();
		return;
	    }
	    /* file offsets */
	    for (j = rand_offset;
		 (j + rand_gran_size <= rand_end + 1)
		 && (remaining >= rand_gran_size); j += rand_gran_size)
	    {
		/* Make uncompressible region. */
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf,
			rand_gran_size);
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tFilling Rand granule 2 %d\n",
			  (int) (j / rand_gran_size));

#endif
		remaining = remaining - rand_gran_size;
		inbuf_offset += rand_gran_size;	/* offset in buffer */
	    }
	    netmist_srand (save_seed);
	    free (rand_buf);
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Rand done \n");

#endif
	}
    }
    /*
     * If the start region begins on the COMP region, then this, and the 
     * uncompressible region.
     */
    if (st_reg == COMP)
    {
	if (remaining > 0LL)
	{
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC,
		      "Writing Comp section. Remaining %lld Comp_size %lld\n",
		      remaining, comp_gran_size);

#endif
	    comp_buf = (int *) MALLOC ((size_t) comp_gran_size);

	    if (remaining < comp_gran_size)
	    {
		int_patt = (int) netmist_rand ();
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "Filling frag Comp with %x \n", int_patt);

#endif
		for (i = 0; i < (int) (comp_gran_size / sizeof (int)); i++)
		    comp_buf[i] = int_patt;
		/* Make random data even more unique. Scattered over time. */
		save_seed = netmist_gseed ();
		local_time = gettime ();
		rel_time = (int) local_time - base_time;
		msec_time = local_time - (int) local_time;
		int_local_time =
		    ((int) (cmdline.real_client_id) << 15) +
		    (int) global_counter + (int) rel_time +
		    (int) (msec_time * 1000000.0);
		global_counter += 1.0;
		tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
		tmp_seed ^= (int) int_local_time;
		netmist_srand ((int) tmp_seed);
		/* Zap on dedup boundaries */
		for (i = 0; i < (int) (comp_gran_size / dedup_gran_size) * 2;
		     i++)
		{
		    if (i == (comp_gran_size / dedup_gran_size))
		    {
			local_time = gettime ();
			rel_time = (int) local_time - base_time;
			msec_time = local_time - (int) local_time;
			int_local_time =
			    ((int) (cmdline.real_client_id) << 15) +
			    (int) global_counter + (int) rel_time +
			    (int) (msec_time * 1000000.0);
			global_counter += 1.0;
			tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
			tmp_seed ^= (int) int_local_time;
			netmist_srand ((int) tmp_seed);
		    }
		    comp_buf[((i * dedup_gran_size) / 2) / sizeof (int)] =
			(int) netmist_rand ();
		}
		netmist_srand (save_seed);
		memcpy (&bufp[inbuf_offset], (char *) comp_buf, remaining);
		free (comp_buf);
		return;
	    }

	    /* File offsets */
	    int_patt = (int) netmist_rand ();
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC,
		      "Comp offset %lld Comp_end %lld  Where %lld \n",
		      comp_offset, comp_end, j + comp_gran_size);

#endif
	    for (j = comp_offset;
		 (j + comp_gran_size <= comp_end + 1)
		 && (remaining >= comp_gran_size); j += comp_gran_size)
	    {
		/* Make compressible region, but not dedupable. */
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "Filling granule Comp with %x \n",
			  int_patt);

#endif
		for (i = 0; i < (int) (comp_gran_size / sizeof (int)); i++)
		    comp_buf[i] = int_patt;
		/* Make random data even more unique. Scattered over time. */
		save_seed = netmist_gseed ();
		local_time = gettime ();
		rel_time = (int) local_time - base_time;
		msec_time = local_time - (int) local_time;
		int_local_time =
		    ((int) (cmdline.real_client_id) << 15) +
		    (int) global_counter + (int) rel_time +
		    (int) (msec_time * 1000000.0);
		global_counter += 1.0;
		tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
		tmp_seed ^= (int) int_local_time;
		netmist_srand ((int) tmp_seed);
		/* Zap on dedup boundaries */
		for (i = 0; i < (int) (comp_gran_size / dedup_gran_size) * 2;
		     i++)
		{
		    if (i == (comp_gran_size / dedup_gran_size))
		    {
			local_time = gettime ();
			rel_time = (int) local_time - base_time;
			msec_time = local_time - (int) local_time;
			int_local_time =
			    ((int) (cmdline.real_client_id) << 15) +
			    (int) global_counter + (int) rel_time +
			    (int) (msec_time * 1000000.0);
			global_counter += 1.0;
			tmp_seed = netmist_gseed ();	/* Get previous key, and apply scattering field */
			tmp_seed ^= (int) int_local_time;
			netmist_srand ((int) tmp_seed);
		    }
		    comp_buf[((i * dedup_gran_size) / 2) / sizeof (int)] =
			(int) netmist_rand ();
		}
		netmist_srand (save_seed);

		memcpy (&bufp[inbuf_offset], (char *) comp_buf,
			comp_gran_size);
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tFilling Comp granule  %d\n",
			  (int) (j / comp_gran_size));

#endif
		remaining = remaining - comp_gran_size;
		inbuf_offset += comp_gran_size;	/* Offset in buffer */
	    }
	    free (comp_buf);
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Comp done \n");

#endif
	}
	dedup_offset = j;
	comp_offset = j;
	rand_offset = j;
	/* 
	 * If there is more, then generate the uncompressible, and undedupable 
	 * region.
	 */
	if (remaining > 0LL)
	{
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "Writing Rand section. 4 Size = %lld \n",
		      remaining);

#endif
	    rand_buf = (int *) MALLOC ((size_t) rand_gran_size);
	    save_seed = netmist_gseed ();
	    /* Make this very unique. Scattered over time. */
	    local_time = gettime ();
	    rel_time = (int) local_time - base_time;
	    msec_time = local_time - (int) local_time;
	    int_local_time =
		((int) (cmdline.real_client_id) << 15) +
		(int) global_counter + (int) rel_time +
		(int) (msec_time * 1000000.0);
	    global_counter += 1.0;
	    tmp_seed = netmist_gseed ();
	    tmp_seed ^= (int) int_local_time;
	    netmist_srand ((int) tmp_seed);	/* Get previous key, and apply scattering field */
	    if (remaining < rand_gran_size)
	    {
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf, remaining);
		free (rand_buf);
		netmist_srand (save_seed);
		return;
	    }
	    /* File offsets */
	    for (j = rand_offset;
		 (j + rand_gran_size <= rand_end + 1)
		 && (remaining >= rand_gran_size); j += rand_gran_size)
	    {
		/* Make uncompressible region. */
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf,
			rand_gran_size);
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tFilling Rand 5 granule  %d\n",
			  (int) (j / rand_gran_size));

#endif
		remaining = remaining - rand_gran_size;
		inbuf_offset += rand_gran_size;	/* Offset in buffer */
	    }
	    netmist_srand (save_seed);
	    free (rand_buf);
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Rand done \n");

#endif
	}
    }
    /*
     * If the start region begins on the RAND region, then this is all we need.
     * Generate an uncompressible, and undedupable region.
     */
    if (st_reg == RAND)
    {
#if defined(DEDUP_DEBUG)
	log_file (LOG_EXEC, "Writing Rand section 6.\n");

#endif
	if (remaining > 0LL)
	{
	    rand_buf = (int *) MALLOC ((size_t) rand_gran_size);
	    save_seed = netmist_gseed ();
	    /* Make this very unique, scattered over time */
	    local_time = gettime ();
	    rel_time = (int) local_time - base_time;
	    msec_time = local_time - (int) local_time;
	    int_local_time =
		((int) (cmdline.real_client_id) << 15) +
		(int) global_counter + (int) rel_time +
		(int) (msec_time * 1000000.0);
	    global_counter += 1.0;
	    tmp_seed = netmist_gseed ();
	    tmp_seed ^= (int) int_local_time;
	    netmist_srand ((int) tmp_seed);
	    if (remaining < rand_gran_size)
	    {
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf, remaining);
		free (rand_buf);
		netmist_srand (save_seed);
		return;
	    }
	    /* File offsets */
	    for (j = rand_offset;
		 (j + rand_gran_size <= rand_end + 1)
		 && (remaining >= rand_gran_size); j += rand_gran_size)
	    {
		local_time = gettime ();
		rel_time = (int) local_time - base_time;
		msec_time = local_time - (int) local_time;
		int_local_time =
		    ((int) (cmdline.real_client_id) << 15) +
		    (int) global_counter + (int) rel_time +
		    (int) (msec_time * 1000000.0);
		global_counter += 1.0;
		tmp_seed = netmist_gseed ();
		tmp_seed ^= (int) int_local_time;
		netmist_srand ((int) tmp_seed);
		/* Make uncompressible region. */
		for (i = 0; i < (int) (rand_gran_size / sizeof (int)); i++)
		    rand_buf[i] = (int) netmist_rand ();
		memcpy (&bufp[inbuf_offset], (char *) rand_buf,
			rand_gran_size);
#if defined(DEDUP_DEBUG)
		log_file (LOG_EXEC, "\tFilling Rand 7 granule  %d\n",
			  (int) (j / rand_gran_size));

#endif
		remaining = remaining - rand_gran_size;
		inbuf_offset += rand_gran_size;	/* Offset in buffer */
	    }
	    netmist_srand (save_seed);
	    free (rand_buf);
#if defined(DEDUP_DEBUG)
	    log_file (LOG_EXEC, "\tFilling Rand done \n");

#endif
	}
    }
#if defined(DEDUP_DEBUG)
    log_file (LOG_EXEC, "\tReturn Fill\n");

#endif
}


void
generic_empty_fileop (struct file_object *fp)
{
    (void) fp;
    return;
}

void
generic_empty_fileop_int (struct file_object *fp, int x)
{
    (void) fp;
    (void) x;
    return;
}

void
generic_empty_int_fileop (int x, struct file_object *fp)
{
    (void) fp;
    (void) x;
    return;
}

int
generic_iempty_dir (struct netmist_vfs_dir *d)
{
    (void) d;
    return 0;
}

int
generic_iempty_dir_s (struct netmist_vfs_dir *d, char *rfn)
{
    (void) d;
    (void) rfn;
    return 0;
}

/**
 * @brief : This is the code that implements the "block read rand op. 
 *
 * @param fp : contains unique file.
 */
/*
 * __doc__
 * __doc__  Function : void generic_bread_rand(struct file_object *fp)
 * __doc__  Arguments: struct file_object contains unique file.
 * __doc__  Returns  : void
 * __doc__  Performs : This is the code that implements the "block read rand 
 * __doc__             op. 
 * __doc__             
 */
void
generic_bread_rand (struct file_object *fp)
{
    struct netmist_vfs_object *fd = NULL;
    char *buf;
    int err;
#if defined(_LARGEFILE64_SOURCE) || defined(WIN32)
    off64_t where;
#else
    off_t where;
#endif
    uint64_t tlen = 0;
    uint64_t trans;
    double lat = 0.0;
    double entry_time = 0.0;
    enum netmist_vfs_open_flags flags = 0;
    int probability = 0;

    log_file (LOG_DEBUG, "Doing bread rand op\n");

    entry_time = gettime ();
    min_acc_per_spot = my_mix_table[my_workload].min_acc_per_spot;
    if (min_acc_per_spot <= 0)
	min_acc_per_spot = 1;
    spot_pct = my_mix_table[my_workload].percent_per_spot;
    if (spot_pct <= 0)
	spot_pct = 1;
    nspots = (100 / spot_pct);
    if (nspots < 1)
	nspots = 1;
    buf = main_buf;
    set_direct_flag (work_obj[my_workload].read_rand_op_count,
		     my_mix_table[my_workload].percent_direct, &flags);
    probability = (long) (netmist_rand () % 100);
    set_fadvise_seq ((long) probability,
		     my_mix_table[my_workload].percent_fadvise_seq, &flags);
    set_fadvise_rand ((long) probability,
		      my_mix_table[my_workload].percent_fadvise_rand, &flags);
    set_fadvise_dont_need ((long) probability,
			   my_mix_table[my_workload].
			   percent_fadvise_dont_need, &flags);

    set_osync_flag (work_obj[my_workload].read_rand_op_count,
		    my_mix_table[my_workload].percent_osync, &flags);

    fd_accesses++;
    file_accesses++;

    if ((fp->file_desc) && (fp->file_open_flags == flags)
	&& (fp->fd_link) && (fp->fd_link->key == fp))
    {
	fd = fp->file_desc;
	/*
	 * Move to head of fd LRU list.
	 */
	if (lru_on)
	{
	    fd_promote_to_head (fp);
	}
    }
    else
    {
	fd_cache_misses++;
	total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].total_file_ops += 1.0;	/* Open */
	work_obj[my_workload].open_op_count++;

	if (fp->file_desc)
	{
	    fd_drop_from_cache (fp);
	    work_obj[my_workload].close_op_count++;
	    total_file_ops += 1.0;	/* Close */
	    work_obj[my_workload].total_file_ops += 1.0;	/* Close */
	}

	err =
	    VFS (open, fp->dir, fp->relfilename, NETMIST_OPEN_WRITE | flags,
		 &fd);
	if (err != 0)
	{
	    const char *nes = netmist_vfs_errstr (err);
	    log_file (LOG_ERROR,
		      "Open for bread rand op failed. Error value = %d, Filename = %s in %s Error: %s\n",
		      err, fp->relfilename, VFS (pathptr, fp->dir), nes);
	    R_exit (NETMIST_OP_VFS_ERR + 15, err);
	}

	/* Got more free descriptor space ? */
	if (cached_open_count + guard_band < max_open)
	{
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
	else
	{
	    /* Go take one off the oldest (LRU) */
	    if (fd_cache_reuse () == 1)
	    {
		total_file_ops += 1.0;
		work_obj[my_workload].total_file_ops += 1.0;	/* Close */
		work_obj[my_workload].close_op_count++;
	    }

	    /* Now add to the head of the list */
	    fd_add_to_cache (fp);
	    fp->file_desc = fd;
	    fp->file_open_flags = flags;
	}
    }

    netmist_directio (fp->file_desc,
		      (work_obj[my_workload].read_rand_op_count % 100) <
		      my_mix_table[my_workload].percent_direct);

    /* Select read size to use */
    trans = get_next_read_size ();

    /* Default to uniform random */
    if (fp->Original_fsize == 1)
	where = 0;
    else
    {
	where = (netmist_llrand () % (fp->Original_fsize - 1));
	where = where & ~7;	/* Where is in Kbytes */
    }

    /* Uniform Random access */
    if (my_mix_table[my_workload].rand_dist_behavior == UNIFORM_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_RAND)
    {
	/* Pick random offset within the file on 8k boundary */
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where =
		geometric_rand (fp->Original_fsize -
				1) % (fp->Original_fsize - 1);
	    where = where & ~7;	/* Where is in Kbytes */
	}
    }
    if (my_mix_table[my_workload].rand_dist_behavior == GEOMETRIC_SPOT_RAND)
    {
	if (fp->Original_fsize == 1)
	    where = 0;
	else
	{
	    where = (netmist_llrand () % (fp->Original_fsize - 1));
	    where = where & ~7;	/* Where is in Kbytes */
	}
	/* Pick random offset within the file on 8k boundary */

	spot_size =
	    (fp->Original_fsize * (long long) spot_pct) / (long long) 100;
	if (spot_size <= 0)
	    spot_size = (long long) 1;

	/* Default 5 times as many 8k records as fits in a spot */
	acc_mult_spot = my_mix_table[my_workload].acc_mult_spot;
	if (acc_mult_spot <= 0)
	    acc_mult_spot = 1;
	hit_in_spot = (spot_size / 8) * acc_mult_spot;

	if ((hit_in_spot < min_acc_per_spot) || (hit_in_spot == 0))
	    hit_in_spot = min_acc_per_spot;
	if (hit_in_spot == 0)
	    hit_in_spot = 1;

	/* 
	 * Special logic for workloads that use "affinity", dividing into 
	 * spots for geometric, and jumping out of geometric PCT percent 
	 * of the time.
	 */
	if (my_mix_table[my_workload].percent_affinity != 0)
	{
	    if (((fp->db_access_count) % hit_in_spot) == 0)
		fp->which_spot =
		    (long long) (geometric_rand (nspots) % nspots);
	    /* a bit of jitter */
	    fp->which_spot = picker (fp->which_spot, nspots);
	    fp->db_access_count++;
	}
	else
	{
	    fp->which_spot = (long long) (geometric_rand (nspots) % nspots);
	}

	/* Geometric offsets within spot */
	if (my_mix_table[my_workload].spot_shape == GEOMETRIC_RAND_SPOT)
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = geometric_rand (spot_size) % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	    where = where & ~7;	/* Where is in Kbytes */
	}

	/* Uniform random offsets within spot */
	if ((my_mix_table[my_workload].spot_shape == UNIFORM_RAND_SPOT) ||
	    (my_mix_table[my_workload].spot_shape != GEOMETRIC_RAND_SPOT))
	{
	    if (spot_size == 0)
		offset_in_spot = 0;
	    else
		offset_in_spot = netmist_llrand () % spot_size;
	    where = (fp->which_spot * spot_size) + offset_in_spot;
	}
	where = where & ~7;	/* Where is in Kbytes */

	log_file (LOG_DEBUG,
		  "%lld Offset %lld Spsize %lld Whichsp %lld fCount \n",
		  (long long) where, spot_size,
		  (long long) (fp->which_spot),
		  (long long) (fp->db_access_count));

    }

    /* If it is beyond eof, start over */
    if (where + (trans / (unsigned long long) 1024) > (fp->Original_fsize))
	where = 0;
    fp->offset = (where * 1024);

    /* align things for O_DIRECT */
    adjust_size_direct (fp->file_desc, 0, &fp->offset);

    if (my_mix_table[my_workload].align)
    {
	fp->offset =
	    fp->offset & (off64_t) (~(my_mix_table[my_workload].align - 1));
    }
    if (cmdline.gg_flag)
    {
	if ((fp->offset + cmdline.gg_offset) / (unsigned long long) 1024 >
	    (fp->Original_fsize))
	    fp->offset = cmdline.gg_offset;
	else
	    fp->offset += cmdline.gg_offset;
    }
    VFS (seek, fd, fp->offset, NETMIST_SEEK_SET, NULL);

    /* align min size for O_DIRECT */
    adjust_size_direct (fp->file_desc, 1, &trans);

    err = netmist_try_large_read (fd, buf, trans, &tlen);
    if (err)
    {
	const char *nes = netmist_vfs_errstr (err);
	log_file (LOG_ERROR,
		  "Read file rand op failed file %s in %s Error: %s Error value %d Ret %"
		  PRIu64 "\n", fp->relfilename, VFS (pathptr, fp->dir), nes,
		  err, tlen);
	/*free(buf); */
	fd_drop_from_cache (fp);	/* includes close */
	R_exit (NETMIST_OP_VFS_ERR + 16, err);
    }
    if (cmdline.Op_lat_flag)
    {
	lat = gettime () - entry_time;
	hist_insert (lat);
	work_obj[my_workload].read_rand_op_time += lat;
	if ((lat < work_obj[my_workload].min_read_rand_op_latency)
	    || (work_obj[my_workload].min_read_rand_op_latency == 0.0))
	    work_obj[my_workload].min_read_rand_op_latency = lat;
	if ((lat > work_obj[my_workload].max_read_rand_op_latency)
	    || (work_obj[my_workload].max_read_rand_op_latency == 0.0))
	    work_obj[my_workload].max_read_rand_op_latency = lat;
    }

    total_read_bytes += (double) (tlen);
    total_read_rand_bytes += (double) (tlen);
    total_file_ops += 1.0;	/*Read */
    work_obj[my_workload].total_file_ops += 1.0;	/*Read */
    VFS (seek, fd, 0, NETMIST_SEEK_CUR, &fp->offset);

    fp->op_count++;
    work_obj[my_workload].read_rand_op_count++;
    if (in_validate)
    {
	if (fp->fd_link)
	{
	    fd_drop_from_cache (fp);	/* includes close */
	}
	work_obj[my_workload].close_op_count++;
	total_file_ops += 1.0;	/* Close */
	work_obj[my_workload].total_file_ops += 1.0;	/* Close */
    }

    /*free(buf); */
}

/**
 * @brief Old pattern layout from version 1 (SP1) 
 *        Does not comprehend dedupe, just compression and 
 *        non-compression.
 *
 * @param buf : Buffer to make non-dedup
 * @param size : Size of the buffer.
 */
/*
 * __doc__
 * __doc__  Function : void make_non_dedup(long *buf, int size)
 * __doc__  Arguments: buffer and size
 * __doc__  Returns  : void
 * __doc__  Performs : Old pattern layout from version 1 (SP1) 
 * __doc__             Does not comprehend dedupe, just compression and 
 * __doc__             non-compression.
 * __doc__             
 */

void
make_non_dedup (long *buf, int size)
{
    int i, j;
    int where, start;
    int percent_compress;
    int switch_count;
    long patt, compress_patt;

    patt = netmist_rand () & 0x7fffffff;
    percent_compress = my_mix_table[my_workload].percent_compress;

    switch_count = (DEDUP_CHUNK * percent_compress) / 100;

    compress_patt = (long) netmist_rand () & 0xffffffff;

    for (i = 0; i < size; i += DEDUP_CHUNK)
    {
	where = i / sizeof (long);

	/* Set first long in 512 byte buffer */
	buf[where] = (long) netmist_rand () & 0xffffffff;
	/* Now create compressibility within the 512 byte */
	start = i + sizeof (long);
	for (j = start; j < start + DEDUP_CHUNK; j += sizeof (long))
	{
	    where = j / sizeof (long);

	    if ((j % DEDUP_CHUNK) <= switch_count)
		buf[where] = (long) compress_patt;
	    else
		buf[where] ^= (patt * (j + i));
	}
    }
}

#if defined(WIN32)
/**
 * @brief If we are running in a Windows environment (a.k.a Hell) 
 *        then we need to deal with the nightmarish network 
 *        security model that was introduced in Vista (another 
 *        nightmare) This involves re-logging-in to one's self 
 *        with different credentials, because the horrid security 
 *        model has you logged in as "network user" and NOT who 
 *        you intended and all of your mapped shares have 
 *        disappeared !!. Egad, I hope they fired the lunatic
 *        that did this !! Note: This relogin nightmare is also 
 *        why one MUST use an extra client for the benchmark when 
 *        running with Windows clients. This God-awful security 
 *        mess will not let one re-login to one's self. Thus, the 
 *        extra physical client requirement.
 */
/*
 * __doc__
 * __doc__  Function : int Windows_authenticate_and_login(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int : success or failure.
 * __doc__  Performs : If we are running in a Windows environment (a.k.a Hell) 
 * __doc__             then we need to deal with the nightmarish network 
 * __doc__             security model that was introduced in Vista (another 
 * __doc__             nightmare) This involves re-logging-in to one's self 
 * __doc__             with different credentials, because the horrid security 
 * __doc__             model has you logged in as "network user" and NOT who 
 * __doc__             you intended and all of your mapped shares have 
 * __doc__             disappeared !!. Egad, I hope they fired the lunatic
 * __doc__             that did this !! Note: This relogin nightmare is also 
 * __doc__             why one MUST use an extra client for the benchmark when 
 * __doc__             running with Windows clients. This God-awful security 
 * __doc__             mess will not let one re-login to one's self. Thus, the 
 * __doc__             extra physical client requirement.
 * __doc__             
 */
int
Windows_authenticate_and_login (void)
{
    int foundpass = 0;
    int found = 0;
    int retval = 0;
    char temp[MAXNAME];
    char empty[1] = "";
    char *xx, *tt, *yyy;
    /* Usr_dom contains the user's name and domain. */
    /* The Password variable contains the user's password. */

    cmdline.usr_dom[strlen (cmdline.usr_dom)] = 0;
    /* Make a temporary copy of domain and user */
    my_strncpy (temp, cmdline.usr_dom, MAXNAME);
    temp[strlen (cmdline.usr_dom)] = 0;

    log_file (LOG_EXEC, "User_dom %s\n", temp);

    /* REMEMBER... DOUBLE BACK SLASHES seems to work best !!! */

    xx = strchr (temp, 0x5C);	/* find the backslash */
    if (xx != 0)
    {
	*xx = 0;		/* Make that a null seperator */
	xx++;			/* Point this at user name */
	found = 1;
    }
    if (xx != 0)		/* Any more ? */
    {
	tt = strchr (xx, 0x5C);	/* find the backslash */
	if (tt != 0)
	{
	    xx = tt;
	    *xx = 0;		/* Make that a null seperator */
	    xx++;		/* Point this at user name */
	    found = 1;
	}
    }
    if (!found)
    {
	tt = strchr (temp, 0x2F);	/* find the forward slash */
	if (tt != 0)
	{
	    xx = tt;
	    *xx = 0;		/* Make that a null seperator */
	    xx++;		/* Point this at user name */
	    found = 1;
	}
    }
    if (found == 1)
    {
	foundpass = 0;
	yyy = strchr (xx, 0x25);	/* Strip any trailing %password */
	if (yyy != 0)
	{
	    *yyy = 0;		/* Make that a null seperator */
	    yyy++;		/* point at the password      */
	    foundpass = 1;	/* Found a password           */
	}
    }
    if (!found)
	xx = empty;		/* Point this at an empty string */
    if (cmdline.Password_flag || foundpass)
    {
	/* logon */
	{
	    DWORD res;
	    TCHAR szUserName[32];
	    TCHAR szPassword[32];
	    TCHAR szDomain[32];
	    HANDLE hToken;

	    memset (szUserName, 0, 32);
	    memset (szPassword, 0, 32);
	    memset (szDomain, 0, 32);

	    my_strncpy (szDomain, temp, 32);	/* First half of domain\user    */
	    if (foundpass)
		/* Handed in via Domain\account%pass   */
		my_strncpy (szPassword, yyy, 32);
	    else
		/* Handed in via command line   */
		my_strncpy (szPassword, cmdline.password, 32);
	    /* Second half of domain\user   */
	    my_strncpy (szUserName, xx, 32);

	    log_file (LOG_EXEC, "User %s Domain %s Password %s\n", szUserName,
		      szDomain, szPassword);

	    my_strncpy ((char *) &wmiUser[0], (char *) &szUserName[0],
			MAXNAME);
	    my_strncpy ((char *) &wmiDomain[0], (char *) &szDomain[0],
			MAXNAME);
	    my_strncpy ((char *) &wmiPassword[0], (char *) &szPassword[0],
			MAXNAME);

	    /* Logon to the local computer with the intended credentials */
	    if (!cmdline.localflag)
	    {

		log_file (LOG_EXEC, "Calling LogonUser\n");

		res =
		    LogonUser (szUserName, szDomain, szPassword,
			       LOGON32_LOGON_NEW_CREDENTIALS,
			       LOGON32_PROVIDER_DEFAULT, &hToken);
		LogonUser_res = res;
		if (res == 0)
		{
		    LogonUser_res = GetLastError ();
		    log_stdout (LOG_EXEC, "Error %d from LogonUser\n", res);
		    retval = 1;
		}
		/* Become that user */

		log_file (LOG_EXEC, "Calling ImpersonateLoggedOnUser\n");

		res = ImpersonateLoggedOnUser (hToken);
		ImpersonateLoggedOnUser_res = res;
		if (res == 0)
		{
		    ImpersonateLoggedOnUser_res = GetLastError ();
		    log_stdout (LOG_EXEC,
				"Error %d from ImpersonateLoggedOnUser\n",
				res);
		    retval = 1;
		}
	    }
	}
    }
    return (retval);
}
#endif

/*
 * @brief Encrypts a buffer using the rotor encryption method.
 *
 * @param buf Pointer to buffer to encrypt
 * @param size Size of buffer in bytes
 *
 * __doc__
 * __doc__  Function : void make_cypher( char *buf, int size )
 * __doc__  Arguments: Pointer to the buffer and its size
 * __doc__  Returns  : void
 * __doc__  Performs : Encryptst the contents of the buffer
 * __doc__             
 */

void
make_cypher (char *buf, int size)
{
    int k;
    char *c_buf_ptr;

    c_buf_ptr = (char *) buf;

    /* If encryption is enabled */
    if (my_mix_table[my_workload].cipher)
    {
	if (global_start_enc == 0)
	{
	    /* Key, Num_rotors, shuffle_freq */
	    _start_encryption ((unsigned long long) child_port, 5, 13);
	}
	for (k = 0; k < size; k++)
	{
	    *c_buf_ptr = _crypt_char (*c_buf_ptr);
	    c_buf_ptr++;
	}
    }
}



/* This is the Windows version of gettimeofday() with high resolution timers. */
#if defined(WIN32)

struct w_timezone
{
    int tz_minuteswest;		/* minutes W of Greenwich */
    int tz_dsttime;		/* type of dst correction */
};

/**
 * @brief Windows version of gettimeofday() Returns the timeofday
 *        in the timeval structure in seconds, and microseconds.
 *
 * @param tv : Struct timeval
 * @param tv : Struct w_timezone timezone structure.
 */
/*
 * __doc__
 * __doc__  Function : int w_gettimeofday()
 * __doc__  Arguments: Struct timeval, struct w_timezone
 * __doc__  Returns  : 0 for success or 1 for failure.
 * __doc__  Performs : The Windows version of gettimeofday.
 * __doc__
 */
int
w_gettimeofday (struct timeval *tv, struct w_timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int _tzflag;

    memset (&ft, 0, sizeof (ft));
    if (NULL != tv)
    {
	GetSystemTimeAsFileTime (&ft);

	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;

	/*converting file time to unix epoch */
	tmpres /= 10;		/*convert into microseconds */
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tv->tv_sec = (time_t) (tmpres / 1000000UL);
	tv->tv_usec = (time_t) (tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
	if (!_tzflag)
	{
	    _tzset ();
	    _tzflag++;
	}
	tz->tz_minuteswest = _timezone / 60;
	tz->tz_dsttime = _daylight;
    }
    return 0;
}
#endif

/*
 * @brief Helper function that does the right thing for O_DIRECT for all 
 * platforms. Also sets a flag for trace to know if O_DIRECT was used. 
 * This makes it easier for trace to be platform independent.
 *
 * @param op_count : Number of ops done so far.
 * @param percent : Percent of ops that triggers enabling flag
 * @param flags : Existing flags.
 */
void
set_direct_flag (long op_count, int percent,
		 enum netmist_vfs_open_flags *flags)
{
    if ((op_count % 100) < percent)
    {
	*flags |= NETMIST_OPEN_DIRECT;
    }
}

/*
 * @brief Helper function that does the right thing for fadvise (ADV_SEQ) for all 
 * platforms. Also sets a flag for trace to know if fadvise (ADV_SEQ) was used. 
 * This makes it easier for trace to be platform independent.
 *
 * @param op_count : Number of ops done so far.
 * @param percent : Percent of ops that triggers enabling flag
 * @param flags : Existing flags.
 */
void
set_fadvise_seq (long op_count, int percent,
		 enum netmist_vfs_open_flags *flags)
{
    if ((op_count % 100) < percent)
    {
	log_file (LOG_DEBUG, "set fadvise_seq\n");
	*flags |= NETMIST_OPEN_SEQ;
    }
}

/*
 * @brief Helper function that does the right thing for fadvise (ADV_RAND) for all 
 * platforms. Also sets a flag for trace to know if fadvise (ADV_RAND) was used. 
 * This makes it easier for trace to be platform independent.
 *
 * @param op_count : Number of ops done so far.
 * @param percent : Percent of ops that triggers enabling flag
 * @param flags : Existing flags.
 */
void
set_fadvise_rand (long op_count, int percent,
		  enum netmist_vfs_open_flags *flags)
{
    if ((op_count % 100) < percent)
    {
	log_file (LOG_DEBUG, "set fadvise_rand\n");
	*flags |= NETMIST_OPEN_RAND;
    }
}

/*
 * @brief Helper function that does the right thing for fadvise (ADV_DONT_NEED) for all 
 * platforms. Also sets a flag for trace to know if fadvise (ADV_DONT_NEED) was used. 
 * This makes it easier for trace to be platform independent.
 *
 * @param op_count : Number of ops done so far.
 * @param percent : Percent of ops that triggers enabling flag
 * @param flags : Existing flags.
 */
void
set_fadvise_dont_need (long op_count, int percent,
		       enum netmist_vfs_open_flags *flags)
{
    if ((op_count % 100) < percent)
    {
	log_file (LOG_DEBUG, "set fadvise_dont_need\n");
	*flags |= NETMIST_OPEN_DONT_NEED;
    }
}

/*
 * @brief Helper function that does the right thing for O_SYNC for all 
 * platforms. Also sets a flag for trace to know if O_SYNC was used. 
 * This makes it easier for trace to be platform independent.
 *
 * @param op_count :  Current op count
 * @param percent : Percent to trigger enabling
 * @param flags : Pointer to existing flags
 */
void
set_osync_flag (long op_count, int percent,
		enum netmist_vfs_open_flags *flags)
{
    if ((op_count % 100) < percent)
    {
	*flags |= NETMIST_OPEN_SYNC;
    }
}

/**
 * @brief slinky: Function to build depth of directories and fill with filenames
 *        so that lookup will have to deal with large and deep directories.
 *
 * @param cur_value : Current depth value. (Used for recursion)
 * @param limit : Maximum depth to create
 * @param chaff_count : Number of chaff files to place in each level.
 * @param path : netmist vfs directory handle
 * @param mode : Create or Remove modes.
 */
/*
 * __doc__
 * __doc__  Function    : slinky. Creates directory depth and some chaff files.
 * __doc__  Arguments   :
 * __doc__  cur_value   : Current depth value. (Used for recursion)
 * __doc__  limit       : Maximum depth to create
 * __doc__  chaff_count : Number of chaff files to place in each level.
 * __doc__  path        : netmist vfs directory handle
 * __doc__  mode        : Create or Remove modes.
 * __doc__  Returns     : No return value.
 * __doc__
 */

void
slinky (int cur_value, int limit, int chaff_count,
	struct netmist_vfs_dir **path, int mode)
{
    char pathbuf[MAXNAME];
    int ret;

    struct netmist_vfs_dir *prepath = NULL;

    if (limit == 0)
	return;

    snprintf (pathbuf, sizeof (pathbuf), "dir_%d", cur_value);

    quiet_now = 1;		/* turn off stat_workdir messages */

    ret = netmist_vfs_walkme (path, pathbuf);
    if (ret)
    {
	log_file (LOG_ERROR, "slinky VFS walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    ret = VFS (walk, *path, "", &prepath);
    if (ret)
    {
	log_file (LOG_ERROR, "slinky VFS prepath walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    if (mode == SLINKY_CREATE)
    {
	ret = (*Netmist_Ops[my_workload].stat_workdir) (*path);
	if (ret != 0)
	{
	    log_file (LOG_DEBUG, "Slinky: creating %s\n",
		      VFS (pathptr, *path));
	    /* Use virtualized interface to create the directory. */
	    ret = (*Netmist_Ops[my_workload].init_dir) (*path);
	    if (ret != 0)
	    {
		log_file (LOG_ERROR,
			  "Create of directory %s failed. Error: %d\n",
			  VFS (pathptr, *path), ret);
		R_exit (NETMIST_VFS_ERR, ret);
	    }
	}
    }
    if ((mode == SLINKY_CREATE) || (mode == SLINKY_REMOVE))
    {
	make_chaff (*path, cur_value, chaff_count, mode);
    }
    cur_value += 1;
    /* Recursion here... dive and create files on the way down */
    if (cur_value < limit)
	slinky (cur_value, limit, chaff_count, path, mode);

    /* If mode is remove, then remove the directory as the recursion unwinds */
    if (mode == SLINKY_REMOVE)
    {
	(*Netmist_Ops[my_workload].remove_dir) (prepath);
    }

    quiet_now = 0;
    VFS (dfree, &prepath);
}

/**
 * @brief make_chaff: Function to create the chaff files within a directory. This
 *        is to bloat the directories so that later lookups will have more work to do.
 *
 * @param dir : VFS dir in which to make chaff
 * @param count : Number of chaff files to create.
 * @param mode : Create or Remove modes.
 */
/*
 * __doc__
 * __doc__  Function    : make_chaff. Function to create the chaff files within a
 * __doc__              : directory. This is to bloat the directories so that later
 * __doc__              : lookups will have more work to do.
 * __doc__  Arguments   :
 * __doc__  dir         : VFS dir in which to make chaff
 * __doc__  count       : Number of chaff files to place in each level.
 * __doc__  mode        : Create or Remove modes.
 * __doc__  Returns     : No return value.
 * __doc__
 */
#if !defined(RAND_SHORT_NAME)
volatile int compiler_noise;
#endif

void
make_chaff (struct netmist_vfs_dir *dir, int cur_value, int count, int mode)
{
    char t_buff[MAXNAME + 10];
    char sn_buff[MAXNAME];
    int i;
    int my_delta_time;
#if defined(WIN32)
    DWORD ret = 0;
#else
    int ret;
#endif
#if defined(PDSM_RO_ACTIVE)
    int my_pdsm_time;
#endif

    if (count == 0)
	return;
    for (i = 0; i < count; i++)
    {
	memset (sn_buff, 0, MAXNAME);

#if defined(RAND_SHORT_NAME)
	make_short_name (sn_buff, ((i * count) + cur_value), 0);
#else
	my_strncpy (sn_buff, "Chaff", MAXNAME);
	compiler_noise = cur_value;
#endif
	snprintf (t_buff, sizeof t_buff, "%s_%08d", sn_buff, i);

	if (mode == SLINKY_CREATE)
	{
	    if (VFS (stat, dir, t_buff, NULL) != 0)
	    {
		(*Netmist_Ops[my_workload].init_empty_file) (dir, t_buff);
	    }
	}
	else if (mode == SLINKY_REMOVE)
	{
	    (*Netmist_Ops[my_workload].remove_file) (dir, t_buff);
	}
	    /*-----------------*/
	/* heartbeat       */
	    /*-----------------*/
	cur_time = gettime ();

	/* 
	 * We need all heartbeats from all procs, let the nodeManager 
	 * do the filtering. 
	 */
	my_delta_time = (int) (cur_time - last_heart_beat);

	if (cmdline.heartbeat_flag && init_phase &&
	    (my_delta_time >= HEARTBEAT_TICK))
	{
	    if (mode == SLINKY_CREATE)
	    {
		tell_prime_heartbeat (cmdline.real_client_id, INIT_BEAT,
				      (double) 0.0);
	    }
	    else
	    {
		tell_prime_heartbeat (cmdline.real_client_id, CLEAN_BEAT,
				      (double) 0.0);
	    }
	    last_heart_beat = cur_time;
#if defined(PDSM_RO_ACTIVE)
	    if (pdsm_stats)
	    {
		pdsm_stats->epoch_time = time (NULL);
		pdsm_stats->cur_state = (int) INIT_BEAT;
		pdsm_stats->client_id = cmdline.client_id;
		pdsm_stats->mode = pdsm_mode;
		pdsm_stats->requested_op_rate = (double) cmdline.op_rate;
		pdsm_stats->achieved_op_rate = (double) 0.0;
		pdsm_stats->run_time =
		    (double) (last_heart_beat - my_start_time);
		my_strncpy (pdsm_stats->client_name,
			    client_localname, MAXHNAME);
		pdsm_stats->client_name[strlen (client_localname)] = 0;
		my_strncpy (pdsm_stats->workload_name,
			    work_obj[my_workload].work_obj_name, MAXWLNAME);
		pdsm_stats->workload_name[strlen
					  (work_obj
					   [my_workload].work_obj_name)] = 0;
	    }
	    if (pdsm_file_fd)
	    {
		my_pdsm_time = (int) (cur_time - last_pdsm_beat);
		if (my_pdsm_time >= pdsm_interval)
		{
		    if (pdsm_mode == 0)	/* Over-write mode */
		    {
#if defined(WIN32)
			largeoffset.QuadPart =
			    sizeof (struct pdsm_remote_stats) *
			    cmdline.client_id;
			SetFilePointerEx (pdsm_file_fd, largeoffset, NULL,
					  FILE_BEGIN);
#else
			I_LSEEK (pdsm_file_fd,
				 sizeof (struct pdsm_remote_stats) *
				 cmdline.client_id, SEEK_SET);
#endif
		    }
#if defined(WIN32)
		    WriteFile (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats), &ret, NULL);
#else
		    ret =
			write (pdsm_file_fd, pdsm_stats,
			       sizeof (struct pdsm_remote_stats));
#endif
		    if (ret <= 0)
			log_file (LOG_DEBUG,
				  "Writing of pdsm_stats failed\n");
#if defined(FSYNC_PDSM)
		    if (my_pdsm_time >= (2 * pdsm_interval))
			fsync (pdsm_file_fd);
#endif
		    last_pdsm_beat = cur_time;
		}
	    }
#endif
	}
    }
}

/*
 * Test to see if a client_id has a workdir for this proc 
 */
int
dir_exist (void)
{
    char buf[MAXNAME2];
    int ret = 0;
    struct netmist_vfs_dir *procdir;

    if (cmdline.sharing_flag)
    {
	snprintf (buf, sizeof (buf), "SM_%s",
		  work_obj[my_workload].work_obj_name);
    }
    else
    {
	snprintf (buf, sizeof (buf), "CL%d_%s", cmdline.client_id,
		  work_obj[my_workload].work_obj_name);
    }
    procdir = netmist_vfs_root ();
    ret = netmist_vfs_walkme (&procdir, buf);
    if (ret)
    {
	log_file (LOG_ERROR, "dir_exist VFS walk error %d\n", ret);
	R_exit (NETMIST_VFS_ERR, ret);
    }

    quiet_now = 1;		/* Turn off logging any errors for this call */
    ret = (*Netmist_Ops[my_workload].stat_workdir) (procdir);
    quiet_now = 0;		/* Turn on logging after this */

    VFS (dfree, &procdir);

    return ret;
}

/* Generic routine to convert errno to its string meaning.
 * Uses VFS version, if it is active, to work across platform types.
 * Else, is uses the platform type directly.
 *
 * @brief netmist_strerror converts errno into a string, across platform types.
 * @param x : Errno value, or error for Windows.
 */
char *
netmist_strerror (int x)
{
    /* If vfst is initialized, then it is safe to call through 
     * the abstraction 
     */
    if (vfst && vfst->netmist_vfs_errstr)
	return (char *) netmist_vfs_errstr (x);
#if defined(WIN32)
    return win32_strerror (x);
#else
    return strerror (x);
#endif
}
