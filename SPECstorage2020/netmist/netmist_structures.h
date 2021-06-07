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

#ifndef __NETMIST_STRUCTURES_H__
#define __NETMIST_STRUCTURES_H__


#define NODEMANAGER_SUCCESS 1
#define NODEMANAGER_FAILURE 0

#define WORKLOAD_FOUND 1
#define WORKLOAD_NOT_FOUND 0

extern unsigned int get_nodeManager_count (void);

extern char *get_nodeManager_name (int nodeManager_id);

extern int get_nodeManager_pid (int nodeManager_id);
extern int set_nodeManager_pid (int nodeManager_id, int pid);

extern int get_nodeManager_port (int nodeManager_id);
extern int set_nodeManager_port (int nodeManager_id, int port);

extern int get_nodeManager_socket (int nodeManager_id);
extern int set_nodeManager_socket (int nodeManager_id, int socket);
extern int set_nodeManager_keepalive (int nodeManager_id, unsigned int keepalive);

extern int *get_nodeManager_client_list (int nodeManager_id,
					 int *client_count);

extern int get_nodeManager_client_id (int i);

extern int prime_check_and_add_nodeManager_object (char *NodeManager_name,
						   int client_id);

extern int nm_add_nodeManager_object (char *name, int id, int pid, int port,
				      int listen_socket, int prime_socket,
				      int num_clients, int *client_ids);

extern int get_nm_client_port (int client_id);
extern int set_nm_client_port (int client_id, int port);

extern int get_nm_client_pid (int client_id);
extern int set_nm_client_pid (int client_id, int pid);

extern int get_nm_client_socket (int client_id);
extern int set_nm_client_socket (int client_id, int socket);

extern int set_nm_client_keepalive (int client_id, unsigned int keepalive);
extern int set_nm_prime_keepalive (unsigned int keepalive);

extern int get_nm_client_files (int client_id);
extern int set_nm_client_files (int client_id, int files);

extern int get_nm_client_dirs (int client_id);
extern int set_nm_client_dirs (int client_id, int dirs);

extern void print_nodeManager_objects (void);

extern int check_and_add_workloads (char *workload_name, char *workdir);
extern void cleanup_workloads (void);

extern int run_prime_keepalive_scanner (unsigned int keepalive);

extern int run_nodeManager_keepalive_scanner (unsigned int keepalive);

#endif
