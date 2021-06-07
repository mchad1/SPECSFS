/*****************************************************************************
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
 *****************************************************************************/
#include "./copyright.txt"
#include "./license.txt"
#ifdef WIN32
#include <stdint.h>
#define u_int32_t  uint32_t
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <limits.h>
#include "netmist_if.h"

#include "netmist_utils.h"
#include "netmist_logger.h"
#if defined(WIN32)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4133)
#pragma warning(disable:4244)
#pragma warning(disable:4102)
#pragma warning(disable:4018)
#endif

extern char *my_strncpy (char *, char *, size_t);
extern u_int32_t crc32_generate (void *data, size_t length);
extern u_int32_t crc32_check (void *data, size_t length);
extern void lock_logging(void);
extern void unlock_logging(void);

struct prime_command *g_mc = NULL;
struct ext_prime_command *g_emc = NULL;

struct nodeManager_command *g_nmc = NULL;
struct ext_nodeManager_command *g_enmc = NULL;

struct client_command *g_cc = NULL;
struct ext_client_command *g_ecc = NULL;

struct keepalive_command *g_kc = NULL;
struct ext_keepalive_command *g_ekc = NULL;

const char *heartbeat_names[] = {
    "DUMMY",
    "Init",
    "Warm",
    "Run",
    "PRE_WARM",
    "CLEAN",
    "VAL"
};

static char prime_string[] = "prime";
static char nodeManager_string[] = "nodeManager";
static char client_string[] = "client";

/*
 * Functions to convert data between the network format to the internal
 * representation. These are custom routines for Netmist/SFS2014. While
 * one could use something like Google's protobuf, or PBio (portable
 * binary I/O) these are small, simple, portable, and do not introduce
 * dependencies on additional external software packages. e.g. 400,000+ lines
 * of code in Google's protobuf.
 *
 * Block diagram:
 *     Local variables -> internal command struct -> external/neutral command struct
 *     External/neutral command struct -> internal command struct -> local variables.
 * 
 * Encode/decode method: 
 *     -------------------------------------------------------------------
 *     | int X                 |  -> snprintf -> |  char X[INT_LEN]      | 
 *     | char X[INT_LEN]       |  -> sscanf  ->  |  int X                | 
 *     | long X                |  -> snprintf -> |  char X[LONG_LEN]     | 
 *     | char X[LONG_LEN]      |  -> sscanf  ->  |  long X               | 
 *     | long long X           |  -> snprintf -> |  char X[LONG_LONG_LEN]| 
 *     | char X[LONG_LONG_LEN] |  -> sscanf  ->  |  long long X          | 
 *     | double Y              |  -> snprintf -> |  char Y[DOUBLE_LEN]   | 
 *     | char Y[DOUBLE_LEN]    |  -> sscanf  ->  |  double Y             | 
 *     -------------------------------------------------------------------
 */

/**
 * @brief Function to convert internal (architecturally dependent)
 *        information into string format for sending to the 
 *        remote client in a neutral format that they can convert 
 *        back to their internal representation.
 *        Think of this as a version of XDR that works with all 
 *        data types. :-)
 *        prime_int_to_ext() is used by the remote child to 
 *        encode information that will be sent over the network 
 *        to the prime client.
 * @param mc  : Pointer to Prime_command internal format
 * @param emc : Pointer to Prime_command external format
 * @param op_lat_flag : Op_latency_flag.. enable op latency reporting.
 */
/*
 * __doc__
 * __doc__  Function : void prime_int_to_ext(struct prime_command *mc, 
 * __doc__                    struct ext_prime_command *emc, int op_lat_flag) 
 * __doc__  Arguments: struct prime_command: The internal command format
 * __doc__             struct ext_prime_command: The commnand in network 
 * __doc__                                       neutral format
 * __doc__             int op_lat_flag: Flag to enable latency collection.
 * __doc__  Returns  : void
 * __doc__             remote client in a neutral format that they can convert 
 * __doc__             back to their internal representation.
 * __doc__             Think of this as a version of XDR that works with all 
 * __doc__             data types. :-)
 * __doc__
 * __doc__             prime_int_to_ext() is used by the remote child to 
 * __doc__             encode information that will be sent over the network 
 * __doc__             to the prime client.
 * __doc__
 * __note__          : Note: If one changes the value of OP_STR_LEN or any 
 * __note__            of the sizes of the variables in the string based 
 * __note__            structures ext_prime_command, or ext_client_command,
 * __note__            then the lengths would need to be adjusted in the 
 * __note__            fields below.
 * __doc__             
 */
void
prime_int_to_ext (struct prime_command *mc, struct ext_prime_command *emc,
		  int op_lat_flag)
{
    int u;
    char format_intx[MY_MAX_NUM_TYPE_LEN], format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN],
	format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN], format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and sscanf
     * routines.  This ensures that the functions do not accidentally over write
     * the fields in the structures. The defines can be changed, if needed, and
     * all of the space and the routines that touch that space are in sync.
     */
    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    snprintf (emc->magic, MY_INT_WIDTH, format_intu, mc->magic);
    snprintf (emc->crc32, MY_INT_WIDTH, format_intu, mc->crc32);

    my_strncpy (emc->m_host_name, mc->m_host_name, MAXHNAME);
    my_strncpy (emc->m_client_name, mc->m_client_name, MAXHNAME);
    snprintf (emc->m_client_eval, MY_INT_WIDTH, format_intx,
	      mc->m_client_eval);
    snprintf (emc->m_client_plat_err, MY_INT_WIDTH, format_intx,
	      mc->m_client_plat_err);
    snprintf (emc->m_client_number, MY_INT_WIDTH, format_intd,
	      mc->m_client_number);
    snprintf (emc->m_nm_number, MY_INT_WIDTH, format_intd, mc->m_nm_number);
    snprintf (emc->m_work_obj_index, MY_INT_WIDTH, format_intd,
	      mc->m_work_obj_index);
    snprintf (emc->m_child_port, MY_INT_WIDTH, format_intx, mc->m_child_port);
    snprintf (emc->m_child_pid, MY_INT_WIDTH, format_intx, mc->m_child_pid);
    snprintf (emc->m_child_flag, MY_INT_WIDTH, format_intx, mc->m_child_flag);
    snprintf (emc->m_command, MY_INT_WIDTH, format_intx, mc->m_command);
    snprintf (emc->m_percent_complete, MY_INT_WIDTH, format_intd,
	      mc->m_percent_complete);
    /* now for the results */
    my_strncpy (emc->m_workload_name, mc->m_workload_name, MAXWLNAME - 1);
    snprintf (emc->m_run_time, MY_LONG_WIDTH, format_longd, mc->m_run_time);
    snprintf (emc->m_min_direct_size, MY_LONG_WIDTH, format_longd,
	      mc->m_min_direct_size);
    snprintf (emc->m_total_file_ops, MY_DOUBLE_WIDTH, format_double,
	      mc->m_total_file_ops);
    snprintf (emc->m_total_file_op_time, MY_DOUBLE_WIDTH, format_double,
	      mc->m_total_file_op_time);
    snprintf (emc->m_ops_per_second, MY_DOUBLE_WIDTH, format_double,
	      mc->m_ops_per_second);
    snprintf (emc->m_average_latency, MY_DOUBLE_WIDTH, format_double,
	      mc->m_average_latency);
    snprintf (emc->m_read_throughput, MY_DOUBLE_WIDTH, format_double,
	      mc->m_read_throughput);
    snprintf (emc->m_read_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_read_kbytes);
    snprintf (emc->m_write_throughput, MY_DOUBLE_WIDTH, format_double,
	      mc->m_write_throughput);
    snprintf (emc->m_write_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_write_kbytes);
    snprintf (emc->m_Nread_throughput, MY_DOUBLE_WIDTH, format_double,
	      mc->m_Nread_throughput);
    snprintf (emc->m_Nread_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_Nread_kbytes);
    snprintf (emc->m_Nwrite_throughput, MY_DOUBLE_WIDTH, format_double,
	      mc->m_Nwrite_throughput);
    snprintf (emc->m_Nwrite_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_Nwrite_kbytes);
    snprintf (emc->m_meta_r_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_meta_r_kbytes);
    snprintf (emc->m_meta_w_kbytes, MY_DOUBLE_WIDTH, format_double,
	      mc->m_meta_w_kbytes);
    snprintf (emc->m_init_files, MY_INT_WIDTH, format_intx, mc->m_init_files);
    snprintf (emc->m_init_files_ws, MY_INT_WIDTH, format_intx,
	      mc->m_init_files_ws);
    snprintf (emc->m_init_dirs, MY_INT_WIDTH, format_intx, mc->m_init_dirs);
    snprintf (emc->m_file_space_mb, MY_DOUBLE_WIDTH, format_double,
	      mc->m_file_space_mb);
    snprintf (emc->m_current_op_rate, MY_DOUBLE_WIDTH, format_double,
	      mc->m_current_op_rate);
    snprintf (emc->m_read_count, MY_INT_WIDTH, format_intx, mc->m_read_count);
    snprintf (emc->m_read_string, OP_STR_LEN, format_opstr,
	      mc->m_read_string);
    snprintf (emc->m_read_file_count, MY_INT_WIDTH, format_intx,
	      mc->m_read_file_count);
    snprintf (emc->m_read_file_string, OP_STR_LEN, format_opstr,
	      mc->m_read_file_string);
    snprintf (emc->m_read_rand_count, MY_INT_WIDTH, format_intx,
	      mc->m_read_rand_count);
    snprintf (emc->m_read_rand_string, OP_STR_LEN, format_opstr,
	      mc->m_read_rand_string);
    snprintf (emc->m_write_count, MY_INT_WIDTH, format_intx,
	      mc->m_write_count);
    snprintf (emc->m_write_string, OP_STR_LEN, format_opstr,
	      mc->m_write_string);
    snprintf (emc->m_write_file_count, MY_INT_WIDTH, format_intx,
	      mc->m_write_file_count);
    snprintf (emc->m_write_file_string, OP_STR_LEN, format_opstr,
	      mc->m_write_file_string);
    snprintf (emc->m_open_count, MY_INT_WIDTH, format_intx, mc->m_open_count);
    snprintf (emc->m_close_count, MY_INT_WIDTH, format_intx,
	      mc->m_close_count);
    snprintf (emc->m_mmap_write_count, MY_INT_WIDTH, format_intx,
	      mc->m_mmap_write_count);
    snprintf (emc->m_mmap_write_string, OP_STR_LEN, format_opstr,
	      mc->m_mmap_write_string);
    snprintf (emc->m_mmap_read_count, MY_INT_WIDTH, format_intx,
	      mc->m_mmap_read_count);
    snprintf (emc->m_mmap_read_string, OP_STR_LEN, format_opstr,
	      mc->m_mmap_read_string);
    snprintf (emc->m_write_rand_count, MY_INT_WIDTH, format_intx,
	      mc->m_write_rand_count);
    snprintf (emc->m_write_rand_string, OP_STR_LEN, format_opstr,
	      mc->m_write_rand_string);
    snprintf (emc->m_rmw_count, MY_INT_WIDTH, format_intx, mc->m_rmw_count);
    snprintf (emc->m_rmw_string, OP_STR_LEN, format_opstr, mc->m_rmw_string);
    snprintf (emc->m_mkdir_count, MY_INT_WIDTH, format_intx,
	      mc->m_mkdir_count);
    snprintf (emc->m_mkdir_string, OP_STR_LEN, format_opstr,
	      mc->m_mkdir_string);
    snprintf (emc->m_rmdir_count, MY_INT_WIDTH, format_intx,
	      mc->m_rmdir_count);
    snprintf (emc->m_rmdir_string, OP_STR_LEN, format_opstr,
	      mc->m_rmdir_string);
    snprintf (emc->m_unlink_count, MY_INT_WIDTH, format_intx,
	      mc->m_unlink_count);
    snprintf (emc->m_unlink_string, OP_STR_LEN, format_opstr,
	      mc->m_unlink_string);
    snprintf (emc->m_unlink2_count, MY_INT_WIDTH, format_intx,
	      mc->m_unlink2_count);
    snprintf (emc->m_unlink2_string, OP_STR_LEN, format_opstr,
	      mc->m_unlink2_string);
    snprintf (emc->m_create_count, MY_INT_WIDTH, format_intx,
	      mc->m_create_count);
    snprintf (emc->m_create_string, OP_STR_LEN, format_opstr,
	      mc->m_create_string);
    snprintf (emc->m_append_count, MY_INT_WIDTH, format_intx,
	      mc->m_append_count);
    snprintf (emc->m_append_string, OP_STR_LEN, format_opstr,
	      mc->m_append_string);
    snprintf (emc->m_locking_count, MY_INT_WIDTH, format_intx,
	      mc->m_locking_count);
    snprintf (emc->m_locking_string, OP_STR_LEN, format_opstr,
	      mc->m_locking_string);
    snprintf (emc->m_access_count, MY_INT_WIDTH, format_intx,
	      mc->m_access_count);
    snprintf (emc->m_access_string, OP_STR_LEN, format_opstr,
	      mc->m_access_string);
    snprintf (emc->m_chmod_count, MY_INT_WIDTH, format_intx,
	      mc->m_chmod_count);
    snprintf (emc->m_chmod_string, OP_STR_LEN, format_opstr,
	      mc->m_chmod_string);
    snprintf (emc->m_readdir_count, MY_INT_WIDTH, format_intx,
	      mc->m_readdir_count);
    snprintf (emc->m_readdir_string, OP_STR_LEN, format_opstr,
	      mc->m_readdir_string);
    snprintf (emc->m_stat_count, MY_INT_WIDTH, format_intx, mc->m_stat_count);
    snprintf (emc->m_stat_string, OP_STR_LEN, format_opstr,
	      mc->m_stat_string);
    snprintf (emc->m_neg_stat_count, MY_INT_WIDTH, format_intx,
	      mc->m_neg_stat_count);
    snprintf (emc->m_neg_stat_string, OP_STR_LEN, format_opstr,
	      mc->m_neg_stat_string);
    snprintf (emc->m_copyfile_count, MY_INT_WIDTH, format_intx,
	      mc->m_copyfile_count);
    snprintf (emc->m_copyfile_string, OP_STR_LEN, format_opstr,
	      mc->m_copyfile_string);
    snprintf (emc->m_rename_count, MY_INT_WIDTH, format_intx,
	      mc->m_rename_count);
    snprintf (emc->m_rename_string, OP_STR_LEN, format_opstr,
	      mc->m_rename_string);
    snprintf (emc->m_statfs_count, MY_INT_WIDTH, format_intx,
	      mc->m_statfs_count);
    snprintf (emc->m_statfs_string, OP_STR_LEN, format_opstr,
	      mc->m_statfs_string);
    snprintf (emc->m_pathconf_count, MY_INT_WIDTH, format_intx,
	      mc->m_pathconf_count);
    snprintf (emc->m_pathconf_string, OP_STR_LEN, format_opstr,
	      mc->m_pathconf_string);
    snprintf (emc->m_trunc_count, MY_INT_WIDTH, format_intx,
	      mc->m_trunc_count);
    snprintf (emc->m_trunc_string, OP_STR_LEN, format_opstr,
	      mc->m_trunc_string);
    snprintf (emc->m_custom1_count, MY_INT_WIDTH, format_intx,
	      mc->m_custom1_count);
    snprintf (emc->m_custom1_string, OP_STR_LEN, format_opstr,
	      mc->m_custom1_string);
    snprintf (emc->m_custom2_count, MY_INT_WIDTH, format_intx,
	      mc->m_custom2_count);
    snprintf (emc->m_custom2_string, OP_STR_LEN, format_opstr,
	      mc->m_custom2_string);
    snprintf (emc->m_heartbeat_type, MY_INT_WIDTH, format_intx,
	      mc->m_heartbeat_type);
    snprintf (emc->m_modified_run, MY_INT_WIDTH, format_intx,
	      mc->m_modified_run);
    snprintf (emc->m_background, MY_INT_WIDTH, format_intx, mc->m_background);
    snprintf (emc->m_sharemode, MY_INT_WIDTH, format_intx, mc->m_sharemode);
    snprintf (emc->m_uniform_file_size_dist, MY_INT_WIDTH, format_intx,
	      mc->m_uniform_file_size_dist);
    snprintf (emc->m_rand_dist_behavior, MY_INT_WIDTH, format_intx,
	      mc->m_rand_dist_behavior);
    snprintf (emc->m_cipher, MY_INT_WIDTH, format_intx, mc->m_cipher);
    snprintf (emc->m_notify, MY_INT_WIDTH, format_intx, mc->m_notify);
    snprintf (emc->m_lru_on, MY_INT_WIDTH, format_intx, mc->m_lru_on);

    if (op_lat_flag)
    {
	snprintf (emc->m_read_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_read_time);
	snprintf (emc->m_min_read_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_read_latency);
	snprintf (emc->m_max_read_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_read_latency);

	snprintf (emc->m_read_file_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_read_file_time);
	snprintf (emc->m_min_read_file_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_read_file_latency);
	snprintf (emc->m_max_read_file_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_read_file_latency);

	snprintf (emc->m_read_rand_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_read_rand_time);
	snprintf (emc->m_min_read_rand_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_read_rand_latency);
	snprintf (emc->m_max_read_rand_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_read_rand_latency);

	snprintf (emc->m_write_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_write_time);
	snprintf (emc->m_min_write_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_write_latency);
	snprintf (emc->m_max_write_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_write_latency);

	snprintf (emc->m_write_file_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_write_file_time);
	snprintf (emc->m_min_write_file_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_write_file_latency);
	snprintf (emc->m_max_write_file_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_write_file_latency);

	snprintf (emc->m_mmap_write_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_mmap_write_time);
	snprintf (emc->m_min_mmap_write_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_mmap_write_latency);
	snprintf (emc->m_max_mmap_write_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_mmap_write_latency);

	snprintf (emc->m_mmap_read_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_mmap_read_time);
	snprintf (emc->m_min_mmap_read_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_mmap_read_latency);
	snprintf (emc->m_max_mmap_read_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_mmap_read_latency);

	snprintf (emc->m_write_rand_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_write_rand_time);
	snprintf (emc->m_min_write_rand_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_min_write_rand_latency);
	snprintf (emc->m_max_write_rand_latency, MY_DOUBLE_WIDTH,
		  format_double, mc->m_max_write_rand_latency);

	snprintf (emc->m_rmw_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_rmw_time);
	snprintf (emc->m_min_rmw_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_rmw_latency);
	snprintf (emc->m_max_rmw_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_rmw_latency);

	snprintf (emc->m_mkdir_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_mkdir_time);
	snprintf (emc->m_min_mkdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_mkdir_latency);
	snprintf (emc->m_max_mkdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_mkdir_latency);

	snprintf (emc->m_rmdir_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_rmdir_time);
	snprintf (emc->m_min_rmdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_rmdir_latency);
	snprintf (emc->m_max_rmdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_rmdir_latency);

	snprintf (emc->m_unlink_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_unlink_time);
	snprintf (emc->m_min_unlink_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_unlink_latency);
	snprintf (emc->m_max_unlink_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_unlink_latency);

	snprintf (emc->m_unlink2_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_unlink2_time);
	snprintf (emc->m_min_unlink2_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_unlink2_latency);
	snprintf (emc->m_max_unlink2_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_unlink2_latency);

	snprintf (emc->m_create_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_create_time);
	snprintf (emc->m_min_create_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_create_latency);
	snprintf (emc->m_max_create_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_create_latency);

	snprintf (emc->m_append_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_append_time);
	snprintf (emc->m_min_append_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_append_latency);
	snprintf (emc->m_max_append_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_append_latency);

	snprintf (emc->m_locking_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_locking_time);
	snprintf (emc->m_min_locking_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_locking_latency);
	snprintf (emc->m_max_locking_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_locking_latency);

	snprintf (emc->m_access_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_access_time);
	snprintf (emc->m_min_access_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_access_latency);
	snprintf (emc->m_max_access_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_access_latency);

	snprintf (emc->m_chmod_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_chmod_time);
	snprintf (emc->m_min_chmod_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_chmod_latency);
	snprintf (emc->m_max_chmod_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_chmod_latency);

	snprintf (emc->m_readdir_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_readdir_time);
	snprintf (emc->m_min_readdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_readdir_latency);
	snprintf (emc->m_max_readdir_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_readdir_latency);

	snprintf (emc->m_stat_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_stat_time);
	snprintf (emc->m_min_stat_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_stat_latency);
	snprintf (emc->m_max_stat_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_stat_latency);

	snprintf (emc->m_neg_stat_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_neg_stat_time);
	snprintf (emc->m_min_neg_stat_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_neg_stat_latency);
	snprintf (emc->m_max_neg_stat_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_neg_stat_latency);

	snprintf (emc->m_copyfile_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_copyfile_time);
	snprintf (emc->m_min_copyfile_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_copyfile_latency);
	snprintf (emc->m_max_copyfile_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_copyfile_latency);

	snprintf (emc->m_rename_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_rename_time);
	snprintf (emc->m_min_rename_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_rename_latency);
	snprintf (emc->m_max_rename_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_rename_latency);

	snprintf (emc->m_statfs_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_statfs_time);
	snprintf (emc->m_min_statfs_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_statfs_latency);
	snprintf (emc->m_max_statfs_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_statfs_latency);

	snprintf (emc->m_pathconf_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_pathconf_time);
	snprintf (emc->m_min_pathconf_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_pathconf_latency);
	snprintf (emc->m_max_pathconf_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_pathconf_latency);

	snprintf (emc->m_trunc_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_trunc_time);
	snprintf (emc->m_min_trunc_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_trunc_latency);
	snprintf (emc->m_max_trunc_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_trunc_latency);

	snprintf (emc->m_custom1_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_custom1_time);
	snprintf (emc->m_min_custom1_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_custom1_latency);
	snprintf (emc->m_max_custom1_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_custom1_latency);

	snprintf (emc->m_custom2_time, MY_DOUBLE_WIDTH, format_double,
		  mc->m_custom2_time);
	snprintf (emc->m_min_custom2_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_min_custom2_latency);
	snprintf (emc->m_max_custom2_latency, MY_DOUBLE_WIDTH, format_double,
		  mc->m_max_custom2_latency);

	for (u = 0; u < BUCKETS; u++)
	{
	    snprintf (&emc->m_bands[u][0], MY_LONG_LONG_WIDTH,
		      format_longlongd, mc->m_bands[u]);

	    log_file (LOG_DEBUG, "c->m Band[%d] = %s\n", u,
		      &emc->m_bands[u][0]);
	}
    }
    unlock_logging();
}

/**
 * @brief Function to convert neutral formatted string data 
 *        from the client to an internal representation on the 
 *        prime.
 *        prime_ext_to_int() is used by the Prime to convert 
 *        the network neutral formatted message from the 
 *        remote client into the Prime's native binary 
 *        representation.
 * @param mc  : Pointer to Prime_command internal format
 * @param emc : Pointer to Prime_command external format
 * @param op_lat_flag : Op_latency_flag.. enable op latency reporting.
 */
/*
 * __doc__
 * __doc__  Function : void prime_ext_to_int(struct prime_command *mc, 
 * __doc__                     struct ext_prime_command *emc, int op_lat_flag, 
 * __doc__                     int mdebug)
 * __doc__  Arguments: struct prime_command *mc: Interal command format
 * __doc__             struct ext_prime_command *emc: External command format
 * __doc__             int op_lat_flag: Enables latency collection.
 * __doc__             int mdebug: Enables prime debug.
 * __doc__  Returns  : void
 * __doc__  Performs : Function to convert neutral formatted string data 
 * __doc__             from the client to an internal representation on the 
 * __doc__             prime.
 * __doc__             prime_ext_to_int() is used by the Prime to convert 
 * __doc__             the network neutral formatted message from the 
 * __doc__             remote client into the Prime's native binary 
 * __doc__             representation.
 * __doc__
 * __note__          : Note: If one changes the value of OP_STR_LEN or any 
 * __note__            of the sizes of the variables in the string based 
 * __note__            structures ext_prime_command, or ext_client_command,
 * __note__            then the lengths would need to be adjusted in the 
 * __note__            fields below.
 * __doc__             
 */
void
prime_ext_to_int (struct prime_command *mc, struct ext_prime_command *emc,
		  int op_lat_flag)
{
    int u;
    char format_intx[MY_MAX_NUM_TYPE_LEN], format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN],
	format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN], format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and 
     * sscanf routines.  This ensures that the functions do not 
     * accidentally over write the fields in the structures. The defines 
     * can be changed, if needed, and all of the space and the routines 
     * that touch that space are in sync.
     */
    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    (void) sscanf (emc->magic, format_intu, &mc->magic);
    (void) sscanf (emc->crc32, format_intu, &mc->crc32);

    my_strncpy (mc->m_host_name, emc->m_host_name, MAXHNAME);
    my_strncpy (mc->m_client_name, emc->m_client_name, MAXHNAME);
    (void) sscanf (emc->m_client_eval, format_intx, &mc->m_client_eval);
    (void) sscanf (emc->m_client_number, format_intd, &mc->m_client_number);
    (void) sscanf (emc->m_nm_number, format_intd, &mc->m_nm_number);
    (void) sscanf (emc->m_work_obj_index, format_intd, &mc->m_work_obj_index);
    (void) sscanf (emc->m_child_port, format_intx, &mc->m_child_port);
    (void) sscanf (emc->m_child_pid, format_intx, &mc->m_child_pid);
    (void) sscanf (emc->m_child_flag, format_intx, &mc->m_child_flag);
    (void) sscanf (emc->m_command, format_intx, &mc->m_command);
    (void) sscanf (emc->m_percent_complete, format_intd,
		   &mc->m_percent_complete);
    /* now for the results */
    my_strncpy (mc->m_client_name, emc->m_client_name, MAXHNAME);
    my_strncpy (mc->m_workload_name, emc->m_workload_name, MAXWLNAME);
    (void) sscanf (emc->m_run_time, format_longd, &mc->m_run_time);
    (void) sscanf (emc->m_min_direct_size, format_longd,
		   &mc->m_min_direct_size);
    /* Yes, %lf is really needed for doubles!!! */
    (void) sscanf (emc->m_total_file_ops, format_double,
		   &mc->m_total_file_ops);
    (void) sscanf (emc->m_total_file_op_time, format_double,
		   &mc->m_total_file_op_time);
    (void) sscanf (emc->m_ops_per_second, format_double,
		   &mc->m_ops_per_second);
    (void) sscanf (emc->m_average_latency, format_double,
		   &mc->m_average_latency);
    (void) sscanf (emc->m_read_throughput, format_double,
		   &mc->m_read_throughput);
    (void) sscanf (emc->m_read_kbytes, format_double, &mc->m_read_kbytes);
    (void) sscanf (emc->m_write_throughput, format_double,
		   &mc->m_write_throughput);
    (void) sscanf (emc->m_write_kbytes, format_double, &mc->m_write_kbytes);
    (void) sscanf (emc->m_Nread_throughput, format_double,
		   &mc->m_Nread_throughput);
    (void) sscanf (emc->m_Nread_kbytes, format_double, &mc->m_Nread_kbytes);
    (void) sscanf (emc->m_Nwrite_throughput, format_double,
		   &mc->m_Nwrite_throughput);
    (void) sscanf (emc->m_Nwrite_kbytes, format_double, &mc->m_Nwrite_kbytes);
    (void) sscanf (emc->m_meta_r_kbytes, format_double, &mc->m_meta_r_kbytes);
    (void) sscanf (emc->m_meta_w_kbytes, format_double, &mc->m_meta_w_kbytes);
    (void) sscanf (emc->m_init_files, format_intx, &mc->m_init_files);
    (void) sscanf (emc->m_init_files_ws, format_intx, &mc->m_init_files_ws);
    (void) sscanf (emc->m_init_dirs, format_intx, &mc->m_init_dirs);
    (void) sscanf (emc->m_file_space_mb, format_double, &mc->m_file_space_mb);
    (void) sscanf (emc->m_current_op_rate, format_double,
		   &mc->m_current_op_rate);
    /* Yes, %lf is really needed for doubles!!! */
    (void) sscanf (emc->m_read_count, format_intx, &mc->m_read_count);
    (void) sscanf (emc->m_read_string, format_opstr, mc->m_read_string);
    (void) sscanf (emc->m_read_file_count, format_intx,
		   &mc->m_read_file_count);
    (void) sscanf (emc->m_read_file_string, format_opstr,
		   mc->m_read_file_string);
    (void) sscanf (emc->m_read_rand_count, format_intx,
		   &mc->m_read_rand_count);
    (void) sscanf (emc->m_read_rand_string, format_opstr,
		   mc->m_read_rand_string);
    (void) sscanf (emc->m_write_count, format_intx, &mc->m_write_count);
    (void) sscanf (emc->m_write_string, format_opstr, mc->m_write_string);
    (void) sscanf (emc->m_write_file_count, format_intx,
		   &mc->m_write_file_count);
    (void) sscanf (emc->m_write_file_string, format_opstr,
		   mc->m_write_file_string);
    (void) sscanf (emc->m_open_count, format_intx, &mc->m_open_count);
    (void) sscanf (emc->m_close_count, format_intx, &mc->m_close_count);
    (void) sscanf (emc->m_mmap_write_count, format_intx,
		   &mc->m_mmap_write_count);
    (void) sscanf (emc->m_mmap_write_string, format_opstr,
		   mc->m_mmap_write_string);
    (void) sscanf (emc->m_mmap_read_count, format_intx,
		   &mc->m_mmap_read_count);
    (void) sscanf (emc->m_mmap_read_string, format_opstr,
		   mc->m_mmap_read_string);
    (void) sscanf (emc->m_write_rand_count, format_intx,
		   &mc->m_write_rand_count);
    (void) sscanf (emc->m_write_rand_string, format_opstr,
		   mc->m_write_rand_string);
    (void) sscanf (emc->m_rmw_count, format_intx, &mc->m_rmw_count);
    (void) sscanf (emc->m_rmw_string, format_opstr, mc->m_rmw_string);
    (void) sscanf (emc->m_mkdir_count, format_intx, &mc->m_mkdir_count);
    (void) sscanf (emc->m_mkdir_string, format_opstr, mc->m_mkdir_string);
    (void) sscanf (emc->m_rmdir_count, format_intx, &mc->m_rmdir_count);
    (void) sscanf (emc->m_rmdir_string, format_opstr, mc->m_rmdir_string);
    (void) sscanf (emc->m_unlink_count, format_intx, &mc->m_unlink_count);
    (void) sscanf (emc->m_unlink_string, format_opstr, mc->m_unlink_string);
    (void) sscanf (emc->m_unlink2_count, format_intx, &mc->m_unlink2_count);
    (void) sscanf (emc->m_unlink2_string, format_opstr, mc->m_unlink2_string);
    (void) sscanf (emc->m_create_count, format_intx, &mc->m_create_count);
    (void) sscanf (emc->m_create_string, format_opstr, mc->m_create_string);
    (void) sscanf (emc->m_append_count, format_intx, &mc->m_append_count);
    (void) sscanf (emc->m_append_string, format_opstr, mc->m_append_string);
    (void) sscanf (emc->m_locking_count, format_intx, &mc->m_locking_count);
    (void) sscanf (emc->m_locking_string, format_opstr, mc->m_locking_string);
    (void) sscanf (emc->m_access_count, format_intx, &mc->m_access_count);
    (void) sscanf (emc->m_access_string, format_opstr, mc->m_access_string);
    (void) sscanf (emc->m_chmod_count, format_intx, &mc->m_chmod_count);
    (void) sscanf (emc->m_chmod_string, format_opstr, mc->m_chmod_string);
    (void) sscanf (emc->m_readdir_count, format_intx, &mc->m_readdir_count);
    (void) sscanf (emc->m_readdir_string, format_opstr, mc->m_readdir_string);
    (void) sscanf (emc->m_stat_count, format_intx, &mc->m_stat_count);
    (void) sscanf (emc->m_stat_string, format_opstr, mc->m_stat_string);
    (void) sscanf (emc->m_neg_stat_count, format_intx, &mc->m_neg_stat_count);
    (void) sscanf (emc->m_neg_stat_string, format_opstr,
		   mc->m_neg_stat_string);
    (void) sscanf (emc->m_copyfile_count, format_intx, &mc->m_copyfile_count);
    (void) sscanf (emc->m_copyfile_string, format_opstr,
		   mc->m_copyfile_string);
    (void) sscanf (emc->m_rename_count, format_intx, &mc->m_rename_count);
    (void) sscanf (emc->m_rename_string, format_opstr, mc->m_rename_string);
    (void) sscanf (emc->m_statfs_count, format_intx, &mc->m_statfs_count);
    (void) sscanf (emc->m_statfs_string, format_opstr, mc->m_statfs_string);
    (void) sscanf (emc->m_pathconf_count, format_intx, &mc->m_pathconf_count);
    (void) sscanf (emc->m_pathconf_string, format_opstr,
		   mc->m_pathconf_string);
    (void) sscanf (emc->m_trunc_count, format_intx, &mc->m_trunc_count);
    (void) sscanf (emc->m_trunc_string, format_opstr, mc->m_trunc_string);
    (void) sscanf (emc->m_custom1_count, format_intx, &mc->m_custom1_count);
    (void) sscanf (emc->m_custom1_string, format_opstr, mc->m_custom1_string);
    (void) sscanf (emc->m_custom2_count, format_intx, &mc->m_custom2_count);
    (void) sscanf (emc->m_custom2_string, format_opstr, mc->m_custom2_string);
    (void) sscanf (emc->m_heartbeat_type, format_intx, &mc->m_heartbeat_type);
    (void) sscanf (emc->m_modified_run, format_intx, &mc->m_modified_run);
    (void) sscanf (emc->m_background, format_intx, &mc->m_background);
    (void) sscanf (emc->m_sharemode, format_intx, &mc->m_sharemode);
    (void) sscanf (emc->m_uniform_file_size_dist, format_intx,
		   &mc->m_uniform_file_size_dist);
    (void) sscanf (emc->m_rand_dist_behavior, format_intx,
		   &mc->m_rand_dist_behavior);
    (void) sscanf (emc->m_cipher, format_intx, &mc->m_cipher);
    (void) sscanf (emc->m_notify, format_intx, &mc->m_notify);
    (void) sscanf (emc->m_lru_on, format_intx, &mc->m_lru_on);

    if (op_lat_flag)
    {
	(void) sscanf (emc->m_read_time, format_double, &mc->m_read_time);
	(void) sscanf (emc->m_min_read_latency, format_double,
		       &mc->m_min_read_latency);
	(void) sscanf (emc->m_max_read_latency, format_double,
		       &mc->m_max_read_latency);

	(void) sscanf (emc->m_read_file_time, format_double,
		       &mc->m_read_file_time);
	(void) sscanf (emc->m_min_read_file_latency, format_double,
		       &mc->m_min_read_file_latency);
	(void) sscanf (emc->m_max_read_file_latency, format_double,
		       &mc->m_max_read_file_latency);

	(void) sscanf (emc->m_read_rand_time, format_double,
		       &mc->m_read_rand_time);
	(void) sscanf (emc->m_min_read_rand_latency, format_double,
		       &mc->m_min_read_rand_latency);
	(void) sscanf (emc->m_max_read_rand_latency, format_double,
		       &mc->m_max_read_rand_latency);

	(void) sscanf (emc->m_write_time, format_double, &mc->m_write_time);
	(void) sscanf (emc->m_min_write_latency, format_double,
		       &mc->m_min_write_latency);
	(void) sscanf (emc->m_max_write_latency, format_double,
		       &mc->m_max_write_latency);

	(void) sscanf (emc->m_write_file_time, format_double,
		       &mc->m_write_file_time);
	(void) sscanf (emc->m_min_write_file_latency, format_double,
		       &mc->m_min_write_file_latency);
	(void) sscanf (emc->m_max_write_file_latency, format_double,
		       &mc->m_max_write_file_latency);

	(void) sscanf (emc->m_mmap_write_time, format_double,
		       &mc->m_mmap_write_time);
	(void) sscanf (emc->m_min_mmap_write_latency, format_double,
		       &mc->m_min_mmap_write_latency);
	(void) sscanf (emc->m_max_mmap_write_latency, format_double,
		       &mc->m_max_mmap_write_latency);

	(void) sscanf (emc->m_mmap_read_time, format_double,
		       &mc->m_mmap_read_time);
	(void) sscanf (emc->m_min_mmap_read_latency, format_double,
		       &mc->m_min_mmap_read_latency);
	(void) sscanf (emc->m_max_mmap_read_latency, format_double,
		       &mc->m_max_mmap_read_latency);

	(void) sscanf (emc->m_write_rand_time, format_double,
		       &mc->m_write_rand_time);
	(void) sscanf (emc->m_min_write_rand_latency, format_double,
		       &mc->m_min_write_rand_latency);
	(void) sscanf (emc->m_max_write_rand_latency, format_double,
		       &mc->m_max_write_rand_latency);

	(void) sscanf (emc->m_rmw_time, format_double, &mc->m_rmw_time);
	(void) sscanf (emc->m_min_rmw_latency, format_double,
		       &mc->m_min_rmw_latency);
	(void) sscanf (emc->m_max_rmw_latency, format_double,
		       &mc->m_max_rmw_latency);

	(void) sscanf (emc->m_mkdir_time, format_double, &mc->m_mkdir_time);
	(void) sscanf (emc->m_min_mkdir_latency, format_double,
		       &mc->m_min_mkdir_latency);
	(void) sscanf (emc->m_max_mkdir_latency, format_double,
		       &mc->m_max_mkdir_latency);

	(void) sscanf (emc->m_rmdir_time, format_double, &mc->m_rmdir_time);
	(void) sscanf (emc->m_min_rmdir_latency, format_double,
		       &mc->m_min_rmdir_latency);
	(void) sscanf (emc->m_max_rmdir_latency, format_double,
		       &mc->m_max_rmdir_latency);

	(void) sscanf (emc->m_unlink_time, format_double, &mc->m_unlink_time);
	(void) sscanf (emc->m_min_unlink_latency, format_double,
		       &mc->m_min_unlink_latency);
	(void) sscanf (emc->m_max_unlink_latency, format_double,
		       &mc->m_max_unlink_latency);

	(void) sscanf (emc->m_unlink2_time, format_double,
		       &mc->m_unlink2_time);
	(void) sscanf (emc->m_min_unlink2_latency, format_double,
		       &mc->m_min_unlink2_latency);
	(void) sscanf (emc->m_max_unlink2_latency, format_double,
		       &mc->m_max_unlink2_latency);

	(void) sscanf (emc->m_create_time, format_double, &mc->m_create_time);
	(void) sscanf (emc->m_min_create_latency, format_double,
		       &mc->m_min_create_latency);
	(void) sscanf (emc->m_max_create_latency, format_double,
		       &mc->m_max_create_latency);

	(void) sscanf (emc->m_append_time, format_double, &mc->m_append_time);
	(void) sscanf (emc->m_min_append_latency, format_double,
		       &mc->m_min_append_latency);
	(void) sscanf (emc->m_max_append_latency, format_double,
		       &mc->m_max_append_latency);

	(void) sscanf (emc->m_locking_time, format_double,
		       &mc->m_locking_time);
	(void) sscanf (emc->m_min_locking_latency, format_double,
		       &mc->m_min_locking_latency);
	(void) sscanf (emc->m_max_locking_latency, format_double,
		       &mc->m_max_locking_latency);

	(void) sscanf (emc->m_access_time, format_double, &mc->m_access_time);
	(void) sscanf (emc->m_min_access_latency, format_double,
		       &mc->m_min_access_latency);
	(void) sscanf (emc->m_max_access_latency, format_double,
		       &mc->m_max_access_latency);

	(void) sscanf (emc->m_chmod_time, format_double, &mc->m_chmod_time);
	(void) sscanf (emc->m_min_chmod_latency, format_double,
		       &mc->m_min_chmod_latency);
	(void) sscanf (emc->m_max_chmod_latency, format_double,
		       &mc->m_max_chmod_latency);

	(void) sscanf (emc->m_readdir_time, format_double,
		       &mc->m_readdir_time);
	(void) sscanf (emc->m_min_readdir_latency, format_double,
		       &mc->m_min_readdir_latency);
	(void) sscanf (emc->m_max_readdir_latency, format_double,
		       &mc->m_max_readdir_latency);

	(void) sscanf (emc->m_stat_time, format_double, &mc->m_stat_time);
	(void) sscanf (emc->m_min_stat_latency, format_double,
		       &mc->m_min_stat_latency);
	(void) sscanf (emc->m_max_stat_latency, format_double,
		       &mc->m_max_stat_latency);

	(void) sscanf (emc->m_neg_stat_time, format_double,
		       &mc->m_neg_stat_time);
	(void) sscanf (emc->m_min_neg_stat_latency, format_double,
		       &mc->m_min_neg_stat_latency);
	(void) sscanf (emc->m_max_neg_stat_latency, format_double,
		       &mc->m_max_neg_stat_latency);

	(void) sscanf (emc->m_copyfile_time, format_double,
		       &mc->m_copyfile_time);
	(void) sscanf (emc->m_min_copyfile_latency, format_double,
		       &mc->m_min_copyfile_latency);
	(void) sscanf (emc->m_max_copyfile_latency, format_double,
		       &mc->m_max_copyfile_latency);

	(void) sscanf (emc->m_rename_time, format_double, &mc->m_rename_time);
	(void) sscanf (emc->m_min_rename_latency, format_double,
		       &mc->m_min_rename_latency);
	(void) sscanf (emc->m_max_rename_latency, format_double,
		       &mc->m_max_rename_latency);

	(void) sscanf (emc->m_statfs_time, format_double, &mc->m_statfs_time);
	(void) sscanf (emc->m_min_statfs_latency, format_double,
		       &mc->m_min_statfs_latency);
	(void) sscanf (emc->m_max_statfs_latency, format_double,
		       &mc->m_max_statfs_latency);

	(void) sscanf (emc->m_pathconf_time, format_double,
		       &mc->m_pathconf_time);
	(void) sscanf (emc->m_min_pathconf_latency, format_double,
		       &mc->m_min_pathconf_latency);
	(void) sscanf (emc->m_max_pathconf_latency, format_double,
		       &mc->m_max_pathconf_latency);

	(void) sscanf (emc->m_trunc_time, format_double, &mc->m_trunc_time);
	(void) sscanf (emc->m_min_trunc_latency, format_double,
		       &mc->m_min_trunc_latency);
	(void) sscanf (emc->m_max_trunc_latency, format_double,
		       &mc->m_max_trunc_latency);

	(void) sscanf (emc->m_custom1_time, format_double,
		       &mc->m_custom1_time);
	(void) sscanf (emc->m_min_custom1_latency, format_double,
		       &mc->m_min_custom1_latency);
	(void) sscanf (emc->m_max_custom1_latency, format_double,
		       &mc->m_max_custom1_latency);

	(void) sscanf (emc->m_custom2_time, format_double,
		       &mc->m_custom2_time);
	(void) sscanf (emc->m_min_custom2_latency, format_double,
		       &mc->m_min_custom2_latency);
	(void) sscanf (emc->m_max_custom2_latency, format_double,
		       &mc->m_max_custom2_latency);

	for (u = 0; u < BUCKETS; u++)
	{
	    (void) sscanf (&emc->m_bands[u][0], format_longlongd,
			   &mc->m_bands[u]);

	    log_file (LOG_DEBUG, "M Band[%d] = %19lld\n", u, mc->m_bands[u]);
	}
    }
    unlock_logging();
}


/**
 * @brief Function to convert neutral formatted string data 
 *        from the nodeManager internal representation to external format
 * @param nmc  : Pointer to nodeManager internal format
 * @param enmc : Pointer to nodeManager external format
 * @param num_workload_obj : number of workload objects
 */
void
nodeManager_int_to_ext (struct nodeManager_command *nmc,
			struct ext_nodeManager_command *enmc,
			int num_workload_obj)
{
    int i, j;
    char format_intx[MY_MAX_NUM_TYPE_LEN];
    char format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN];
    char format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN];
    char format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and 
     * sscanf routines.  This ensures that the functions do not 
     * accidentally over write the fields in the structures. The defines 
     * can be changed, if needed, and all of the space and the routines 
     * that touch that space are in sync.
     */

    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    snprintf (enmc->magic, MY_INT_WIDTH, format_intu, nmc->magic);
    snprintf (enmc->crc32, MY_INT_WIDTH, format_intu, nmc->crc32);

    snprintf (enmc->nm_command, MY_INT_WIDTH, format_intd, nmc->nm_command);
    snprintf (enmc->nm_num_of_children, MY_INT_WIDTH, format_intd,
	      nmc->nm_num_of_children);
    snprintf (enmc->nm_runtime, MY_INT_WIDTH, format_intd, nmc->nm_runtime);
    snprintf (enmc->nm_workload_count, MY_INT_WIDTH, format_intd,
	      nmc->nm_workload_count);
    snprintf (enmc->nm_ipv6_enable, MY_INT_WIDTH, format_intd,
	      nmc->nm_ipv6_enable);
    my_strncpy (enmc->nm_client_log_dir, nmc->nm_client_log_dir, MAXNAME);
    my_strncpy (enmc->nm_client_windows_log_dir,
		nmc->nm_client_windows_log_dir, MAXNAME);

    for (i = 0; i < nmc->nm_num_of_children; i++)
    {
	my_strncpy ((enmc->nm_workload_name[i]), nmc->nm_workload_name[i],
		    MAXWLNAME);
	snprintf (enmc->nm_my_oprate[i], MY_DOUBLE_WIDTH, format_double,
		  nmc->nm_my_oprate[i]);
	snprintf ((enmc->nm_children_ids[i]), MY_INT_WIDTH, format_intd,
		  nmc->nm_children_ids[i]);
	snprintf ((enmc->nm_client_files[i]), MY_INT_WIDTH, format_intd,
		  nmc->nm_client_files[i]);
	snprintf ((enmc->nm_client_dirs[i]), MY_INT_WIDTH, format_intd,
		  nmc->nm_client_dirs[i]);
	snprintf ((enmc->nm_file_size[i]), MY_INT_WIDTH, format_intd,
		  nmc->nm_file_size[i]);
	my_strncpy ((enmc->nm_workdir[i]), nmc->nm_workdir[i], MAXNAME);
    }

    my_strncpy (enmc->c_host_name, nmc->c_host_name, MAXHNAME);
    my_strncpy (enmc->c_pit_hostname, nmc->c_pit_hostname, PIT_NAME_LEN);
    my_strncpy (enmc->c_pit_service, nmc->c_pit_service, PIT_SERV_LEN);
    my_strncpy (enmc->c_client_name, nmc->c_client_name, MAXHNAME);
    my_strncpy (enmc->c_unix_pdsm_file, nmc->c_unix_pdsm_file, MAXNAME);
    my_strncpy (enmc->c_unix_pdsm_control_file, nmc->c_unix_pdsm_control_file,
		MAXNAME);
    my_strncpy (enmc->c_windows_pdsm_file, nmc->c_windows_pdsm_file, MAXNAME);
    my_strncpy (enmc->c_windows_pdsm_control_file,
		nmc->c_windows_pdsm_control_file, MAXNAME);
    my_strncpy (enmc->c_client_version, nmc->c_client_version, VERS_WIDTH);
    snprintf (enmc->c_start_epoch, MY_DOUBLE_WIDTH, format_double,
	      nmc->c_start_epoch);
    snprintf (enmc->c_prime_eclock_time, MY_DOUBLE_WIDTH, format_double,
	      nmc->c_prime_eclock_time);
    snprintf (enmc->c_gg_offset, MY_INT_WIDTH, format_intd, nmc->c_gg_offset);
    snprintf (enmc->c_gg_flag, MY_INT_WIDTH, format_intd, nmc->c_gg_flag);
    snprintf (enmc->c_workload_count, MY_INT_WIDTH, format_intd,
	      nmc->c_workload_count);
    snprintf (enmc->c_client_number, MY_INT_WIDTH, format_intd,
	      nmc->c_client_number);
    snprintf (enmc->c_pdsm_interval, MY_INT_WIDTH, format_intd,
	      nmc->c_pdsm_interval);
    snprintf (enmc->c_pdsm_mode, MY_INT_WIDTH, format_intd, nmc->c_pdsm_mode);
    snprintf (enmc->c_child_flag, MY_INT_WIDTH, format_intd,
	      nmc->c_child_flag);
    snprintf (enmc->c_flush_flag, MY_INT_WIDTH, format_intd,
	      nmc->c_flush_flag);
    snprintf (enmc->c_unlink2_no_recreate, MY_INT_WIDTH, format_intd,
	      nmc->c_unlink2_no_recreate);
    snprintf (enmc->c_cleanup, MY_INT_WIDTH, format_intd, nmc->c_cleanup);
    snprintf (enmc->c_dump_files_flag, MY_INT_WIDTH, format_intd,
	      nmc->c_dump_files_flag);
    snprintf (enmc->c_do_validate, MY_INT_WIDTH, format_intd,
	      nmc->c_do_validate);
    snprintf (enmc->c_ipv6_enable, MY_INT_WIDTH, format_intd,
	      nmc->c_ipv6_enable);
    snprintf (enmc->c_licensed, MY_INT_WIDTH, format_intd, nmc->c_licensed);
    snprintf (enmc->c_heartbeat, MY_INT_WIDTH, format_intd, nmc->c_heartbeat);
    snprintf (enmc->c_client_files, MY_INT_WIDTH, format_intd,
	      nmc->c_client_files);
    snprintf (enmc->c_client_dirs, MY_INT_WIDTH, format_intd,
	      nmc->c_client_dirs);
    snprintf (enmc->c_num_clients, MY_INT_WIDTH, format_intd,
	      nmc->c_num_clients);
    snprintf (enmc->c_fd_caching_limit, MY_INT_WIDTH, format_intd,
	      nmc->c_fd_caching_limit);
    snprintf (enmc->c_op_lat_flag, MY_INT_WIDTH, format_intd,
	      nmc->c_op_lat_flag);


    /* transpose OPs */

    for (j = 0; j < num_workload_obj; j++)
    {
	my_strncpy (enmc->ext_worker[j].c_workload_name,
		    nmc->client_worker[j].c_workload_name, MAXWLNAME);
	my_strncpy (enmc->ext_worker[j].c_fs_type,
		    nmc->client_worker[j].c_fs_type, MAXFSNAME);
	my_strncpy (enmc->ext_worker[j].c_platform_type,
		    nmc->client_worker[j].c_platform_type, MAXFSNAME);
	for (i = 0; i < NUM_OP_TYPES; i++)
	{
	    snprintf (enmc->ext_worker[j].c_op_array[i], MY_INT_WIDTH,
		      format_intd, nmc->client_worker[j].c_op_array[i]);
	    snprintf (enmc->ext_worker[j].c_op_array_string[i], OP_STR_LEN,
		      format_opstr,
		      nmc->client_worker[j].c_op_array_string[i]);
	}

	/* transpose xfer mixes */
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    snprintf (enmc->ext_worker[j].c_rdist_array_size_min[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_rdist_array[i].size_min);
	    snprintf (enmc->ext_worker[j].c_rdist_array_size_max[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_rdist_array[i].size_max);
	    snprintf (enmc->ext_worker[j].c_rdist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_rdist_array[i].percent);

	    snprintf (enmc->ext_worker[j].c_wdist_array_size_min[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_wdist_array[i].size_min);
	    snprintf (enmc->ext_worker[j].c_wdist_array_size_max[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_wdist_array[i].size_max);
	    snprintf (enmc->ext_worker[j].c_wdist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_wdist_array[i].percent);

	    snprintf (enmc->ext_worker[j].c_file_size_dist_array_size_min[i],
		      MY_LONG_LONG_WIDTH, format_longlongd,
		      nmc->client_worker[j].c_file_size_dist_array[i].
		      size_min);
	    snprintf (enmc->ext_worker[j].c_file_size_dist_array_size_max[i],
		      MY_LONG_LONG_WIDTH, format_longlongd,
		      nmc->client_worker[j].c_file_size_dist_array[i].
		      size_max);
	    snprintf (enmc->ext_worker[j].c_file_size_dist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      nmc->client_worker[j].c_file_size_dist_array[i].
		      percent);
	}

	snprintf (enmc->ext_worker[j].c_min_pre_name_length, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_min_pre_name_length);
	snprintf (enmc->ext_worker[j].c_max_pre_name_length, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_max_pre_name_length);
	snprintf (enmc->ext_worker[j].c_min_post_name_length, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_min_post_name_length);
	snprintf (enmc->ext_worker[j].c_max_post_name_length, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_max_post_name_length);
	snprintf (enmc->ext_worker[j].c_percent_commit, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_commit);
	snprintf (enmc->ext_worker[j].c_percent_direct, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_direct);
	snprintf (enmc->ext_worker[j].c_percent_fadvise_seq, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_fadvise_seq);
	snprintf (enmc->ext_worker[j].c_percent_fadvise_rand, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_fadvise_rand);
	snprintf (enmc->ext_worker[j].c_percent_fadvise_dont_need,
		  MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_percent_fadvise_dont_need);
	snprintf (enmc->ext_worker[j].c_percent_madvise_seq, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_madvise_seq);
	snprintf (enmc->ext_worker[j].c_percent_madvise_rand, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_madvise_rand);
	snprintf (enmc->ext_worker[j].c_percent_madvise_dont_need,
		  MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_percent_madvise_dont_need);
	snprintf (enmc->ext_worker[j].c_align, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_align);
	snprintf (enmc->ext_worker[j].c_percent_osync, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_osync);
	snprintf (enmc->ext_worker[j].c_percent_geometric, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_geometric);
	snprintf (enmc->ext_worker[j].c_percent_compress, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_compress);
	snprintf (enmc->ext_worker[j].c_percent_dedup, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_dedup);
	snprintf (enmc->ext_worker[j].c_percent_dedup_within, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_dedup_within);
	snprintf (enmc->ext_worker[j].c_percent_dedup_across, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_dedup_across);
	snprintf (enmc->ext_worker[j].c_dedup_group_count, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_dedup_group_count);
	snprintf (enmc->ext_worker[j].c_percent_per_spot, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_per_spot);
	snprintf (enmc->ext_worker[j].c_min_acc_per_spot, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_min_acc_per_spot);
	snprintf (enmc->ext_worker[j].c_acc_mult_spot, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_acc_mult_spot);
	snprintf (enmc->ext_worker[j].c_percent_affinity, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_percent_affinity);
	snprintf (enmc->ext_worker[j].c_spot_shape, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_spot_shape);
	snprintf (enmc->ext_worker[j].c_dedup_granule_size, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_dedup_granule_size);
	snprintf (enmc->ext_worker[j].c_dedup_gran_rep_limit, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_dedup_gran_rep_limit);
	snprintf (enmc->ext_worker[j].c_use_file_size_dist, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_use_file_size_dist);
	snprintf (enmc->ext_worker[j].c_comp_granule_size, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_comp_granule_size);
	snprintf (enmc->ext_worker[j].c_background, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_background);
	snprintf (enmc->ext_worker[j].c_sharemode, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_sharemode);
	snprintf (enmc->ext_worker[j].c_uniform_file_size_dist, MY_INT_WIDTH,
		  format_intd,
		  nmc->client_worker[j].c_uniform_file_size_dist);
	snprintf (enmc->ext_worker[j].c_rand_dist_behavior, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_rand_dist_behavior);
	snprintf (enmc->ext_worker[j].c_cipher, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_cipher);
	snprintf (enmc->ext_worker[j].c_notify, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_notify);
	snprintf (enmc->ext_worker[j].c_lru_on, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_lru_on);
	snprintf (enmc->ext_worker[j].c_patt_vers, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_patt_vers);
	snprintf (enmc->ext_worker[j].c_init_rate_enable, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_init_rate_enable);
	snprintf (enmc->ext_worker[j].c_init_rate_speed, MY_DOUBLE_WIDTH,
		  format_double, nmc->client_worker[j].c_init_rate_speed);
	snprintf (enmc->ext_worker[j].c_init_read, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_init_read);
	snprintf (enmc->ext_worker[j].c_shared_buckets, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_shared_buckets);
	snprintf (enmc->ext_worker[j].c_unlink2_no_recreate, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_unlink2_no_recreate);
	snprintf (enmc->ext_worker[j].c_extra_dir_levels, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_extra_dir_levels);
	snprintf (enmc->ext_worker[j].c_chaff_count, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_chaff_count);
	snprintf (enmc->ext_worker[j].c_files_per_dir, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_files_per_dir);
	snprintf (enmc->ext_worker[j].c_instances, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_instances);
	snprintf (enmc->ext_worker[j].c_op_rate, MY_DOUBLE_WIDTH,
		  format_double, nmc->client_worker[j].c_op_rate);
	snprintf (enmc->ext_worker[j].c_dir_count, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_dir_count);
	snprintf (enmc->ext_worker[j].c_warm_time, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_warm_time);
	snprintf (enmc->ext_worker[j].c_file_size, MY_INT_WIDTH, format_intd,
		  nmc->client_worker[j].c_file_size);
	snprintf (enmc->ext_worker[j].c_rel_version, MY_INT_WIDTH,
		  format_intd, nmc->client_worker[j].c_rel_version);
    }
    unlock_logging();
}

/**
 * @brief Function to convert neutral formatted string data 
 *        from the nodeManager external representation to internal format
 * @param enmc : Pointer to nodeManager external format
 * @param nmc  : Pointer to nodeManager internal format
 * @param num_workload_obj : number of workload objects
 */
void
nodeManager_ext_to_int (struct ext_nodeManager_command *enmc,
			struct nodeManager_command *nmc, int num_workload_obj)
{
    int i, j;
    char format_intx[MY_MAX_NUM_TYPE_LEN];
    char format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN];
    char format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN];
    char format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and 
     * sscanf routines.  This ensures that the functions do not 
     * accidentally over write the fields in the structures. The defines 
     * can be changed, if needed, and all of the space and the routines 
     * that touch that space are in sync.
     */
    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    (void) sscanf (enmc->magic, format_intu, &nmc->magic);
    (void) sscanf (enmc->crc32, format_intu, &nmc->crc32);

    (void) sscanf (enmc->nm_command, format_intd, &nmc->nm_command);
    (void) sscanf (enmc->nm_num_of_children, format_intd,
		   &nmc->nm_num_of_children);
    (void) sscanf (enmc->nm_runtime, format_intd, &nmc->nm_runtime);
/* Pre YAML. Have moved to per workload component warmup times.
    (void) sscanf (enmc->nm_warmtime, format_intd, &nmc->nm_warmtime);
*/
    (void) sscanf (enmc->nm_ipv6_enable, format_intd, &nmc->nm_ipv6_enable);
    my_strncpy (nmc->nm_client_log_dir, enmc->nm_client_log_dir, MAXNAME);
    my_strncpy (nmc->nm_client_windows_log_dir,
		enmc->nm_client_windows_log_dir, MAXNAME);

    for (i = 0; i < nmc->nm_num_of_children; i++)
    {
	my_strncpy ((nmc->nm_workload_name[i]),
		    enmc->nm_workload_name[i], MAXWLNAME);
	(void) sscanf (enmc->nm_my_oprate[i], format_double,
		       &nmc->nm_my_oprate[i]);
	(void) sscanf ((enmc->nm_children_ids[i]), format_intd,
		       &nmc->nm_children_ids[i]);
	(void) sscanf ((enmc->nm_client_files[i]), format_intd,
		       &nmc->nm_client_files[i]);
	(void) sscanf ((enmc->nm_client_dirs[i]), format_intd,
		       &nmc->nm_client_dirs[i]);
	(void) sscanf ((enmc->nm_file_size[i]), format_intd,
		       &nmc->nm_file_size[i]);
	my_strncpy ((nmc->nm_workdir[i]), enmc->nm_workdir[i], MAXNAME);
    }

    my_strncpy (nmc->c_host_name, enmc->c_host_name, MAXHNAME);
    my_strncpy (nmc->c_pit_hostname, enmc->c_pit_hostname, PIT_NAME_LEN);
    my_strncpy (nmc->c_pit_service, enmc->c_pit_service, PIT_SERV_LEN);
    my_strncpy (nmc->c_client_name, enmc->c_client_name, MAXHNAME);
    my_strncpy (nmc->c_unix_pdsm_file, enmc->c_unix_pdsm_file, MAXNAME);
    my_strncpy (nmc->c_unix_pdsm_control_file, enmc->c_unix_pdsm_control_file,
		MAXNAME);
    my_strncpy (nmc->c_windows_pdsm_file, enmc->c_windows_pdsm_file, MAXNAME);
    my_strncpy (nmc->c_windows_pdsm_control_file,
		enmc->c_windows_pdsm_control_file, MAXNAME);
    my_strncpy (nmc->c_client_version, enmc->c_client_version, VERS_WIDTH);
    (void) sscanf (enmc->c_start_epoch, format_double, &nmc->c_start_epoch);
    (void) sscanf (enmc->c_prime_eclock_time, format_double,
		   &nmc->c_prime_eclock_time);
    (void) sscanf (enmc->c_gg_offset, format_intd, &nmc->c_gg_offset);
    (void) sscanf (enmc->c_gg_flag, format_intd, &nmc->c_gg_flag);
    (void) sscanf (enmc->c_pdsm_mode, format_intd, &nmc->c_pdsm_mode);
    (void) sscanf (enmc->c_pdsm_interval, format_intd, &nmc->c_pdsm_interval);
    (void) sscanf (enmc->c_workload_count, format_intd,
		   &nmc->c_workload_count);
    (void) sscanf (enmc->c_client_number, format_intd, &nmc->c_client_number);
    (void) sscanf (enmc->c_child_flag, format_intd, &nmc->c_child_flag);
    (void) sscanf (enmc->c_flush_flag, format_intd, &nmc->c_flush_flag);
    (void) sscanf (enmc->c_unlink2_no_recreate, format_intd,
		   &nmc->c_unlink2_no_recreate);
    (void) sscanf (enmc->c_cleanup, format_intd, &nmc->c_cleanup);
    (void) sscanf (enmc->c_dump_files_flag, format_intd,
		   &nmc->c_dump_files_flag);
    (void) sscanf (enmc->c_do_validate, format_intd, &nmc->c_do_validate);
    (void) sscanf (enmc->c_ipv6_enable, format_intd, &nmc->c_ipv6_enable);
    (void) sscanf (enmc->c_licensed, format_intd, &nmc->c_licensed);
    (void) sscanf (enmc->c_heartbeat, format_intd, &nmc->c_heartbeat);
    (void) sscanf (enmc->c_client_files, format_intd, &nmc->c_client_files);
    (void) sscanf (enmc->c_client_dirs, format_intd, &nmc->c_client_dirs);
    (void) sscanf (enmc->c_num_clients, format_intd, &nmc->c_num_clients);
    (void) sscanf (enmc->c_fd_caching_limit, format_intd,
		   &nmc->c_fd_caching_limit);
    (void) sscanf (enmc->c_op_lat_flag, format_intd, &nmc->c_op_lat_flag);

    for (j = 0; j < num_workload_obj; j++)
    {
	my_strncpy (nmc->client_worker[j].c_workload_name,
		    enmc->ext_worker[j].c_workload_name, MAXWLNAME);
	my_strncpy (nmc->client_worker[j].c_fs_type,
		    enmc->ext_worker[j].c_fs_type, MAXFSNAME);
	my_strncpy (nmc->client_worker[j].c_platform_type,
		    enmc->ext_worker[j].c_platform_type, MAXFSNAME);
	/* Transpose the OPs */
	for (i = 0; i < NUM_OP_TYPES; i++)
	{
	    (void) sscanf (enmc->ext_worker[j].c_op_array[i], format_intd,
			   &nmc->client_worker[j].c_op_array[i]);
	    (void) sscanf (enmc->ext_worker[j].c_op_array_string[i],
			   format_opstr,
			   nmc->client_worker[j].c_op_array_string[i]);
	}
	/* Transpose the xfer mix */
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    (void) sscanf (enmc->ext_worker[j].c_rdist_array_size_min[i],
			   format_intd,
			   &nmc->client_worker[j].c_rdist_array[i].size_min);
	    (void) sscanf (enmc->ext_worker[j].c_rdist_array_size_max[i],
			   format_intd,
			   &nmc->client_worker[j].c_rdist_array[i].size_max);
	    (void) sscanf (enmc->ext_worker[j].c_rdist_array_percent[i],
			   format_intd,
			   &nmc->client_worker[j].c_rdist_array[i].percent);
	    (void) sscanf (enmc->ext_worker[j].c_wdist_array_size_min[i],
			   format_intd,
			   &nmc->client_worker[j].c_wdist_array[i].size_min);
	    (void) sscanf (enmc->ext_worker[j].c_wdist_array_size_max[i],
			   format_intd,
			   &nmc->client_worker[j].c_wdist_array[i].size_max);
	    (void) sscanf (enmc->ext_worker[j].c_wdist_array_percent[i],
			   format_intd,
			   &nmc->client_worker[j].c_wdist_array[i].percent);

	    (void) sscanf (enmc->ext_worker[j].
			   c_file_size_dist_array_size_min[i],
			   format_longlongd,
			   &nmc->client_worker[j].c_file_size_dist_array[i].
			   size_min);
	    (void) sscanf (enmc->ext_worker[j].
			   c_file_size_dist_array_size_max[i],
			   format_longlongd,
			   &nmc->client_worker[j].c_file_size_dist_array[i].
			   size_max);
	    (void) sscanf (enmc->ext_worker[j].
			   c_file_size_dist_array_percent[i], format_intd,
			   &nmc->client_worker[j].c_file_size_dist_array[i].
			   percent);
	}
	(void) sscanf (enmc->ext_worker[j].c_min_pre_name_length, format_intd,
		       &nmc->client_worker[j].c_min_pre_name_length);
	(void) sscanf (enmc->ext_worker[j].c_max_pre_name_length, format_intd,
		       &nmc->client_worker[j].c_max_pre_name_length);
	(void) sscanf (enmc->ext_worker[j].c_min_post_name_length,
		       format_intd,
		       &nmc->client_worker[j].c_min_post_name_length);
	(void) sscanf (enmc->ext_worker[j].c_max_post_name_length,
		       format_intd,
		       &nmc->client_worker[j].c_max_post_name_length);
	(void) sscanf (enmc->ext_worker[j].c_percent_commit, format_intd,
		       &nmc->client_worker[j].c_percent_commit);
	(void) sscanf (enmc->ext_worker[j].c_percent_direct, format_intd,
		       &nmc->client_worker[j].c_percent_direct);
	(void) sscanf (enmc->ext_worker[j].c_percent_fadvise_seq, format_intd,
		       &nmc->client_worker[j].c_percent_fadvise_seq);
	(void) sscanf (enmc->ext_worker[j].c_percent_fadvise_rand,
		       format_intd,
		       &nmc->client_worker[j].c_percent_fadvise_rand);
	(void) sscanf (enmc->ext_worker[j].c_percent_fadvise_dont_need,
		       format_intd,
		       &nmc->client_worker[j].c_percent_fadvise_dont_need);
	(void) sscanf (enmc->ext_worker[j].c_percent_madvise_seq, format_intd,
		       &nmc->client_worker[j].c_percent_madvise_seq);
	(void) sscanf (enmc->ext_worker[j].c_percent_madvise_rand,
		       format_intd,
		       &nmc->client_worker[j].c_percent_madvise_rand);
	(void) sscanf (enmc->ext_worker[j].c_percent_madvise_dont_need,
		       format_intd,
		       &nmc->client_worker[j].c_percent_madvise_dont_need);
	(void) sscanf (enmc->ext_worker[j].c_align, format_intd,
		       &nmc->client_worker[j].c_align);
	(void) sscanf (enmc->ext_worker[j].c_percent_osync, format_intd,
		       &nmc->client_worker[j].c_percent_osync);
	(void) sscanf (enmc->ext_worker[j].c_percent_geometric, format_intd,
		       &nmc->client_worker[j].c_percent_geometric);
	(void) sscanf (enmc->ext_worker[j].c_percent_compress, format_intd,
		       &nmc->client_worker[j].c_percent_compress);
	(void) sscanf (enmc->ext_worker[j].c_percent_dedup, format_intd,
		       &nmc->client_worker[j].c_percent_dedup);
	(void) sscanf (enmc->ext_worker[j].c_percent_dedup_within,
		       format_intd,
		       &nmc->client_worker[j].c_percent_dedup_within);
	(void) sscanf (enmc->ext_worker[j].c_percent_dedup_across,
		       format_intd,
		       &nmc->client_worker[j].c_percent_dedup_across);
	(void) sscanf (enmc->ext_worker[j].c_dedup_group_count, format_intd,
		       &nmc->client_worker[j].c_dedup_group_count);
	(void) sscanf (enmc->ext_worker[j].c_percent_per_spot, format_intd,
		       &nmc->client_worker[j].c_percent_per_spot);
	(void) sscanf (enmc->ext_worker[j].c_min_acc_per_spot, format_intd,
		       &nmc->client_worker[j].c_min_acc_per_spot);
	(void) sscanf (enmc->ext_worker[j].c_acc_mult_spot, format_intd,
		       &nmc->client_worker[j].c_acc_mult_spot);
	(void) sscanf (enmc->ext_worker[j].c_percent_affinity, format_intd,
		       &nmc->client_worker[j].c_percent_affinity);
	(void) sscanf (enmc->ext_worker[j].c_spot_shape, format_intd,
		       &nmc->client_worker[j].c_spot_shape);
	(void) sscanf (enmc->ext_worker[j].c_dedup_granule_size, format_intd,
		       &nmc->client_worker[j].c_dedup_granule_size);
	(void) sscanf (enmc->ext_worker[j].c_dedup_gran_rep_limit,
		       format_intd,
		       &nmc->client_worker[j].c_dedup_gran_rep_limit);
	(void) sscanf (enmc->ext_worker[j].c_use_file_size_dist, format_intd,
		       &nmc->client_worker[j].c_use_file_size_dist);
	(void) sscanf (enmc->ext_worker[j].c_comp_granule_size, format_intd,
		       &nmc->client_worker[j].c_comp_granule_size);
	(void) sscanf (enmc->ext_worker[j].c_background, format_intd,
		       &nmc->client_worker[j].c_background);
	(void) sscanf (enmc->ext_worker[j].c_sharemode, format_intd,
		       &nmc->client_worker[j].c_sharemode);
	(void) sscanf (enmc->ext_worker[j].c_uniform_file_size_dist,
		       format_intd,
		       &nmc->client_worker[j].c_uniform_file_size_dist);
	(void) sscanf (enmc->ext_worker[j].c_rand_dist_behavior, format_intd,
		       &nmc->client_worker[j].c_rand_dist_behavior);
	(void) sscanf (enmc->ext_worker[j].c_cipher, format_intd,
		       &nmc->client_worker[j].c_cipher);
	(void) sscanf (enmc->ext_worker[j].c_notify, format_intd,
		       &nmc->client_worker[j].c_notify);
	(void) sscanf (enmc->ext_worker[j].c_lru_on, format_intd,
		       &nmc->client_worker[j].c_lru_on);
	(void) sscanf (enmc->ext_worker[j].c_patt_vers, format_intd,
		       &nmc->client_worker[j].c_patt_vers);
	(void) sscanf (enmc->ext_worker[j].c_init_rate_enable, format_intd,
		       &nmc->client_worker[j].c_init_rate_enable);
	(void) sscanf (enmc->ext_worker[j].c_init_rate_speed, format_double,
		       &nmc->client_worker[j].c_init_rate_speed);
	(void) sscanf (enmc->ext_worker[j].c_init_read, format_intd,
		       &nmc->client_worker[j].c_init_read);
	(void) sscanf (enmc->ext_worker[j].c_shared_buckets, format_intd,
		       &nmc->client_worker[j].c_shared_buckets);
	(void) sscanf (enmc->ext_worker[j].c_unlink2_no_recreate, format_intd,
		       &nmc->client_worker[j].c_unlink2_no_recreate);
	(void) sscanf (enmc->ext_worker[j].c_extra_dir_levels, format_intd,
		       &nmc->client_worker[j].c_extra_dir_levels);
	(void) sscanf (enmc->ext_worker[j].c_chaff_count, format_intd,
		       &nmc->client_worker[j].c_chaff_count);
	(void) sscanf (enmc->ext_worker[j].c_files_per_dir, format_intd,
		       &nmc->client_worker[j].c_files_per_dir);
	(void) sscanf (enmc->ext_worker[j].c_instances, format_intd,
		       &nmc->client_worker[j].c_instances);
	(void) sscanf (enmc->ext_worker[j].c_op_rate, format_double,
		       &nmc->client_worker[j].c_op_rate);
	(void) sscanf (enmc->ext_worker[j].c_dir_count, format_intd,
		       &nmc->client_worker[j].c_dir_count);
	(void) sscanf (enmc->ext_worker[j].c_warm_time, format_intd,
		       &nmc->client_worker[j].c_warm_time);
	(void) sscanf (enmc->ext_worker[j].c_file_size, format_intd,
		       &nmc->client_worker[j].c_file_size);
	(void) sscanf (enmc->ext_worker[j].c_rel_version, format_intd,
		       &nmc->client_worker[j].c_rel_version);
    }
    unlock_logging();
}

/*
 * @brief Function to convert neutral formatted string data 
 *        from the client internal representation to external format
 * @param nmc : Pointer to client command internal format
 * @param enmc : Pointer to client command external format
 * @param worker_obj : number of workload objects
 *
 * Note: If one changes the value of OP_STR_LEN or any of the sizes of the
 * variables in the string based structures ext_master_command, or ext_client_command,
 * then the lengths would need to be adjusted in the fields below.
 */
void
client_int_to_ext (struct client_command *cc, struct ext_client_command *ecc)
{

    int i, j;
    int num_workload_obj = 0;
    char format_intx[MY_MAX_NUM_TYPE_LEN], format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN],
	format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN], format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and sscanf
     * routines.  This ensures that the functions do not accidentally over write
     * the fields in the structures. The defines can be changed, if needed, and
     * all of the space and the routines that touch that space are in sync.
     */
    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    snprintf (ecc->magic, MY_INT_WIDTH, format_intu, cc->magic);
    snprintf (ecc->crc32, MY_INT_WIDTH, format_intu, cc->crc32);

    my_strncpy (ecc->c_host_name, cc->c_host_name, MAXHNAME);
    my_strncpy (ecc->c_pit_hostname, cc->c_pit_hostname, PIT_NAME_LEN);
    my_strncpy (ecc->c_pit_service, cc->c_pit_service, PIT_SERV_LEN);
    my_strncpy (ecc->c_client_name, cc->c_client_name, MAXHNAME);
    my_strncpy (ecc->c_unix_pdsm_file, cc->c_unix_pdsm_file, MAXNAME);
    my_strncpy (ecc->c_unix_pdsm_control_file, cc->c_unix_pdsm_control_file,
		MAXNAME);
    my_strncpy (ecc->c_windows_pdsm_file, cc->c_windows_pdsm_file, MAXNAME);
    my_strncpy (ecc->c_windows_pdsm_control_file,
		cc->c_windows_pdsm_control_file, MAXNAME);
    my_strncpy (ecc->c_client_version, cc->c_client_version, VERS_WIDTH);
    snprintf (ecc->c_start_epoch, MY_DOUBLE_WIDTH, format_double,
	      cc->c_start_epoch);
    snprintf (ecc->c_prime_eclock_time, MY_DOUBLE_WIDTH, format_double,
	      cc->c_prime_eclock_time);
    snprintf (ecc->c_gg_offset, MY_INT_WIDTH, format_intd, cc->c_gg_offset);
    snprintf (ecc->c_gg_flag, MY_INT_WIDTH, format_intd, cc->c_gg_flag);
    snprintf (ecc->c_pdsm_mode, MY_INT_WIDTH, format_intd, cc->c_pdsm_mode);
    snprintf (ecc->c_pdsm_interval, MY_INT_WIDTH, format_intd,
	      cc->c_pdsm_interval);
    snprintf (ecc->c_workload_count, MY_INT_WIDTH, format_intd,
	      cc->c_workload_count);
    snprintf (ecc->c_client_number, MY_INT_WIDTH, format_intd,
	      cc->c_client_number);
    snprintf (ecc->c_child_flag, MY_INT_WIDTH, format_intd, cc->c_child_flag);
    snprintf (ecc->c_flush_flag, MY_INT_WIDTH, format_intd, cc->c_flush_flag);
    snprintf (ecc->c_unlink2_no_recreate, MY_INT_WIDTH, format_intd,
	      cc->c_unlink2_no_recreate);
    snprintf (ecc->c_cleanup, MY_INT_WIDTH, format_intd, cc->c_cleanup);
    snprintf (ecc->c_dump_files_flag, MY_INT_WIDTH, format_intd,
	      cc->c_dump_files_flag);
    snprintf (ecc->c_do_validate, MY_INT_WIDTH, format_intd,
	      cc->c_do_validate);
    snprintf (ecc->c_ipv6_enable, MY_INT_WIDTH, format_intd,
	      cc->c_ipv6_enable);
    snprintf (ecc->c_licensed, MY_INT_WIDTH, format_intd, cc->c_licensed);
    snprintf (ecc->c_heartbeat, MY_INT_WIDTH, format_intd, cc->c_heartbeat);
    snprintf (ecc->c_client_files, MY_INT_WIDTH, format_intd,
	      cc->c_client_files);
    snprintf (ecc->c_client_dirs, MY_INT_WIDTH, format_intd,
	      cc->c_client_dirs);
    snprintf (ecc->c_num_clients, MY_INT_WIDTH, format_intd,
	      cc->c_num_clients);
    snprintf (ecc->c_fd_caching_limit, MY_INT_WIDTH, format_intd,
	      cc->c_fd_caching_limit);
    snprintf (ecc->c_op_lat_flag, MY_INT_WIDTH, format_intd,
	      cc->c_op_lat_flag);
    snprintf (ecc->c_command, MY_INT_WIDTH, format_intd, cc->c_command);

    /* transpose OPs */
    num_workload_obj = cc->c_workload_count;

    for (j = 0; j < num_workload_obj; j++)
    {

	log_file (LOG_DEBUG,
		  "Client_int_to_ext: Transposing workload # %d Name %s\n", j,
		  cc->client_worker[j].c_workload_name);
	my_strncpy (ecc->ext_worker[j].c_workload_name,
		    cc->client_worker[j].c_workload_name, MAXWLNAME);
	my_strncpy (ecc->ext_worker[j].c_fs_type,
		    cc->client_worker[j].c_fs_type, MAXFSNAME);
	my_strncpy (ecc->ext_worker[j].c_platform_type,
		    cc->client_worker[j].c_platform_type, MAXFSNAME);
	for (i = 0; i < NUM_OP_TYPES; i++)
	{
	    snprintf (ecc->ext_worker[j].c_op_array[i], MY_INT_WIDTH,
		      format_intd, cc->client_worker[j].c_op_array[i]);
	    snprintf (ecc->ext_worker[j].c_op_array_string[i], OP_STR_LEN,
		      format_opstr,
		      cc->client_worker[j].c_op_array_string[i]);
	}
	/* transpose xfer mixes */
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    snprintf (ecc->ext_worker[j].c_rdist_array_size_min[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_rdist_array[i].size_min);
	    snprintf (ecc->ext_worker[j].c_rdist_array_size_max[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_rdist_array[i].size_max);
	    snprintf (ecc->ext_worker[j].c_rdist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_rdist_array[i].percent);

	    snprintf (ecc->ext_worker[j].c_wdist_array_size_min[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_wdist_array[i].size_min);
	    snprintf (ecc->ext_worker[j].c_wdist_array_size_max[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_wdist_array[i].size_max);
	    snprintf (ecc->ext_worker[j].c_wdist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_wdist_array[i].percent);

	    snprintf (ecc->ext_worker[j].c_file_size_dist_array_size_min[i],
		      MY_LONG_LONG_WIDTH, format_longlongd,
		      cc->client_worker[j].c_file_size_dist_array[i].
		      size_min);
	    snprintf (ecc->ext_worker[j].c_file_size_dist_array_size_max[i],
		      MY_LONG_LONG_WIDTH, format_longlongd,
		      cc->client_worker[j].c_file_size_dist_array[i].
		      size_max);
	    snprintf (ecc->ext_worker[j].c_file_size_dist_array_percent[i],
		      MY_INT_WIDTH, format_intd,
		      cc->client_worker[j].c_file_size_dist_array[i].percent);

	}
	snprintf (ecc->ext_worker[j].c_min_pre_name_length, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_min_pre_name_length);
	snprintf (ecc->ext_worker[j].c_max_pre_name_length, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_max_pre_name_length);
	snprintf (ecc->ext_worker[j].c_min_post_name_length, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_min_post_name_length);
	snprintf (ecc->ext_worker[j].c_max_post_name_length, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_max_post_name_length);
	snprintf (ecc->ext_worker[j].c_percent_commit, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_commit);
	snprintf (ecc->ext_worker[j].c_percent_direct, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_direct);
	snprintf (ecc->ext_worker[j].c_percent_fadvise_seq, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_fadvise_seq);
	snprintf (ecc->ext_worker[j].c_percent_fadvise_rand, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_fadvise_rand);
	snprintf (ecc->ext_worker[j].c_percent_fadvise_dont_need,
		  MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_percent_fadvise_dont_need);
	snprintf (ecc->ext_worker[j].c_percent_madvise_seq, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_madvise_seq);
	snprintf (ecc->ext_worker[j].c_percent_madvise_rand, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_madvise_rand);
	snprintf (ecc->ext_worker[j].c_percent_madvise_dont_need,
		  MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_percent_madvise_dont_need);
	snprintf (ecc->ext_worker[j].c_align, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_align);
	snprintf (ecc->ext_worker[j].c_percent_osync, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_osync);
	snprintf (ecc->ext_worker[j].c_percent_geometric, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_geometric);
	snprintf (ecc->ext_worker[j].c_percent_compress, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_compress);
	snprintf (ecc->ext_worker[j].c_percent_dedup, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_dedup);
	snprintf (ecc->ext_worker[j].c_percent_dedup_within, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_dedup_within);
	snprintf (ecc->ext_worker[j].c_percent_dedup_across, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_dedup_across);
	snprintf (ecc->ext_worker[j].c_dedup_group_count, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_dedup_group_count);
	snprintf (ecc->ext_worker[j].c_percent_per_spot, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_per_spot);
	snprintf (ecc->ext_worker[j].c_min_acc_per_spot, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_min_acc_per_spot);
	snprintf (ecc->ext_worker[j].c_acc_mult_spot, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_acc_mult_spot);
	snprintf (ecc->ext_worker[j].c_percent_affinity, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_percent_affinity);
	snprintf (ecc->ext_worker[j].c_spot_shape, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_spot_shape);
	snprintf (ecc->ext_worker[j].c_dedup_granule_size, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_dedup_granule_size);
	snprintf (ecc->ext_worker[j].c_dedup_gran_rep_limit, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_dedup_gran_rep_limit);
	snprintf (ecc->ext_worker[j].c_use_file_size_dist, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_use_file_size_dist);
	snprintf (ecc->ext_worker[j].c_comp_granule_size, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_comp_granule_size);
	snprintf (ecc->ext_worker[j].c_background, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_background);
	snprintf (ecc->ext_worker[j].c_sharemode, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_sharemode);
	snprintf (ecc->ext_worker[j].c_uniform_file_size_dist, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_uniform_file_size_dist);
	snprintf (ecc->ext_worker[j].c_rand_dist_behavior, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_rand_dist_behavior);
	snprintf (ecc->ext_worker[j].c_cipher, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_cipher);
	snprintf (ecc->ext_worker[j].c_notify, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_notify);
	snprintf (ecc->ext_worker[j].c_lru_on, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_lru_on);
	snprintf (ecc->ext_worker[j].c_patt_vers, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_patt_vers);
	snprintf (ecc->ext_worker[j].c_init_rate_enable, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_init_rate_enable);
	snprintf (ecc->ext_worker[j].c_init_rate_speed, MY_DOUBLE_WIDTH,
		  format_double, cc->client_worker[j].c_init_rate_speed);
	snprintf (ecc->ext_worker[j].c_init_read, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_init_read);
	snprintf (ecc->ext_worker[j].c_shared_buckets, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_shared_buckets);
	snprintf (ecc->ext_worker[j].c_unlink2_no_recreate, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_unlink2_no_recreate);
	snprintf (ecc->ext_worker[j].c_extra_dir_levels, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_extra_dir_levels);
	snprintf (ecc->ext_worker[j].c_chaff_count, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_chaff_count);
	snprintf (ecc->ext_worker[j].c_files_per_dir, MY_INT_WIDTH,
		  format_intd, cc->client_worker[j].c_files_per_dir);
	snprintf (ecc->ext_worker[j].c_instances, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_instances);
	snprintf (ecc->ext_worker[j].c_op_rate, MY_DOUBLE_WIDTH,
		  format_double, cc->client_worker[j].c_op_rate);
	snprintf (ecc->ext_worker[j].c_dir_count, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_dir_count);
	snprintf (ecc->ext_worker[j].c_warm_time, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_warm_time);
	snprintf (ecc->ext_worker[j].c_file_size, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_file_size);
	snprintf (ecc->ext_worker[j].c_rel_version, MY_INT_WIDTH, format_intd,
		  cc->client_worker[j].c_rel_version);
    }
    unlock_logging();
}

/*
 * @brief Function to convert neutral formatted string data 
 *        from the client external representation to interal format
 * @param cc  : Pointer to client command internal format
 * @param ecc : Pointer to client command external format
 * 
 * Function that converts neutral format data from the Prime into client 
 * dependent internal representation.
 *
 * client_ext_to_int() is used by the clients to convert the neutral format 
 * data back into their binary representation.
 *
 * Note: If one changes the value of OP_STR_LEN or any of the sizes of the
 * variables in the string based structures ext_master_command, or ext_client_command,
 * then the lengths would need to be adjusted in the fields below.
 */
void
client_ext_to_int (struct client_command *cc, struct ext_client_command *ecc)
{
    int i, j;
    int num_workload_obj = 0;
    char format_intx[MY_MAX_NUM_TYPE_LEN], format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_longd[MY_MAX_NUM_TYPE_LEN],
	format_longlongd[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN], format_opstr[OP_STR_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    /* 
     * This provides a way to have the defines control the structure space
     * allocation, and also control the field widths in the snprintf and sscanf
     * routines.  This ensures that the functions do not accidentally over write
     * the fields in the structures. The defines can be changed, if needed, and
     * all of the space and the routines that touch that space are in sync.
     */
    snprintf (format_intx, MY_INT_WIDTH, "%%%dx", MY_INT_WIDTH - 1);
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_longd, MY_LONG_WIDTH, "%%%dld", MY_LONG_WIDTH - 1);
    snprintf (format_longlongd, MY_LONG_LONG_WIDTH, "%%%dlld",
	      MY_LONG_LONG_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);
    snprintf (format_opstr, OP_STR_LEN, "%%%ds", OP_STR_LEN - 1);

    (void) sscanf (ecc->magic, format_intu, &cc->magic);
    (void) sscanf (ecc->crc32, format_intu, &cc->crc32);

    my_strncpy (cc->c_host_name, ecc->c_host_name, MAXHNAME);
    my_strncpy (cc->c_pit_hostname, ecc->c_pit_hostname, PIT_NAME_LEN);
    my_strncpy (cc->c_pit_service, ecc->c_pit_service, PIT_SERV_LEN);
    my_strncpy (cc->c_client_name, ecc->c_client_name, MAXHNAME);
    my_strncpy (cc->c_unix_pdsm_file, ecc->c_unix_pdsm_file, MAXNAME);
    my_strncpy (cc->c_unix_pdsm_control_file, ecc->c_unix_pdsm_control_file,
		MAXNAME);
    my_strncpy (cc->c_windows_pdsm_file, ecc->c_windows_pdsm_file, MAXNAME);
    my_strncpy (cc->c_windows_pdsm_control_file,
		ecc->c_windows_pdsm_control_file, MAXNAME);
    my_strncpy (cc->c_client_version, ecc->c_client_version, VERS_WIDTH);
    (void) sscanf (ecc->c_start_epoch, format_double, &cc->c_start_epoch);
    (void) sscanf (ecc->c_prime_eclock_time, format_double,
		   &cc->c_prime_eclock_time);
    (void) sscanf (ecc->c_gg_offset, format_intd, &cc->c_gg_offset);
    (void) sscanf (ecc->c_gg_flag, format_intd, &cc->c_gg_flag);
    (void) sscanf (ecc->c_pdsm_mode, format_intd, &cc->c_pdsm_mode);
    (void) sscanf (ecc->c_pdsm_interval, format_intd, &cc->c_pdsm_interval);
    (void) sscanf (ecc->c_workload_count, format_intd, &cc->c_workload_count);
    (void) sscanf (ecc->c_client_number, format_intd, &cc->c_client_number);
    (void) sscanf (ecc->c_child_flag, format_intd, &cc->c_child_flag);
    (void) sscanf (ecc->c_flush_flag, format_intd, &cc->c_flush_flag);
    (void) sscanf (ecc->c_unlink2_no_recreate, format_intd,
		   &cc->c_unlink2_no_recreate);
    (void) sscanf (ecc->c_cleanup, format_intd, &cc->c_cleanup);
    (void) sscanf (ecc->c_dump_files_flag, format_intd,
		   &cc->c_dump_files_flag);
    (void) sscanf (ecc->c_do_validate, format_intd, &cc->c_do_validate);
    (void) sscanf (ecc->c_ipv6_enable, format_intd, &cc->c_ipv6_enable);
    (void) sscanf (ecc->c_licensed, format_intd, &cc->c_licensed);
    (void) sscanf (ecc->c_heartbeat, format_intd, &cc->c_heartbeat);
    (void) sscanf (ecc->c_client_files, format_intd, &cc->c_client_files);
    (void) sscanf (ecc->c_client_dirs, format_intd, &cc->c_client_dirs);
    (void) sscanf (ecc->c_num_clients, format_intd, &cc->c_num_clients);
    (void) sscanf (ecc->c_fd_caching_limit, format_intd,
		   &cc->c_fd_caching_limit);
    (void) sscanf (ecc->c_op_lat_flag, format_intd, &cc->c_op_lat_flag);
    (void) sscanf (ecc->c_command, format_intd, &cc->c_command);

    num_workload_obj = cc->c_workload_count;

    for (j = 0; j < num_workload_obj; j++)
    {
	my_strncpy (cc->client_worker[j].c_workload_name,
		    ecc->ext_worker[j].c_workload_name, MAXWLNAME);
	my_strncpy (cc->client_worker[j].c_fs_type,
		    ecc->ext_worker[j].c_fs_type, MAXFSNAME);
	my_strncpy (cc->client_worker[j].c_platform_type,
		    ecc->ext_worker[j].c_platform_type, MAXFSNAME);
	/* Transpose the OPs */
	for (i = 0; i < NUM_OP_TYPES; i++)
	{
	    (void) sscanf (ecc->ext_worker[j].c_op_array[i], format_intd,
			   &cc->client_worker[j].c_op_array[i]);
	    (void) sscanf (ecc->ext_worker[j].c_op_array_string[i],
			   format_opstr,
			   cc->client_worker[j].c_op_array_string[i]);
	}
	/* Transpose the xfer mix */
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    (void) sscanf (ecc->ext_worker[j].c_rdist_array_size_min[i],
			   format_intd,
			   &cc->client_worker[j].c_rdist_array[i].size_min);
	    (void) sscanf (ecc->ext_worker[j].c_rdist_array_size_max[i],
			   format_intd,
			   &cc->client_worker[j].c_rdist_array[i].size_max);
	    (void) sscanf (ecc->ext_worker[j].c_rdist_array_percent[i],
			   format_intd,
			   &cc->client_worker[j].c_rdist_array[i].percent);
	    (void) sscanf (ecc->ext_worker[j].c_wdist_array_size_min[i],
			   format_intd,
			   &cc->client_worker[j].c_wdist_array[i].size_min);
	    (void) sscanf (ecc->ext_worker[j].c_wdist_array_size_max[i],
			   format_intd,
			   &cc->client_worker[j].c_wdist_array[i].size_max);
	    (void) sscanf (ecc->ext_worker[j].c_wdist_array_percent[i],
			   format_intd,
			   &cc->client_worker[j].c_wdist_array[i].percent);
	    (void) sscanf (ecc->ext_worker[j].
			   c_file_size_dist_array_size_min[i],
			   format_longlongd,
			   &cc->client_worker[j].c_file_size_dist_array[i].
			   size_min);
	    (void) sscanf (ecc->ext_worker[j].
			   c_file_size_dist_array_size_max[i],
			   format_longlongd,
			   &cc->client_worker[j].c_file_size_dist_array[i].
			   size_max);
	    (void) sscanf (ecc->ext_worker[j].
			   c_file_size_dist_array_percent[i], format_intd,
			   &cc->client_worker[j].c_file_size_dist_array[i].
			   percent);
	}
	(void) sscanf (ecc->ext_worker[j].c_min_pre_name_length, format_intd,
		       &cc->client_worker[j].c_min_pre_name_length);
	(void) sscanf (ecc->ext_worker[j].c_max_pre_name_length, format_intd,
		       &cc->client_worker[j].c_max_pre_name_length);
	(void) sscanf (ecc->ext_worker[j].c_min_post_name_length, format_intd,
		       &cc->client_worker[j].c_min_post_name_length);
	(void) sscanf (ecc->ext_worker[j].c_max_post_name_length, format_intd,
		       &cc->client_worker[j].c_max_post_name_length);
	(void) sscanf (ecc->ext_worker[j].c_percent_commit, format_intd,
		       &cc->client_worker[j].c_percent_commit);
	(void) sscanf (ecc->ext_worker[j].c_percent_direct, format_intd,
		       &cc->client_worker[j].c_percent_direct);
	(void) sscanf (ecc->ext_worker[j].c_percent_fadvise_seq, format_intd,
		       &cc->client_worker[j].c_percent_fadvise_seq);
	(void) sscanf (ecc->ext_worker[j].c_percent_fadvise_rand, format_intd,
		       &cc->client_worker[j].c_percent_fadvise_rand);
	(void) sscanf (ecc->ext_worker[j].c_percent_fadvise_dont_need,
		       format_intd,
		       &cc->client_worker[j].c_percent_fadvise_dont_need);
	(void) sscanf (ecc->ext_worker[j].c_percent_madvise_seq, format_intd,
		       &cc->client_worker[j].c_percent_madvise_seq);
	(void) sscanf (ecc->ext_worker[j].c_percent_madvise_rand, format_intd,
		       &cc->client_worker[j].c_percent_madvise_rand);
	(void) sscanf (ecc->ext_worker[j].c_percent_madvise_dont_need,
		       format_intd,
		       &cc->client_worker[j].c_percent_madvise_dont_need);
	(void) sscanf (ecc->ext_worker[j].c_align, format_intd,
		       &cc->client_worker[j].c_align);
	(void) sscanf (ecc->ext_worker[j].c_percent_osync, format_intd,
		       &cc->client_worker[j].c_percent_osync);
	(void) sscanf (ecc->ext_worker[j].c_percent_geometric, format_intd,
		       &cc->client_worker[j].c_percent_geometric);
	(void) sscanf (ecc->ext_worker[j].c_percent_compress, format_intd,
		       &cc->client_worker[j].c_percent_compress);
	(void) sscanf (ecc->ext_worker[j].c_percent_dedup, format_intd,
		       &cc->client_worker[j].c_percent_dedup);
	(void) sscanf (ecc->ext_worker[j].c_percent_dedup_within, format_intd,
		       &cc->client_worker[j].c_percent_dedup_within);
	(void) sscanf (ecc->ext_worker[j].c_percent_dedup_across, format_intd,
		       &cc->client_worker[j].c_percent_dedup_across);
	(void) sscanf (ecc->ext_worker[j].c_dedup_group_count, format_intd,
		       &cc->client_worker[j].c_dedup_group_count);
	(void) sscanf (ecc->ext_worker[j].c_percent_per_spot, format_intd,
		       &cc->client_worker[j].c_percent_per_spot);
	(void) sscanf (ecc->ext_worker[j].c_min_acc_per_spot, format_intd,
		       &cc->client_worker[j].c_min_acc_per_spot);
	(void) sscanf (ecc->ext_worker[j].c_acc_mult_spot, format_intd,
		       &cc->client_worker[j].c_acc_mult_spot);
	(void) sscanf (ecc->ext_worker[j].c_percent_affinity, format_intd,
		       &cc->client_worker[j].c_percent_affinity);
	(void) sscanf (ecc->ext_worker[j].c_spot_shape, format_intd,
		       &cc->client_worker[j].c_spot_shape);
	(void) sscanf (ecc->ext_worker[j].c_dedup_granule_size, format_intd,
		       &cc->client_worker[j].c_dedup_granule_size);
	(void) sscanf (ecc->ext_worker[j].c_dedup_gran_rep_limit, format_intd,
		       &cc->client_worker[j].c_dedup_gran_rep_limit);
	(void) sscanf (ecc->ext_worker[j].c_use_file_size_dist, format_intd,
		       &cc->client_worker[j].c_use_file_size_dist);
	(void) sscanf (ecc->ext_worker[j].c_comp_granule_size, format_intd,
		       &cc->client_worker[j].c_comp_granule_size);
	(void) sscanf (ecc->ext_worker[j].c_background, format_intd,
		       &cc->client_worker[j].c_background);
	(void) sscanf (ecc->ext_worker[j].c_sharemode, format_intd,
		       &cc->client_worker[j].c_sharemode);
	(void) sscanf (ecc->ext_worker[j].c_uniform_file_size_dist,
		       format_intd,
		       &cc->client_worker[j].c_uniform_file_size_dist);
	(void) sscanf (ecc->ext_worker[j].c_rand_dist_behavior, format_intd,
		       &cc->client_worker[j].c_rand_dist_behavior);
	(void) sscanf (ecc->ext_worker[j].c_cipher, format_intd,
		       &cc->client_worker[j].c_cipher);
	(void) sscanf (ecc->ext_worker[j].c_notify, format_intd,
		       &cc->client_worker[j].c_notify);
	(void) sscanf (ecc->ext_worker[j].c_lru_on, format_intd,
		       &cc->client_worker[j].c_lru_on);
	(void) sscanf (ecc->ext_worker[j].c_patt_vers, format_intd,
		       &cc->client_worker[j].c_patt_vers);
	(void) sscanf (ecc->ext_worker[j].c_init_rate_enable, format_intd,
		       &cc->client_worker[j].c_init_rate_enable);
	(void) sscanf (ecc->ext_worker[j].c_init_rate_speed, format_double,
		       &cc->client_worker[j].c_init_rate_speed);
	(void) sscanf (ecc->ext_worker[j].c_init_read, format_intd,
		       &cc->client_worker[j].c_init_read);
	(void) sscanf (ecc->ext_worker[j].c_shared_buckets, format_intd,
		       &cc->client_worker[j].c_shared_buckets);
	(void) sscanf (ecc->ext_worker[j].c_unlink2_no_recreate, format_intd,
		       &cc->client_worker[j].c_unlink2_no_recreate);
	(void) sscanf (ecc->ext_worker[j].c_extra_dir_levels, format_intd,
		       &cc->client_worker[j].c_extra_dir_levels);
	(void) sscanf (ecc->ext_worker[j].c_chaff_count, format_intd,
		       &cc->client_worker[j].c_chaff_count);
	(void) sscanf (ecc->ext_worker[j].c_files_per_dir, format_intd,
		       &cc->client_worker[j].c_files_per_dir);
	(void) sscanf (ecc->ext_worker[j].c_instances, format_intd,
		       &cc->client_worker[j].c_instances);
	(void) sscanf (ecc->ext_worker[j].c_op_rate, format_double,
		       &cc->client_worker[j].c_op_rate);
	(void) sscanf (ecc->ext_worker[j].c_dir_count, format_intd,
		       &cc->client_worker[j].c_dir_count);
	(void) sscanf (ecc->ext_worker[j].c_warm_time, format_intd,
		       &cc->client_worker[j].c_warm_time);
	(void) sscanf (ecc->ext_worker[j].c_file_size, format_intd,
		       &cc->client_worker[j].c_file_size);
	(void) sscanf (ecc->ext_worker[j].c_rel_version, format_intd,
		       &cc->client_worker[j].c_rel_version);
    }
    unlock_logging();
}

/**
 * @brief  Convert internal representation of keep-alive to external representation.
 *
 * @param kc : Internal formatted buffer
 * @param ekc : External formatted buffer
 */
void
keepalive_int_to_ext (struct keepalive_command *kc,
		      struct ext_keepalive_command *ekc)
{
    char format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */

    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);

    snprintf (ekc->magic, MY_INT_WIDTH, format_intu, kc->magic);
    snprintf (ekc->crc32, MY_INT_WIDTH, format_intu, kc->crc32);

    snprintf (ekc->k_command, MY_INT_WIDTH, format_intd, kc->k_command);
    snprintf (ekc->k_source_role, MY_INT_WIDTH, format_intd,
	      kc->k_source_role);
    snprintf (ekc->k_source_id, MY_INT_WIDTH, format_intd, kc->k_source_id);
    snprintf (ekc->k_destination_role, MY_INT_WIDTH, format_intd,
	      kc->k_destination_role);
    snprintf (ekc->k_destination_id, MY_INT_WIDTH, format_intd,
	      kc->k_destination_id);
    snprintf (ekc->k_count, MY_INT_WIDTH, format_intu, kc->k_count);
    snprintf (ekc->k_user_time, MY_DOUBLE_WIDTH, format_double,
	      kc->k_user_time);
    snprintf (ekc->k_system_time, MY_DOUBLE_WIDTH, format_double,
	      kc->k_system_time);
    snprintf (ekc->k_cpu_percent_busy, MY_DOUBLE_WIDTH, format_double,
	      kc->k_cpu_percent_busy);
    snprintf (ekc->k_cpu_warning, MY_INT_WIDTH, format_intd,
	      kc->k_cpu_warning);
    snprintf (ekc->k_minflt, MY_INT_WIDTH, format_intd, kc->k_minflt);
    snprintf (ekc->k_majflt, MY_INT_WIDTH, format_intd, kc->k_majflt);
    snprintf (ekc->k_inblock, MY_INT_WIDTH, format_intd, kc->k_inblock);
    snprintf (ekc->k_oublock, MY_INT_WIDTH, format_intd, kc->k_oublock);
    snprintf (ekc->k_maxrss, MY_INT_WIDTH, format_intd, kc->k_maxrss);
    unlock_logging();
}

/**
 * @brief  Convert external representation of keep-alive to internal representation.
 *
 * @param kc : Internal formatted buffer
 * @param ekc : External formatted buffer
 */
void
keepalive_ext_to_int (struct keepalive_command *kc,
		      struct ext_keepalive_command *ekc)
{
    char format_intd[MY_MAX_NUM_TYPE_LEN];
    char format_intu[MY_MAX_NUM_TYPE_LEN];
    char format_double[MY_MAX_NUM_TYPE_LEN];

    lock_logging(); /* Prevent race through printf/scanf with logging */
    snprintf (format_intd, MY_INT_WIDTH, "%%%dd", MY_INT_WIDTH - 1);
    snprintf (format_intu, MY_INT_WIDTH, "%%%du", MY_INT_WIDTH - 1);
    snprintf (format_double, MY_DOUBLE_WIDTH, "%%%dlf", MY_DOUBLE_WIDTH - 1);

    (void) sscanf (ekc->magic, format_intu, &kc->magic);
    (void) sscanf (ekc->crc32, format_intu, &kc->crc32);

    (void) sscanf (ekc->k_command, format_intd, &kc->k_command);
    (void) sscanf (ekc->k_source_role, format_intd, &kc->k_source_role);
    (void) sscanf (ekc->k_source_id, format_intd, &kc->k_source_id);
    (void) sscanf (ekc->k_destination_role, format_intd,
		   &kc->k_destination_role);
    (void) sscanf (ekc->k_destination_id, format_intd, &kc->k_destination_id);
    (void) sscanf (ekc->k_count, format_intu, &kc->k_count);
    (void) sscanf (ekc->k_user_time, format_double, &kc->k_user_time);
    (void) sscanf (ekc->k_system_time, format_double, &kc->k_system_time);
    (void) sscanf (ekc->k_cpu_percent_busy, format_double,
		   &kc->k_cpu_percent_busy);
    (void) sscanf (ekc->k_cpu_warning, format_intd, &kc->k_cpu_warning);
    (void) sscanf (ekc->k_minflt, format_intd, &kc->k_minflt);
    (void) sscanf (ekc->k_majflt, format_intd, &kc->k_majflt);
    (void) sscanf (ekc->k_inblock, format_intd, &kc->k_inblock);
    (void) sscanf (ekc->k_oublock, format_intd, &kc->k_oublock);
    (void) sscanf (ekc->k_maxrss, format_intd, &kc->k_maxrss);
    unlock_logging();
}

/**
 * @brief  Initialize the keepalive command in the message.
 *
 * @param kc : Internal format keepalive_command structure.
 * @param role : Which role is this command. Prime, NM, or Client.
 * @param id : Source identifier.
 */
void
init_k_keepalive_command (struct keepalive_command *kc, int role, int id)
{
    memset (kc, 0, sizeof (struct keepalive_command));

    kc->k_command = M_KEEPALIVE;
    kc->k_source_role = role;
    kc->k_source_id = id;
}

/**
 * @brief  Initialize the send_sigint command in the message.
 *
 * @param kc : Internal format keepalive_command structure.
 * @param role : Which role is this command. Prime;
 * @param id : Source identifier.
 */
void
init_k_send_sigint_command (struct keepalive_command *kc, int role, int id)
{
    memset (kc, 0, sizeof (struct keepalive_command));
    kc->k_command = M_SIGINT;
    kc->k_source_role = role;
    kc->k_source_id = id;
}

/**
 * @brief Reset all of the keepalive's comm buffers
 *        There are no parameters. (void)
 */
void
reset_all_keepalive_comm_buffers (void)
{
    if (g_kc == NULL || g_ekc == NULL)
    {
	return;
    }

    memset (g_kc, 0, sizeof (struct keepalive_command));
    memset (g_ekc, 0, sizeof (struct ext_keepalive_command));
}

/**
 * @brief Prints the keepalive info. Command and Source.
 * @param kc : Pointer to the struct keepalive_command.
 */
void
print_keepalive_command (struct keepalive_command *kc)
{
    char *command;
    char *source_role;

    switch (kc->k_command)
    {
    case M_KEEPALIVE:
	command = "keepalive";
	break;
    case M_SIGINT:
	command = "SIGINT";
	break;
    default:
	command = "unknown";
    }

    switch (kc->k_source_role)
    {
    case 1:
	source_role = prime_string;
	break;
    case 2:
	source_role = nodeManager_string;
	break;
    case 3:
	source_role = client_string;
	break;
    case 4:
	source_role = prime_string;
	break;
    default:
	source_role = "unknown";
    }

    if (is_prime ())
    {
	log_all (LOG_THREAD, "%s from %s %d: %u\n", command, source_role,
		 kc->k_source_id, kc->k_count);
	return;
    }

    log_file (LOG_THREAD, "%s from %s %d: %u\n", command, source_role,
	      kc->k_source_id, kc->k_count);
}

/**
 * @brief Reset all of the prime's comm buffers
 *        This function takes no parameters.
 */
void
reset_all_prime_comm_buffers (void)
{
    if (g_mc == NULL || g_emc == NULL)
    {
	return;
    }

    memset (g_mc, 0, sizeof (struct prime_command));
    memset (g_emc, 0, sizeof (struct ext_prime_command));
}

/**
 * @brief reset_all_nodeManager_comm_buffers: Reset all o the nodeManager's comm buffers
 *        This function has no parameters. (void)
 */
void
reset_all_nodeManager_comm_buffers (void)
{
    if (g_nmc == NULL || g_enmc == NULL)
    {
	return;
    }

    memset (g_nmc, 0, sizeof (struct nodeManager_command));
    memset (g_enmc, 0, sizeof (struct ext_nodeManager_command));
}

/**
 * @brief reset_all_client_comm_buffers: Reset all of the client's comm buffers
 *        There are no params
 */
void
reset_all_client_comm_buffers (void)
{
    if (g_cc == NULL || g_ecc == NULL)
    {
	return;
    }

    memset (g_cc, 0, sizeof (struct client_command));
    memset (g_ecc, 0, sizeof (struct ext_client_command));
}


/**
 * @brief print_nm_command: Print the nodeManager command
 * @param *nmc : Pointer to nodeManager_command (internal format)
 */
void
print_nm_command (struct nodeManager_command *nmc)
{
    int i;

    if (is_prime ())
    {
	log_stdout (LOG_CONFIG_VERBOSE, "\nprint_nm_command\n");
	log_stdout (LOG_CONFIG_VERBOSE, "nm_command: %d\n", nmc->nm_command);
	log_stdout (LOG_CONFIG_VERBOSE, "total clients: %d\n",
		    nmc->nm_num_of_children);

	for (i = 0; i < nmc->nm_num_of_children; i++)
	{
	    log_stdout (LOG_CONFIG_VERBOSE,
			" %d : %s : %.2lf files %d, dirs %d size %lld\n",
			nmc->nm_children_ids[i], nmc->nm_workload_name[i],
			nmc->nm_my_oprate[i], nmc->nm_client_files[i],
			nmc->nm_client_dirs[i], nmc->nm_file_size[i]);
	}

	log_stdout (LOG_CONFIG_VERBOSE, "\n");
    }

    log_file (LOG_CONFIG_VERBOSE, "\nprint_nm_command\n");
    log_file (LOG_CONFIG_VERBOSE, "nm_command: %d\n", nmc->nm_command);
    log_file (LOG_CONFIG_VERBOSE, "total clients: %d\n",
	      nmc->nm_num_of_children);

    for (i = 0; i < nmc->nm_num_of_children; i++)
    {
	log_file (LOG_CONFIG_VERBOSE,
		  " %d : %s : %.2lf files %d, dirs %d size %lld\n",
		  nmc->nm_children_ids[i], nmc->nm_workload_name[i],
		  nmc->nm_my_oprate[i], nmc->nm_client_files[i],
		  nmc->nm_client_dirs[i], nmc->nm_file_size[i]);
    }

    log_file (LOG_CONFIG_VERBOSE, "\n");
}

/**
 * @brief print_enm_command: Print the nodeManager command
 * @param *enmc : Pointer to nodeManager_command (external format)
 */
void
print_enm_command (struct ext_nodeManager_command *enmc)
{
    int i, count;

    if (is_prime ())
    {
	log_stdout (LOG_CONFIG_VERBOSE, "\nprint_enm_command\n");
	log_stdout (LOG_CONFIG_VERBOSE, "nm_command: %s\n", enmc->nm_command);
	log_stdout (LOG_CONFIG_VERBOSE, "total clients: %s\n",
		    enmc->nm_num_of_children);

	count = atoi (enmc->nm_num_of_children);

	for (i = 0; i < count; i++)
	{
	    log_stdout (LOG_CONFIG_VERBOSE, " %s : %s : %s\n",
			enmc->nm_children_ids[i], enmc->nm_workload_name[i],
			enmc->nm_my_oprate[i]);
	}

	log_stdout (LOG_CONFIG_VERBOSE, "\n");
    }

    log_file (LOG_CONFIG_VERBOSE, "\nprint_enm_command\n");
    log_file (LOG_CONFIG_VERBOSE, "nm_command: %s\n", enmc->nm_command);
    log_file (LOG_CONFIG_VERBOSE, "total clients: %s\n",
	      enmc->nm_num_of_children);

    count = atoi (enmc->nm_num_of_children);

    for (i = 0; i < count; i++)
    {
	log_file (LOG_CONFIG_VERBOSE, " %s : %s : %s\n",
		  enmc->nm_children_ids[i], enmc->nm_workload_name[i],
		  enmc->nm_my_oprate[i]);
    }

    log_file (LOG_CONFIG_VERBOSE, "\n");
}

/**
 * @brief Prepare the distribution tables for messaging to the 
 *        clients.  All of the information is placed in a client 
 *        command structure, in the the local machine's binary 
 *        format.  The routines [prime | client]_int_to_ext are 
 *        used to convert this into an external client command 
 *        (network neutral) format.
 * @param cc : Pointer to client_command structure.
 * @param my_mix_table : Pointer to the mix_table for this client.
 * @param num_workload_obj : Number of workloads
 */
/*
 * __doc__
 * __doc__  Function : void export_dist_table(struct client_command *cc, 
 * __doc__                      struct mix_table *my_mix_table)
 * __doc__  Arguments: struct client_command *cc: Internal format
 * __doc__             struct mix_table *my_mix_table: Internal format of
 * __doc__             the workload mix.
 * __doc__  Returns  : void
 * __doc__  Performs : Prepare the distribution tables for messaging to the 
 * __doc__             clients.  All of the information is placed in a client 
 * __doc__             command structure, in the the local machine's binary 
 * __doc__             format.  The routines [prime | client]_int_to_ext are 
 * __doc__             used to convert this into an external client command 
 * __doc__             (network neutral) format.
 * __doc__             
 */
void
export_dist_table (struct client_command *cc,
		   struct mix_table *my_mix_table, int num_workload_obj)
{
    int i, j;

    log_file (LOG_DEBUG, "Exporting %d workloads \n", num_workload_obj);
    for (j = 0; j < num_workload_obj; j++)
    {
	my_strncpy (cc->client_worker[j].c_workload_name,
		    my_mix_table[j].workload_name, MAXWLNAME);
	log_file (LOG_DEBUG, "Exporting workload %d name %s\n", j,
		  cc->client_worker[j].c_workload_name);
	my_strncpy (cc->client_worker[j].c_fs_type, my_mix_table[j].fs_type,
		    MAXFSNAME);
	my_strncpy (cc->client_worker[j].c_platform_type,
		    my_mix_table[j].platform_type, MAXFSNAME);

	my_strncpy (cc->client_worker[j].c_op_array_string[OP_READ],
		    my_mix_table[j].read_string, OP_STR_LEN);
	cc->client_worker[j].c_op_array[OP_READ] =
	    my_mix_table[j].percent_read;

	cc->client_worker[j].c_op_array[OP_READ_FILE] =
	    my_mix_table[j].percent_read_file;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_READ_FILE],
		    my_mix_table[j].read_file_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_MM_READ] =
	    my_mix_table[j].percent_mmap_read;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_MM_READ],
		    my_mix_table[j].mmap_read_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_READ_RAND] =
	    my_mix_table[j].percent_read_rand;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_READ_RAND],
		    my_mix_table[j].read_rand_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_WRITE] =
	    my_mix_table[j].percent_write;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_WRITE],
		    my_mix_table[j].write_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_WRITE_FILE] =
	    my_mix_table[j].percent_write_file;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_WRITE_FILE],
		    my_mix_table[j].write_file_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_MM_WRITE] =
	    my_mix_table[j].percent_mmap_write;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_MM_WRITE],
		    my_mix_table[j].mmap_write_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_WRITE_RAND] =
	    my_mix_table[j].percent_write_rand;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_WRITE_RAND],
		    my_mix_table[j].write_rand_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_RMW] = my_mix_table[j].percent_rmw;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_RMW],
		    my_mix_table[j].rmw_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_MKDIR] =
	    my_mix_table[j].percent_mkdir;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_MKDIR],
		    my_mix_table[j].mkdir_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_RMDIR] =
	    my_mix_table[j].percent_rmdir;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_RMDIR],
		    my_mix_table[j].rmdir_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_UNLINK] =
	    my_mix_table[j].percent_unlink;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_UNLINK],
		    my_mix_table[j].unlink_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_UNLINK2] =
	    my_mix_table[j].percent_unlink2;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_UNLINK2],
		    my_mix_table[j].unlink2_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_CREATE] =
	    my_mix_table[j].percent_create;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_CREATE],
		    my_mix_table[j].create_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_APPEND] =
	    my_mix_table[j].percent_append;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_APPEND],
		    my_mix_table[j].append_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_LOCKING] =
	    my_mix_table[j].percent_locking;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_LOCKING],
		    my_mix_table[j].locking_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_ACCESS] =
	    my_mix_table[j].percent_access;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_ACCESS],
		    my_mix_table[j].access_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_STAT] =
	    my_mix_table[j].percent_stat;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_STAT],
		    my_mix_table[j].stat_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_NEG_STAT] =
	    my_mix_table[j].percent_neg_stat;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_NEG_STAT],
		    my_mix_table[j].neg_stat_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_CHMOD] =
	    my_mix_table[j].percent_chmod;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_CHMOD],
		    my_mix_table[j].chmod_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_READDIR] =
	    my_mix_table[j].percent_readdir;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_READDIR],
		    my_mix_table[j].readdir_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_COPYFILE] =
	    my_mix_table[j].percent_copyfile;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_COPYFILE],
		    my_mix_table[j].copyfile_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_RENAME] =
	    my_mix_table[j].percent_rename;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_RENAME],
		    my_mix_table[j].rename_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_STATFS] =
	    my_mix_table[j].percent_statfs;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_STATFS],
		    my_mix_table[j].statfs_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_PATHCONF] =
	    my_mix_table[j].percent_pathconf;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_PATHCONF],
		    my_mix_table[j].pathconf_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_TRUNC] =
	    my_mix_table[j].percent_trunc;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_TRUNC],
		    my_mix_table[j].trunc_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_CUSTOM1] =
	    my_mix_table[j].percent_custom1;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_CUSTOM1],
		    my_mix_table[j].custom1_string, OP_STR_LEN);

	cc->client_worker[j].c_op_array[OP_CUSTOM2] =
	    my_mix_table[j].percent_custom2;
	my_strncpy (cc->client_worker[j].c_op_array_string[OP_CUSTOM2],
		    my_mix_table[j].custom2_string, OP_STR_LEN);

	cc->client_worker[j].c_min_pre_name_length =
	    my_mix_table[j].min_pre_name_length;
	cc->client_worker[j].c_max_pre_name_length =
	    my_mix_table[j].max_pre_name_length;
	cc->client_worker[j].c_min_post_name_length =
	    my_mix_table[j].min_post_name_length;
	cc->client_worker[j].c_max_post_name_length =
	    my_mix_table[j].max_post_name_length;
	cc->client_worker[j].c_percent_commit =
	    my_mix_table[j].percent_commit;
	cc->client_worker[j].c_percent_direct =
	    my_mix_table[j].percent_direct;
	cc->client_worker[j].c_percent_fadvise_seq =
	    my_mix_table[j].percent_fadvise_seq;
	cc->client_worker[j].c_percent_fadvise_rand =
	    my_mix_table[j].percent_fadvise_rand;
	cc->client_worker[j].c_percent_fadvise_dont_need =
	    my_mix_table[j].percent_fadvise_dont_need;
	cc->client_worker[j].c_percent_madvise_seq =
	    my_mix_table[j].percent_madvise_seq;
	cc->client_worker[j].c_percent_madvise_rand =
	    my_mix_table[j].percent_madvise_rand;
	cc->client_worker[j].c_percent_madvise_dont_need =
	    my_mix_table[j].percent_madvise_dont_need;
	cc->client_worker[j].c_align = my_mix_table[j].align;
	cc->client_worker[j].c_percent_osync = my_mix_table[j].percent_osync;
	cc->client_worker[j].c_percent_geometric =
	    my_mix_table[j].percent_geometric;
	cc->client_worker[j].c_percent_compress =
	    my_mix_table[j].percent_compress;
	cc->client_worker[j].c_percent_dedup = my_mix_table[j].percent_dedup;
	cc->client_worker[j].c_percent_dedup_within =
	    my_mix_table[j].percent_dedup_within;
	cc->client_worker[j].c_percent_dedup_across =
	    my_mix_table[j].percent_dedup_across;
	cc->client_worker[j].c_dedup_group_count =
	    my_mix_table[j].dedup_group_count;
	cc->client_worker[j].c_percent_per_spot =
	    my_mix_table[j].percent_per_spot;
	cc->client_worker[j].c_min_acc_per_spot =
	    my_mix_table[j].min_acc_per_spot;
	cc->client_worker[j].c_acc_mult_spot = my_mix_table[j].acc_mult_spot;
	cc->client_worker[j].c_percent_affinity =
	    my_mix_table[j].percent_affinity;
	cc->client_worker[j].c_spot_shape = my_mix_table[j].spot_shape;
	cc->client_worker[j].c_dedup_granule_size =
	    my_mix_table[j].dedup_granule_size;
	cc->client_worker[j].c_dedup_gran_rep_limit =
	    my_mix_table[j].dedup_gran_rep_limit;
	cc->client_worker[j].c_use_file_size_dist =
	    my_mix_table[j].use_file_size_dist;
	cc->client_worker[j].c_comp_granule_size =
	    my_mix_table[j].comp_granule_size;
	cc->client_worker[j].c_background = my_mix_table[j].background;
	cc->client_worker[j].c_sharemode = my_mix_table[j].sharemode;
	cc->client_worker[j].c_uniform_file_size_dist =
	    my_mix_table[j].uniform_file_size_dist;
	cc->client_worker[j].c_rand_dist_behavior =
	    my_mix_table[j].rand_dist_behavior;
	cc->client_worker[j].c_cipher = my_mix_table[j].cipher;
	cc->client_worker[j].c_notify = my_mix_table[j].notify;
	cc->client_worker[j].c_lru_on = my_mix_table[j].lru_on;
	cc->client_worker[j].c_patt_vers = my_mix_table[j].patt_vers;
	cc->client_worker[j].c_init_rate_enable =
	    my_mix_table[j].init_rate_enable;
	cc->client_worker[j].c_init_rate_speed =
	    my_mix_table[j].init_rate_speed;
	cc->client_worker[j].c_init_read = my_mix_table[j].init_read;
	cc->client_worker[j].c_chaff_count = my_mix_table[j].chaff_count;
	cc->client_worker[j].c_files_per_dir = my_mix_table[j].files_per_dir;
	cc->client_worker[j].c_instances = my_mix_table[j].instances;
	cc->client_worker[j].c_op_rate = my_mix_table[j].op_rate;
	cc->client_worker[j].c_dir_count = my_mix_table[j].dir_count;
	cc->client_worker[j].c_warm_time = my_mix_table[j].warm_time;
	cc->client_worker[j].c_file_size = my_mix_table[j].file_size;
	cc->client_worker[j].c_extra_dir_levels =
	    my_mix_table[j].extra_dir_levels;
	cc->client_worker[j].c_shared_buckets =
	    my_mix_table[j].shared_buckets;
	cc->client_worker[j].c_unlink2_no_recreate =
	    my_mix_table[j].unlink2_no_recreate;
	cc->client_worker[j].c_rel_version = my_mix_table[j].rel_version;

	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    cc->client_worker[j].c_rdist_array[i].size_min =
		my_mix_table[j].read_dist[i].size_min;
	    cc->client_worker[j].c_rdist_array[i].size_max =
		my_mix_table[j].read_dist[i].size_max;
	    cc->client_worker[j].c_rdist_array[i].percent =
		my_mix_table[j].read_dist[i].percent;
	    cc->client_worker[j].c_wdist_array[i].size_min =
		my_mix_table[j].write_dist[i].size_min;
	    cc->client_worker[j].c_wdist_array[i].size_max =
		my_mix_table[j].write_dist[i].size_max;
	    cc->client_worker[j].c_wdist_array[i].percent =
		my_mix_table[j].write_dist[i].percent;
	    cc->client_worker[j].c_file_size_dist_array[i].size_min =
		my_mix_table[j].file_size_dist[i].size_min;
	    cc->client_worker[j].c_file_size_dist_array[i].size_max =
		my_mix_table[j].file_size_dist[i].size_max;
	    cc->client_worker[j].c_file_size_dist_array[i].percent =
		my_mix_table[j].file_size_dist[i].percent;
	}
    }
}

/**
 * @brief The routine to import (from the messaging system) the 
 *        distribution tables. The data is extracted from the 
 *        client command structure (local binary representation) 
 *        and placed back into the local variables. The client 
 *        command structure contains the local machine's binary 
 *        representation of the data, as it was already converted 
 *        from network neutral to local representation by the 
 *        [prime | client]_ext_to_int() routines.
 * @param cc : Pointer to the command structure 
 * @param my_mix_table : Pointer to the mix_table for this client
 * @param num_workload_obj : Number of workload objects in the mix table.
 */
/*
 * __doc__
 * __doc__  Function : int import_dist_table(struct client_command *cc, 
 * __doc__                     struct mix_table *my_mix_table, 
 * __doc__                     FILE* newstdout, int cdebug, int ldebug)
 * __doc__  Arguments: struct client_command *cc: Internal format
 * __doc__             struct mix_table my_mix_table: Workload mix.
 * __doc__             FILE newstdout: New stdout for logging.
 * __doc__             int cdebug: Enable client debugging.
 * __doc__             int ldebug: Enable more client debugging.
 * __doc__  Returns  : void
 * __doc__  Performs : The routine to import (from the messaging system) the 
 * __doc__             distribution tables. The data is extracted from the 
 * __doc__             client command structure (local binary representation) 
 * __doc__             and placed back into the local variables. The client 
 * __doc__             command structure contains the local machine's binary 
 * __doc__             representation of the data, as it was already converted 
 * __doc__             from network neutral to local representation by the 
 * __doc__             [prime | client]_ext_to_int() routines.
 * __doc__             
 * __note__          : Note: This routine also sanity checks the distribution 
 * __note__            tables to make sure that the total of the percentages 
 * __note__            add up to 100 %.  If not, then it logs an error, and
 * __note__            tells the Prime to terminate the benchmark.
 * __doc__             
 */
int
import_dist_table (struct client_command *cc,
		   struct mix_table *my_mix_table, int num_workload_obj)
{
    int i, j;
    int total = 0;
    int rtotal, wtotal, stotal;

    log_file (LOG_DEBUG, "Importing %d workloads\n", num_workload_obj);
    for (j = 0; j < num_workload_obj; j++)
    {

	/* Guard against workloads that have no name */
	if (strlen (cc->client_worker[j].c_workload_name) == 0)
	    break;
	my_strncpy (my_mix_table[j].workload_name,
		    cc->client_worker[j].c_workload_name, MAXWLNAME);
	log_file (LOG_DEBUG, "Importing workload %d workloads. Name: %s\n", j,
		  cc->client_worker[j].c_workload_name);
	my_strncpy (my_mix_table[j].fs_type, cc->client_worker[j].c_fs_type,
		    MAXFSNAME);
	my_strncpy (my_mix_table[j].platform_type,
		    cc->client_worker[j].c_platform_type, MAXFSNAME);
	my_mix_table[j].percent_read =
	    cc->client_worker[j].c_op_array[OP_READ];
	my_strncpy (my_mix_table[j].read_string,
		    cc->client_worker[j].c_op_array_string[OP_READ],
		    OP_STR_LEN);

	my_mix_table[j].percent_read_file =
	    cc->client_worker[j].c_op_array[OP_READ_FILE];
	my_strncpy (my_mix_table[j].read_file_string,
		    cc->client_worker[j].c_op_array_string[OP_READ_FILE],
		    OP_STR_LEN);

	my_mix_table[j].percent_mmap_read =
	    cc->client_worker[j].c_op_array[OP_MM_READ];
	my_strncpy (my_mix_table[j].mmap_read_string,
		    cc->client_worker[j].c_op_array_string[OP_MM_READ],
		    OP_STR_LEN);

	my_mix_table[j].percent_read_rand =
	    cc->client_worker[j].c_op_array[OP_READ_RAND];
	my_strncpy (my_mix_table[j].read_rand_string,
		    cc->client_worker[j].c_op_array_string[OP_READ_RAND],
		    OP_STR_LEN);

	my_mix_table[j].percent_write =
	    cc->client_worker[j].c_op_array[OP_WRITE];
	my_strncpy (my_mix_table[j].write_string,
		    cc->client_worker[j].c_op_array_string[OP_WRITE],
		    OP_STR_LEN);

	my_mix_table[j].percent_write_file =
	    cc->client_worker[j].c_op_array[OP_WRITE_FILE];
	my_strncpy (my_mix_table[j].write_file_string,
		    cc->client_worker[j].c_op_array_string[OP_WRITE_FILE],
		    OP_STR_LEN);

	my_mix_table[j].percent_mmap_write =
	    cc->client_worker[j].c_op_array[OP_MM_WRITE];
	my_strncpy (my_mix_table[j].mmap_write_string,
		    cc->client_worker[j].c_op_array_string[OP_MM_WRITE],
		    OP_STR_LEN);

	my_mix_table[j].percent_write_rand =
	    cc->client_worker[j].c_op_array[OP_WRITE_RAND];
	my_strncpy (my_mix_table[j].write_rand_string,
		    cc->client_worker[j].c_op_array_string[OP_WRITE_RAND],
		    OP_STR_LEN);

	my_mix_table[j].percent_rmw = cc->client_worker[j].c_op_array[OP_RMW];
	my_strncpy (my_mix_table[j].rmw_string,
		    cc->client_worker[j].c_op_array_string[OP_RMW],
		    OP_STR_LEN);

	my_mix_table[j].percent_mkdir =
	    cc->client_worker[j].c_op_array[OP_MKDIR];
	my_strncpy (my_mix_table[j].mkdir_string,
		    cc->client_worker[j].c_op_array_string[OP_MKDIR],
		    OP_STR_LEN);

	my_mix_table[j].percent_rmdir =
	    cc->client_worker[j].c_op_array[OP_RMDIR];
	my_strncpy (my_mix_table[j].rmdir_string,
		    cc->client_worker[j].c_op_array_string[OP_RMDIR],
		    OP_STR_LEN);

	my_mix_table[j].percent_unlink =
	    cc->client_worker[j].c_op_array[OP_UNLINK];
	my_strncpy (my_mix_table[j].unlink_string,
		    cc->client_worker[j].c_op_array_string[OP_UNLINK],
		    OP_STR_LEN);

	my_mix_table[j].percent_unlink2 =
	    cc->client_worker[j].c_op_array[OP_UNLINK2];
	my_strncpy (my_mix_table[j].unlink2_string,
		    cc->client_worker[j].c_op_array_string[OP_UNLINK2],
		    OP_STR_LEN);

	my_mix_table[j].percent_create =
	    cc->client_worker[j].c_op_array[OP_CREATE];
	my_strncpy (my_mix_table[j].create_string,
		    cc->client_worker[j].c_op_array_string[OP_CREATE],
		    OP_STR_LEN);

	my_mix_table[j].percent_append =
	    cc->client_worker[j].c_op_array[OP_APPEND];
	my_strncpy (my_mix_table[j].append_string,
		    cc->client_worker[j].c_op_array_string[OP_APPEND],
		    OP_STR_LEN);

	my_mix_table[j].percent_locking =
	    cc->client_worker[j].c_op_array[OP_LOCKING];
	my_strncpy (my_mix_table[j].locking_string,
		    cc->client_worker[j].c_op_array_string[OP_LOCKING],
		    OP_STR_LEN);

	my_mix_table[j].percent_access =
	    cc->client_worker[j].c_op_array[OP_ACCESS];
	my_strncpy (my_mix_table[j].access_string,
		    cc->client_worker[j].c_op_array_string[OP_ACCESS],
		    OP_STR_LEN);

	my_mix_table[j].percent_stat =
	    cc->client_worker[j].c_op_array[OP_STAT];
	my_strncpy (my_mix_table[j].stat_string,
		    cc->client_worker[j].c_op_array_string[OP_STAT],
		    OP_STR_LEN);

	my_mix_table[j].percent_neg_stat =
	    cc->client_worker[j].c_op_array[OP_NEG_STAT];
	my_strncpy (my_mix_table[j].neg_stat_string,
		    cc->client_worker[j].c_op_array_string[OP_NEG_STAT],
		    OP_STR_LEN);

	my_mix_table[j].percent_chmod =
	    cc->client_worker[j].c_op_array[OP_CHMOD];
	my_strncpy (my_mix_table[j].chmod_string,
		    cc->client_worker[j].c_op_array_string[OP_CHMOD],
		    OP_STR_LEN);

	my_mix_table[j].percent_readdir =
	    cc->client_worker[j].c_op_array[OP_READDIR];
	my_strncpy (my_mix_table[j].readdir_string,
		    cc->client_worker[j].c_op_array_string[OP_READDIR],
		    OP_STR_LEN);

	my_mix_table[j].percent_copyfile =
	    cc->client_worker[j].c_op_array[OP_COPYFILE];
	my_strncpy (my_mix_table[j].copyfile_string,
		    cc->client_worker[j].c_op_array_string[OP_COPYFILE],
		    OP_STR_LEN);

	my_mix_table[j].percent_rename =
	    cc->client_worker[j].c_op_array[OP_RENAME];
	my_strncpy (my_mix_table[j].rename_string,
		    cc->client_worker[j].c_op_array_string[OP_RENAME],
		    OP_STR_LEN);

	my_mix_table[j].percent_statfs =
	    cc->client_worker[j].c_op_array[OP_STATFS];
	my_strncpy (my_mix_table[j].statfs_string,
		    cc->client_worker[j].c_op_array_string[OP_STATFS],
		    OP_STR_LEN);

	my_mix_table[j].percent_pathconf =
	    cc->client_worker[j].c_op_array[OP_PATHCONF];
	my_strncpy (my_mix_table[j].pathconf_string,
		    cc->client_worker[j].c_op_array_string[OP_PATHCONF],
		    OP_STR_LEN);

	my_mix_table[j].percent_trunc =
	    cc->client_worker[j].c_op_array[OP_TRUNC];
	my_strncpy (my_mix_table[j].trunc_string,
		    cc->client_worker[j].c_op_array_string[OP_TRUNC],
		    OP_STR_LEN);

	my_mix_table[j].percent_custom1 =
	    cc->client_worker[j].c_op_array[OP_CUSTOM1];
	my_strncpy (my_mix_table[j].custom1_string,
		    cc->client_worker[j].c_op_array_string[OP_CUSTOM1],
		    OP_STR_LEN);

	my_mix_table[j].percent_custom2 =
	    cc->client_worker[j].c_op_array[OP_CUSTOM2];
	my_strncpy (my_mix_table[j].custom2_string,
		    cc->client_worker[j].c_op_array_string[OP_CUSTOM2],
		    OP_STR_LEN);

	my_mix_table[j].min_pre_name_length =
	    cc->client_worker[j].c_min_pre_name_length;
	my_mix_table[j].max_pre_name_length =
	    cc->client_worker[j].c_max_pre_name_length;
	my_mix_table[j].min_post_name_length =
	    cc->client_worker[j].c_min_post_name_length;
	my_mix_table[j].max_post_name_length =
	    cc->client_worker[j].c_max_post_name_length;
	my_mix_table[j].percent_commit =
	    cc->client_worker[j].c_percent_commit;
	my_mix_table[j].percent_direct =
	    cc->client_worker[j].c_percent_direct;
	my_mix_table[j].percent_fadvise_seq =
	    cc->client_worker[j].c_percent_fadvise_seq;
	my_mix_table[j].percent_fadvise_rand =
	    cc->client_worker[j].c_percent_fadvise_rand;
	my_mix_table[j].percent_fadvise_dont_need =
	    cc->client_worker[j].c_percent_fadvise_dont_need;
	my_mix_table[j].percent_madvise_seq =
	    cc->client_worker[j].c_percent_madvise_seq;
	my_mix_table[j].percent_madvise_rand =
	    cc->client_worker[j].c_percent_madvise_rand;
	my_mix_table[j].percent_madvise_dont_need =
	    cc->client_worker[j].c_percent_madvise_dont_need;
	my_mix_table[j].align = cc->client_worker[j].c_align;
	my_mix_table[j].percent_osync = cc->client_worker[j].c_percent_osync;
	my_mix_table[j].percent_geometric =
	    cc->client_worker[j].c_percent_geometric;
	my_mix_table[j].percent_compress =
	    cc->client_worker[j].c_percent_compress;
	my_mix_table[j].percent_dedup = cc->client_worker[j].c_percent_dedup;
	my_mix_table[j].percent_dedup_within =
	    cc->client_worker[j].c_percent_dedup_within;
	my_mix_table[j].percent_dedup_across =
	    cc->client_worker[j].c_percent_dedup_across;
	my_mix_table[j].dedup_group_count =
	    cc->client_worker[j].c_dedup_group_count;
	my_mix_table[j].percent_per_spot =
	    cc->client_worker[j].c_percent_per_spot;
	my_mix_table[j].min_acc_per_spot =
	    cc->client_worker[j].c_min_acc_per_spot;
	my_mix_table[j].acc_mult_spot = cc->client_worker[j].c_acc_mult_spot;
	my_mix_table[j].percent_affinity =
	    cc->client_worker[j].c_percent_affinity;
	my_mix_table[j].spot_shape = cc->client_worker[j].c_spot_shape;
	my_mix_table[j].dedup_granule_size =
	    cc->client_worker[j].c_dedup_granule_size;
	my_mix_table[j].dedup_gran_rep_limit =
	    cc->client_worker[j].c_dedup_gran_rep_limit;
	my_mix_table[j].use_file_size_dist =
	    cc->client_worker[j].c_use_file_size_dist;
	my_mix_table[j].comp_granule_size =
	    cc->client_worker[j].c_comp_granule_size;
	my_mix_table[j].background = cc->client_worker[j].c_background;
	my_mix_table[j].sharemode = cc->client_worker[j].c_sharemode;
	my_mix_table[j].uniform_file_size_dist =
	    cc->client_worker[j].c_uniform_file_size_dist;
	my_mix_table[j].rand_dist_behavior =
	    cc->client_worker[j].c_rand_dist_behavior;
	my_mix_table[j].cipher = cc->client_worker[j].c_cipher;
	my_mix_table[j].notify = cc->client_worker[j].c_notify;
	my_mix_table[j].lru_on = cc->client_worker[j].c_lru_on;
	my_mix_table[j].patt_vers = cc->client_worker[j].c_patt_vers;
	my_mix_table[j].init_rate_enable =
	    cc->client_worker[j].c_init_rate_enable;
	my_mix_table[j].init_rate_speed =
	    cc->client_worker[j].c_init_rate_speed;
	my_mix_table[j].init_read = cc->client_worker[j].c_init_read;
	my_mix_table[j].shared_buckets =
	    cc->client_worker[j].c_shared_buckets;
	my_mix_table[j].unlink2_no_recreate =
	    cc->client_worker[j].c_unlink2_no_recreate;
	my_mix_table[j].chaff_count = cc->client_worker[j].c_chaff_count;
	my_mix_table[j].files_per_dir = cc->client_worker[j].c_files_per_dir;
	my_mix_table[j].instances = cc->client_worker[j].c_instances;
	my_mix_table[j].op_rate = cc->client_worker[j].c_op_rate;
	my_mix_table[j].dir_count = cc->client_worker[j].c_dir_count;
	my_mix_table[j].warm_time = cc->client_worker[j].c_warm_time;
	my_mix_table[j].file_size = cc->client_worker[j].c_file_size;
	my_mix_table[j].extra_dir_levels =
	    cc->client_worker[j].c_extra_dir_levels;
	my_mix_table[j].rel_version = cc->client_worker[j].c_rel_version;

	total = 0;
	for (i = 0; i < NUM_OP_TYPES; i++)
	{
	    log_file (LOG_DEBUG, "Op type %d Name: %s count %d\n", j,
		      cc->client_worker[j].c_op_array_string[i],
		      cc->client_worker[j].c_op_array[i]);
	    total += cc->client_worker[j].c_op_array[i];
	}
	if (total != 100)
	{
	    log_file (LOG_ERROR,
		      "Invalid OP distribution. Total = %d WL %d\n", total,
		      j);
	    return (1);
	}
	wtotal = rtotal = stotal = 0;
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    my_mix_table[j].read_dist[i].size_min =
		cc->client_worker[j].c_rdist_array[i].size_min;
	    if (my_mix_table[j].read_dist[i].size_min == 0)
		my_mix_table[j].read_dist[i].size_min = 1;
	    my_mix_table[j].read_dist[i].size_max =
		cc->client_worker[j].c_rdist_array[i].size_max;
	    my_mix_table[j].read_dist[i].percent =
		cc->client_worker[j].c_rdist_array[i].percent;

	    my_mix_table[j].write_dist[i].size_min =
		cc->client_worker[j].c_wdist_array[i].size_min;
	    if (my_mix_table[j].write_dist[i].size_min == 0)
		my_mix_table[j].write_dist[i].size_min = 1;
	    my_mix_table[j].write_dist[i].size_max =
		cc->client_worker[j].c_wdist_array[i].size_max;
	    my_mix_table[j].write_dist[i].percent =
		cc->client_worker[j].c_wdist_array[i].percent;

	    my_mix_table[j].file_size_dist[i].size_min =
		cc->client_worker[j].c_file_size_dist_array[i].size_min;
	    if (my_mix_table[j].file_size_dist[i].size_min == 0LL)
		my_mix_table[j].file_size_dist[i].size_min = 1LL;
	    my_mix_table[j].file_size_dist[i].size_max =
		cc->client_worker[j].c_file_size_dist_array[i].size_max;
	    my_mix_table[j].file_size_dist[i].percent =
		cc->client_worker[j].c_file_size_dist_array[i].percent;

	    rtotal += cc->client_worker[j].c_rdist_array[i].percent;
	    wtotal += cc->client_worker[j].c_wdist_array[i].percent;
	    if(!my_mix_table[j].use_file_size_dist)
		stotal = 100;
	    else
	        stotal += cc->client_worker[j].c_file_size_dist_array[i].percent;
	}
	if ((rtotal != 100) || (wtotal != 100) || (stotal != 100))
	{
	    log_all (LOG_ERROR, "Invalid xfer distribution. \n");
	    log_all (LOG_ERROR, "Read total percent = %d \n", rtotal);
	    log_all (LOG_ERROR, "Write total percent = %d \n", wtotal);
	    log_all (LOG_ERROR, "Filesize total percent = %d \n", stotal);

	    return (1);
	}

	log_file (LOG_CONFIG_VERBOSE, "-------------------\n");
	log_file (LOG_CONFIG_VERBOSE, "Distribution table:\n");
	log_file (LOG_CONFIG_VERBOSE, "-------------------\n");
	log_file (LOG_CONFIG_VERBOSE, "Workload name	     = %14s\n",
		  my_mix_table[j].workload_name);
	log_file (LOG_CONFIG_VERBOSE, "Percent read	     = %d\n",
		  my_mix_table[j].percent_read);
	log_file (LOG_CONFIG_VERBOSE, "Percent read file    = %d\n",
		  my_mix_table[j].percent_read_file);
	log_file (LOG_CONFIG_VERBOSE, "Percent mmap read    = %d\n",
		  my_mix_table[j].percent_mmap_read);
	log_file (LOG_CONFIG_VERBOSE, "Percent rand read    = %d\n",
		  my_mix_table[j].percent_read_rand);
	log_file (LOG_CONFIG_VERBOSE, "Percent write	    = %d\n",
		  my_mix_table[j].percent_write);
	log_file (LOG_CONFIG_VERBOSE, "Percent write file   = %d\n",
		  my_mix_table[j].percent_write_file);
	log_file (LOG_CONFIG_VERBOSE, "Percent mmap write   = %d\n",
		  my_mix_table[j].percent_mmap_write);
	log_file (LOG_CONFIG_VERBOSE, "Percent rand write   = %d\n",
		  my_mix_table[j].percent_write_rand);
	log_file (LOG_CONFIG_VERBOSE, "Percent rmw	    = %d\n",
		  my_mix_table[j].percent_rmw);
	log_file (LOG_CONFIG_VERBOSE, "Percent mkdir	    = %d\n",
		  my_mix_table[j].percent_mkdir);
	log_file (LOG_CONFIG_VERBOSE, "Percent rmdir	    = %d\n",
		  my_mix_table[j].percent_rmdir);
	log_file (LOG_CONFIG_VERBOSE, "Percent unlink	    = %d\n",
		  my_mix_table[j].percent_unlink);
	log_file (LOG_CONFIG_VERBOSE, "Percent unlink2      = %d\n",
		  my_mix_table[j].percent_unlink2);
	log_file (LOG_CONFIG_VERBOSE, "Percent create	    = %d\n",
		  my_mix_table[j].percent_create);
	log_file (LOG_CONFIG_VERBOSE, "Percent append	    = %d\n",
		  my_mix_table[j].percent_append);
	log_file (LOG_CONFIG_VERBOSE, "Percent locking      = %d\n",
		  my_mix_table[j].percent_locking);
	log_file (LOG_CONFIG_VERBOSE, "Percent access	    = %d\n",
		  my_mix_table[j].percent_access);
	log_file (LOG_CONFIG_VERBOSE, "Percent stat	    = %d\n",
		  my_mix_table[j].percent_stat);
	log_file (LOG_CONFIG_VERBOSE, "Percent neg_stat	    = %d\n",
		  my_mix_table[j].percent_neg_stat);
	log_file (LOG_CONFIG_VERBOSE, "Percent chmod	    = %d\n",
		  my_mix_table[j].percent_chmod);
	log_file (LOG_CONFIG_VERBOSE, "Percent readdir      = %d\n",
		  my_mix_table[j].percent_readdir);
	log_file (LOG_CONFIG_VERBOSE, "Percent copyfile     = %d\n",
		  my_mix_table[j].percent_copyfile);
	log_file (LOG_CONFIG_VERBOSE, "Percent rename	    = %d\n",
		  my_mix_table[j].percent_rename);
	log_file (LOG_CONFIG_VERBOSE, "Percent statfs	    = %d\n",
		  my_mix_table[j].percent_statfs);
	log_file (LOG_CONFIG_VERBOSE, "Percent pathconf     = %d\n",
		  my_mix_table[j].percent_pathconf);
	log_file (LOG_CONFIG_VERBOSE, "Percent trunc        = %d\n",
		  my_mix_table[j].percent_trunc);
	log_file (LOG_CONFIG_VERBOSE, "Percent custom1      = %d\n",
		  my_mix_table[j].percent_custom1);
	log_file (LOG_CONFIG_VERBOSE, "Percent custom2      = %d\n",
		  my_mix_table[j].percent_custom2);
	log_file (LOG_CONFIG_VERBOSE, "Min_pre_name_length  commit = %d\n",
		  my_mix_table[j].min_pre_name_length);
	log_file (LOG_CONFIG_VERBOSE, "Max_pre_name_length  commit = %d\n",
		  my_mix_table[j].max_pre_name_length);
	log_file (LOG_CONFIG_VERBOSE, "Min_post_name_length  commit = %d\n",
		  my_mix_table[j].min_post_name_length);
	log_file (LOG_CONFIG_VERBOSE, "Max_post_name_length  commit = %d\n",
		  my_mix_table[j].max_post_name_length);
	log_file (LOG_CONFIG_VERBOSE, "Percent write commit = %d\n",
		  my_mix_table[j].percent_commit);
	log_file (LOG_CONFIG_VERBOSE, "Percent osync	    = %d\n",
		  my_mix_table[j].percent_osync);
	log_file (LOG_CONFIG_VERBOSE, "Percent direct	    = %d\n",
		  my_mix_table[j].percent_direct);
	log_file (LOG_CONFIG_VERBOSE, "Percent fadvise_seq  = %d\n",
		  my_mix_table[j].percent_fadvise_seq);
	log_file (LOG_CONFIG_VERBOSE, "Percent fadvise_rand = %d\n",
		  my_mix_table[j].percent_fadvise_rand);
	log_file (LOG_CONFIG_VERBOSE, "Percent fadvise_dont_need = %d\n",
		  my_mix_table[j].percent_fadvise_dont_need);
	log_file (LOG_CONFIG_VERBOSE, "Percent madvise_seq  = %d\n",
		  my_mix_table[j].percent_madvise_seq);
	log_file (LOG_CONFIG_VERBOSE, "Percent madvise_rand = %d\n",
		  my_mix_table[j].percent_madvise_rand);
	log_file (LOG_CONFIG_VERBOSE, "Percent madvise_dont_need = %d\n",
		  my_mix_table[j].percent_madvise_dont_need);
	log_file (LOG_CONFIG_VERBOSE, "Align		    = %d\n",
		  my_mix_table[j].align);
	log_file (LOG_CONFIG_VERBOSE, "Percent geometric    = %d\n",
		  my_mix_table[j].percent_geometric);
	log_file (LOG_CONFIG_VERBOSE, "Percent compress     = %d\n",
		  my_mix_table[j].percent_compress);
	log_file (LOG_CONFIG_VERBOSE, "Percent dedup	    = %d\n",
		  my_mix_table[j].percent_dedup);
	log_file (LOG_CONFIG_VERBOSE, "Percent dedup_within = %d\n",
		  my_mix_table[j].percent_dedup_within);
	log_file (LOG_CONFIG_VERBOSE, "Percent dedup_across = %d\n",
		  my_mix_table[j].percent_dedup_across);
	log_file (LOG_CONFIG_VERBOSE, "Dedupe group count   = %d\n",
		  my_mix_table[j].dedup_group_count);
	log_file (LOG_CONFIG_VERBOSE, "Percent per_spot     = %d\n",
		  my_mix_table[j].percent_per_spot);
	log_file (LOG_CONFIG_VERBOSE, "Min acc_per_spot     = %d\n",
		  my_mix_table[j].min_acc_per_spot);
	log_file (LOG_CONFIG_VERBOSE, "Acc_mult_spot	    = %d\n",
		  my_mix_table[j].acc_mult_spot);
	log_file (LOG_CONFIG_VERBOSE, "Percent affinity     = %d\n",
		  my_mix_table[j].percent_affinity);
	log_file (LOG_CONFIG_VERBOSE, "Spot shape	    = %d\n",
		  my_mix_table[j].spot_shape);
	log_file (LOG_CONFIG_VERBOSE, "Dedup Granule size   = %d\n",
		  my_mix_table[j].dedup_granule_size);
	log_file (LOG_CONFIG_VERBOSE, "Dedup gran rep limit = %d\n",
		  my_mix_table[j].dedup_gran_rep_limit);
	log_file (LOG_CONFIG_VERBOSE, "Use file size dist = %d\n",
		  my_mix_table[j].use_file_size_dist);
	log_file (LOG_CONFIG_VERBOSE, "Comp Granule size    = %d\n",
		  my_mix_table[j].comp_granule_size);
	log_file (LOG_CONFIG_VERBOSE, "Background	    = %d\n",
		  my_mix_table[j].background);
	log_file (LOG_CONFIG_VERBOSE, "Sharemode	    = %d\n",
		  my_mix_table[j].sharemode);
	log_file (LOG_CONFIG_VERBOSE, "Uniform size dist    = %d\n",
		  my_mix_table[j].uniform_file_size_dist);
	log_file (LOG_CONFIG_VERBOSE, "Rand dist mode	    = %d\n",
		  my_mix_table[j].rand_dist_behavior);
	log_file (LOG_CONFIG_VERBOSE, "Cipher behavior      = %d\n",
		  my_mix_table[j].cipher);
	log_file (LOG_CONFIG_VERBOSE, "Notification percent = %d\n",
		  my_mix_table[j].notify);
	log_file (LOG_CONFIG_VERBOSE, "LRU		 = %d\n",
		  my_mix_table[j].lru_on);
	log_file (LOG_CONFIG_VERBOSE, "Patt version	 = %d\n",
		  my_mix_table[j].patt_vers);
	log_file (LOG_CONFIG_VERBOSE, "Init rate enable   = %d\n",
		  my_mix_table[j].init_rate_enable);
	log_file (LOG_CONFIG_VERBOSE, "Init rate speed   = %f\n",
		  my_mix_table[j].init_rate_speed);
	log_file (LOG_CONFIG_VERBOSE, "Init read flag	    = %d\n",
		  my_mix_table[j].init_read);
	log_file (LOG_CONFIG_VERBOSE, "Release version      = %d\n",
		  my_mix_table[j].rel_version);
	log_file (LOG_CONFIG_VERBOSE, "Platform type  	    = %s\n",
		  my_mix_table[j].platform_type);
	log_file (LOG_CONFIG_VERBOSE, "FS type  	    = %s\n",
		  my_mix_table[j].fs_type);
	log_file (LOG_CONFIG_VERBOSE, "-------------------\n");
    }
    return (0);
}

/**
 * @brief If using an external Programmable Interval Timer, here 
 *        is the routine to import the PIT service information.
 *        The PIT is only used in the event that "time" is not 
 *        moving at the same speed on all clients.  Gravity, 
 *        doppler effect, high velocity travel, or, just running 
 *        virtualized clients, where time doesn't move as one 
 *        might expect inside of the virtual machines.
 *        The concept is, if you have fractured time, then 
 *        everyone should use the same wrist watch.  In this 
 *        case, a PIT server acts as that wrist watch.
 *
 * @param cc : Pointer to Internal command structure.
 * @param pit_hostname : Hostname of the PIT server.
 * @param pit_service : Name of the service on the PIT server.
 */
/*
 * __doc__
 * __doc__  Function : int import_pit(struct client_command *cc, 
 * __doc__                     char *pit_hostname, char *pit_service)
 * __doc__  Arguments: struct client_command: Internal format
 * __doc__             char *pit_hostname: Host with the time service
 * __doc__             char *pit_service: Name of the PIT service
 * __doc__  Returns  : int : Success or failure
 * __doc__  Performs : If using an external Programmable Interval Timer, here 
 * __doc__             is the routine to import the PIT service information.
 * __doc__             The PIT is only used in the event that "time" is not 
 * __doc__             moving at the same speed on all clients.  Gravity, 
 * __doc__             doppler effect, high velocity travel, or, just running 
 * __doc__             virtualized clients, where time doesn't move as one 
 * __doc__             might expect inside of the virtual machines.
 * __doc__             The concept is, if you have fractured time, then 
 * __doc__             everyone should use the same wrist watch.  In this 
 * __doc__             case, a PIT server acts as that wrist watch.
 * __doc__             
 */
int
import_pit (struct client_command *cc, char *pit_hostname, char *pit_service)
{
    pit_hostname[0] = 0;
    pit_service[0] = 0;
    if (cc->c_pit_hostname[0] != 0)
    {
	my_strncpy (pit_hostname, &cc->c_pit_hostname[0], PIT_NAME_LEN);
    }
    if (cc->c_pit_service[0] != 0)
    {
	my_strncpy (pit_service, &cc->c_pit_service[0], PIT_SERV_LEN);
    }
    else
    {
	my_strncpy (pit_service, "PIT", PIT_SERV_LEN);	/* default */
    }
    return (0);
}

/**
 * @brief If using an external Programmable Interval Timer, 
 *        here is the routine to exports the PIT service 
 *        information.
 *        The PIT is only used in the event that "time" is not 
 *        moving at the same speed on all clients.  Gravity, 
 *        doppler effect, high velocity, or, just running 
 *        virtualized clients where time doesn't move as one 
 *        might expect inside of the virtual machines.
 * @param cc : Pointer to the command structure
 * @param pit_hostname : Hostname of the PIT server
 * @param pit_service : Name of the service on the PIT server.
 */
/*
 * __doc__
 * __doc__  Function : void export_pit(struct client_command *cc, 
 * __doc__                      char *pit_hostname, char *pit_service)
 * __doc__  Arguments: struct client_command: Internal format
 * __doc__             char *pit_hostname: PIT server host
 * __doc__             char *pit_service: PIT service name
 * __doc__  Returns  : void
 * __doc__  Performs : If using an external Programmable Interval Timer, 
 * __doc__             here is the routine to exports the PIT service 
 * __doc__             information.
 * __doc__             The PIT is only used in the event that "time" is not 
 * __doc__             moving at the same speed on all clients.  Gravity, 
 * __doc__             doppler effect, high velocity, or, just running 
 * __doc__             virtualized clients where time doesn't move as one 
 * __doc__             might expect inside of the virtual machines.
 * __doc__             
 */
void
export_pit (struct client_command *cc, char *pit_hostname, char *pit_service)
{
    cc->c_pit_hostname[0] = 0;
    cc->c_pit_service[0] = 0;
    if (pit_hostname[0] != 0)
    {
	my_strncpy (&cc->c_pit_hostname[0], pit_hostname, PIT_NAME_LEN);
    }
    if (pit_service[0] != 0)
    {
	my_strncpy (&cc->c_pit_service[0], pit_service, PIT_SERV_LEN);
    }
}
