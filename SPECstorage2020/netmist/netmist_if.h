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
 */
#ifndef __NETMIST_IFACE_H__
#define __NETMIST_IFACE_H__

/*
 * Interface declarations between Netmist and the outside world.
 */

#ifdef WIN32
#include <stdint.h>
#define u_int32_t uint32_t
#endif
#if defined(_solaris_)
#define u_int32_t  uint32_t
#endif
#include <sys/types.h>
#include "netmist_defines.h"
#include "netmist.h"


/*
 * The operation types that are sent back and forth.
 */
#define R_CHILD_JOIN                     1
#define R_FLAG_DATA                      2
#define R_FAILED                         3
#define R_TERMINATE                      4
#define R_GO                             5
#define R_PERCENT_COMPLETE               6
#define R_SENDING_RESULTS                7
#define R_IM_DONE                        8
#define R_HEARTBEAT                      9
#define R_PERCENT_W_COMPLETE            10
#define R_PERCENT_I_COMPLETE            11
#define INIT_PHASE                      12
#define RUN_PHASE                       13
#define WARM_PHASE                      14
#define R_NODEMANAGER_JOIN              15
#define R_NODEMANAGER_SEND_TOKEN_FILE   16
#define R_NODEMANAGER_ALL_READY         17
#define R_TIME_SYNC                     18
#define R_TIME_SYNC_DONE                19
#define R_GO_DONE                       20
#define INIT_PHASE_DONE                 21
#define WARM_PHASE_DONE                 22
#define ASK_RESULTS                     23
#define SHUTDOWN_PHASE                  24
#define R_EPOCH                         25
#define R_EPOCH_DONE                    26
#define ASK_INIT_RESULTS                27
#define R_VERSION_CHECK                 28
#define R_VERSION_CHECK_DONE            29
#define R_NM_FAILED                     30
#define RUN_PHASE_DONE                  31
#define R_SYNC_OK                       32

/*
 * Secondary Socket messages
 */
#define M_KEEPALIVE                     50
#define M_SIGINT                        51

/*
 * Heartbeat types.
 * If you change/add anything here, update heartbeat_names in netmist_if.c
 */
#define INIT_BEAT 1
#define WARM_BEAT 2
#define RUN_BEAT 3
#define PRE_WARM_BEAT 4
#define CLEAN_BEAT 5
#define VAL_BEAT 6
#define STOP_BEAT 7

/*
 * Histogram
 */
#define BUCKETS 40

/*
 * These define the string space needed to hold the various data types.
 * This is used to create the string based messages that pass data
 * between the clients and the Prime.  These defines are used for the
 * allocation of the space in the structures, as well as the format
 * statements used in sprintf and sscanf. ( the encoding scheme that
 * provides the network neutral data exchange format )
 */
#define MY_MAX_NUM_TYPE_LEN 26
#define MY_INT_WIDTH 20
#define MY_LONG_WIDTH 20
#define MY_FLOAT_WIDTH 24
#define MY_DOUBLE_WIDTH 24
#define MY_LONG_LONG_WIDTH 24

#define PIT_NAME_LEN MAXHNAME
#define PIT_SERV_LEN 20
#define VERS_WIDTH 100
#define NM_MAX_CHILDREN_ARG_LEN 100
#define NM_MAX_NM_ARG_SIZE (NM_MAX_CHILDREN_ARG_LEN * MAXCLIENTS)

/*
 * Internal command structure that is sent from the clients to the prime.
 */
struct prime_command
{
    unsigned int magic;
    int m_command;
    int m_client_number;
	int m_nm_number;
    int m_work_obj_index;
    int m_child_flag;
    char m_host_name[MAXHNAME];
    char m_client_name[MAXHNAME];
    enum netmist_child_err m_client_eval;	/* Netmist internal error code */
    int m_client_plat_err;	/* Platform error code */
    int m_child_port;
    int m_child_pid;
    int m_percent_complete;
    int m_background;
    int m_sharemode;
    int m_rand_dist_behavior;
    int m_cipher;
    int m_notify;		/* State of notify events */
    int m_lru_on;		/* State of LRU recycling of file descriptors */
    int m_uniform_file_size_dist;
    int m_fs_type[MAXFSNAME];
    char m_workload_name[MAXWLNAME];
    long m_run_time;
    long m_min_direct_size;
    double m_total_file_op_time;
    double m_total_file_ops;
    double m_ops_per_second;
    double m_average_latency;
    double m_read_throughput;
    double m_read_kbytes;
    double m_write_throughput;
    double m_write_kbytes;
    double m_Nread_throughput;
    double m_Nread_kbytes;
    double m_Nwrite_throughput;
    double m_Nwrite_kbytes;
    double m_meta_r_kbytes;
    double m_meta_w_kbytes;
    int m_init_files;
    int m_init_files_ws;
    int m_init_dirs;
    double m_file_space_mb;
    double m_current_op_rate;
    int m_read_count;
    char m_read_string[OP_STR_LEN];
    int m_read_file_count;
    char m_read_file_string[OP_STR_LEN];
    int m_mmap_read_count;
    char m_mmap_read_string[OP_STR_LEN];
    int m_read_rand_count;
    char m_read_rand_string[OP_STR_LEN];
    int m_write_count;
    char m_write_string[OP_STR_LEN];
    int m_write_file_count;
    char m_write_file_string[OP_STR_LEN];
    int m_open_count;
    int m_close_count;
    int m_mmap_write_count;
    char m_mmap_write_string[OP_STR_LEN];
    int m_write_rand_count;
    char m_write_rand_string[OP_STR_LEN];
    int m_rmw_count;
    char m_rmw_string[OP_STR_LEN];
    int m_mkdir_count;
    char m_mkdir_string[OP_STR_LEN];
    int m_rmdir_count;
    char m_rmdir_string[OP_STR_LEN];
    int m_unlink_count;
    char m_unlink_string[OP_STR_LEN];
    int m_unlink2_count;
    char m_unlink2_string[OP_STR_LEN];
    int m_create_count;
    char m_create_string[OP_STR_LEN];
    int m_append_count;
    char m_append_string[OP_STR_LEN];
    int m_locking_count;
    char m_locking_string[OP_STR_LEN];
    int m_access_count;
    char m_access_string[OP_STR_LEN];
    int m_chmod_count;
    char m_chmod_string[OP_STR_LEN];
    int m_readdir_count;
    char m_readdir_string[OP_STR_LEN];
    int m_stat_count;
    char m_stat_string[OP_STR_LEN];
    int m_neg_stat_count;
    char m_neg_stat_string[OP_STR_LEN];
    int m_copyfile_count;
    char m_copyfile_string[OP_STR_LEN];
    int m_rename_count;
    char m_rename_string[OP_STR_LEN];
    int m_statfs_count;
    char m_statfs_string[OP_STR_LEN];
    int m_pathconf_count;
    char m_pathconf_string[OP_STR_LEN];
    int m_trunc_count;
    char m_trunc_string[OP_STR_LEN];
    int m_custom1_count;
    char m_custom1_string[OP_STR_LEN];
    int m_custom2_count;
    char m_custom2_string[OP_STR_LEN];

    double m_read_time;
    double m_min_read_latency;
    double m_max_read_latency;

    double m_read_file_time;
    double m_min_read_file_latency;
    double m_max_read_file_latency;

    double m_mmap_read_time;
    double m_min_mmap_read_latency;
    double m_max_mmap_read_latency;

    double m_read_rand_time;
    double m_min_read_rand_latency;
    double m_max_read_rand_latency;

    double m_write_time;
    double m_min_write_latency;
    double m_max_write_latency;

    double m_write_file_time;
    double m_min_write_file_latency;
    double m_max_write_file_latency;

    double m_mmap_write_time;
    double m_min_mmap_write_latency;
    double m_max_mmap_write_latency;

    double m_write_rand_time;
    double m_min_write_rand_latency;
    double m_max_write_rand_latency;

    double m_rmw_time;
    double m_min_rmw_latency;
    double m_max_rmw_latency;

    double m_mkdir_time;
    double m_min_mkdir_latency;
    double m_max_mkdir_latency;

    double m_rmdir_time;
    double m_min_rmdir_latency;
    double m_max_rmdir_latency;

    double m_unlink_time;
    double m_min_unlink_latency;
    double m_max_unlink_latency;

    double m_unlink2_time;
    double m_min_unlink2_latency;
    double m_max_unlink2_latency;

    double m_create_time;
    double m_min_create_latency;
    double m_max_create_latency;

    double m_append_time;
    double m_min_append_latency;
    double m_max_append_latency;

    double m_locking_time;
    double m_min_locking_latency;
    double m_max_locking_latency;

    double m_access_time;
    double m_min_access_latency;
    double m_max_access_latency;

    double m_chmod_time;
    double m_min_chmod_latency;
    double m_max_chmod_latency;

    double m_readdir_time;
    double m_min_readdir_latency;
    double m_max_readdir_latency;

    double m_stat_time;
    double m_min_stat_latency;
    double m_max_stat_latency;

    double m_neg_stat_time;
    double m_min_neg_stat_latency;
    double m_max_neg_stat_latency;

    double m_copyfile_time;
    double m_min_copyfile_latency;
    double m_max_copyfile_latency;

    double m_rename_time;
    double m_min_rename_latency;
    double m_max_rename_latency;

    double m_statfs_time;
    double m_min_statfs_latency;
    double m_max_statfs_latency;

    double m_pathconf_time;
    double m_min_pathconf_latency;
    double m_max_pathconf_latency;

    double m_trunc_time;
    double m_min_trunc_latency;
    double m_max_trunc_latency;

    double m_custom1_time;
    double m_min_custom1_latency;
    double m_max_custom1_latency;

    double m_custom2_time;
    double m_min_custom2_latency;
    double m_max_custom2_latency;
    long long m_bands[BUCKETS];
    int m_heartbeat_type;
    int m_modified_run;
    u_int32_t crc32;
};

/*
 * External command structure that is sent from the clients to the prime.
 */
struct ext_prime_command
{
    char magic[MY_INT_WIDTH];
    char m_command[MY_INT_WIDTH];
    char m_client_number[MY_INT_WIDTH];
	char m_nm_number[MY_INT_WIDTH];
    char m_work_obj_index[MY_INT_WIDTH];
    char m_child_flag[MY_INT_WIDTH];
    char m_host_name[MAXHNAME];
    char m_client_name[MAXHNAME];
    char m_client_eval[MY_INT_WIDTH];
    char m_client_plat_err[MY_INT_WIDTH];
    char m_child_port[MY_INT_WIDTH];
    char m_child_pid[MY_INT_WIDTH];
    char m_percent_complete[MY_INT_WIDTH];	/* update msgs */
    char m_background[MY_INT_WIDTH];
    char m_sharemode[MY_INT_WIDTH];
    char m_rand_dist_behavior[MY_INT_WIDTH];
    char m_cipher[MY_INT_WIDTH];
    char m_notify[MY_INT_WIDTH];	/* State of notify events */
    char m_lru_on[MY_INT_WIDTH];	/* State of LRU recycling of file descs */
    char m_uniform_file_size_dist[MY_INT_WIDTH];
    char m_fs_type[MAXFSNAME];
    char m_workload_name[MAXWLNAME];	/* result msgs */
    char m_run_time[MY_LONG_WIDTH];
    char m_min_direct_size[MY_LONG_WIDTH];
    char m_total_file_op_time[MY_DOUBLE_WIDTH];
    char m_total_file_ops[MY_DOUBLE_WIDTH];
    char m_ops_per_second[MY_DOUBLE_WIDTH];
    char m_average_latency[MY_DOUBLE_WIDTH];
    char m_read_throughput[MY_DOUBLE_WIDTH];
    char m_read_kbytes[MY_DOUBLE_WIDTH];
    char m_write_throughput[MY_DOUBLE_WIDTH];
    char m_write_kbytes[MY_DOUBLE_WIDTH];
    char m_Nread_throughput[MY_DOUBLE_WIDTH];
    char m_Nread_kbytes[MY_DOUBLE_WIDTH];
    char m_Nwrite_throughput[MY_DOUBLE_WIDTH];
    char m_Nwrite_kbytes[MY_DOUBLE_WIDTH];
    char m_meta_r_kbytes[MY_DOUBLE_WIDTH];
    char m_meta_w_kbytes[MY_DOUBLE_WIDTH];
    char m_init_files[MY_INT_WIDTH];
    char m_init_files_ws[MY_INT_WIDTH];
    char m_init_dirs[MY_INT_WIDTH];
    char m_file_space_mb[MY_DOUBLE_WIDTH];
    char m_current_op_rate[MY_DOUBLE_WIDTH];
    char m_read_count[MY_INT_WIDTH];
    char m_read_string[OP_STR_LEN];
    char m_read_file_count[MY_INT_WIDTH];
    char m_read_file_string[OP_STR_LEN];
    char m_mmap_read_count[MY_INT_WIDTH];
    char m_mmap_read_string[OP_STR_LEN];
    char m_read_rand_count[MY_INT_WIDTH];
    char m_read_rand_string[OP_STR_LEN];
    char m_write_count[MY_INT_WIDTH];
    char m_write_string[OP_STR_LEN];
    char m_write_file_count[MY_INT_WIDTH];
    char m_write_file_string[OP_STR_LEN];
    char m_open_count[MY_INT_WIDTH];
    char m_close_count[MY_INT_WIDTH];
    char m_mmap_write_count[MY_INT_WIDTH];
    char m_mmap_write_string[OP_STR_LEN];
    char m_write_rand_count[MY_INT_WIDTH];
    char m_write_rand_string[OP_STR_LEN];
    char m_rmw_count[MY_INT_WIDTH];
    char m_rmw_string[OP_STR_LEN];
    char m_mkdir_count[MY_INT_WIDTH];
    char m_mkdir_string[OP_STR_LEN];
    char m_rmdir_count[MY_INT_WIDTH];
    char m_rmdir_string[OP_STR_LEN];
    char m_unlink_count[MY_INT_WIDTH];
    char m_unlink_string[OP_STR_LEN];
    char m_unlink2_count[MY_INT_WIDTH];
    char m_unlink2_string[OP_STR_LEN];
    char m_create_count[MY_INT_WIDTH];
    char m_create_string[OP_STR_LEN];
    char m_append_count[MY_INT_WIDTH];
    char m_append_string[OP_STR_LEN];
    char m_locking_count[MY_INT_WIDTH];
    char m_locking_string[OP_STR_LEN];
    char m_access_count[MY_INT_WIDTH];
    char m_access_string[OP_STR_LEN];
    char m_chmod_count[MY_INT_WIDTH];
    char m_chmod_string[OP_STR_LEN];
    char m_readdir_count[MY_INT_WIDTH];
    char m_readdir_string[OP_STR_LEN];
    char m_stat_count[MY_INT_WIDTH];
    char m_stat_string[OP_STR_LEN];
    char m_neg_stat_count[MY_INT_WIDTH];
    char m_neg_stat_string[OP_STR_LEN];
    char m_copyfile_count[MY_INT_WIDTH];
    char m_copyfile_string[OP_STR_LEN];
    char m_rename_count[MY_INT_WIDTH];
    char m_rename_string[OP_STR_LEN];
    char m_statfs_count[MY_INT_WIDTH];
    char m_statfs_string[OP_STR_LEN];
    char m_pathconf_count[MY_INT_WIDTH];
    char m_pathconf_string[OP_STR_LEN];
    char m_trunc_count[MY_INT_WIDTH];
    char m_trunc_string[OP_STR_LEN];
    char m_custom1_count[MY_INT_WIDTH];
    char m_custom1_string[OP_STR_LEN];
    char m_custom2_count[MY_INT_WIDTH];
    char m_custom2_string[OP_STR_LEN];

    char m_read_time[MY_DOUBLE_WIDTH];
    char m_min_read_latency[MY_DOUBLE_WIDTH];
    char m_max_read_latency[MY_DOUBLE_WIDTH];

    char m_read_file_time[MY_DOUBLE_WIDTH];
    char m_min_read_file_latency[MY_DOUBLE_WIDTH];
    char m_max_read_file_latency[MY_DOUBLE_WIDTH];

    char m_mmap_read_time[MY_DOUBLE_WIDTH];
    char m_min_mmap_read_latency[MY_DOUBLE_WIDTH];
    char m_max_mmap_read_latency[MY_DOUBLE_WIDTH];

    char m_read_rand_time[MY_DOUBLE_WIDTH];
    char m_min_read_rand_latency[MY_DOUBLE_WIDTH];
    char m_max_read_rand_latency[MY_DOUBLE_WIDTH];

    char m_write_time[MY_DOUBLE_WIDTH];
    char m_min_write_latency[MY_DOUBLE_WIDTH];
    char m_max_write_latency[MY_DOUBLE_WIDTH];

    char m_write_file_time[MY_DOUBLE_WIDTH];
    char m_min_write_file_latency[MY_DOUBLE_WIDTH];
    char m_max_write_file_latency[MY_DOUBLE_WIDTH];

    char m_mmap_write_time[MY_DOUBLE_WIDTH];
    char m_min_mmap_write_latency[MY_DOUBLE_WIDTH];
    char m_max_mmap_write_latency[MY_DOUBLE_WIDTH];

    char m_write_rand_time[MY_DOUBLE_WIDTH];
    char m_min_write_rand_latency[MY_DOUBLE_WIDTH];
    char m_max_write_rand_latency[MY_DOUBLE_WIDTH];

    char m_rmw_time[MY_DOUBLE_WIDTH];
    char m_min_rmw_latency[MY_DOUBLE_WIDTH];
    char m_max_rmw_latency[MY_DOUBLE_WIDTH];

    char m_mkdir_time[MY_DOUBLE_WIDTH];
    char m_min_mkdir_latency[MY_DOUBLE_WIDTH];
    char m_max_mkdir_latency[MY_DOUBLE_WIDTH];

    char m_rmdir_time[MY_DOUBLE_WIDTH];
    char m_min_rmdir_latency[MY_DOUBLE_WIDTH];
    char m_max_rmdir_latency[MY_DOUBLE_WIDTH];

    char m_unlink_time[MY_DOUBLE_WIDTH];
    char m_min_unlink_latency[MY_DOUBLE_WIDTH];
    char m_max_unlink_latency[MY_DOUBLE_WIDTH];

    char m_unlink2_time[MY_DOUBLE_WIDTH];
    char m_min_unlink2_latency[MY_DOUBLE_WIDTH];
    char m_max_unlink2_latency[MY_DOUBLE_WIDTH];

    char m_create_time[MY_DOUBLE_WIDTH];
    char m_min_create_latency[MY_DOUBLE_WIDTH];
    char m_max_create_latency[MY_DOUBLE_WIDTH];

    char m_append_time[MY_DOUBLE_WIDTH];
    char m_min_append_latency[MY_DOUBLE_WIDTH];
    char m_max_append_latency[MY_DOUBLE_WIDTH];

    char m_locking_time[MY_DOUBLE_WIDTH];
    char m_min_locking_latency[MY_DOUBLE_WIDTH];
    char m_max_locking_latency[MY_DOUBLE_WIDTH];

    char m_access_time[MY_DOUBLE_WIDTH];
    char m_min_access_latency[MY_DOUBLE_WIDTH];
    char m_max_access_latency[MY_DOUBLE_WIDTH];

    char m_chmod_time[MY_DOUBLE_WIDTH];
    char m_min_chmod_latency[MY_DOUBLE_WIDTH];
    char m_max_chmod_latency[MY_DOUBLE_WIDTH];

    char m_readdir_time[MY_DOUBLE_WIDTH];
    char m_min_readdir_latency[MY_DOUBLE_WIDTH];
    char m_max_readdir_latency[MY_DOUBLE_WIDTH];

    char m_stat_time[MY_DOUBLE_WIDTH];
    char m_min_stat_latency[MY_DOUBLE_WIDTH];
    char m_max_stat_latency[MY_DOUBLE_WIDTH];

    char m_neg_stat_time[MY_DOUBLE_WIDTH];
    char m_min_neg_stat_latency[MY_DOUBLE_WIDTH];
    char m_max_neg_stat_latency[MY_DOUBLE_WIDTH];

    char m_copyfile_time[MY_DOUBLE_WIDTH];
    char m_min_copyfile_latency[MY_DOUBLE_WIDTH];
    char m_max_copyfile_latency[MY_DOUBLE_WIDTH];

    char m_rename_time[MY_DOUBLE_WIDTH];
    char m_min_rename_latency[MY_DOUBLE_WIDTH];
    char m_max_rename_latency[MY_DOUBLE_WIDTH];

    char m_statfs_time[MY_DOUBLE_WIDTH];
    char m_min_statfs_latency[MY_DOUBLE_WIDTH];
    char m_max_statfs_latency[MY_DOUBLE_WIDTH];

    char m_pathconf_time[MY_DOUBLE_WIDTH];
    char m_min_pathconf_latency[MY_DOUBLE_WIDTH];
    char m_max_pathconf_latency[MY_DOUBLE_WIDTH];

    char m_trunc_time[MY_DOUBLE_WIDTH];
    char m_min_trunc_latency[MY_DOUBLE_WIDTH];
    char m_max_trunc_latency[MY_DOUBLE_WIDTH];

    char m_custom1_time[MY_DOUBLE_WIDTH];
    char m_min_custom1_latency[MY_DOUBLE_WIDTH];
    char m_max_custom1_latency[MY_DOUBLE_WIDTH];

    char m_custom2_time[MY_DOUBLE_WIDTH];
    char m_min_custom2_latency[MY_DOUBLE_WIDTH];
    char m_max_custom2_latency[MY_DOUBLE_WIDTH];

    char m_bands[BUCKETS][MY_LONG_LONG_WIDTH];
    char m_heartbeat_type[MY_INT_WIDTH];
    char m_modified_run[MY_INT_WIDTH];
    char crc32[MY_INT_WIDTH];
};


/*
 * Internal command structure that is sent from the prime to the clients.
 */
struct work_desc
{
    int c_op_array[NUM_OP_TYPES];	/* Types of operations */
    char c_op_array_string[NUM_OP_TYPES][OP_STR_LEN];	/* names of ops */
    struct dist_entry c_rdist_array[MAX_DIST_ELEM];	/* read distribution table */
    struct dist_entry c_wdist_array[MAX_DIST_ELEM];	/* write distribution table */
    struct file_size_dist_entry c_file_size_dist_array[MAX_DIST_ELEM];	/* file size dist table */
    int c_min_pre_name_length;
    int c_max_pre_name_length;
    int c_min_post_name_length;
    int c_max_post_name_length;
    int c_percent_commit;
    int c_percent_direct;
    int c_percent_fadvise_seq;
    int c_percent_fadvise_rand;
    int c_percent_fadvise_dont_need;
    int c_percent_madvise_seq;
    int c_percent_madvise_rand;
    int c_percent_madvise_dont_need;
    int c_percent_osync;
    int c_percent_geometric;
    int c_percent_compress;
    int c_percent_dedup;
    int c_percent_dedup_within;
    int c_percent_dedup_across;
    int c_dedup_group_count;
    int c_dedup_granule_size;
    int c_dedup_gran_rep_limit;
    int c_use_file_size_dist;
    int c_comp_granule_size;
    int c_percent_per_spot;
    int c_min_acc_per_spot;
    int c_acc_mult_spot;
    int c_percent_affinity;
    int c_spot_shape;
    int c_background;
    int c_sharemode;
    int c_rand_dist_behavior;
    int c_cipher;		/* Enable encrypted data sets */
    int c_notify;		/* Enable event notifications */
    int c_lru_on;		/* Enable LRU file descriptor recycling */
    int c_patt_vers;		/* Data pattern layout version */
    int c_init_rate_enable;	/* Init rate throttle enabled */
    double c_init_rate_speed;	/* Init rate throttle enabled */
    int c_init_read;		/* Init read flag */
    int c_rel_version;		/* Release version tag */
    int c_uniform_file_size_dist;
    int c_align;
    int c_shared_buckets;
    int c_unlink2_no_recreate;
    int c_extra_dir_levels;
    int c_chaff_count;
    int c_files_per_dir;
    int c_instances;
    double c_op_rate;
    int c_file_size;
    int c_dir_count;
    int c_warm_time;
    char c_fs_type[MAXFSNAME];
    char c_platform_type[MAXFSNAME];
    char c_workload_name[MAXWLNAME];
};

/*
 * External command structure that is sent from the prime to the clients.
 */
struct ext_work
{
    char c_op_array[NUM_OP_TYPES][MY_INT_WIDTH];
    char c_op_array_string[NUM_OP_TYPES][OP_STR_LEN];
    char c_rdist_array_size_min[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_rdist_array_size_max[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_rdist_array_percent[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_wdist_array_size_min[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_wdist_array_size_max[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_wdist_array_percent[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_file_size_dist_array_size_min[MAX_DIST_ELEM][MY_LONG_LONG_WIDTH];
    char c_file_size_dist_array_size_max[MAX_DIST_ELEM][MY_LONG_LONG_WIDTH];
    char c_file_size_dist_array_percent[MAX_DIST_ELEM][MY_INT_WIDTH];
    char c_min_pre_name_length[MY_INT_WIDTH];
    char c_max_pre_name_length[MY_INT_WIDTH];
    char c_min_post_name_length[MY_INT_WIDTH];
    char c_max_post_name_length[MY_INT_WIDTH];
    char c_percent_commit[MY_INT_WIDTH];
    char c_percent_direct[MY_INT_WIDTH];
    char c_percent_fadvise_seq[MY_INT_WIDTH];
    char c_percent_fadvise_rand[MY_INT_WIDTH];
    char c_percent_fadvise_dont_need[MY_INT_WIDTH];
    char c_percent_madvise_seq[MY_INT_WIDTH];
    char c_percent_madvise_rand[MY_INT_WIDTH];
    char c_percent_madvise_dont_need[MY_INT_WIDTH];
    char c_percent_osync[MY_INT_WIDTH];
    char c_percent_geometric[MY_INT_WIDTH];
    char c_percent_compress[MY_INT_WIDTH];
    char c_percent_dedup[MY_INT_WIDTH];
    char c_percent_dedup_within[MY_INT_WIDTH];
    char c_percent_dedup_across[MY_INT_WIDTH];
    char c_dedup_group_count[MY_INT_WIDTH];
    char c_percent_per_spot[MY_INT_WIDTH];
    char c_min_acc_per_spot[MY_INT_WIDTH];
    char c_acc_mult_spot[MY_INT_WIDTH];
    char c_percent_affinity[MY_INT_WIDTH];
    char c_spot_shape[MY_INT_WIDTH];
    char c_dedup_granule_size[MY_INT_WIDTH];
    char c_dedup_gran_rep_limit[MY_INT_WIDTH];
    char c_use_file_size_dist[MY_INT_WIDTH];
    char c_comp_granule_size[MY_INT_WIDTH];
    char c_background[MY_INT_WIDTH];
    char c_sharemode[MY_INT_WIDTH];
    char c_rand_dist_behavior[MY_INT_WIDTH];
    char c_cipher[MY_INT_WIDTH];
    char c_notify[MY_INT_WIDTH];	/* Enable notify events */
    char c_lru_on[MY_INT_WIDTH];	/* Enable notify events */
    char c_patt_vers[MY_INT_WIDTH];
    char c_init_rate_enable[MY_INT_WIDTH];
    char c_init_rate_speed[MY_DOUBLE_WIDTH];
    char c_init_read[MY_INT_WIDTH];
    char c_rel_version[MY_INT_WIDTH];
    char c_uniform_file_size_dist[MY_INT_WIDTH];
    char c_align[MY_INT_WIDTH];
    char c_shared_buckets[MY_INT_WIDTH];	
    char c_unlink2_no_recreate[MY_INT_WIDTH];	
    char c_extra_dir_levels[MY_INT_WIDTH];
    char c_chaff_count[MY_INT_WIDTH];
    char c_files_per_dir[MY_INT_WIDTH];
    char c_instances[MY_INT_WIDTH];
    char c_op_rate[MY_DOUBLE_WIDTH];
    char c_file_size[MY_INT_WIDTH];
    char c_dir_count[MY_INT_WIDTH];
    char c_warm_time[MY_INT_WIDTH];
    char c_fs_type[MAXFSNAME];
    char c_platform_type[MAXFSNAME];
    char c_workload_name[MAXWLNAME];
};

/*
 * Internal command structure that is sent from the prime to Node Managers
 */
struct nodeManager_command
{
    unsigned int magic;
    int nm_command;
    int nm_num_of_children;
    /*
     * These fields are common to all clients under the nodeManager
     */
    int nm_runtime;
    int nm_dir_levels;
    int nm_chaff_count;
    int nm_pdsm_mode;
    int nm_pdsm_interval;
    int nm_workload_count;

    int nm_ipv6_enable;
    char nm_client_log_dir[MAXNAME];
    char nm_client_windows_log_dir[MAXNAME];
    /*
     * These fields are specific to each client
     */
    char nm_workload_name[MAXCLIENTS_PER_NM][MAXWLNAME];
    double nm_my_oprate[MAXCLIENTS_PER_NM];
    int nm_children_ids[MAXCLIENTS_PER_NM];
    char nm_workdir[MAXCLIENTS_PER_NM][MAXNAME];

    int nm_client_files[MAXCLIENTS_PER_NM];
    int nm_client_dirs[MAXCLIENTS_PER_NM];
    long long nm_file_size[MAXCLIENTS_PER_NM];

    double c_start_epoch;
    double c_prime_eclock_time;

    int c_cleanup;
    int c_dump_files_flag;
    int c_tracedebug;
    int c_do_validate;
    int c_ipv6_enable;
    int c_licensed;
    int c_client_files;
    int c_client_dirs;
    int c_num_clients;
    int c_heartbeat;
    int c_fd_caching_limit;
    int c_op_lat_flag;
    int c_gg_offset;
    int c_gg_flag;
    int c_dir_levels;
    int c_chaff_count;
    int c_files_per_dir;
    int c_instances;
    double c_op_rate;
    int c_file_size;
    int c_dir_count;
    int c_warm_time;
    int c_pdsm_mode;
    int c_pdsm_interval;
    int c_workload_count;
    int c_client_number;
    int c_child_flag;
    int c_flush_flag;
    int c_unlink2_no_recreate;
    char c_host_name[MAXHNAME];
    char c_pit_hostname[PIT_NAME_LEN];
    char c_pit_service[PIT_SERV_LEN];
    char c_client_name[MAXHNAME];
    char c_unix_pdsm_file[MAXNAME];
    char c_unix_pdsm_control_file[MAXNAME];
    char c_windows_pdsm_file[MAXNAME];
    char c_windows_pdsm_control_file[MAXNAME];
    char c_client_version[VERS_WIDTH];
    struct work_desc client_worker[MAX_WORK_OBJ];
    u_int32_t crc32;
};

/*
 * External command structure that is sent from the prime to Node Managers
 */
struct ext_nodeManager_command
{
    char magic[MY_INT_WIDTH];
    char nm_command[MY_INT_WIDTH];
    char nm_num_of_children[MY_INT_WIDTH];

    char nm_runtime[MY_INT_WIDTH];
    char nm_dir_levels[MY_INT_WIDTH];
    char nm_chaff_count[MY_INT_WIDTH];
    char nm_files_per_dir[MY_INT_WIDTH];
    char nm_instances[MY_INT_WIDTH];
    char nm_pdsm_mode[MY_INT_WIDTH];
    char nm_pdsm_interval[MY_INT_WIDTH];
    char nm_workload_count[MY_INT_WIDTH];
    char nm_ipv6_enable[MY_INT_WIDTH];
    char nm_client_log_dir[MAXNAME];
    char nm_client_windows_log_dir[MAXNAME];

    char nm_workload_name[MAXCLIENTS_PER_NM][MAXWLNAME];
    char nm_my_oprate[MAXCLIENTS_PER_NM][MY_DOUBLE_WIDTH];
    char nm_children_ids[MAXCLIENTS_PER_NM][MY_INT_WIDTH];
    char nm_workdir[MAXCLIENTS_PER_NM][MAXNAME];

    char nm_client_files[MAXCLIENTS_PER_NM][MY_INT_WIDTH];
    char nm_client_dirs[MAXCLIENTS_PER_NM][MY_INT_WIDTH];
    char nm_file_size[MAXCLIENTS_PER_NM][MY_LONG_LONG_WIDTH];

    char c_start_epoch[MY_DOUBLE_WIDTH];
    char c_prime_eclock_time[MY_DOUBLE_WIDTH];

    char c_cleanup[MY_INT_WIDTH];
    char c_dump_files_flag[MY_INT_WIDTH];
    char c_tracedebug[MY_INT_WIDTH];
    char c_do_validate[MY_INT_WIDTH];
    char c_ipv6_enable[MY_INT_WIDTH];
    char c_licensed[MY_INT_WIDTH];
    char c_client_files[MY_INT_WIDTH];
    char c_client_dirs[MY_INT_WIDTH];
    char c_num_clients[MY_INT_WIDTH];
    char c_heartbeat[MY_INT_WIDTH];
    char c_fd_caching_limit[MY_INT_WIDTH];
    char c_op_lat_flag[MY_INT_WIDTH];
    char c_gg_offset[MY_INT_WIDTH];
    char c_gg_flag[MY_INT_WIDTH];
    char c_dir_levels[MY_INT_WIDTH];
    char c_chaff_count[MY_INT_WIDTH];
    char c_files_per_dir[MY_INT_WIDTH];
    char c_file_size[MY_INT_WIDTH];
    char c_dir_count[MY_INT_WIDTH];
    char c_warm_time[MY_INT_WIDTH];
    char c_pdsm_mode[MY_INT_WIDTH];
    char c_pdsm_interval[MY_INT_WIDTH];
    char c_workload_count[MY_INT_WIDTH];
    char c_client_number[MY_INT_WIDTH];
    char c_child_flag[MY_INT_WIDTH];
    char c_flush_flag[MY_INT_WIDTH];
    char c_unlink2_no_recreate[MY_INT_WIDTH];
    char c_host_name[MAXHNAME];
    char c_client_name[MAXHNAME];
    char c_unix_pdsm_file[MAXNAME];
    char c_unix_pdsm_control_file[MAXNAME];
    char c_windows_pdsm_file[MAXNAME];
    char c_windows_pdsm_control_file[MAXNAME];
    char c_pit_hostname[PIT_NAME_LEN];
    char c_pit_service[PIT_SERV_LEN];
    char c_client_version[VERS_WIDTH];
    struct ext_work ext_worker[MAX_WORK_OBJ];
    char crc32[MY_INT_WIDTH];
};

/*
 * Internal command structure that is sent from the nodeManager to the clients.
 */
struct client_command
{
    unsigned int magic;
    double c_start_epoch;
    double c_prime_eclock_time;
    int c_command;
    int c_cleanup;
    int c_dump_files_flag;
    int c_do_validate;
    int c_ipv6_enable;
    int c_licensed;
    int c_client_files;
    int c_client_dirs;
    int c_num_clients;
    int c_heartbeat;
    int c_fd_caching_limit;
    int c_op_lat_flag;
    int c_gg_offset;
    int c_gg_flag;
    int c_dir_levels;
    int c_chaff_count;
    int c_files_per_dir;
    int c_file_size;
    int c_dir_count;
    int c_warm_time;
    int c_pdsm_mode;
    int c_pdsm_interval;
    int c_workload_count;
    int c_client_number;
    int c_child_flag;
    int c_flush_flag;
    int c_unlink2_no_recreate;
    char c_host_name[MAXHNAME];
    char c_pit_hostname[PIT_NAME_LEN];
    char c_pit_service[PIT_SERV_LEN];
    char c_client_name[MAXHNAME];
    char c_unix_pdsm_file[MAXNAME];
    char c_unix_pdsm_control_file[MAXNAME];
    char c_windows_pdsm_file[MAXNAME];
    char c_windows_pdsm_control_file[MAXNAME];
    char c_client_version[VERS_WIDTH];
    struct work_desc client_worker[MAX_WORK_OBJ];
    u_int32_t crc32;
};

/*
 * External command structure that is sent from the nodeManager to the clients.
 */
struct ext_client_command
{
    char magic[MY_INT_WIDTH];
    char c_start_epoch[MY_DOUBLE_WIDTH];
    char c_prime_eclock_time[MY_DOUBLE_WIDTH];
    char c_command[MY_INT_WIDTH];
    char c_cleanup[MY_INT_WIDTH];
    char c_dump_files_flag[MY_INT_WIDTH];
    char c_do_validate[MY_INT_WIDTH];
    char c_ipv6_enable[MY_INT_WIDTH];
    char c_licensed[MY_INT_WIDTH];
    char c_client_files[MY_INT_WIDTH];
    char c_client_dirs[MY_INT_WIDTH];
    char c_num_clients[MY_INT_WIDTH];
    char c_heartbeat[MY_INT_WIDTH];
    char c_fd_caching_limit[MY_INT_WIDTH];
    char c_op_lat_flag[MY_INT_WIDTH];
    char c_gg_offset[MY_INT_WIDTH];
    char c_gg_flag[MY_INT_WIDTH];
    char c_dir_levels[MY_INT_WIDTH];
    char c_chaff_count[MY_INT_WIDTH];
    char c_files_per_dir[MY_INT_WIDTH];
    char c_file_size[MY_INT_WIDTH];
    char c_dir_count[MY_INT_WIDTH];
    char c_pdsm_mode[MY_INT_WIDTH];
    char c_pdsm_interval[MY_INT_WIDTH];
    char c_workload_count[MY_INT_WIDTH];
    char c_client_number[MY_INT_WIDTH];
    char c_child_flag[MY_INT_WIDTH];
    char c_flush_flag[MY_INT_WIDTH];
    char c_unlink2_no_recreate[MY_INT_WIDTH];
    char c_host_name[MAXHNAME];
    char c_client_name[MAXHNAME];
    char c_unix_pdsm_file[MAXNAME];
    char c_unix_pdsm_control_file[MAXNAME];
    char c_windows_pdsm_file[MAXNAME];
    char c_windows_pdsm_control_file[MAXNAME];
    char c_pit_hostname[PIT_NAME_LEN];
    char c_pit_service[PIT_SERV_LEN];
    char c_client_version[VERS_WIDTH];
    char c_warm_time[MY_INT_WIDTH];
    struct ext_work ext_worker[MAX_WORK_OBJ];
    char crc32[MY_INT_WIDTH];
};

struct keepalive_command
{
    unsigned int magic;
    int k_command;
    int k_source_role;
    int k_source_id;
    int k_destination_role;
    int k_destination_id;
    unsigned int k_count;
    double k_user_time;
    double k_system_time;
    double k_cpu_percent_busy;
    int k_cpu_warning;
    int k_minflt;
    int k_majflt;
    int k_inblock;
    int k_oublock;
    int k_maxrss;
    u_int32_t crc32;
};

struct ext_keepalive_command
{
    char magic[MY_INT_WIDTH];
    char k_command[MY_INT_WIDTH];
    char k_source_role[MY_INT_WIDTH];
    char k_source_id[MY_INT_WIDTH];
    char k_destination_role[MY_INT_WIDTH];
    char k_destination_id[MY_INT_WIDTH];
    char k_count[MY_INT_WIDTH];
    char k_user_time[MY_DOUBLE_WIDTH];
    char k_system_time[MY_DOUBLE_WIDTH];
    char k_cpu_percent_busy[MY_DOUBLE_WIDTH];
    char k_cpu_warning[MY_INT_WIDTH];
    char k_minflt[MY_INT_WIDTH];
    char k_majflt[MY_INT_WIDTH];
    char k_inblock[MY_INT_WIDTH];
    char k_oublock[MY_INT_WIDTH];
    char k_maxrss[MY_INT_WIDTH];
    char crc32[MY_INT_WIDTH];
};

extern const char *heartbeat_names[];

extern struct prime_command *g_mc;
extern struct ext_prime_command *g_emc;

extern struct nodeManager_command *g_nmc;
extern struct ext_nodeManager_command *g_enmc;

extern struct client_command *g_cc;
extern struct ext_client_command *g_ecc;

extern struct keepalive_command *g_kc;
extern struct ext_keepalive_command *g_ekc;

/*
 * The prototypes used for the comm mechanism.
 */
void prime_int_to_ext (struct prime_command *, struct ext_prime_command *,
		       int op_lat_flag);
void prime_ext_to_int (struct prime_command *, struct ext_prime_command *,
		       int op_lat_flag);

int import_dist_table (struct client_command *, struct mix_table *, int);
void export_dist_table (struct client_command *, struct mix_table *, int);
int import_pit (struct client_command *, char *, char *);
void export_pit (struct client_command *, char *, char *);

void nodeManager_int_to_ext (struct nodeManager_command *,
			     struct ext_nodeManager_command *, int);

void nodeManager_ext_to_int (struct ext_nodeManager_command *,
			     struct nodeManager_command *, int);

void client_int_to_ext (struct client_command *, struct ext_client_command *);

void client_ext_to_int (struct client_command *, struct ext_client_command *);

void keepalive_int_to_ext (struct keepalive_command *,
			   struct ext_keepalive_command *);

void keepalive_ext_to_int (struct keepalive_command *,
			   struct ext_keepalive_command *);

void init_k_keepalive_command (struct keepalive_command *, int, int);

void init_k_send_sigint_command (struct keepalive_command *kc, int, int);

void reset_all_prime_comm_buffers (void);

void reset_all_nodeManager_comm_buffers (void);

void reset_all_client_comm_buffers (void);

void reset_all_keepalive_comm_buffers (void);

void print_keepalive_command (struct keepalive_command *kc);

void print_nm_command (struct nodeManager_command *);
void print_enm_command (struct ext_nodeManager_command *);

#endif
