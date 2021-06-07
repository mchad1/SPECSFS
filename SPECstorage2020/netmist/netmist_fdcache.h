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

#include <sys/types.h>
#include <netmist.h>
#include "netmist_vfs.h"

extern struct file_object *op_X_array[NUM_OP_TYPES];
extern int cached_open_count;
extern long long lru_promote_count;
extern long long lru_reuse_count;
extern struct fd_node *fd_head;
extern struct fd_node *fd_tail;
extern long long file_accesses;
extern long long fd_accesses;
extern long long fd_cache_misses;
extern int lru_on;

struct fd_node
{
    struct file_object *key;
    struct fd_node *next;
    struct fd_node *prev;
};

struct file_object
{
    struct netmist_vfs_dir *dir;
    char *relfilename;		/* Relative to dir; VFS API cannot express absolute paths */

    long op_count;
    long access_count;
    long long db_access_count;
    long long which_spot;
    unsigned long long Current_fsize;
    unsigned long long Original_fsize;

    struct netmist_vfs_object *file_desc;

    struct fd_node *fd_link;	/* Pointer to fd_node caching struct entry */
    struct netmist_vfs_watch *vfs_notify;
    enum netmist_vfs_open_flags file_open_flags;

    /* Concurency for random ops */
    uint64_t offset;

    /* Concurency for sequential read ops */
    uint64_t seqread_offset;

    /* Concurency for sequential write ops */
    uint64_t seqwrite_offset;

    /* Concurency for whole file read ops */
    uint64_t wholefileread_offset;

    /* Concurency for whole file write ops */
    uint64_t wholefilewrite_offset;

    /* Concurency for copy file ops */
    uint64_t copyfile_offset;

    /* Concurency for rmw ops */
    uint64_t rmw_offset;

    /* Concurency for mmapread ops */
    uint64_t mmapread_offset;

    /* Concurency for mmapwrite ops */
    uint64_t mmapwrite_offset;

    int dedup_group;
};

int fd_isEmpty (void);
int fd_list_length (void);

void fd_add_to_cache (struct file_object *);
void fd_drop_from_cache (struct file_object *);
int fd_cache_reuse (void);
void fd_remove_from_cache (struct file_object *);
void fd_promote_to_head (struct file_object *);
