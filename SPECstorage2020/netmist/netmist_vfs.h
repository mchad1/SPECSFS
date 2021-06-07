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

/*
 * Provide an opaque-pointer based representation of a "file" or "directory"
 * upon which netmist is to operate.  This will be instantiated with drivers
 * for POSIX and Windows and other plugins.
 *
 * In order to hide issues of path separators, the VFS API operates
 * directory-at-a-time and requires the use of handles to refer to directories.
 * In a sense, it behaves like a subset of POSIX openat() and friends, but
 * without the escape to absolute paths.
 */

#ifndef _NETMIST_VFS_H_
#define _NETMIST_VFS_H_

#include <stdint.h>

/**
 * @defgroup vfs VFS API
 * @{
 */

/**
 * @defgroup handles Opaque handle types
 * @{
 */

/**
 * @brief
 * Any internal state; initialization of the VFS object will yield a pointer to
 * this type that must be passed to each VFS function
 */
struct netmist_vfs_rock;

/** @brief Opaque handle to an open file-like object */
struct netmist_vfs_object;

/** @brief Opaque handle to a directory-like path, which may or may not exist */
struct netmist_vfs_dir;

/** @brief Opaque handle to a memory mapping of an object */
struct netmist_vfs_mmo;

/** @brief Opaque handle to an active watch on a file */
struct netmist_vfs_watch;

/** @} */

/**
 * @defgroup stats Status and Informative Structures
 * @{
 */

/** @brief A summarized version of a directory entry */
struct netmist_vfs_dentry
{
    char name[256];
};

/** @brief A summarized version of a file's attributes */
struct netmist_vfs_stat
{
    uint64_t netmist_st_ctime; /**< creation time, UNIX timestamp */
    uint64_t netmist_st_mtime; /**< modification time */
    uint64_t netmist_st_size;  /**< size in bytes */
};

/** @brief A summarized version of a file system's attributes */
struct netmist_vfs_statfs
{
    /*
     * So far, there's no need for any of the information returned by statfs,
     * but C requires that we have at least one member.
     */
    uint64_t netmist_stfs_bavail;	/* blocks available to unpriv. user */
};

/** @} */

/**
 * @defgroup flags Control Flags
 * @{
 */

/** @brief Flags to the netmist_vfs_open() function */
enum netmist_vfs_open_flags
{
    NETMIST_OPEN_WRITE = 0x01,	/* Read always implied */
    NETMIST_OPEN_CREATE = 0x03,	/* Create implies write */
    NETMIST_OPEN_APPEND = 0x04,
    NETMIST_OPEN_DIRECT = 0x08,
    NETMIST_OPEN_SYNC = 0x10,
    NETMIST_OPEN_DONT_NEED = 0x20,
    NETMIST_OPEN_SEQ = 0x40,
    NETMIST_OPEN_RAND = 0x80,
};

/** @brief Control the interpretation of netmist_vfs_see() */
enum netmist_vfs_seek_whence
{
    NETMIST_SEEK_SET = 0, /**< Measure position from start */
    NETMIST_SEEK_CUR = 1, /**< Relative to current position */
    NETMIST_SEEK_END = 2, /**< Measure position from end */
};

/** @brief Control the behavior of netmist_vfs_lock() */
enum netmist_vfs_lock
{
    NETMIST_VFS_LOCK_UNLOCK = 0,	/**< Release lock */
    NETMIST_VFS_LOCK_TRY = 1,		/**< Try to acquire lock */
    NETMIST_VFS_LOCK_WAIT = 2,		/**< Wait for lock to be free */
};

/** @} */

/**
 * @brief VFS virtual method table type definition.
 *
 * Unless otherwise specified, all int-returning functions return 0 on success
 * or a platform-specific error code on failure.  Use netmist_vfs_errstr to to
 * interpret these codes.
 */
struct netmist_vfs_v0
{

/* VFS-level Interface */

    /** @brief The name of this vfs implementation */
    const char *netmist_vfs_name;

    /**
     * @brief Map an error code into a string in static storage.
     *
     * This is "static" in that it does not take a netmist_vfs_rock.
     */
    const char *(*netmist_vfs_errstr) (int);

    /* @brief The minimum size and alignment of an O_DIRECT request in this
     * implementation.
     *
     * If this implementation does not support direct requests, this should
     * return 0.
     *
     * This is a function for somewhat silly reasons: some devices pretend to
     * be 512-byte sectored up until someone asks for O_DIRECT access, at which
     * point, they are suddenly unable to fulfill a request!  The VFS plugin is
     * responsible for catching this error and increasing the size reported by
     * this function.  The caller of a read/write operation in direct mode (see
     * NETMIST_VFS_OPEN_DIRECT and netmist_vfs_direct) should, on error, check
     * that it has aligned and sized its request in concordance with the
     * *present*, *post-error* return value of this function and retry if not.
     * In general, there will be only a few calls that fail before the
     * implementation reports a constant size throughout the rest of its
     * operation.
     */
        uint64_t (*netmist_vfs_direct_size) (struct netmist_vfs_rock *);

/* directory interface */

    /** @brief Represents the root of the test environment.
     *
     * The handle returned here is like any other and must be passed to
     * netmist_vfs_dfree(), explicitly or implicitly.
     *
     * This is quite natural when the use of the root directory is to then
     * walk down to another path; the returned reference can be used to
     * initialize a struct netmist_vfs_dir * which is then overwritten, thus:
     *
     *   struct netmist_vfs_dir *d = vfst->netmist_vfs_root(rock);
     *   int code = vfst->netmist_vfs_walk(rock, d, "foo", &d);
     *
     * rather than the more verbose, but equally correct,
     *
     *   struct netmist_vfs_dir *r = vfst->netmist_vfs_root(rock);
     *   struct netmist_vfs_dir *d = NULL;
     *   int code = vfst->netmist_vfs_walk(rock, r, "foo", &d);
     *   vfst->netmist_vfs_dfree(rock, &r);
     *
     */
    struct netmist_vfs_dir *(*netmist_vfs_root) (struct netmist_vfs_rock *);

    /**
     * @brief Walk into the directory named n within p and yield a handle in
     * w.
     *
     * The directory need not exist (and VFS implementations must be prepared
     * for this); use netmist_vfs_stat to test, but note that race conditions
     * abound.  Very little is guaranteed until netmist_vfs_open returns a vfs
     * object handle.
     *
     * If n is the empty string, *w will be a clone of p with independent
     * lifetime.  (Alternatively, the implementation may refcount the
     * underlying object and return it again, so long as _dfree leaves it live
     * until the refcount is zero.)
     *
     * *w must be initialized on entry, as it will be passed to
     * netmist_vfs_dfree().  It is acceptable if *w == p: the walk will be done
     * internally before *w is released and replaced with the result.  On
     * failure, *w will not be modified.
     */
    int (*netmist_vfs_walk) (struct netmist_vfs_rock *,
			     struct netmist_vfs_dir * p, char *n,
			     struct netmist_vfs_dir ** w);

    /**
     * @brief Ascend a level of the directory hierarchy; *w must be initialized
     * on entry and will be replaced with a reference to p's parent directory.
     *
     * The root directory has no parent, but it is not an error to attempt to
     * walk up from it; *w will be NULL and the return value will be 0 in only
     * this case.
     */
    int (*netmist_vfs_pdir) (struct netmist_vfs_rock *,
			     struct netmist_vfs_dir * p,
			     struct netmist_vfs_dir ** w);

    /**
     * @brief Rename a file, possibly between directories.
     *
     * Rename s in sd to t in td, if s and t are both not NULL.
     * Rename sd to t in td, if only s is NULL.
     * Rename s in sd to td, if only t is NULL.
     * Rename sd to td, if s and t are both NULL.
     */
    int (*netmist_vfs_rename) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir * sd, char *s,
			       struct netmist_vfs_dir * td, char *t);

    /**
     * @brief Return a pointer to the path (relative to the vfs root); this
     * pointer is valid so long as the directory is live and must not be
     * independently freed.  This should only be used for debugging purposes.
     */
    char *(*netmist_vfs_pathptr) (struct netmist_vfs_rock *, struct
				  netmist_vfs_dir * p);

    /**
     * @brief Create the last directory in the path p.
     *
     * If the directory already exists, success will be returned but e will be
     * set to 1; e may be NULL if the caller does not care about exclusivity.
     */
    int (*netmist_vfs_mkdir) (struct netmist_vfs_rock *,
			      struct netmist_vfs_dir * p, int *e);

    /** @brief Remove the directory named n relative to p */
    int (*netmist_vfs_rmdir) (struct netmist_vfs_rock *,
			      struct netmist_vfs_dir * p);

    /**
     * @brief Release a directory handle, which will be set to NULL upon return
     */
    void (*netmist_vfs_dfree) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir **);

    /**
     * @brief Read one directory entry from directory handle
     *
     * This is an inefficient API if the desire is to enumerate all entries in
     * a directory, but that's not what netmist does.  If that changes, we will
     * need a more rich directory API.
     */
    int (*netmist_vfs_readden) (struct netmist_vfs_rock *,
				struct netmist_vfs_dir * d,
				uint64_t ix, struct netmist_vfs_dentry * e);

    /**
     * @brief Perform a pathconf or analog probe; the result is discarded.
     *
     * Plugins must not cheat, as with chmod.
     */
    int (*netmist_vfs_pathconf) (struct netmist_vfs_rock *,
				 struct netmist_vfs_dir *);

    /**
     * @brief fstat calls on a named object in a given vfs directory.
     *
     * Pass NULL for the name to perform the operation on the directory itself.
     *
     * For purposes of abstraction, the returned structures contain only
     * minimal data about the target vfs object.  However, the plugins are not
     * to take shortcuts; they should back these operations with whatever the
     * "full" or "natural" set of "stat-like" calls are.
     *
     * On success, the indicated structures are updated; on failure, the
     * structures may or may not be modified.  The structure pointers may be
     * NULL, in which case, this function merely tests for the existence of
     * the file and indicates via its return value.
     */
    int (*netmist_vfs_stat) (struct netmist_vfs_rock *,
			     struct netmist_vfs_dir * d, char *n,
			     struct netmist_vfs_stat * s);

    /**
     * @brief Like netmist_vfs_stat, but stat(v)fs instead.
     */
    int (*netmist_vfs_statfs) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir * d, char *n,
			       struct netmist_vfs_statfs * s);

    /** @brief Check access rights to the object named n in directory d */
    int (*netmist_vfs_access) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir * d, char *n);

    /**
     * @brief
     * Simulate manipulation of access rights of the object n in directory d.
     *
     * Despite being a simulation, with no actual change taking place, plugins
     * must not cheat.
     */
    int (*netmist_vfs_chmod) (struct netmist_vfs_rock *,
			      struct netmist_vfs_dir * d, char *n);

    /** @brief Unlink by name the object n in directory d */
    int (*netmist_vfs_unlink) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir * d, char *n);
/* Open Object Interface */

    /**
     * @brief
     * Open a file by name, relative to the indicated directory.
     *
     * If flags include NETMIST_OPEN_CREATE, the file will be created with
     * global read/write permissions.  The name must not be empty.
     *
     * *o must be initialized as it will be passed to netmist_vfs_close.
     */
    int (*netmist_vfs_open) (struct netmist_vfs_rock *,
			     struct netmist_vfs_dir * d, char *n,
			     enum netmist_vfs_open_flags,
			     struct netmist_vfs_object ** o);

    /** @brief Set the current position of a vfs object */
    int (*netmist_vfs_seek) (struct netmist_vfs_rock *,
			     struct netmist_vfs_object *, int64_t off,
			     enum netmist_vfs_seek_whence, uint64_t * ooff);

    /** @brief Truncate (or elongate) a vfs object */
    int (*netmist_vfs_trunc) (struct netmist_vfs_rock *,
			      struct netmist_vfs_object *, uint64_t);

    /**
     * @brief Read a vfs object.
     *
     * Returns 0 on success, even for short reads and writes; the olen
     * parameter contains the number of bytes actually transferred.
     */
    int (*netmist_vfs_read) (struct netmist_vfs_rock *,
			     struct netmist_vfs_object *,
			     char *buf, uint64_t len, uint64_t * olen);

    /** @brief Like netmist_vfs_read, but for writes instead */
    int (*netmist_vfs_write) (struct netmist_vfs_rock *,
			      struct netmist_vfs_object *,
			      char *buf, uint64_t len, uint64_t * olen);

    /**
     * @brief mmap a vfs object.
     *
     * This generally requires kernel-backed plugins;
     * plugins not able to mmap should provide functions which always fail.
     *
     * Always creates "shared" mappings, since the goal is to test the
     * filesystem, not the kernel paging layer.
     */
    int (*netmist_vfs_mmap) (struct netmist_vfs_rock *,
			     struct netmist_vfs_object *,
			     uint64_t off, uint64_t len, int rw,
			     struct netmist_vfs_mmo ** mmo, char **);

    /** @brief Unmap a VFS object */
    void (*netmist_vfs_munmap) (struct netmist_vfs_rock *,
				struct netmist_vfs_mmo ** mmo);

    /**
     * @brief Query or set the direct I/O disposition of the vfs object.
     *
     * Set ndirect to 0 or 1 to set the value or to -1 to leave it unchanged.
     * Sets odirect to the current value (before any change).
     */
    int (*netmist_vfs_direct) (struct netmist_vfs_rock *,
			       struct netmist_vfs_object *,
			       int ndirect, int *odirect);

    /**
     * @brief Hold an exclusive lock on the vfs object.
     *
     * If locking, will wait until the lock is held.  Implementations that do
     * not support locking should always fail the request.
     *
     * The scope of the lock is backend-specific.  For example, when using
     * POSIX fcntl locks, locks are associated with the underlying *file*, so
     * that any netmist_vfs_close() of *any* VFS object resulting from
     * different calls (with equivalent parameters) to netmist_vfs_open() will
     * also release any held locks.  This is bogus, but it is what we have.
     * Other platforms implement better semantics.  In the case of netmist,
     * assume that locking, if provided, does *something* but does not actually
     * provide mutual exclusivity in any reliable way.
     */
    int (*netmist_vfs_lock) (struct netmist_vfs_rock *,
			     struct netmist_vfs_object *,
			     enum netmist_vfs_lock lock);

    /** @brief Flush any write buffers */
    void (*netmist_vfs_fsync) (struct netmist_vfs_rock *,
			       struct netmist_vfs_object *);

    /** @brief Close, and free, a vfs object.  Sets its argument to NULL. */
    int (*netmist_vfs_close) (struct netmist_vfs_rock *,
			      struct netmist_vfs_object **);

/* Extended Attributes Interface */

    /*
     * WIP; can we use analogues of
     * fsetxattr, fgetxattr, and flistxattr, fremovexattr?
     * Or do we want to do this by name, i.e.
     * setxattr, getxattr, and listxattr, removexattr?
     */

/* Open Object Notification Interface */

    /**
     * @brief
     * Register a watch for modification of an object within a VFS directory.
     *
     * Implementations that do not support watch and wait should provide
     * a function which always fails.
     *
     * *w must be NULL on entry.
     */
    int (*netmist_vfs_wwatch) (struct netmist_vfs_rock *,
			       struct netmist_vfs_dir *, char *,
			       struct netmist_vfs_watch ** w);

    /**
     * @brief
     * Wait for a single event on a watch and de-register it.
     *
     * Implementations not providing watch and wait should provide a function
     * which always fails.
     *
     * In both success and failure, the watch is torn down and freed; *w will
     * be set to NULL in either case.
     */
    int (*netmist_vfs_wwait) (struct netmist_vfs_rock *,
			      struct netmist_vfs_watch ** w);
};

/**
 * @defgroup constr The VFS Constructor
 * @{
 */

#define NETMIST_VFS_VERMAGIC_0	0x937F5000

/** @brief The netmist VFS object handle itself.
 *
 * As this interface evolves, new function tables are to be added to the end of
 * this structure.  The vermagic field will implicitly describe how large the
 * structure is.  Newer netmists will understand all version numbers, and so
 * will be able to understand the result from all older plugins (of course,
 * they can reject very old ones, rather than adapt to them).  Older netmists
 * will not understand newer vermagic fields, but because the structure only
 * changes by adding fields, this should pose no problem: so long as the
 * function tables this older version cares about are not NULL, everything
 * should be fine.
 */
struct netmist_vfs_init_ret
{

    /** @brief The version identifier of this plugin's implementation.
     *
     * This must come first in this or any replacement VFS interface, so
     * that netmist can either reject or adapt for old interface providers.
     *
     * See NETMIST_VFS_VERMAGIC_*.
     */
    uintptr_t vermagic;

    /** @brief The "this" parameter for all VFS methods.
     *
     * Opaque to all of netmist, sensibly opened only wthin the VFS layer.
     */
    struct netmist_vfs_rock *rock;

    /** @brief The virtual function table */
    struct netmist_vfs_v0 const *vtable_v0;

    /* End of NETMIST_VFS_VERMAGIC_0 fields */
};

/**
 * @brief
 * Type of function initializing a VFS plugin.
 *
 * A plugin should have an exported function of this type, named
 * "netmist_vfs_init"; it will be found by the platform's dlopen()-like
 * mechanism and entered.  It should do no static work (that is, it must expect
 * to be entered more than once) but should fill out the contents of the given
 * struct netmist_vfs_init_ret.  On error, it should return non-zero; the
 * return code will not be otherwise interpreted, but is probably of interest
 * to humans.
 *
 * argc/argv is intended to be consumed by getopt()-like parsers and so the
 * following options are defined as part of the plugin interface.  Positional
 * arguments are left for platform-specific use, as are lower-case letter flags
 * (-a through -z), with or without option arguments; all other flags are
 * reserved for future expansion of the netmist VFS API.
 *
 *   -R [path]          specifies the root path, a string understood by the VFS
 *
 * Note that, because the C ecosystem is what it is, that argv[0] will be a
 * useless string (but not NULL and not empty, to avoid confusion) so that
 * argument processing with getopt() can begin at &argv[1].
 *
 * Despite its name, out is not entirely an out-parameter: the vermagic field
 * will be valid on entry and will indicate the maximum interface version as
 * understood by the netmist program and, more importantly, the maximum size of
 * the netmist_vfs_init_ret structure.  The VFS layer should update this field
 * with its maximum version to indicate which fields it has updated (a prefix
 * of the structure's definition).
 *
 */
typedef int (*netmist_vfs_init_t) (int argc, char **argv,
				   struct netmist_vfs_init_ret * out);

/** @} */

/** @} */

#endif
