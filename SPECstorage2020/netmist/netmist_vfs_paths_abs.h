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

#ifndef NETMIST_VFS_PATH_ABS_H_
#define NETMIST_VFS_PATH_ABS_H_

struct netmist_vfs_rock;

struct netmist_vfs_dir
{
    struct netmist_vfs_dir *parent;
    int refcount;
    char *abspath;
};

int nvpa_init_root (char *rootdir, struct netmist_vfs_dir **out);
int nvpa_walk (struct netmist_vfs_rock *,
	       struct netmist_vfs_dir *, char *, struct netmist_vfs_dir **);
int nvpa_pdir (struct netmist_vfs_rock *,
	       struct netmist_vfs_dir *, struct netmist_vfs_dir **);
char *nvpa_pathptr (struct netmist_vfs_rock *, struct netmist_vfs_dir *);
void nvpa_dfree (struct netmist_vfs_rock *, struct netmist_vfs_dir **);

#endif
