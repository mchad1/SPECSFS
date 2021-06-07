#include "./copyright.txt"
#include "./license.txt"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(_linux_)
#include <sys/inotify.h>
#endif

#if defined(_solaris_)
#include <port.h>
#endif

#include "netmist_defines.h"
#include "netmist_vfs.h"
#include "netmist_logger.h"

#include "netmist_vfs_paths_abs.h"

#ifndef MAXNAME2
#define MAXNAME2 PATH_MAX
#endif
#include "netmist_vfs_fs_common.c"

extern int netmist_get_error (void);

struct netmist_vfs_rock
{
    struct netmist_vfs_dir *rootdir;
    int direct_size;
};

struct netmist_vfs_object
{
    int nvo_fd;
    int direct;
    int seq;
    int rand;
    int dont_need;
};

/**
 * @brief Posix funtion to return the error string associated with the err value.
 *
 * @param err : Errno value.
 */
static const char *
nvp_errstr (int err)
{
    return strerror (err);
}

/**
 * @brief Posix funtion to return the size of a direct I/O transfer.
 *
 * @param rock : Pointer to the file system object.
 */
static uint64_t
nvp_direct_size (struct netmist_vfs_rock *rock)
{
    return rock->direct_size;
}

/**
 * @brief Posix funtion to return the root vfs_dir object.
 *
 * @param rock : Pointer to the file system object.
 */
static struct netmist_vfs_dir *
nvp_vfs_root (struct netmist_vfs_rock *rock)
{
    rock->rootdir->refcount++;
    return rock->rootdir;
}

/**
 * @brief Posix funtion to close a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvop : Pointer to the file object.
 */
static int
nvp_close (struct netmist_vfs_rock *rock, struct netmist_vfs_object **nvop)
{
    struct netmist_vfs_object *nvo = *nvop;

    (void) rock;

    if (nvo == NULL)
	return 0;

    close (nvo->nvo_fd);
    free (nvo);
    *nvop = NULL;
    return 0;
}

/**
 * @brief Posix funtion to implement the "stat" function on a file.
 *
 * @param _rock : Pointer to the file system object.
 * @param d : Pointer to the directory that contains the file.
 * @param n : file name.
 * @param s : Pointer to the stat data.
 */
static int
nvp_stat (struct netmist_vfs_rock *_rock,
	  struct netmist_vfs_dir *d, char *n, struct netmist_vfs_stat *s)
{
    int ret;
    int error_value = 0;
    char pbuf[PATH_MAX], *path;
#ifdef _LARGEFILE64_SOURCE
    /*
     * Heavy magic happens with the size of the stat and statfs structures
     * about the time that 10.6 came out.
     */
#if defined(_macos_)
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  > 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  < 1050
    struct stat stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  == 1050
    struct stat64 stbuf;
#endif
#else
    struct stat64 stbuf;
#endif
#else
    struct stat stbuf;
#endif

    (void) _rock;

    if (n == NULL)
    {
	path = d->abspath;
    }
    else
    {
	ret = snprintf (pbuf, sizeof (pbuf), "%s/%s", d->abspath, n);
	if (ret <= 0 || (size_t) ret >= sizeof (pbuf))
	    return ENAMETOOLONG;
	path = pbuf;
    }

    ret = I_STAT (path, &stbuf);
    if (ret != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }

    if (s == NULL)
	return 0;

    s->netmist_st_ctime = stbuf.st_ctime;
    s->netmist_st_mtime = stbuf.st_mtime;
    s->netmist_st_size = stbuf.st_size;

    return 0;
}

/**
 * @brief Posix funtion to open a file.
 *
 * @param rock : Pointer to the file system object.
 * @param d : Pointer to the directory that contains the file.
 * @param rfn : Relative file name.
 * @param flags : Flags for the open call.
 * @param nvop : Pointer to the file object.
 */
static int
nvp_open (struct netmist_vfs_rock *rock,
	  struct netmist_vfs_dir *d, char *rfn,
	  enum netmist_vfs_open_flags flags, struct netmist_vfs_object **nvop)
{
    int res;
    int error_value = 0;
    char buf[PATH_MAX];
    struct netmist_vfs_object *nvo;
    int oflags;

    if (flags & NETMIST_OPEN_CREATE)
	flags |= NETMIST_OPEN_WRITE;

    res = snprintf (buf, sizeof (buf), "%s/%s", d->abspath, rfn);
    if (res <= 0 || (size_t) res >= sizeof (buf))
    {
	return ENAMETOOLONG;
    }

    nvo = malloc (sizeof (*nvo));
    if (nvo == NULL)
    {
	return ENOMEM;
    }

    nvo->direct = flags & NETMIST_OPEN_DIRECT;
    nvo->seq = flags & NETMIST_OPEN_SEQ;
    nvo->rand = flags & NETMIST_OPEN_RAND;
    nvo->dont_need = flags & NETMIST_OPEN_DONT_NEED;
    /*
     * Can't do this inline: AIX's compiler doesn't like macros
     * in arguments; how about that!
     */
    oflags = (flags & NETMIST_OPEN_WRITE ? O_RDWR : O_RDONLY)
	| (flags & NETMIST_OPEN_CREATE ? O_CREAT : 0)
	| (flags & NETMIST_OPEN_APPEND ? O_APPEND : 0)
	| (flags & NETMIST_OPEN_SYNC ? O_SYNC : 0);

#if !defined(_solaris_) && !defined(_macos_)
    oflags |= (flags & NETMIST_OPEN_DIRECT ? O_DIRECT : 0);
#endif

    nvo->nvo_fd = I_OPEN (buf, oflags, 0666);
    if (nvo->nvo_fd < 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    else
    {
	if (*nvop)
	    nvp_close (rock, nvop);
#if !defined(_macos_)
	/* 
	 * Happens in netmist.c in the generic_* calls. No need to call this again at this level
	 * This may need to change if VFS layer ever adds posix_fadvise.
	 *
	 */
	if (nvo->seq & NETMIST_OPEN_SEQ)
	{
	    posix_fadvise (nvo->nvo_fd, (off_t) 0, (off_t) 0,
			   POSIX_FADV_SEQUENTIAL);
	}
	if (nvo->rand & NETMIST_OPEN_RAND)
	{
	    posix_fadvise (nvo->nvo_fd, (off_t) 0, (off_t) 0,
			   POSIX_FADV_RANDOM);
	}
	if (nvo->dont_need & NETMIST_OPEN_DONT_NEED)
	{
	    posix_fadvise (nvo->nvo_fd, (off_t) 0, (off_t) 0,
			   POSIX_FADV_DONTNEED);
	}
#endif
#if defined(_solaris_)
	if (nvo->direct)
	    directio (nvo->nvo_fd, DIRECTIO_ON);
#elif defined(_macos_)
	if (nvo->direct)
	    fcntl (nvo->nvo_fd, F_NOCACHE, 1);
#endif
	*nvop = nvo;
	return 0;
    }
}

/**
 * @brief Posix funtion to seek within a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo : Pointer to the file object.
 * @param off : Offset
 * @param whence : Where to start.
 * @param ooff : resulting offset.
 */
static int
nvp_seek (struct netmist_vfs_rock *rock,
	  struct netmist_vfs_object *nvo,
	  int64_t off, enum netmist_vfs_seek_whence whence, uint64_t * ooff)
{
    int pwhence;
    off_t noff;
    int error_value = 0;

    (void) rock;

    switch (whence)
    {
    case NETMIST_SEEK_SET:
	pwhence = SEEK_SET;
	break;
    case NETMIST_SEEK_CUR:
	pwhence = SEEK_CUR;
	break;
    case NETMIST_SEEK_END:
	pwhence = SEEK_END;
	break;
    default:
	return EINVAL;
    }
    noff = I_LSEEK (nvo->nvo_fd, off, pwhence);
    if (noff < 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    if (ooff)
	*ooff = noff;
    return 0;
}

/**
 * @brief Enable Direct I/O on this file object.
 *
 * @param rock : Pointer to the filesystem object.
 * @param nvo : Pointer to the file object.
 * @param d   : Direct flag.
 * @param od  : Current setting of the direct option.
 */
static int
nvp_direct (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_object *nvo, int d, int *od)
{
    (void) rock;

    *od = nvo->direct;
    if (d == -1)
	return 0;

    if (d)
    {
	if (!nvo->direct)
	{
#if defined(_solaris_)
	    nvo->direct = 1;
	    directio (nvo->nvo_fd, DIRECTIO_ON);
#elif defined(_macos_)
	    nvo->direct = 1;
	    fcntl (nvo->nvo_fd, F_NOCACHE, 1);
#else
	    return EINVAL;
#endif
	}
    }
    else
    {
	if (nvo->direct)
	{
#if defined(_solaris_)
	    nvo->direct = 0;
	    directio (nvo->nvo_fd, DIRECTIO_OFF);
#elif defined(_macos_)
	    nvo->direct = 0;
	    fcntl (nvo->nvo_fd, F_NOCACHE, 0);
#else
	    return EINVAL;
#endif
	}
    }
    return 0;
}

/**
 * @brief Posix funtion to read from a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo :  Pointer to the file object.
 * @param buf : Buffer to place data.
 * @param len : Requested Length
 * @param olen : Completed length.
 */
static int
nvp_read (struct netmist_vfs_rock *rock,
	  struct netmist_vfs_object *nvo,
	  char *buf, uint64_t len, uint64_t * olen)
{
    int64_t res;
    int error_value = 0;

    (void) rock;

    *olen = 0;

    res = read (nvo->nvo_fd, buf, len);
    if (res < 0)
    {
	error_value = netmist_get_error ();
	if (nvo->direct && (res == EINVAL))
	{
	    rock->direct_size = MIN2_DIRECT_SIZE;
	}
	return error_value;;
    }

    *olen = res;
    return 0;
}

/**
 * @brief Posix funtion to write to a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo :  Pointer to the file object.
 * @param buf : Buffer that contains data.
 * @param len : Requested Length
 * @param olen : Completed length.
 */
static int
nvp_write (struct netmist_vfs_rock *rock,
	   struct netmist_vfs_object *nvo,
	   char *buf, uint64_t len, uint64_t * olen)
{
    int64_t res;
    int error_value = 0;

    *olen = 0;

    (void) rock;

    res = write (nvo->nvo_fd, buf, len);
    if (res < 0)
    {
	error_value = netmist_get_error ();
	if (nvo->direct && (res == EINVAL))
	{
	    rock->direct_size = MIN2_DIRECT_SIZE;
	}
	return error_value;
    }

    *olen = res;
    return 0;
}

/**
 * @brief  Function to implement the "fsync" for the given file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo :  Pointer to the file object.
 */
static void
nvp_fsync (struct netmist_vfs_rock *rock, struct netmist_vfs_object *nvo)
{
    (void) rock;
    fsync (nvo->nvo_fd);
}

/**
 * @brief  Function to read a directory entry.
 *
 * @param rock : Pointer to the file system object.
 * @param d  :  Pointer to the vfs_dir object.
 * @param ix :  Index value.
 * @param e  :  Pointer to the vfs_dentry structure.
 */
static int
nvp_readden (struct netmist_vfs_rock *rock,
	     struct netmist_vfs_dir *d, uint64_t ix,
	     struct netmist_vfs_dentry *e)
{
    DIR *dp;
    struct dirent *de;
    int res;
    int error_value = 0;

    (void) rock;

    dp = opendir (d->abspath);
    if (dp == NULL)
    {
	error_value = netmist_get_error ();
	return error_value;
    }

    do
    {
	error_value = 0;
	de = readdir (dp);
	if (de == NULL)
	{
	    error_value = netmist_get_error ();
	    res = error_value;
	    closedir (dp);
	    if (res == 0)
	    {
		/*
		 * This is not quite right, but it's not wrong either and since we
		 * want to return some nonzero value, this is probably the right
		 * one for us?
		 */
		return ENOENT;
	    }
	    else
	    {
		return res;
	    }
	}
    }
    while (ix-- > 0);

    res = snprintf (e->name, sizeof (e->name), "%s", de->d_name);
    closedir (dp);
    if (res <= 0 || (size_t) res >= sizeof (e->name))
	return ENAMETOOLONG;

    return 0;
}

/**
 * @brief Posix funtion to lock/unlock a file.
 *
 * @param rock : Pointer to the file system object.
 * @param f :  Pointer to the file object.
 * @param lk : Type of lock op.
 */
static int
nvp_lock (struct netmist_vfs_rock *rock,
	  struct netmist_vfs_object *f, enum netmist_vfs_lock lk)
{
    struct flock flk;
    int res;
    int error_value = 0;

    (void) rock;

    switch (lk)
    {
    case NETMIST_VFS_LOCK_UNLOCK:
	{
	    flk.l_type = F_UNLCK;	/* Unlock */
	    res = fcntl (f->nvo_fd, F_SETLKW, &flk);
	    if (res != 0)
	    {
		error_value = netmist_get_error ();
		return error_value;
	    }
	    return 0;
	}
    case NETMIST_VFS_LOCK_TRY:
    case NETMIST_VFS_LOCK_WAIT:
	{
	    int j;
	    int lim = (lk == NETMIST_VFS_LOCK_TRY) ? 1 : LOCK_RETRY;

	    flk.l_type = F_WRLCK;
	    flk.l_whence = SEEK_SET;
	    flk.l_start = 0;
	    flk.l_len = 0;	/* The whole file */
	    flk.l_pid = getpid ();

	    for (j = 0, res = 1; (res != 0) && (j < lim); j++)
	    {
		res = fcntl (f->nvo_fd, F_SETLKW, &flk);
		if (res != 0)
		{
		    res = netmist_get_error ();
		    if (lk == NETMIST_VFS_LOCK_WAIT)
		    {
			struct timeval sleeptime;
			sleeptime.tv_sec = 0;
			sleeptime.tv_usec = 10000;
			select (0, NULL, NULL, NULL, &sleeptime);
		    }
		}
	    }
	    return res;		/* Either 0 or errno */
	}
    default:
	return EINVAL;
    }
}

/**
 * @brief Posix funtion to truncate a file.
 *
 * @param _rock : Pointer to the file system object.
 * @param o :  Pointer to the file object.
 * @param sz : Size specified
 */
static int
nvp_truncate (struct netmist_vfs_rock *_rock,
	      struct netmist_vfs_object *o, uint64_t sz)
{
    int res;
    int error_value = 0;

    (void) _rock;

    res = ftruncate (o->nvo_fd, sz);
    if (res != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    return 0;
}

/**
 * @brief Posix funtion to implement the POSIX pathconf() system call.
 *
 * @param rock : Pointer to the file system object.
 * @param d :  Pointer to the directory object.
 */
static int
nvp_pathconf (struct netmist_vfs_rock *rock, struct netmist_vfs_dir *d)
{
    int ret;
    int error_value = 0;

    (void) rock;

    error_value = 0;		/* Yeah, this is the goofy way that pathconf works */
#if defined(_bsd_)
    ret = pathconf (d->abspath, _PC_ASYNC_IO);
#else
    ret = pathconf (d->abspath, _PC_NAME_MAX);
#endif
    if (ret < 0)
    {
	error_value = netmist_get_error ();
	if (error_value != 0)
	    return error_value;
    }

    return 0;
}

#if defined(_bsd_) || defined(_macos_) || defined(_solaris_)
#include <sys/statvfs.h>
#else
#include <sys/vfs.h>
#endif

/**
 * @brief Posix funtion to implement the statfs() system call.
 *
 * @param rock : Pointer to the file system object.
 * @param d :  Pointer to the directory object.
 * @param rn : Pointer to a relative filename.
 * @param s : Pointer to resulting statfs info.
 */
static int
nvp_statvfs (struct netmist_vfs_rock *rock,
	     struct netmist_vfs_dir *d, char *rn,
	     struct netmist_vfs_statfs *s)
{
#if defined(_bsd_) || defined(_solaris_)
#ifdef _LARGEFILE64_SOURCE
    struct statvfs64 stbuf;
#else
    struct statvfs stbuf;
#endif
#else
#ifdef _LARGEFILE64_SOURCE
    /*
     * Heavy magic happens with the size of the stat and statfs structures
     * about the time that 10.6 came out.
     */
#if defined(_macos_)
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  > 1050
    struct statfs stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  < 1050
    struct statfs stbuf;
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__  == 1050
    struct statfs64 stbuf;
#endif
#else
    struct statfs64 stbuf;
#endif
#else
    struct statfs stbuf;
#endif
#endif
    char pbuf[PATH_MAX];
    int ret;

    (void) rock;

    ret = snprintf (pbuf, sizeof (pbuf), "%s/%s", d->abspath, rn);
    if ((ret <= 0) || ((size_t) ret >= sizeof (pbuf)))
    {
	return ENAMETOOLONG;
    }

#if defined(_bsd_) || defined(_solaris_)
    ret = I_STATVFS (pbuf, &stbuf);
#else
    ret = I_STATFS (pbuf, &stbuf);
#endif
    if (ret != 0)
	return netmist_get_error ();

    if (s == NULL)
	return 0;

    s->netmist_stfs_bavail = stbuf.f_bavail;

    return 0;
}

struct netmist_vfs_mmo
{
    char *addr;
    uint64_t len;
};

/**
 * @brief Posix funtion to implement mumap 
 *
 * @param rock : Pointer to the file system object.
 * @param mmop :  Pointer to the memory mapped object.
 */
static void
nvp_munmap (struct netmist_vfs_rock *rock, struct netmist_vfs_mmo **mmop)
{
    (void) rock;
    munmap ((*mmop)->addr, (*mmop)->len);
    free (*mmop);
    *mmop = NULL;
}

/**
 * @brief Posix funtion to implement mmap 
 *
 * @param rock : Pointer to the file system object.
 * @param o    : Pointer to the file object.
 * @param off  : Offset in the file to start the map.
 * @param len  : Length
 * @param w    : flags 
 * @param mmop : Pointer to the memory mapped object.
 * @param p    : Resulting address where object is mapped.
 */
static int
nvp_mmap (struct netmist_vfs_rock *rock, struct netmist_vfs_object *o,
	  uint64_t off, uint64_t len, int w,
	  struct netmist_vfs_mmo **mmop, char **p)
{
    char *pa;
    int mflags, prot;
    uint64_t pagesize;
    struct netmist_vfs_mmo *mmo;

    if (*mmop != NULL)
    {
	nvp_munmap (rock, mmop);
    }

    mmo = malloc (sizeof (*mmo));
    if (mmo == NULL)
    {
	return ENOMEM;
    }

    pagesize = getpagesize ();
    len += (pagesize - 1LL);
    len &= ~(pagesize - 1LL);

    mmo->len = len;

    prot = PROT_READ | (w ? PROT_WRITE : 0);
    mflags = MAP_SHARED;
#if !defined(_solaris_)
    mflags |= MAP_FILE;
#endif

    pa = (char *) I_MMAP (((char *) 0), len, prot, mflags, o->nvo_fd, off);
    if (pa == MAP_FAILED)
    {
	return netmist_get_error ();
    }

    *p = mmo->addr = pa;
    *mmop = mmo;

    return 0;
}

/*
 * Because there is no POSIX standard for inotify or such, this differs pretty
 * drastically by host; I'm so sorry.
 *
 * XXX These definitions work well for the case when there is only one
 * outstanding watch, which happens to be how netmist uses them at the moment,
 * but is not really evident in the API.  We can replace them with better ones
 * down the road.
 */

#if defined(_linux_)

struct netmist_vfs_watch
{
    int nvw_watchdesc;
};

static int inotify_desc = -1;

/**
 * @brief Posix funtion to implement a Watch() operation.
 *
 * @param rock : Pointer to the file system object.
 * @param d    :  Pointer to the directory object.
 * @param rfn  : Relative file name.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwatch (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_dir *d, char *rfn,
	    struct netmist_vfs_watch **wp)
{
    struct netmist_vfs_watch *w = NULL;
    char buf[PATH_MAX];
    int res;

    (void) rock;

    /* Refuse to clobber an existing watch */
    if (*wp != NULL)
	return EINVAL;

    w = malloc (sizeof (struct netmist_vfs_watch));
    if (w == NULL)
	return ENOMEM;

    if (inotify_desc == -1)
	inotify_desc = inotify_init ();
    if (inotify_desc == -1)
    {
	res = netmist_get_error ();
	goto out_err;
    }

    res = snprintf (buf, sizeof (buf), "%s/%s", d->abspath, rfn);
    if (res <= 0 || (size_t) res >= sizeof (buf))
	return ENAMETOOLONG;

    res = inotify_add_watch (inotify_desc, buf, IN_MODIFY);
    if (res < 0)
    {
	res = netmist_get_error ();
	goto out_err;
    }
    w->nvw_watchdesc = res;
    *wp = w;
    return 0;

  out_err:
    free (w);
    return res;
}

/**
 * @brief Posix funtion to implement waiting for a watch() event.
 *
 * @param rock : Pointer to the file system object.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwait (struct netmist_vfs_rock *rock, struct netmist_vfs_watch **wp)
{
    int res;
    int watchdesc;
    struct inotify_event buf;

    (void) rock;


    if ((inotify_desc == -1) || ((*wp) == NULL))
    {
	res = EINVAL;
	goto out_inval;
    }
    watchdesc = (*wp)->nvw_watchdesc;
    if (watchdesc < 0)
    {
	res = EINVAL;
	goto out_inval;
    }

    res = read (inotify_desc, &buf, sizeof (struct inotify_event));
    if (res < 0)
    {
	res = netmist_get_error ();
    }
    else if (((size_t) res) < sizeof (struct inotify_event))
    {
	res = EFAULT;
    }
    else
    {
	res = 0;
    }

    inotify_rm_watch (inotify_desc, watchdesc);
  out_inval:
    free (*wp);
    *wp = NULL;
    return res;
}

#elif defined(_solaris_)

/**
 * @brief On Solaris, we use the more general purpose event
 *        port allocation, association, and notification mechanism.
 *
 * We go for an event associated with the file, and not the parent directory.
 * We have to do this, as we don't get events on the parent, when the file sees
 * a write, without changing the length of the file.  This is different than on
 * all the other systems and requires one to look at the file instead of the
 * parent.
 */

struct netmist_vfs_watch
{
    struct file_obj fobj;
};

static int solaris_event_port = -1;

/**
 * @brief Posix funtion to implement a Watch() operation.
 *
 * @param rock : Pointer to the file system object.
 * @param d    :  Pointer to the directory object.
 * @param rfn  : Relative file name.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwatch (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_dir *d, char *rfn,
	    struct netmist_vfs_watch **wp)
{
    int res;
    struct stat sb;
    struct netmist_vfs_watch *w;

    (void) rock;

    if (*wp != NULL)
	return EINVAL;

    if (solaris_event_port == -1)
	solaris_event_port = port_create ();
    if (solaris_event_port == -1)
    {
	return netmist_get_error ();
    }

    w = malloc (sizeof (struct netmist_vfs_watch));
    if (w == NULL)
	return ENOMEM;

    res = asprintf (&(w->fobj.fo_name), "%s/%s", d->abspath, rfn);
    if ((res == -1) || (w->fobj.fo_name == NULL))
    {
	res = ENOMEM;
	goto out_err_w;
    }

    if (stat (w->fobj.fo_name, &sb) == -1)
    {
	res = netmist_get_error ();
	goto out_err_wfo;
    }

    /*
     * Register.
     */
    w->fobj.fo_atime = sb.st_atim;
    w->fobj.fo_mtime = sb.st_mtim;
    w->fobj.fo_ctime = sb.st_ctim;
    if (port_associate (solaris_event_port, PORT_SOURCE_FILE,
			(uintptr_t) (&w->fobj),
			FILE_MODIFIED | FILE_ACCESS | FILE_ATTRIB,
			(void *) w) == -1)
    {
	/*
	 * Add error processing as required, file may have been
	 * deleted/moved.
	 */
	res = netmist_get_error ();
	goto out_err_wfo;
    }

    *wp = w;
    return 0;

  out_err_wfo:
    free (w->fobj.fo_name);
  out_err_w:
    free (w);
    return res;
}

/**
 * @brief Posix funtion to implement waiting for a watch() event.
 *
 * @param rock : Pointer to the file system object.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwait (struct netmist_vfs_rock *rock, struct netmist_vfs_watch **wp)
{
    int res;
    port_event_t pe;
    struct timespec timeout;

    (void) rock;

    if ((solaris_event_port == -1) || (*wp == NULL))
    {
	res = EINVAL;
	goto out_inval;
    }

    timeout.tv_sec = 0;
    timeout.tv_nsec = (5 * 1000 * 1000);	/* 5 ms */

    if (!port_get (solaris_event_port, &pe, &timeout))
    {
	switch (pe.portev_source)
	{
	case PORT_SOURCE_FILE:
	    res = 0;
	    break;
	default:
	    res = EINVAL;
	    break;
	}
    }
    else
    {
	res = netmist_get_error ();
    }

    if (port_dissociate (solaris_event_port, PORT_SOURCE_FILE,
			 (uintptr_t) (&(*wp)->fobj)))
    {
	/* Report the first error, if there is one */
	if (res != 0)
	    res = netmist_get_error ();
    }

    free ((*wp)->fobj.fo_name);
    free (*wp);
  out_inval:
    *wp = NULL;
    return res;
}

#elif defined(_macos_) || defined(_bsd_)

#include <sys/event.h>

struct netmist_vfs_watch
{
    int nvw_watchfd;
};

static int event_kq = -1;

/**
 * @brief Posix funtion to implement a Watch() operation.
 *
 * @param rock : Pointer to the file system object.
 * @param d    :  Pointer to the directory object.
 * @param rfn  : Relative file name.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwatch (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_dir *d, char *rfn,
	    struct netmist_vfs_watch **wp)
{
    int res;
    struct netmist_vfs_watch *w;
    struct kevent ev;

    (void) rock;

    if (*wp != NULL)
	return EINVAL;

    if (event_kq < 0)
	event_kq = kqueue ();
    if (event_kq < 0)
    {
	return netmist_get_error ();
    }

    w = malloc (sizeof (struct netmist_vfs_watch));
    if (w == NULL)
	return ENOMEM;

    w->nvw_watchfd = -1;

    /* XXX reuse netmist_open internals */
    {
	char fnbuf[PATH_MAX];

	res = snprintf (fnbuf, sizeof (fnbuf), "%s/%s", d->abspath, rfn);
	if (res <= 0 || (size_t) res >= sizeof (fnbuf))
	    return ENAMETOOLONG;

#if defined(_macos_)
	w->nvw_watchfd = open (fnbuf, O_EVTONLY);
#elif defined(_bsd_)
	w->nvw_watchfd = open (fnbuf, O_RDWR, 0666);
#else
#error Unclear how to open for kqueue
#endif
	if (w->nvw_watchfd < 0)
	{
	    res = netmist_get_error ();
	    goto out_err;
	}
    }

    /*
     * Register, but do not wait for, an event with the kernel, using the file
     * descriptor as the event identifier as well, so we can wait specifically
     * for this event later.
     */
    EV_SET (&ev,
	    w->nvw_watchfd,
	    EVFILT_VNODE,
	    EV_ADD | EV_DISABLE,
	    NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_REVOKE,
	    w->nvw_watchfd, NULL);
    res = kevent (event_kq, &ev, 1, NULL, 0, NULL);
    if (res != 0)
    {
	res = netmist_get_error ();
	goto out_err;
    }

    return 0;

  out_err:
    if (w->nvw_watchfd >= 0)
	close (w->nvw_watchfd);
    free (w);
    return res;
}

/**
 * @brief Posix funtion to implement waiting for a watch() event.
 *
 * @param rock : Pointer to the file system object.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwait (struct netmist_vfs_rock *rock, struct netmist_vfs_watch **wp)
{
    int res;
    struct timespec timeout;
    struct kevent ev;

    (void) rock;

    if ((event_kq < 0) || ((*wp) == NULL))
	return EINVAL;

    timeout.tv_sec = 0;
    timeout.tv_nsec = (1000 * 1000 * 5);	/* 5 milliseconds */

    EV_SET (&ev, (*wp)->nvw_watchfd, 0 /* XXX: EVFILT_VNODE ? */ ,
	    EV_ENABLE | EV_ONESHOT, 0
	    /* XXX: NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_REVOKE ? */
	    , 0 /* XXX: (*wp)->nvw_watchfd ? */ ,
	    NULL);
    res = kevent (event_kq, &ev, 1, &ev, 1, &timeout);
    if (res != 1)
    {
	if (res == 0)
	{
	    res = ETIMEDOUT;
	}
	else
	{
	    res = netmist_get_error ();
	}
    }
    else
    {
	res = 0;
    }

    close ((*wp)->nvw_watchfd);
    free (*wp);
    *wp = NULL;

    return res;
}

#else

/**
 * @brief Posix funtion to implement a Watch() operation.
 *
 * @param rock : Pointer to the file system object.
 * @param d    :  Pointer to the directory object.
 * @param rfn  : Relative file name.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwatch (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_dir *d, char *rfn,
	    struct netmist_vfs_watch **wp)
{
    return ENOTSUP;
}

/**
 * @brief Posix funtion to implement waiting for a watch() event.
 *
 * @param rock : Pointer to the file system object.
 * @param wp   : Pointer to the vfs_watch structure.
 */
static int
nvp_wwait (struct netmist_vfs_rock *rock, struct netmist_vfs_watch **wp)
{
    return ENOTSUP;
}

#endif

static const struct netmist_vfs_v0 netmist_vfs_posix = {
    .netmist_vfs_name = "POSIX",
    .netmist_vfs_errstr = nvp_errstr,
    .netmist_vfs_direct_size = nvp_direct_size,
    .netmist_vfs_root = nvp_vfs_root,
    .netmist_vfs_walk = nvpa_walk,
    .netmist_vfs_pdir = nvpa_pdir,
    .netmist_vfs_rename = nvc_rename,
    .netmist_vfs_pathptr = nvpa_pathptr,
    .netmist_vfs_mkdir = nvc_mkdir,
    .netmist_vfs_rmdir = nvc_rmdir,
    .netmist_vfs_dfree = nvpa_dfree,
    .netmist_vfs_readden = nvp_readden,
    .netmist_vfs_stat = nvp_stat,
    .netmist_vfs_statfs = nvp_statvfs,
    .netmist_vfs_access = nvc_access,
    .netmist_vfs_chmod = nvc_chmod,
    .netmist_vfs_unlink = nvc_unlink,
    .netmist_vfs_open = nvp_open,
    .netmist_vfs_seek = nvp_seek,
    .netmist_vfs_trunc = nvp_truncate,
    .netmist_vfs_read = nvp_read,
    .netmist_vfs_write = nvp_write,
    .netmist_vfs_mmap = nvp_mmap,
    .netmist_vfs_munmap = nvp_munmap,
    .netmist_vfs_direct = nvp_direct,
    .netmist_vfs_lock = nvp_lock,
    .netmist_vfs_fsync = nvp_fsync,
    .netmist_vfs_pathconf = nvp_pathconf,
    .netmist_vfs_close = nvp_close,
    .netmist_vfs_wwatch = nvp_wwatch,
    .netmist_vfs_wwait = nvp_wwait,
};

/**
 * @brief Initialize the vfs layer.
 */
#ifdef NETMIST_POSIX_VFS_AS_PLUGIN
int
netmist_vfs_init (int argc, char **argv, struct netmist_vfs_init_ret *out)
#else
static int
netmist_posix_vfs_init (int argc, char **argv,
			struct netmist_vfs_init_ret *out)
#endif
{
    struct netmist_vfs_rock *rock;
    int code;
    char *rootdir = NULL;

    optind = 1;
    while ((code = getopt (argc, argv, "R:")) != -1)
    {
	switch (code)
	{
	case 'R':
	    rootdir = optarg;
	    break;
	default:
	    return EINVAL;
	}
    }

    if (rootdir == NULL)
    {
	return EINVAL;
    }

    rock = malloc (sizeof (*rock));
    if (rock == NULL)
	return ENOMEM;

    rock->rootdir = NULL;
    code = nvpa_init_root (rootdir, &rock->rootdir);
    if (code != 0)
    {
	free (rock);
	return code;
    }
    rock->direct_size = MIN_DIRECT_SIZE;

    if (out->vermagic != NETMIST_VFS_VERMAGIC_0)
    {
	/* Signal that we only speak version zero */
	out->vermagic = NETMIST_VFS_VERMAGIC_0;
    }
    out->rock = rock;
    out->vtable_v0 = &netmist_vfs_posix;

    return 0;
}
