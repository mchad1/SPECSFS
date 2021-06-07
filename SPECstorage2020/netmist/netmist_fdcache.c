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
 */
#include "./copyright.txt"
#include "./license.txt"

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#if defined(_linux_)
#include <fcntl.h>
#include <sys/inotify.h>
#endif

#if defined(_solaris_)
#include <port.h>
#include <fcntl.h>
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
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#if !defined(WIN32)
#include <strings.h>
#endif

#include <netmist.h>
#include "netmist_defines.h"
#include "netmist_copyright.h"
#include "netmist_utils.h"
#include "netmist_prime.h"
#include "netmist_nodeManager.h"
#include "netmist_client.h"
#include "netmist_structures.h"
#include "netmist_logger.h"
#include "netmist_fdcache.h"

/*
 * Netmist file descriptor caching and recycling policies
 *
 * Each workload can choose to use an LRU for recycling of file descriptors, or
 * simple caching with no recycle policy.
 *
 *  File descriptor caching is used to avoid calling open() and close()
 * more than necessary.
 * When a file is opened, its file descriptor is cached, and close() is
 * not called.  If LRU caching is enabled in the workload, then open()s that
 * need a file descriptor may:
 *    A. Acquire a new file descriptor and add it to the cache
 *        or
 *    B. Recycle the oldest file descriptor in the file descriptor cache.
 * If the workload chooses to disable the LRU, then the file descriptors
 * will be cached until it has reached the per proc limit.  At that point,
 * future open()s will simply recycle the MRU cache entry.
 *
 * Why is a file descriptor LRU needed ?
 *
 * If a workload has more files than the per proc limit of open file
 * descriptors and there is locality of reference of those files, e.g.
 * percent geometric is non-zero, then using an LRU would help ensure that the
 * most effective caching of the file descriptors is used.  If an LRU were not
 * used, then the "hot" files could get flushed out by the low frequency, but
 * plentiful "cold" file references.
 *
 * Notes:  The file_object contains a pointer to the fd_node entry, and
 *         the open file descriptor value.  When the file_object->file_desc
 *         is non-zero, then there is a cached entry. When a file is cached
 *         and referenced, it is moved to the head of the LRU.  When a new
 *         file descriptor is allocated, and the cache size is at it's max, then
 *         the oldest entry is re-allocated for the new file descriptor, and
 *         placed at the head of the LRU.
 *	   The file_object->fd_link is used to make direct inserts, and
 *         removals on and off the LRU list.  The LRU is a doublly linked
 *         list, so there are no linear searches by using the
 *         file_object->fd_link pointer.
 *
 */

/* Variables associated with the file_objects and the fd caching mechanism */
long long lru_promote_count = 0LL;
long long lru_reuse_count = 0LL;
int cached_open_count = 0;
struct file_object *op_X_array[NUM_OP_TYPES];
struct fd_node *fd_head = NULL;
struct fd_node *fd_tail = NULL;
long long file_accesses;
long long fd_accesses;
long long fd_cache_misses;
int lru_on = 0;

extern int netmist_vfs_close (struct netmist_vfs_object **);

/* Private cache manipulation functions */
int fd_isEmpty (void);
static struct fd_node *fd_insertFirst (struct file_object *);
static struct fd_node *fd_deleteFirst (void);
static struct fd_node *fd_deleteLast (void);
static struct fd_node *fd_delete_link (struct fd_node *);
static struct fd_node *fd_insertFirst_link (struct fd_node *);
static struct fd_node *fd_delete_link (struct fd_node *);
static struct fd_node *fd_insertFirst_link (struct fd_node *);

/*
 * @brief Drop an item from the cache and close any file descriptors.
 *
 * @param link : Pointer to file_object
 */
void
fd_drop_from_cache (struct file_object *fp)
{
    struct fd_node *fd_temp = NULL;
    if (fp->file_desc != 0)
    {
	netmist_vfs_close (&fp->file_desc);
    }
    fp->file_open_flags = 0;
    if (fp->fd_link)
    {
	cached_open_count--;
	fd_temp = (struct fd_node *) fd_delete_link (fp->fd_link);
    }
    if (fd_temp)
    {
	free (fd_temp);
	fd_temp = NULL;
    }
    fp->fd_link = NULL;
}

/*
 * @brief Remove an item from the cache, but do not close any file descriptors, as
 * there may be more than needs to be closed than just this one.
 *
 * @param link : Pointer to file_object
 */
void
fd_remove_from_cache (struct file_object *fp)
{
    struct fd_node *fd_temp = NULL;
    if (fp->file_desc != 0)
    {
	netmist_vfs_close (&fp->file_desc);
    }
    fp->file_open_flags = 0;
    if (fp->fd_link)
    {
	cached_open_count--;
	fd_temp = (struct fd_node *) fd_delete_link (fp->fd_link);
    }
    if (fd_temp)
    {
	free (fd_temp);
	fd_temp = NULL;
    }
    fp->fd_link = NULL;
}

/*
 * @brief Delete first fd_node (head). This is light weight
 *        in CPU costs and used infrequently
 */
/*
 * __doc__
 * __doc__  Function : struct fd_node* fd_deleteFirst()
 * __doc__  Arguments: void
 * __doc__  Returns  : pointer to fd_node struct.
 * __doc__  Performs : Delete first fd_node (head). This is light weight
 * __doc__             in CPU costs and used infrequently
 * __doc__
 */
struct fd_node *
fd_deleteFirst (void)
{
    struct fd_node *tempLink = fd_head;
    if (fd_head->next == NULL)
    {
	fd_tail = NULL;
    }
    else
    {
	fd_head->next->prev = NULL;
    }
    fd_head = fd_head->next;
    if (tempLink == NULL)
    {
	log_file (LOG_ERROR, "fd_deleteFirst returns NULL\n");

    }
    return tempLink;
}

/**
 * @brief Delete fd_node at the tail location. This is light
 *        weight in CPU costs and used frequently.
 */
/*
 * __doc__
 * __doc__  Function : struct fd_node* fd_deleteLast()
 * __doc__  Arguments: void
 * __doc__  Returns  : pointer to fd_node struct
 * __doc__  Performs : Delete fd_node at the tail location. This is light
 * __doc__             weight in CPU costs and used frequently.
 * __doc__
 */

static struct fd_node *
fd_deleteLast (void)
{
    struct fd_node *tempLink = fd_tail;

    log_file (LOG_DEBUG, "fd_deleteLast\n");

    if (fd_head == NULL)
	goto out;
    if (fd_head->next == NULL)
    {
	fd_head = NULL;
    }
    else
    {
	fd_tail->prev->next = NULL;
    }
    fd_tail = fd_tail->prev;
  out:

    log_file (LOG_DEBUG, "fd_deleteLast return\n");

    if (tempLink == NULL)
    {
	log_file (LOG_ERROR, "fd_deleteLast returns NULL\n");

    }
    return tempLink;
}

/*
 * @brief Add an item to the cache.
 *
 * @param fp : Pointer to file object
 */
void
fd_add_to_cache (struct file_object *fp)
{
    fp->fd_link = fd_insertFirst (fp);
    cached_open_count++;
}

/*
 * @brief Promote this item to the head of the list
 *        for an LRU ring.
 *
 * @param fp : Pointer to file_object
 */
void
fd_promote_to_head (struct file_object *fp)
{
    struct fd_node *fd_temp;
    if (fp->fd_link)
    {
	lru_promote_count++;
	fd_temp = fd_delete_link (fp->fd_link);
	fp->fd_link = fd_insertFirst_link (fd_temp);
    }
}

/*
 * @brief Reallocate a cache entry, and close any file descriptors
 * with the item being reused.
 *
 * @param link : Void
 */
int
fd_cache_reuse (void)
{
    struct fd_node *fd_temp;
    int my_close = 0;
    struct file_object *fp_temp = NULL;

    if (lru_on)
	fd_temp = fd_deleteLast ();
    else
	fd_temp = fd_deleteFirst ();
    if (fd_temp)
    {
	fp_temp = fd_temp->key;
	if (fp_temp->file_desc != NULL)
	{
	    netmist_vfs_close (&fp_temp->file_desc);
	    my_close = 1;
	}
	fp_temp->file_open_flags = 0;
	fp_temp->fd_link = NULL;
	cached_open_count--;
	free (fd_temp);
	fd_temp = NULL;
	lru_reuse_count++;
    }
    if (my_close)
	return (1);
    else
	return (0);
}

/*
 * @brief Return the number of items on the list
 */
/*
 * __doc__
 * __doc__  Function : int fd_list_length(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int: Length of the list.
 * __doc__  Performs : Return the number of items on the list
 * __doc__
 */
int
fd_list_length (void)
{
    int length = 0;
    struct fd_node *current;
    for (current = fd_head; current != NULL; current = current->next)
    {
	length++;
    }
    return length;
}

/**
 * @brief Delete a link with given fd_link.  This is light
 *        weight in CPU costs and used in the LRU.
 *
 * @param[in] link :  Pointer to fd_node struct
 */
/*
 * __doc__
 * __doc__  Function : struct fd_node * fd_delete_link(struct fd_node *link)
 * __doc__  Arguments: pointer to fd_node struct
 * __doc__  Returns  : pointer to fd_node struct.
 * __doc__  Performs : Delete a link with given fd_link.  This is light
 * __doc__             weight in CPU costs and used in the LRU.
 * __doc__
 */

static struct fd_node *
fd_delete_link (struct fd_node *link)
{
    if (link == NULL)
    {
	log_file (LOG_ERROR, "fd_delete_link %p\n", link);

    }
    if (link == NULL)
	return (NULL);
    if (link == fd_head)
    {
	fd_head = fd_head->next;
    }
    else
    {
	link->prev->next = link->next;
    }
    if (link == fd_tail)
    {
	fd_tail = link->prev;
    }
    else
    {
	link->next->prev = link->prev;
    }

    log_file (LOG_DEBUG, "fd_delete_link return\n");

    return (link);
}

/**
 * @brief Move an existing fd_node with given fd_link.  This is
 *        light weight in CPU costs and used frequently for an LRU.
 *
 * @param link : Pointer to struct fd_node to insert
 */
/*
 * __doc__
 * __doc__  Function : struct fd_node *
 * __doc__              fd_insertFirst_link (struct fd_node *link)
 * __doc__  Arguments: Pointer to struct fd_node
 * __doc__  Returns  : Pointer to struct fd_node
 * __doc__  Performs : Move an existing fd_node with given fd_link.  This is
 * __doc__             light weight in CPU costs and used frequently for an LRU.
 * __doc__
 */

static struct fd_node *
fd_insertFirst_link (struct fd_node *link)
{
    log_file (LOG_DEBUG, "fd_insertFirst_link\n");

    if (link == NULL)
    {
	log_file (LOG_ERROR, "fd_insertFirst_link Error.\n");
	return (NULL);
    }
    link->next = NULL;
    if (fd_isEmpty ())
    {
	fd_tail = link;
    }
    else
    {
	fd_head->prev = link;
    }
    link->next = fd_head;
    fd_head = link;

    log_file (LOG_DEBUG, "fd_insertFirst_link return\n");

    return (link);
}

/**
 * @brief Check for empty fd_head of list.
 */
/*
 * Is the list empty
 */
int
fd_isEmpty (void)
{
    if (fd_head == NULL)
	return (1);
    else
	return (0);
}

/**
 * @brief Insert new fd_node at the first location (head of list)
 *        This is light weight in CPU costs and used frequently.
 *        The "file_object" is needed so that recycles can close()
 *        and clear the file_desc and re-allocated the file
 *        descriptor to the new entry.
 *
 * @param key : Unique to a file.
 */
/*
 * __doc__
 * __doc__  Function : struct fd_node * fd_insertFirst(struct file_object *key)
 * __doc__  Arguments: struct file object. Unique to a file.
 * __doc__  Returns  : struct fd_node pointer.
 * __doc__  Performs : Insert new fd_node at the first location (head of list)
 * __doc__             This is light weight in CPU costs and used frequently.
 * __doc__             The "file_object" is needed so that recycles can close()
 * __doc__             and clear the file_desc and re-allocated the file
 * __doc__             descriptor to the new entry.
 * __doc__
 */
struct fd_node *
fd_insertFirst (struct file_object *key)
{
    struct fd_node *link;

    link = (struct fd_node *) my_malloc (sizeof (struct fd_node));
    if (link == 0)
	exit (99);		/* Can't ever get here !! */
    link->next = NULL;
    link->key = key;

    log_file (LOG_DEBUG, "fd_insertFirst\n");

    if (fd_isEmpty ())
    {
	fd_tail = link;
    }
    else
    {
	fd_head->prev = link;
    }
    link->next = fd_head;
    fd_head = link;

    log_file (LOG_DEBUG, "fd_insertFirst return\n");

    if (link == NULL)
    {
	log_file (LOG_ERROR, "fd_insertFirst returns null error\n");

    }
    return (link);
}
