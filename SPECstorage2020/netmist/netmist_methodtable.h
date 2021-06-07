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
#include "./copyright.txt"
#include "./license.txt"

#ifndef _NETMIST_METHODTABLE_H_
#define _NETMIST_METHODTABLE_H_

struct netmist_vfs_dir;
struct netmist_vfs_object;
struct file_object;

/*
 * This is the virtualized switch for all of the functions that
 * will get called in the OP mix.
 * We implicitly maintain a mapping through "union", this mapping should *NOT* be changes.
			void (*read)(); <==> void (*cmdline_file_delete)();
			void (*mmap_read)(); <==> void (*cmdline_file_download)();
			void (*read_rand)(); <==> void (*cmdline_file_upload)();
			void (*write)(); <==> void (*cmdline_navigation)();
			void (*mmap_write)(); <==> void (*explorer_file_download)();
			void (*write_rand)(); <==> void (*explorer_file_upload)();
			void (*rmw)(); <==> void (*explorer_file_delete)();
			void (*mkdir)(); <==> void (*explorer_navigation)();
			void (*rmdir)(); <==> void (*explorer_navigation)();
			void (*unlink)(); <==> void (*explorer_select)();
			void (*append)(); <==> void (*word_edit_save)();
			void (*locking)(); <==> void (*word_file_open)();
			void (*access)(); <==> void (*word_file_close)();
 */

struct method_table
{
    char name[MAXNAME];
    union
    {
	struct
	{
	    /*callbacks for all workloads other than home folder */
	    void (*read) (struct file_object *);
	    void (*read_file) (struct file_object *);
	    void (*mmap_read) (struct file_object *);
	    void (*read_rand) (struct file_object *);
	    void (*write) (struct file_object *);
	    void (*write_file) (struct file_object *);
	    void (*mmap_write) (struct file_object *);
	    void (*write_rand) (struct file_object *);
	    void (*rmw) (struct file_object *);
	    void (*mkdir) (struct file_object *);
	    void (*rmdir) (struct file_object *);
	    void (*unlink) (struct file_object *, int force);
	    void (*unlink2) (struct file_object *, int force);
	    void (*create) (struct file_object *, int);
	    void (*append) (struct file_object *);
	    void (*locking) (struct file_object *);
	    void (*access) (struct file_object *);
	    void (*stat) (struct file_object *);
	    void (*neg_stat) (struct file_object *);
	    void (*chmod) (struct file_object *);
	    void (*readdir) (struct file_object *);
	    void (*copyfile) (struct file_object *);
	    void (*rename) (struct file_object *);
	    void (*statfs) (struct file_object *);
	    void (*pathconf) (struct file_object *);
	    void (*trunc) (struct file_object *, int);
	    void (*custom1) (struct file_object *);
	    void (*custom2) (struct file_object *);
	};

    };

    struct file_object *(*makepath) (int, int);
    void (*prefix_op) (int, struct file_object *);
    void (*postfix_op) (int, struct file_object *);
    void (*fill_buf) (long, long, int, char *);
    int (*init_dir) (struct netmist_vfs_dir *);
    void (*init_file) (int, struct file_object *);
    int (*init_empty_file) (struct netmist_vfs_dir *, char *);
    int (*stat_workdir) (struct netmist_vfs_dir *);
    int (*remove_file) (struct netmist_vfs_dir *, char *);
    int (*remove_dir) (struct netmist_vfs_dir *);
    /*workload specific setup and cleanup */
    void (*setup) (void);
    void (*teardown) (void);
    int (*impersonate) (void *);
    void *(*make_impersonate_context) (void);
};

#endif
