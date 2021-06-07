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
#include "./copyright.txt"
#include "./license.txt"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#if !defined(WIN32)
#include<unistd.h>
#endif
#include<time.h>
#if !defined(_macos_) && !defined(_bsd_)
#include<malloc.h>
#endif

/* This is different for the various versions. */
#include <netmist.h>

#include "netmist_hashtable.h"
#include "netmist_utils.h"
#include "netmist_structures.h"
#include "netmist_logger.h"

/*
 * nodeManager count
 * Only read-only get'er method is exported for this variable
 */
static unsigned int nodeManager_count = 0;

/*
 * This structure cross-references the client_obj structure
 *
 * This structure is used to spawn nodeManager processes and Prime further
 * uses it to cross-reference client_object to send the client related 
 * information to nodeManager when it is initialized.
 *
 * Currently this is implemented as a linked-list but later needs to be 
 * reimplmented as hash table because of performance reasons if the clients
 * scale
 *
 * This gets populated from read_client_tok_file function
 */
struct _client_ids
{
    /* Used by both Prime and NodeManager */
    int client_id;
    int client_pid;
    /* Used by nodeManager only */
    int client_socket;
    int client_port;
    int files;
    int dirs;
    unsigned int k_count;

    struct _client_ids *next;
};

struct _nodeManager
{
    int nodeManager_id;
    int nodeManager_pid;	/* process id on the remote client */
    int nodeManager_port;
    int nodeManager_socket;
    unsigned int k_count;
    /* Used by nodeManager only */
    int nodeManager_prime_socket;
    /* references char client_name[MAXHNAME]; */
    char *nodeManager_name;
    /* single linked list next pointer */
    struct _nodeManager *next;
    int number_of_clients;
    /* linked-list of client ids cross referenced to client_object index */
    struct _client_ids *client_id_list;
};

typedef struct _client_ids client_ids;
typedef struct _nodeManager nodeManager;

static nodeManager *nodeManager_object = NULL;

static nodeManager **nodeManager_array = NULL;

/*
 * Only used by nodeManager program
 */
static void *client_array = NULL;

struct _workdirs
{
    char *workdir_name;
    struct _workdirs *next;
};

typedef struct _workdirs workdirs;

/*
 * This is workload name hash to control op_validate()
 */
struct _workloads
{
    char *workload_name;
    workdirs *workdir_list;
    struct _workloads *next;
};

typedef struct _workloads workloads;

static workloads *workload_list = NULL;

/**
 * @brief Return the node manager count.
 */
unsigned int
get_nodeManager_count (void)
{
    return nodeManager_count;
}

/**
 * @brief Increment the node manager count.
 */
static void
inc_nodeManager_count (void)
{
    nodeManager_count++;
}

/**
 * @brief Return the nodeManager's name.
 * Called by the prime.
 *
 * @param nodeManager_id : Numeric index into the nodeManager_array.
 */
char *
get_nodeManager_name (int nodeManager_id)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return NULL;
    }

    return (nm->nodeManager_name);
}

/**
 * @brief Return the nodeManager's PID.
 * Called by the prime.
 *
 * @param nodeManager_id : Numeric index into the nodeManager_array.
 */
int
get_nodeManager_pid (int nodeManager_id)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    return (nm->nodeManager_pid);
}

/**
 * @brief Set the nodeManager's PID.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 * @param pid : The pid of this nodeManager.
 */
int
set_nodeManager_pid (int nodeManager_id, int pid)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    nm->nodeManager_pid = pid;
    return 1;
}

/**
 * @brief Return the nodeManager's TCP port.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 */
int
get_nodeManager_port (int nodeManager_id)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    return (nm->nodeManager_port);
}

/**
 * @brief Set the nodeManager's TCP port.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 * @param port : TCP port value.
 */
int
set_nodeManager_port (int nodeManager_id, int port)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    nm->nodeManager_port = port;
    return 1;
}

/**
 * @brief Return the nodeManager's socket.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 */
int
get_nodeManager_socket (int nodeManager_id)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    return (nm->nodeManager_socket);
}

/**
 * @brief Set the nodeManager's socket.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 * @param socket : socket fd.
 */
int
set_nodeManager_socket (int nodeManager_id, int socket)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    nm->nodeManager_socket = socket;
    return 1;
}

/**
 * @brief Set the nodeManager's keepalive count.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 * @param keepalive : keepalive count.
 */
int
set_nodeManager_keepalive (int nodeManager_id, unsigned int keepalive)
{
    nodeManager *nm = nodeManager_array[nodeManager_id];

    /*
     * This should not happen
     */
    if (nm == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return 0;
    }

    nm->k_count = keepalive;
    return 1;
}


/**
 * @brief Return the nodemanager's client list.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 * @param client_count : Pointer to the count of the number of clients in the list
 */

int *
get_nodeManager_client_list (int nodeManager_id, int *client_count)
{
    int index = 0;
    int *client_list;
    nodeManager *nm = nodeManager_array[nodeManager_id];
    client_ids *curr_client_id;

    /*
     * This should not happen
     */
    if (nm == NULL || nm->client_id_list == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	*client_count = 0;
	return NULL;
    }

    /*
     * Allocate the array of clients
     * Caller will free this memory
     */
    client_list = my_malloc (sizeof (int) * (nm->number_of_clients));
    if (client_list == NULL)
    {
	*client_count = 0;
	return NULL;
    }

    *client_count = nm->number_of_clients;

    curr_client_id = nm->client_id_list;

    /*
     * iterate through all clients under this nodeManager
     */
    while (curr_client_id)
    {
	client_list[index] = curr_client_id->client_id;
	curr_client_id = curr_client_id->next;
	index++;
    }

    *client_count = nm->number_of_clients;
    return client_list;
}


/** 
 * @brief Allocate the client_ids_objects
 */
static inline client_ids *
allocate_client_ids_object (void)
{
    return ((client_ids *) my_malloc (sizeof (client_ids)));
}

/** 
 * @brief Allocate the nodeManager object.
 */
static inline nodeManager *
allocate_nodeManager_object (void)
{
    return ((nodeManager *) my_malloc (sizeof (nodeManager)));
}

/** 
 * @brief Return the NodeManager's first client_id.
 *
 * @param nodeManager_id : Index into the NodeManager array.
 */
int
get_nodeManager_client_id (int nodeManager_id)
{
    nodeManager *curr_nm_node = nodeManager_array[nodeManager_id];

    /* This should not happen */
    if (curr_nm_node == NULL || curr_nm_node->client_id_list == NULL)
    {
	log_file (LOG_ERROR, "Failed to lookup nodeManager in %s\n",
		  __FUNCTION__);
	return -1;
    }

    /* Just return the first client ID */
    return (curr_nm_node->client_id_list->client_id);
}

/**
 * @brief Check and add a client to the specified nodeManager.
 *
 * @param NodeManager_name : Name of the nodeManager to use.
 * @param client_id : Client id of the client to add to this nodemanager.
 */
int
prime_check_and_add_nodeManager_object (char *NodeManager_name, int client_id)
{
    nodeManager *curr_nm_node = nodeManager_object;
    nodeManager *prev_nm_node = NULL;

    client_ids *curr_c_ids = NULL;
    client_ids *prev_c_ids = NULL;

    static int do_once = 0;

    if (!do_once)
    {
	do_once = 1;
	nodeManager_array =
	    (nodeManager **) my_malloc2 (sizeof (nodeManager *) * MAX_NMS);
	log_file (LOG_DATABASE, "Creating nodeManager hash table\n");
    }

    /* first node in the linked-list */
    if (nodeManager_object == NULL)
    {
	log_file (LOG_DATABASE, "Adding a new nodeManager %s in database\n",
		  NodeManager_name);
	nodeManager_object = allocate_nodeManager_object ();
	if (nodeManager_object == NULL)
	{
	    log_file (LOG_ERROR, "Failed to allocate nodeManager in %s\n",
		      __FUNCTION__);
	    return NODEMANAGER_FAILURE;
	}

	nodeManager_object->nodeManager_name = NodeManager_name;
	/* first node in the client-id linked-list */
	nodeManager_object->client_id_list = allocate_client_ids_object ();
	if (nodeManager_object->client_id_list == NULL)
	{
	    log_file (LOG_ERROR, "Failed to allocate client list in %s\n",
		      __FUNCTION__);
	    return NODEMANAGER_FAILURE;
	}

	nodeManager_object->client_id_list->client_id = client_id;
	nodeManager_object->number_of_clients++;
	nodeManager_object->nodeManager_id = get_nodeManager_count ();
	/* Store in the hash */
	nodeManager_array[nodeManager_object->nodeManager_id] =
	    nodeManager_object;
	inc_nodeManager_count ();

	log_file (LOG_DATABASE, "Client id %d added. Total clients %d\n",
		  client_id, nodeManager_object->number_of_clients);

	return NODEMANAGER_SUCCESS;
    }

    /* 
     * List is not empty
     *
     * Find the name in the list
     *
     * If found, just append the client id to this name
     *
     * If not found, create a new one
     *
     */
    while (curr_nm_node)
    {
	prev_nm_node = curr_nm_node;
	/* Check for a match */
	if (my_strcasecmp (curr_nm_node->nodeManager_name,
			   NodeManager_name) == 0)
	{

	    log_file (LOG_DATABASE,
		      "Found existing nodeManager %s in database\n",
		      NodeManager_name);
	    curr_c_ids = curr_nm_node->client_id_list;

	    /*
	     * Append the client id to the existing list
	     */
	    while (curr_c_ids)
	    {
		prev_c_ids = curr_c_ids;
		/* sanity check 
		 * this should never happen! */
		if (curr_c_ids->client_id == client_id)
		{
		    log_file (LOG_ERROR, "duplicate client id in %s\n",
			      __FUNCTION__);
		    return NODEMANAGER_FAILURE;
		}
		curr_c_ids = curr_c_ids->next;
	    }

	    prev_c_ids->next = allocate_client_ids_object ();
	    if (prev_c_ids->next == NULL)
	    {
		return NODEMANAGER_FAILURE;
	    }
	    prev_c_ids->next->client_id = client_id;
	    curr_nm_node->number_of_clients++;

	    log_file (LOG_DATABASE, "Client id %d added. Total clients %d\n",
		      client_id, curr_nm_node->number_of_clients);

	    return NODEMANAGER_SUCCESS;
	}

	curr_nm_node = curr_nm_node->next;
    }

    log_file (LOG_DATABASE, "Adding a new nodeManager in database\n");
    /* name is not found, allocate a new one */
    prev_nm_node->next = allocate_nodeManager_object ();
    if (prev_nm_node->next == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate nodeManager in %s\n",
		  __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }
    prev_nm_node->next->nodeManager_name = NodeManager_name;
    /* first node in the client-id linked-list */
    prev_nm_node->next->client_id_list = allocate_client_ids_object ();
    if (prev_nm_node->next->client_id_list == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate client list in %s\n",
		  __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }

    prev_nm_node->next->client_id_list->client_id = client_id;
    prev_nm_node->next->number_of_clients++;
    prev_nm_node->next->nodeManager_id = get_nodeManager_count ();

    /* Store in the hash */
    nodeManager_array[prev_nm_node->next->nodeManager_id] =
	prev_nm_node->next;

    inc_nodeManager_count ();

    log_file (LOG_DATABASE, "Client id %d added. Total clients %d\n",
	      client_id, prev_nm_node->next->number_of_clients);

    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Add a nodeManager object.
 *
 * @param name : Name of this nodeManager.
 * @param id : Id of this nodeManager
 * @param pid : PID of this nodeManager
 * @param port : TCP port of this nodeManager.
 * @param listen_socket : Listen socket for this NM.
 * @param prime_socket : Socket fd for the Prime.
 * @param num_clients : Number of clients on this nodeManager.
 * @param client_id_list : Pointer ot the list of client ids.
 */
/*
 * Called from nodeManager 
 */
int
nm_add_nodeManager_object (char *name, int id, int pid, int port,
			   int listen_socket, int prime_socket,
			   int num_clients, int *client_id_list)
{
    int i;
    client_ids *client_node;

    /* 
     * When called from nodeManager process, only one nodeManager is
     * allocated
     */

    if (nodeManager_object != NULL)
    {
	log_file (LOG_ERROR, "Multiple nodeManager addition in %s\n",
		  __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }

    nodeManager_object = allocate_nodeManager_object ();
    if (nodeManager_object == NULL)
    {
	log_file (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }

    nodeManager_object->nodeManager_name = name;
    nodeManager_object->nodeManager_id = id;
    nodeManager_object->nodeManager_pid = pid;
    nodeManager_object->nodeManager_port = port;
    nodeManager_object->nodeManager_socket = listen_socket;
    nodeManager_object->nodeManager_prime_socket = prime_socket;
    nodeManager_object->number_of_clients = num_clients;

    if (num_clients > MAXCLIENTS_PER_NM)
    {
	log_file (LOG_ERROR,
		  "Number of clients %d exceeds program limit of %d in %s\n",
		  num_clients, MAXCLIENTS_PER_NM, __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }

    client_array = create_hash_table(MAXCLIENTS_PER_NM);

    if (client_array == NULL)
    {
	log_file (LOG_ERROR, "malloc failure in %s\n", __FUNCTION__);
	return NODEMANAGER_FAILURE;
    }

    /*
     * Now allocate the client array
     */
    for (i = 0; i < num_clients; i++)
    {
	client_node = allocate_client_ids_object ();
	if (client_node == NULL)
	{
	    log_file (LOG_ERROR, "\nclients: malloc failure\n");
	    return NODEMANAGER_FAILURE;
	}

	client_node->client_id = client_id_list[i];

        if (add_hash_entry(client_array, client_node->client_id, 
                        (void *)client_node) != HASH_SUCCESS) 
        {
            log_file (LOG_ERROR, "\nclients: failed to add entry in hashtable\n");
            return NODEMANAGER_FAILURE;
        }

	/*
	 * Add to the front of the linked list
	 */
	if (nodeManager_object->client_id_list != NULL)
	{
	    client_node->next = nodeManager_object->client_id_list;
	}

	nodeManager_object->client_id_list = client_node;
    }

    inc_nodeManager_count ();

    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Return the specifed client_id's TCP port.
 *
 * @param client_id : Client id specified.
 */
int
get_nm_client_port (int client_id)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);
    if (curr_client == NULL)
    {
	return 0;
    }
    return (curr_client->client_port);
}

/**
 * @brief Set the specifed client_id's TCP port.
 *
 * @param client_id : Client id specified.
 * @param port : TCP port number
 */
int
set_nm_client_port (int client_id, int port)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->client_port = port;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Return the specifed client_id's PID.
 *
 * @param client_id : Client id specified.
 */
int
get_nm_client_pid (int client_id)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return 0;
    }

    return (curr_client->client_pid);
}

/**
 * @brief Set the specifed client_id's PID.
 *
 * @param client_id : Client id specified.
 * @param pid : PID of this client.
 */
int
set_nm_client_pid (int client_id, int pid)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->client_pid = pid;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Return the specifed client_id's socket fd.
 *
 * @param client_id : Client id specified.
 */
int
get_nm_client_socket (int client_id)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return 0;
    }

    return (curr_client->client_socket);
}

/**
 * @brief Set the specifed client_id's socket fd.
 *
 * @param client_id : Client id specified.
 * @param socket : Socket fd for this client.
 */
int
set_nm_client_socket (int client_id, int socket)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->client_socket = socket;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Set the specifed client_id's keepalive count.
 *
 * @param client_id : Client id specified.
 * @param keepalive : client keepalive count.
 */
int
set_nm_client_keepalive (int client_id, unsigned int keepalive)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->k_count = keepalive;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Set primes keepalive count.
 *
 * @param keepalive : prime keepalive count.
 */
int
set_nm_prime_keepalive (unsigned int keepalive)
{
    nodeManager_object->k_count = keepalive;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Return the specifed client_id's number of files
 *
 * @param client_id : Client id specified.
 */
int
get_nm_client_files (int client_id)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return 0;
    }

    return (curr_client->files);
}

/**
 * @brief Set the specifed client_id's files.
 *
 * @param client_id : Client id specified.
 * @param files : number of files for this client.
 */
int
set_nm_client_files (int client_id, int files)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);
    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->files = files;
    return NODEMANAGER_SUCCESS;
}

/**
 * @brief Return the specifed client_id's number of dirs
 *
 * @param client_id : Client id specified.
 */
int
get_nm_client_dirs (int client_id)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return 0;
    }

    return (curr_client->dirs);
}

/**
 * @brief Set the specifed client_id's dirs.
 *
 * @param client_id : Client id specified.
 * @param dirs : number of dirs for this client.
 */
int
set_nm_client_dirs (int client_id, int dirs)
{
    client_ids *curr_client;

    curr_client = (client_ids *) lookup_hash_entry(client_array, client_id);

    if (curr_client == NULL)
    {
	return NODEMANAGER_FAILURE;
    }

    curr_client->dirs = dirs;
    return NODEMANAGER_SUCCESS;
}


/**
 * @brief Print the nodeManager object contents.
 */
void
print_nodeManager_objects (void)
{
    nodeManager *curr_nm_node = nodeManager_object;

    client_ids *curr_c_ids;

    while (curr_nm_node)
    {
	if (is_prime ())
	{
	    log_all (LOG_CONFIG_VERBOSE, "\n\n");
	    log_all (LOG_CONFIG_VERBOSE, "nodeManager: Id->%d name->%s\n",
		     curr_nm_node->nodeManager_id,
		     curr_nm_node->nodeManager_name);
	    log_all (LOG_CONFIG_VERBOSE, "nodeManager clients:\n   ");
	}
	else
	{
	    log_file (LOG_CONFIG_VERBOSE, "\n\n");
	    log_file (LOG_CONFIG_VERBOSE, "nodeManager: Id->%d name->%s\n",
		      curr_nm_node->nodeManager_id,
		      curr_nm_node->nodeManager_name);
	    log_file (LOG_CONFIG_VERBOSE, "nodeManager clients:\n   ");
	}

	curr_c_ids = curr_nm_node->client_id_list;

	while (curr_c_ids)
	{
	    if (is_prime ())
	    {
		log_all (LOG_CONFIG_VERBOSE, " %u ", curr_c_ids->client_id);
	    }
	    else
	    {
		log_file (LOG_CONFIG_VERBOSE, " %u ", curr_c_ids->client_id);
	    }

	    curr_c_ids = curr_c_ids->next;
	}

	curr_nm_node = curr_nm_node->next;

	if (is_prime ())
	{
	    log_all (LOG_CONFIG_VERBOSE, "\n\n");
	}
	else
	{
	    log_file (LOG_CONFIG_VERBOSE, "\n\n");
	}
    }
}

/**
 * @brief  Create the workdir_object for this client.
 *
 * @param workdir_name : Name of the workdir object.
 */
static workdirs *
create_workdir_object (char *workdir_name)
{
    workdirs *tmp_workdir;
    int workdir_length = (int) strlen (workdir_name);

    tmp_workdir = (workdirs *) my_malloc (sizeof (workdirs));
    if (tmp_workdir == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for workdir object\n");
	exit (1);
    }

    tmp_workdir->workdir_name =
	(char *) my_malloc (sizeof (char) * (workdir_length + 1));
    if (tmp_workdir->workdir_name == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate memory for workdir name\n");
	exit (1);
    }

    /* Don't forget that the length is the absolute max, including the NULL at the end !!! */
    my_strncpy (tmp_workdir->workdir_name, workdir_name, workdir_length + 1);

    log_file (LOG_DATABASE, "Added workdir %s to the database\n",
	      workdir_name);

    return tmp_workdir;
}


/** 
 * @brief Create the workload_object.
 *
 * @param workload_name : Name of the workload.
 * @param workdir_name : Name of thw workdir.
 */
static workloads *
create_workload_object (char *workload_name, char *workdir_name)
{
    workloads *tmp_workload;
    int workload_length = (int) strlen (workload_name);

    tmp_workload = (workloads *) my_malloc (sizeof (workloads));
    if (tmp_workload == NULL)
    {
	log_file (LOG_ERROR,
		  "Failed to allocate memory for workload object\n");
	exit (1);
    }

    tmp_workload->workload_name =
	(char *) my_malloc (sizeof (char) * (workload_length + 1));
    if (tmp_workload->workload_name == NULL)
    {
	log_file (LOG_ERROR, "Failed to allocate memory for workload name\n");
	exit (1);
    }

    /* Don't forget that the length is the absolute max, including the NULL at the end !!! */
    my_strncpy (tmp_workload->workload_name, workload_name,
		workload_length + 1);

    tmp_workload->workdir_list = create_workdir_object (workdir_name);

    log_file (LOG_DATABASE, "Added workload %s to the database\n",
	      workload_name);

    return tmp_workload;
}

/**
 * @brief  Check and add workloads.
 *
 * @param workload_name : Workload Name.
 * @param workdir_name : workdir name.
 */
int
check_and_add_workloads (char *workload_name, char *workdir_name)
{
    workloads *curr_workload;
    workdirs *curr_workdir;

    if (workload_name == NULL || workload_name[0] == '\0')
    {
	log_file (LOG_ERROR, "Invalid workload in %s\n", __FUNCTION__);
	return WORKLOAD_NOT_FOUND;
    }

    if (workdir_name == NULL || workdir_name[0] == '\0')
    {
	log_file (LOG_ERROR, "Invalid workdir in %s\n", __FUNCTION__);
	return WORKLOAD_NOT_FOUND;
    }

    log_file (LOG_DATABASE, "Checking for workload %s\n", workload_name);

    if (workload_list == NULL)
    {
	workload_list = create_workload_object (workload_name, workdir_name);
	return WORKLOAD_NOT_FOUND;
    }

    curr_workload = workload_list;

    while (curr_workload)
    {
	if (strcmp (curr_workload->workload_name, workload_name) == 0)
	{
	    log_file (LOG_DATABASE, "workload %s found in the database\n",
		      workload_name);
	    log_file (LOG_DATABASE, "Checking for workdir %s\n",
		      workdir_name);

	    curr_workdir = curr_workload->workdir_list;

	    while (curr_workdir)
	    {
		if (strcmp (curr_workdir->workdir_name, workdir_name) == 0)
		{
		    log_file (LOG_DATABASE,
			      "workdir %s found in the database\n",
			      workdir_name);
		    return WORKLOAD_FOUND;
		}

		curr_workdir = curr_workdir->next;
	    }

	    curr_workdir = create_workdir_object (workdir_name);

	    /*
	     * Add to the head of the list
	     */
	    curr_workdir->next = curr_workload->workdir_list;
	    curr_workload->workdir_list = curr_workdir;

	    return WORKLOAD_NOT_FOUND;
	}

	curr_workload = curr_workload->next;
    }

    curr_workload = create_workload_object (workload_name, workdir_name);

    /*
     * Add to the head of the list
     */
    curr_workload->next = workload_list;
    workload_list = curr_workload;

    return WORKLOAD_NOT_FOUND;
}

/**
 * @brief Cleanup / Free workload resources.
 */
void
cleanup_workloads (void)
{
    workloads *curr_workload = workload_list, *next_workload;
    workdirs *curr_workdir, *next_workdir;

    log_file (LOG_DATABASE, "In %s\n", __FUNCTION__);

    while (curr_workload)
    {
	curr_workdir = curr_workload->workdir_list;

	while (curr_workdir)
	{
	    next_workdir = curr_workdir->next;

	    log_file (LOG_DATABASE, "Freeing up workload dir %s\n",
		      curr_workdir->workdir_name);

	    free (curr_workdir->workdir_name);
	    curr_workdir->workdir_name = NULL;
	    free (curr_workdir);
	    curr_workdir = NULL;

	    curr_workdir = next_workdir;
	}

	next_workload = curr_workload->next;

	log_file (LOG_DATABASE, "Freeing workload name %s\n",
		  curr_workload->workload_name);

	free (curr_workload->workload_name);
	curr_workload->workload_name = NULL;
	free (curr_workload);
	curr_workload = NULL;

	curr_workload = next_workload;
    }

    workload_list = NULL;
}

/**
 * @brief  Check to see if any client has missed sending a heartbeat
 *
 * @param keepalive : Current keepalive value.
 */
int
run_nodeManager_keepalive_scanner (unsigned int keepalive)
{
    int diff;
    client_ids *client_node = nodeManager_object->client_id_list;

    while (client_node)
    {
	diff = keepalive - client_node->k_count;
	if (diff > KEEPALIVE_MISSED_COUNT)
	{
	    return client_node->client_id;
	}

	client_node = client_node->next;
    }

    /*
     * Check for Prime
     */
    diff = keepalive - nodeManager_object->k_count;
    if (diff > 2)
    {
	return -1;
    }

    return -2;
}

/**
 * @brief  Check to see if any nodemanager has missed sending a heartbeat
 *
 * @param keepalive : Current keepalive value.
 */
int
run_prime_keepalive_scanner (unsigned int keepalive)
{
    int diff;
    nodeManager *nm_node = nodeManager_object;

    while (nm_node)
    {
	diff = keepalive - nm_node->k_count;
	if (diff > KEEPALIVE_MISSED_COUNT)
	{
	    return nm_node->nodeManager_id;
	}

	nm_node = nm_node->next;
    }

    return -2;
}
