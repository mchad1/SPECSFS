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

#ifndef _NETMIST_WORKFUN_H_
#define _NETMIST_WORKFUN_H_

extern struct file_object *generic_bmake_path (int, int);
extern struct file_object *generic_make_path (int, int);

extern void generic_empty_fileop_int (struct file_object *, int);
extern void generic_empty_fileop (struct file_object *);
extern void generic_empty_int_fileop (int, struct file_object *);
extern void generic_empty (void);
extern int generic_iempty_dir_s (struct netmist_vfs_dir *, char *);
extern int generic_iempty_dir (struct netmist_vfs_dir *);

extern void generic_access (struct file_object *);
extern void generic_append (struct file_object *);
extern void generic_arm_cancel (struct file_object *);
extern void generic_arm_notify (struct file_object *);
extern void generic_binit_file (int, struct file_object *);
extern void generic_bread_rand (struct file_object *);
extern void generic_bread (struct file_object *);
extern void generic_bsetup (void);
extern void generic_bteardown (void);
extern void generic_bwrite_rand (struct file_object *);
extern void generic_bwrite (struct file_object *);
extern void generic_chmod (struct file_object *);
extern void generic_copyfile (struct file_object *);
extern void generic_create (struct file_object *, int);
extern void generic_custom1 (struct file_object *);
extern void generic_custom2 (struct file_object *);
extern void generic_fill_buf (long, long, int, char *);
extern int generic_impersonate (void *);
extern int generic_init_dir (struct netmist_vfs_dir *);
extern int generic_init_empty_file (struct netmist_vfs_dir *, char *);
extern void generic_init_file (int, struct file_object *);
extern void generic_locking (struct file_object *);
extern void *generic_make_impersonate_context (void);
extern void generic_mkdir (struct file_object *);
extern void generic_mmap_read (struct file_object *);
extern void generic_mmap_write (struct file_object *);
extern void generic_pathconf (struct file_object *);
extern void generic_prefix_op (int, struct file_object *);
extern void generic_postfix_op (int, struct file_object *);
extern void generic_readdir (struct file_object *);
extern void generic_read_file (struct file_object *);
extern void generic_read_rand (struct file_object *);
extern void generic_read (struct file_object *);
extern int generic_remove_dir (struct netmist_vfs_dir *);
extern int generic_remove_file (struct netmist_vfs_dir *, char *);
extern void generic_rename (struct file_object *);
extern void generic_rmdir (struct file_object *);
extern void generic_rmw (struct file_object *);
extern void generic_setup (void);
extern void generic_statfs (struct file_object *);
extern void generic_stat (struct file_object *);
extern void generic_neg_stat (struct file_object *);
extern int generic_stat_workdir (struct netmist_vfs_dir *);
extern void generic_teardown (void);
extern void generic_trunc (struct file_object *, int);
extern void generic_unlink2 (struct file_object *, int);
extern void generic_unlink (struct file_object *, int);
extern void generic_write_file (struct file_object *);
extern void generic_write_rand (struct file_object *);
extern void generic_write (struct file_object *);

#endif
