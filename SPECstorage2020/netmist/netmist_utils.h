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

#ifndef __NETMIST_UTILS_H__
#define __NETMIST_UTILS_H__

#include "netmist.h"
#include "netmist_if.h"

typedef enum _netmist_role
{
    NETMIST_PRIME = 1,
    NETMIST_NODEMANAGER,
    NETMIST_CLIENT,
} _NETMIST_ROLE;


typedef struct _prime_cfg
{
    FILE *log_handle;
} _PRIME_CFG;

typedef struct _nodeManager_cfg
{
    FILE *log_handle;
} _NODEMANAGER_CFG;

typedef struct _client_cfg
{
    FILE *log_handle;
    double start_epoch;
    double prime_eclock_time;
    double time_skew;
    double my_eclock_time;
    double curr_epoch;
    int workload_id;

} _CLIENT_CFG;

typedef struct _cmd_line
{
    int log_flag;
    char log_filename[MAXNAME];
    char license_filename[MAXNAME];
    char unix_pdsm_file[MAXNAME];
    char unix_pdsm_control_file[MAXNAME];
    char windows_pdsm_file[MAXNAME];
    char windows_pdsm_control_file[MAXNAME];
    int m_flag;			/* netmist role - 1:prime, 2:nodeManager, 3:Client */
    int sharing_flag;

    int orflag;			/* Op rate to test */
    double op_rate;

    int gflag;			/* client token file name */
    char client_tok_filename[MAXNAME];

    int flush_flag;

    int heartbeat_flag;
    int Op_lat_flag;
    int fd_caching_limit;

    int Q_flag;
    int client_files;
    int fpd_flag;

    /*
     * Define the directory structure
     */
    int T_flag;
    int client_dirs;

    int nflag;
    int client_id;
    int real_client_id;

    int gg_flag;
    int gg_offset;
    
    long long fsize;		/* In Kbytes */
    int dflag;
    int ppc;			/* procs per client */

    int use_rsh;

    int pflag;			/* Directory to do the work */
    char workdir[MAXNAME];	/* Directory,on the client,  where work will be conducted. */

    int localflag;		/* User sh to run locally */

    int hflag;			/* help text */

    int eflag;			/* path to executable */
    char execdir[MAXNAME];

    int iflag;			/* information flag */

    int p_ext_mon_flag;		/* Name of the external monitor script */
    char u_external_script_name[MAXNAME];
    int p_ext_mon_arg_flag;
    char u_external_script_args[MAXNAME];

    int run_time;
    int run_time_flag;
    int yaml_run_time;
    int twarn;
    int pdsm_mode;
    int pdsm_interval;
    int workload_count;
    int benchmark_count;

    int wflag;
    int warm_time;
    int yaml_warm_time[MAX_WORK_OBJ];
    int wwarn;

    int Mflag;
    char prime_name[MAXNAME];	/* Name of the prime procesws */

    int Password_flag;
    char password[MAXNAME];
    char mypassword[MAXNAME];

    int Usr_dom_flag;
    char usr_dom[MAXNAME];
    char userdomain[MAXNAME];

    int ipv6_enable;

    int Uflag;
    char client_log_dir[MAXNAME];

    int WUflag;
    char client_windows_log_dir[MAXNAME];

    int Rflag;
    char prime_results_dir[MAXNAME];

    int Vflag;
    int my_child_pid;

    int bflag;			/* Memory size of clients in MiBytes */
    long long client_memsize_mb;

    int Bflag;			/* Aggregate data set size */
    long long agg_set_memsize_mb;

    int kflag;			/* kill the tests on the clients and exit */
    int kill_pid;

    int vflag;			/* Version info only */

    int lflag;			/* Local file system flag for debug */
    char var_opt[256];

    int Dflag;			/* Distribute the benchmark to the clients */

    int Pflag;			/* Port the prime is listening on */
    int prime_p;

    int im_flag;		/* Import current table */
    int yaml_flag;		/* Import yaml current table */
    char itable[MAXNAME];

    int cleanup_flag;

    int skip_init_flag;
    int skip_init;
    int yaml_skip_init;

    int do_validate;

    int dump_files_flag;

    int tracelevel;

    int keepalive;

    char pit_hostname[40];
    char pit_service[8];

    char my_tmp_workload_name[MAXWLNAME];

    char vfs_so[MAXNAME];
    char vfs_arg[MAXNAME];

} _CMD_LINE;

typedef struct _internal_cfg
{
    _NETMIST_ROLE netmist_role;

    union
    {
	_PRIME_CFG prime_cfg;
	_NODEMANAGER_CFG nodeManager_cfg;
	_CLIENT_CFG client_cfg;
    } u;

} _INTERNAL_CFG;

struct result_object
{
    int client_id;
    int work_obj_index;
    char work_obj_name[MAXWLNAME];
    char client_name[MAXHNAME];
    long run_time;
    long min_direct_size;
    long write_count;
    char write_string[OP_STR_LEN];
    long write_file_count;
    char write_file_string[OP_STR_LEN];
    long open_count;
    long close_count;
    long mmap_write_count;
    char mmap_write_string[OP_STR_LEN];
    long read_count;
    char read_string[OP_STR_LEN];
    long read_file_count;
    char read_file_string[OP_STR_LEN];
    long mmap_read_count;
    char mmap_read_string[OP_STR_LEN];
    long mkdir_count;
    char mkdir_string[OP_STR_LEN];
    long rmdir_count;
    char rmdir_string[OP_STR_LEN];
    long unlink_count;
    char unlink_string[OP_STR_LEN];
    long unlink2_count;
    char unlink2_string[OP_STR_LEN];
    long create_count;
    char create_string[OP_STR_LEN];
    long stat_count;
    char stat_string[OP_STR_LEN];
    long neg_stat_count;
    char neg_stat_string[OP_STR_LEN];
    long copyfile_count;
    char copyfile_string[OP_STR_LEN];
    long rename_count;
    char rename_string[OP_STR_LEN];
    long statfs_count;
    char statfs_string[OP_STR_LEN];
    long pathconf_count;
    char pathconf_string[OP_STR_LEN];
    long trunc_count;
    char trunc_string[OP_STR_LEN];
    long custom1_count;
    char custom1_string[OP_STR_LEN];
    long custom2_count;
    char custom2_string[OP_STR_LEN];
    long append_count;
    char append_string[OP_STR_LEN];
    long lock_count;
    char lock_string[OP_STR_LEN];
    long access_count;
    char access_string[OP_STR_LEN];
    long chmod_count;
    char chmod_string[OP_STR_LEN];
    long readdir_count;
    char readdir_string[OP_STR_LEN];
    long write_rand_count;
    char write_rand_string[OP_STR_LEN];
    long read_rand_count;
    char read_rand_string[OP_STR_LEN];
    long rmw_count;
    char rmw_string[OP_STR_LEN];
    double write_time;
    double min_write_latency;
    double max_write_latency;
    double write_file_time;
    double min_write_file_latency;
    double max_write_file_latency;
    double mmap_write_time;
    double min_mmap_write_latency;
    double max_mmap_write_latency;
    double read_time;
    double min_read_latency;
    double max_read_latency;
    double read_file_time;
    double min_read_file_latency;
    double max_read_file_latency;
    double mmap_read_time;
    double min_mmap_read_latency;
    double max_mmap_read_latency;
    double mkdir_time;
    double min_mkdir_latency;
    double max_mkdir_latency;
    double rmdir_time;
    double min_rmdir_latency;
    double max_rmdir_latency;
    double unlink_time;
    double min_unlink_latency;
    double max_unlink_latency;
    double unlink2_time;
    double min_unlink2_latency;
    double max_unlink2_latency;
    double create_time;
    double min_create_latency;
    double max_create_latency;
    double stat_time;
    double min_stat_latency;
    double max_stat_latency;
    double neg_stat_time;
    double min_neg_stat_latency;
    double max_neg_stat_latency;
    double copyfile_time;
    double min_copyfile_latency;
    double max_copyfile_latency;
    double rename_time;
    double min_rename_latency;
    double max_rename_latency;
    double statfs_time;
    double min_statfs_latency;
    double max_statfs_latency;
    double pathconf_time;
    double min_pathconf_latency;
    double max_pathconf_latency;
    double trunc_time;
    double min_trunc_latency;
    double max_trunc_latency;
    double custom1_time;
    double min_custom1_latency;
    double max_custom1_latency;
    double custom2_time;
    double min_custom2_latency;
    double max_custom2_latency;
    double append_time;
    double min_append_latency;
    double max_append_latency;
    double lock_time;
    double min_lock_latency;
    double max_lock_latency;
    double access_time;
    double min_access_latency;
    double max_access_latency;
    double chmod_time;
    double min_chmod_latency;
    double max_chmod_latency;
    double readdir_time;
    double min_readdir_latency;
    double max_readdir_latency;
    double write_rand_time;
    double min_write_rand_latency;
    double max_write_rand_latency;
    double read_rand_time;
    double min_read_rand_latency;
    double max_read_rand_latency;
    double rmw_time;
    double min_rmw_latency;
    double max_rmw_latency;
    double ops_per_second;
    double total_file_op_time;
    double total_file_ops;
    double average_latency;
    double read_kbytes;
    double read_throughput;
    double write_kbytes;
    double write_throughput;
    double Nread_kbytes;
    double Nread_throughput;
    double Nwrite_kbytes;
    double Nwrite_throughput;
    double meta_r_kbytes;
    double meta_w_kbytes;
    double file_space_mb;
    long long bands[BUCKETS];
    int init_files;
    int init_files_ws;
    int init_dirs;
    int modified_run;
    int background;
    int sharemode;
    int uniform_file_size_dist;
    int rand_dist_behavior;
    int cipher;
    int notify;
    int lru_on;
    /*char fs_type[MAXFSNAME]; */
};

/*
 * The client object contains all of the properties and attributes of 
 * a given client worker thread/proc.
 */
struct client_object
{
    int client_id;		/* Client number            */
    char client_name[MAXHNAME];	/* Client hostname          */
    char client_workdir[MAXNAME];	/* Client cmdline.workdir           */
    char client_dom_user[MAXNAME];	/* Client login userid      */
    char client_password[MAXNAME];	/* Client login password    */
    char client_execdir[MAXNAME];	/* Client exec path         */
    char client_log_path[MAXNAME];	/* Client log path          */
    char client_windows_log_path[MAXNAME];	/* Client log path          */
    char client_workload_name[MAXWLNAME];	/* Client name of workload */
    int client_type;		/* Type of client           */
    long long memsize;		/* Client's memory size     */
    double op_rate;		/* Requested op rate        */
    int instances;		/* Number of copies         */
    long long file_size;	/* Client's file size       */
    unsigned long dir_count;	/* Client's dir count       */
    unsigned long files_per_dir;	/* Client's files_per_dir   */
    unsigned long open_flags;	/* Flags to open files      */
    int instant_ops_per_sec;	/* Realtime rate            */
    int instant_status;		/* Realtime status          */
    struct result_object results;	/* This child's result      */
};

/*
 * The nm_client_object contains all of the properties and attributes of
 * a given client worker thread/proc that nodeManager maintains.
 * This structure is simply an abridged version of client_object
 */
struct nm_client_object
{
    int client_id;		/* Client number            */
    char client_workdir[MAXNAME];	/* Client cmdline.workdir   */
    char client_dom_user[MAXNAME];	/* Client login userid      */
    char client_password[MAXNAME];	/* Client login password    */
    char client_log_path[MAXNAME];	/* Client log path          */
    char client_windows_log_path[MAXNAME];	/* Client log path          */
    char client_workload_name[MAXWLNAME];	/* Client name of workload  */
    long long memsize;		/* Client's memory size     */
    double op_rate;		/* Requested op rate        */
    long long file_size;	/* Client's file size       */
    unsigned long dir_count;	/* Client's dir count       */
    unsigned long files_per_dir;	/* Client's files_per_dir   */
};

extern _CMD_LINE cmdline;
extern _INTERNAL_CFG internal_cfg;

extern void init_command_line_args (void);
const char *netmist_vfs_errstr (int);

extern void parse_command_line (int argc, char **argv);

extern int is_prime (void);
extern int is_nodeManager (void);
extern int is_client (void);

extern void abort_ifnot_prime (void);
extern void abort_ifnot_nodeManager (void);
extern void abort_ifnot_client (void);

extern void set_netmist_role (int m_flag);

extern char *my_strncpy (char *dest, char *src, size_t n);
extern char *my_strcasestr (char *s, char *find);
extern int my_strcasecmp (char *s1, char *s2);
extern void *my_malloc (size_t size);
extern void *my_malloc2 (size_t size);
extern void my_free (void *ptr, size_t size);

extern int is_empty_line (char *buf);
extern int is_comment (char *buf);

extern void init_vars (void);
extern void re_init_vars (void);

extern void usage (void);
extern void dump_table (void);
extern void import_table (char *iname);

extern int check_dir_w (char *where);

extern void trim_leading_spaces (char *string);

extern void get_my_server_socket (int id, int *socket_id, int *socket_port);

extern int connect_peer_server (char *peer_server_name, int peer_server_port);

extern void socket_send (int socket, char *buffer, int size);

extern unsigned int socket_receive (int socket, char *buffer, unsigned int size);

extern int *server_wait_for_clients (int server_socket, int total_clients);

extern int server_select (int send_socket,
			  int *client_sockets, int total_clients,
			  int result_clients, int state);

extern void server_send_ok_on_socket (int socket, char *send_buffer);
extern void server_send_ok (int id, char *send_buffer);

extern void server_send (int id, char *send_buffer, int size);
extern void server_send_on_socket (int socket_id, char *send_buffer, int size);

extern int my_gethostname (char *buffer, int len);

extern double getepochtime (void);

extern int extern_mon (int flag, char *options);

extern int u_extern_mon (int flag, char *options);

extern int extern_stat (int flag, char *options);

extern void nap (int select_msecs);

extern void strip_extra_slashes (char *, char *);

extern unsigned int crc32_generate(void *data, size_t length);

extern void add_crc32_prime_command (void);
extern void add_crc32_nodeManager_command (void);
extern void add_crc32_client_command (void);
extern void add_crc32_ext_client_command (void);
extern void add_crc32_keepalive_command (void);

extern unsigned int validate_prime_command (const char *from, int command);
extern unsigned int validate_nodeManager_command (const char *from, int command);
extern unsigned int validate_client_command (const char *from, int command);
extern unsigned int validate_keepalive_command (const char *from, int command);

#endif
