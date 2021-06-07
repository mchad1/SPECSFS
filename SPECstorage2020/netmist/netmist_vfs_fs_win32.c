#include "./copyright.txt"
#include "./license.txt"

#include "netmist_vfs_paths_abs.h"
#include "netmist_vfs_fs_common.c"

extern void nap (int);
struct netmist_vfs_rock
{
    struct netmist_vfs_dir *rootdir;
    int direct_size;
};

struct netmist_vfs_object
{
    HANDLE nvo_fd;
    int direct;
    int seq;
    int rand;
    int dont_need;
};

/** 
 * @brief Return the error string for the given err value.
 *
 * @param err : Error value.
 */
const char *
nvw32_errstr (int err)
{
    return win32_strerror (err);
}

/** 
 * @brief Return the minimum size for use with Direct I/O.
 *
 * @param rock : Pointer to the file system object.
 */
static uint64_t
nvw32_direct_size (struct netmist_vfs_rock *rock)
{
    return rock->direct_size;
}

/** 
 * @brief Return the pointer to the root directory.
 *
 * @param rock : Pointer to the file system object.
 */
static struct netmist_vfs_dir *
nvw32_vfs_root (struct netmist_vfs_rock *rock)
{
    rock->rootdir->refcount++;
    return rock->rootdir;
}

/** 
 * @brief Close a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvop : Pointer to the file object.
 */
static int
nvw32_close (struct netmist_vfs_rock *rock, struct netmist_vfs_object **nvop)
{
    struct netmist_vfs_object *nvo = *nvop;

    (void) rock;

    if (nvo == NULL)
	return 0;

    CloseHandle (nvo->nvo_fd);
    free (nvo);
    *nvop = NULL;
    return 0;

}

/** 
 * @brief "stat" a file.
 *
 * @param _rock : Pointer to the file system object.
 * @param d : Pointer to the directory object.
 * @param n : Relative file name.
 * @param s : Pointer to the stat structure.
 */
static int
nvw32_stat (struct netmist_vfs_rock *_rock,
	    struct netmist_vfs_dir *d, char *n, struct netmist_vfs_stat *s)
{
    char pbuf[MAX_PATH], *path;
    DWORD ret;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    GET_FILEEX_INFO_LEVELS fInfoLevelId = GetFileExInfoStandard;
    ULARGE_INTEGER li;

    (void) _rock;

    if (n == NULL)
    {
	path = d->abspath;
    }
    else
    {
	ret = snprintf (pbuf, sizeof (pbuf), "%s\\%s", d->abspath, n);
	if (ret <= 0 || (size_t) ret >= sizeof (pbuf))
	    return ERROR_BUFFER_OVERFLOW;
	path = pbuf;
    }

    /*
       Using 'Ex' so that it more closely resembles the functionality
       of Unix systems.  Unix stat() gets more of the attributes than the
       standard GetFileAttributes() call does.
     */
    ret = GetFileAttributesEx (path, fInfoLevelId, &fileInfo);
    if (ret == 0)
    {
	return GetLastError ();
    }

    if (s == NULL)
	return 0;

    li.LowPart = fileInfo.ftCreationTime.dwLowDateTime;
    li.HighPart = fileInfo.ftCreationTime.dwHighDateTime;
    s->netmist_st_ctime = li.QuadPart;

    li.LowPart = fileInfo.ftLastWriteTime.dwLowDateTime;
    li.HighPart = fileInfo.ftLastWriteTime.dwHighDateTime;
    s->netmist_st_mtime = li.QuadPart;

    li.LowPart = fileInfo.nFileSizeLow;
    li.HighPart = fileInfo.nFileSizeHigh;
    s->netmist_st_size = li.QuadPart;

    return 0;
}

/** 
 * @brief Open a file.
 *
 * @param rock  : Pointer to the file system object.
 * @param d     : Pointer to the directory object.
 * @param rfn   : Relative file name.
 * @param flags : Flags for the open.
 * @param nvop  : Pointer to the file object.
 */
static int
nvw32_open (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_dir *d, char *rfn,
	    enum netmist_vfs_open_flags flags,
	    struct netmist_vfs_object **nvop)
{
    int res;
    char buf[MAXNAME2];
    struct netmist_vfs_object *nvo;

    if (flags & NETMIST_OPEN_CREATE)
	flags |= NETMIST_OPEN_WRITE;

    res = snprintf (buf, sizeof (buf), "%s\\%s", d->abspath, rfn);
    if (res <= 0 || (size_t) res >= sizeof (buf))
    {
	return ERROR_BUFFER_OVERFLOW;
    }

    /* unfailable allocation */
    nvo = my_malloc (sizeof (*nvo));

    nvo->direct = flags & NETMIST_OPEN_DIRECT;
    nvo->seq = flags & NETMIST_OPEN_SEQ;
    nvo->rand = flags & NETMIST_OPEN_RAND;
    nvo->dont_need = flags & NETMIST_OPEN_DONT_NEED;
    nvo->nvo_fd = CreateFile (buf,
			      GENERIC_READ
			      | (flags & NETMIST_OPEN_WRITE ? GENERIC_WRITE :
				 0),
			      FILE_SHARE_READ | FILE_SHARE_WRITE |
			      FILE_SHARE_DELETE, NULL,
			      flags & NETMIST_OPEN_CREATE ? OPEN_ALWAYS :
			      OPEN_EXISTING,
			      FILE_FLAG_POSIX_SEMANTICS | (flags &
							   NETMIST_OPEN_DIRECT
							   ?
							   FILE_FLAG_NO_BUFFERING
							   : 0) | (flags &
								   NETMIST_OPEN_SYNC
								   ?
								   FILE_FLAG_WRITE_THROUGH
								   : 0) |
			      (flags & NETMIST_OPEN_SEQ ?
			       FILE_FLAG_SEQUENTIAL_SCAN : 0) | (flags &
								 NETMIST_OPEN_RAND
								 ?
								 FILE_FLAG_RANDOM_ACCESS
								 : 0) | (flags
									 &
									 NETMIST_OPEN_DONT_NEED
									 ?
									 FILE_FLAG_NO_BUFFERING
									 : 0),
			      NULL);
    if (nvo->nvo_fd == INVALID_HANDLE_VALUE)
    {
	return GetLastError ();
    }
    else
    {
	if (*nvop)
	    nvw32_close (rock, nvop);
	*nvop = nvo;
	return 0;
    }
}

/** 
 * @brief "seek" in a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo : Pointer to the file object.
 * @param off : Offset
 * @param whence : Where to start
 * @param ooff : Current offset after the operation.
 */
static int
nvw32_seek (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_object *nvo,
	    int64_t off, enum netmist_vfs_seek_whence whence, uint64_t * ooff)
{
    LARGE_INTEGER in, out;
    int wwhence;

    (void) rock;

    in.QuadPart = off;
    out.QuadPart = 0;
    switch (whence)
    {
    case NETMIST_SEEK_SET:
	wwhence = FILE_BEGIN;
	break;
    case NETMIST_SEEK_CUR:
	wwhence = FILE_CURRENT;
	break;
    case NETMIST_SEEK_END:
	wwhence = FILE_END;
	break;
    default:
	return ERROR_INVALID_PARAMETER;
    }
    if (SetFilePointerEx (nvo->nvo_fd, in, &out, wwhence) == 0)
	return GetLastError ();
    if (ooff)
	*ooff = out.QuadPart;
    return 0;
}

/**
 * @brief  Enable / disable Direct I/O on a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo : Pointer to the file object.
 * @param d : Flag to enable/disable O_direct.
 * @param od : Return current value of direct setting.
 */
static int
nvw32_direct (struct netmist_vfs_rock *rock,
	      struct netmist_vfs_object *nvo, int d, int *od)
{
    (void) rock;

    *od = nvo->direct;
    if ((d != -1) && (d != nvo->direct))
	return ERROR_INVALID_PARAMETER;
    return 0;
}

/** 
 * @brief "read" from a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo : Pointer to the directory object.
 * @param buf : Buffer
 * @param len : Length
 * @param olen : Resulting transfer size.
 */
static int
nvw32_read (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_object *nvo,
	    char *buf, uint64_t len, uint64_t * olen)
{
    int err;
    int ret;

    *olen = 0;

  retry:
    err = ReadFile (nvo->nvo_fd, buf, len, &ret, NULL);
    if (err == FALSE)
    {
	err = GetLastError ();

	/*
	 * Netmist didn't lock anything but there was an Internal Windows
	 * lock collision. Take a nap and then try again.
	 */
	if (err == ERROR_LOCK_VIOLATION)
	{
	    nap (100);
	    goto retry;
	}

	if (nvo->direct && (err == ERROR_INVALID_PARAMETER))
	{
	    rock->direct_size = MIN2_DIRECT_SIZE;
	}

	return err;
    }

    *olen = ret;
    return 0;
}

/** 
 * @brief "write" to a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo  : Pointer to the directory object.
 * @param buf  : Buffer
 * @param len  : Length
 * @param olen : Resulting transfer size.
 */
static int
nvw32_write (struct netmist_vfs_rock *rock,
	     struct netmist_vfs_object *nvo,
	     char *buf, uint64_t len, uint64_t * olen)
{
    int err;
    int ret;

    (void) rock;

    *olen = 0;

    err = WriteFile (nvo->nvo_fd, buf, len, &ret, NULL);
    if (err == FALSE)
    {
	err = GetLastError ();
	if (nvo->direct && (err == ERROR_INVALID_PARAMETER))
	{
	    rock->direct_size = MIN2_DIRECT_SIZE;
	}
	return err;
    }

    *olen = ret;
    return 0;
}

/** 
 * @brief "fsync" a file.
 *
 * @param rock : Pointer to the file system object.
 * @param nvo : Pointer to the directory object.
 */
static void
nvw32_fsync (struct netmist_vfs_rock *rock, struct netmist_vfs_object *nvo)
{
    (void) rock;
/*
 * There are two ways to get here in Windows:
 *    fsync(x)
 *    FlushFileBuffers(x)
 * For now, it looks like sticking with vanilla fsync() is ok.
 */
    fsync (nvo->nvo_fd);
}

/** 
 * @brief "readdir" from a file.
 *
 * @param rock : Pointer to the file system object.
 * @param d : Pointer to the directory object.
 * @param ix : Index
 * @param e : Resulting directory entry.
 */
static int
nvw32_readden (struct netmist_vfs_rock *rock,
	       struct netmist_vfs_dir *d, uint64_t ix,
	       struct netmist_vfs_dentry *e)
{
    int res;
    char searchexpr[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATAA ffd;

    (void) rock;

    res = snprintf (searchexpr, sizeof (searchexpr), "%s\\*", d->abspath);
    if (res <= 0 || (size_t) res >= sizeof (searchexpr))
    {
	return ERROR_BUFFER_OVERFLOW;
    }

    hFind = FindFirstFile (searchexpr, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
	return GetLastError ();
    }

    for (; ix > 0; ix--)
    {
	if (FindNextFile (hFind, &ffd) == 0)
	{
	    res = GetLastError ();
	    FindClose (hFind);
	    return res;
	}
    }
    FindClose (hFind);

    snprintf (e->name, sizeof (e->name), "%s", ffd.cFileName);

    return 0;
}

/** 
 * @brief lock / unlock a file.
 *
 * @param rock : Pointer to the file system object.
 * @param f    : Pointer to the file object.
 * @param lk   : Lock operation.
 */
static int
nvw32_lock (struct netmist_vfs_rock *rock,
	    struct netmist_vfs_object *f, enum netmist_vfs_lock lk)
{
    int j;

    (void) rock;

    switch (lk)
    {
    case NETMIST_VFS_LOCK_UNLOCK:
	UnlockFile (f->nvo_fd, 0, 0, (DWORD) - 1, (DWORD) - 1);
	return 0;
    case NETMIST_VFS_LOCK_TRY:
	if (LockFile (f->nvo_fd, 0, 0, (DWORD) - 1, (DWORD) - 1))
	{
	    return 0;
	}
	else
	{
	    return ERROR_PATH_BUSY;
	}
    case NETMIST_VFS_LOCK_WAIT:
	{
	    int res = 0;
	    for (j = 0; j < LOCK_RETRY; j++)
	    {
		if (LockFile (f->nvo_fd, 0, 0, (DWORD) - 1, (DWORD) - 1))
		{
		    break;
		}
		else
		{
		    res = GetLastError ();
		    Sleep (10);
		}
	    }
	    return res;
	}
    default:
	return ERROR_INVALID_PARAMETER;
    }
}

/** 
 * @brief "truncate" a file.
 *
 * @param rock : Pointer to the file system object.
 * @param o : Pointer to the directory object.
 * @param sz : Requested size.
 */
static int
nvw32_truncate (struct netmist_vfs_rock *rock,
		struct netmist_vfs_object *o, uint64_t sz)
{
    LONG res;
    LONG szlow = sz;
    LONG szhigh = sz >> 32;

    (void) rock;

    res = SetFilePointer (o->nvo_fd, szlow, &szhigh, FILE_BEGIN);
    if (res == INVALID_SET_FILE_POINTER)
    {
	return GetLastError ();
    }
    res = SetEndOfFile (o->nvo_fd);
    if (res == 0)
	return GetLastError ();
    return 0;
}

/** 
 * @brief "pathconf" system call.
 *
 * @param rock : Pointer to the file system object.
 * @param d : Pointer to the directory object.
 */
static int
nvw32_pathconf (struct netmist_vfs_rock *rock, struct netmist_vfs_dir *d)
{
    char root_path[MAX_PATH + 1];
    DWORD MaxComponentLength;
    char VolumeName[MAX_PATH + 1];

    (void) rock;

    if (!GetRootPathName (d->abspath, root_path))
    {
	return GetLastError ();
    }

    if (!GetVolumeInformationA (root_path,
				VolumeName,
				sizeof (VolumeName) / sizeof (VolumeName[0]),
				NULL, &MaxComponentLength, NULL, NULL, 0))
    {
	return GetLastError ();
    }

    return 0;
}

/** 
 * @brief "statfs" system call.
 *
 * @param rock : Pointer to the file system object.
 * @param d : Pointer to the directory object.
 * @param rn : Relative file name
 * @param s : Pointer to the fsstat structure.
 */
static int
nvw32_statvfs (struct netmist_vfs_rock *rock,
	       struct netmist_vfs_dir *d, char *rn,
	       struct netmist_vfs_statfs *s)
{
    int ret;
    char root_path[MAX_PATH] = { 0 };
    char pbuf[MAX_PATH] = { 0 };
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;

    (void) rock;

    ret = snprintf (pbuf, sizeof (pbuf), "%s\\%s", d->abspath, rn);
    if ((ret <= 0) || ((size_t) ret >= sizeof (pbuf)))
    {
	return ERROR_BUFFER_OVERFLOW;
    }

    /*GetDiskFreeSpace() may be the closest one */
    if (!GetRootPathName (pbuf, root_path) ||
	!GetDiskFreeSpace (root_path,
			   &SectorsPerCluster,
			   &BytesPerSector,
			   &NumberOfFreeClusters, &TotalNumberOfClusters))
    {
	return GetLastError ();
    }

    if (s == NULL)
	return 0;

    s->netmist_stfs_bavail = NumberOfFreeClusters;
    return 0;
}

struct netmist_vfs_mmo
{
    HANDLE h;
    void *addr;
};

/** 
 * @brief "mumap" a file.
 *
 * @param rock : Pointer to the file system object.
 * @param mmop : Pointer to the mmap object.
 */
static void
nvw32_munmap (struct netmist_vfs_rock *rock, struct netmist_vfs_mmo **mmop)
{
    (void) rock;

    UnmapViewOfFile ((*mmop)->addr);
    CloseHandle ((*mmop)->h);
    free (*mmop);

    *mmop = NULL;
}

/** 
 * @brief "mmap" a file.
 *
 * @param rock : Pointer to the file system object.
 * @param o    : Pointer to the directory object.
 * @param off  : Offset
 * @param len  : length
 * @param w    : Flags
 * @param mmop : Pointer to the memory map object.
 * @param p    : Resulting address of the mapping.
 */
static int
nvw32_mmap (struct netmist_vfs_rock *rock, struct netmist_vfs_object *o,
	    uint64_t off, uint64_t len, int w,
	    struct netmist_vfs_mmo **mmop, char **p)
{
    HANDLE hMap;
    char *pa;
    int prot;
    struct netmist_vfs_mmo *mmo;

    if (*mmop != NULL)
    {
	nvw32_munmap (rock, mmop);
    }

    mmo = malloc (sizeof (*mmo));
    if (mmo == NULL)
    {
	return ERROR_OUTOFMEMORY;
    }

    prot = w ? PAGE_READWRITE : PAGE_READONLY;

    hMap = CreateFileMapping (o->nvo_fd,	/* use paging file */
			      NULL,	/* default security */
			      prot, (DWORD) 0,	/* maximum object size (high-order DWORD) */
			      (DWORD) 0,	/* maximum object size (low-order DWORD) */
			      NULL);	/* name of mapping object */
    if (hMap == NULL)
    {
	int e = GetLastError ();
	free (mmo);
	return e;
    }

    prot = w ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;

    pa = (char *) MapViewOfFile (hMap, prot, 0, 0, 0);
    if (pa == NULL)
    {
	int e = GetLastError ();
	CloseHandle (hMap);
	free (mmo);
	return e;
    }

    mmo->h = hMap;
    *p = mmo->addr = pa;
    *mmop = mmo;

    return 0;
}


struct netmist_vfs_watch
{
    HANDLE nvw_handle;
};

/**
 * @brief With Windows, we use FindFirstChangeNotification()
 *        and WaitForSingleObject() to handle the notifications.
 *
 * XXX ignores the relative filename component, which is in keeping
 * with the original netmist implementation, but seems plausibly
 * like a bug?
 */

/** 
 * @brief Implement a "watch" 
 *
 * @param rock : Pointer to the file system object.
 * @param d    : Pointer to the directory object.
 * @param rfn  : Relative file/directory name.
 * @param wp   : Pointer to the watch structure.
 */
static int
nvw32_wwatch (struct netmist_vfs_rock *rock,
	      struct netmist_vfs_dir *d, char *rfn,
	      struct netmist_vfs_watch **wp)
{
    struct netmist_vfs_watch *w = NULL;
    HANDLE h;

    (void) rock;

    /* Refuse to clobber an existing watch */
    if (*wp != NULL)
	return ERROR_INVALID_PARAMETER;

    w = malloc (sizeof (struct netmist_vfs_watch));
    if (w == NULL)
	return ERROR_OUTOFMEMORY;

    h = FindFirstChangeNotification (d->abspath, TRUE,
				     FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (h == INVALID_HANDLE_VALUE)
	return GetLastError ();

    w->nvw_handle = h;
    *wp = w;

    return 0;
}

/** 
 * @brief Wait on a "watch"
 *
 * @param rock : Pointer to the file system object.
 * @param wp   : Pointer to the watch structure.
 */
static int
nvw32_wwait (struct netmist_vfs_rock *rock, struct netmist_vfs_watch **wp)
{
    DWORD res;
    int ret;

    (void) rock;

    /*  Method #2:
     * Let's try just waiting/polling for an event, and timing out if it
     * doesn't arrive. Using zero for the timeout means that the function
     * will return immediately. The returned value will tell us if the
     * process was "signaled" or just returned without being signaled.
     * We can't really block and wait because the notification may happen
     * many seconds/minutes/hours/days/weeks later.  The API permits one
     * to batch up a bunch of notifications and return them all at once.
     * BUT, this introduces a fixed wait/delay and makes any attempt
     * to measure the (triger to notification) timing impossible.
     */

    res = WaitForSingleObject ((*wp)->nvw_handle, 0);
    switch (res)
    {
    case WAIT_FAILED:
	ret = GetLastError ();
	break;
    case WAIT_TIMEOUT:
	ret = WAIT_TIMEOUT;
	break;
    case WAIT_ABANDONED:
	ret = ERROR_ABANDONED_WAIT_0;
	break;
    case WAIT_OBJECT_0:
	ret = 0;
	break;
    default:
	ret = ERROR_INVALID_PARAMETER;	/* XXX? */
	break;
    }

    FindCloseChangeNotification ((*wp)->nvw_handle);
    free (*wp);
    *wp = NULL;

    return ret;
}

static const struct netmist_vfs_v0 netmist_vfs_win32 = {
    .netmist_vfs_name = "WIN32",
    .netmist_vfs_errstr = nvw32_errstr,
    .netmist_vfs_direct_size = nvw32_direct_size,
    .netmist_vfs_root = nvw32_vfs_root,
    .netmist_vfs_walk = nvpa_walk,
    .netmist_vfs_pdir = nvpa_pdir,
    .netmist_vfs_rename = nvc_rename,
    .netmist_vfs_pathptr = nvpa_pathptr,
    .netmist_vfs_mkdir = nvc_mkdir,
    .netmist_vfs_rmdir = nvc_rmdir,
    .netmist_vfs_dfree = nvpa_dfree,
    .netmist_vfs_readden = nvw32_readden,
    .netmist_vfs_stat = nvw32_stat,
    .netmist_vfs_statfs = nvw32_statvfs,
    .netmist_vfs_access = nvc_access,
    .netmist_vfs_chmod = nvc_chmod,
    .netmist_vfs_unlink = nvc_unlink,
    .netmist_vfs_open = nvw32_open,
    .netmist_vfs_seek = nvw32_seek,
    .netmist_vfs_trunc = nvw32_truncate,
    .netmist_vfs_read = nvw32_read,
    .netmist_vfs_write = nvw32_write,
    .netmist_vfs_mmap = nvw32_mmap,
    .netmist_vfs_munmap = nvw32_munmap,
    .netmist_vfs_direct = nvw32_direct,
    .netmist_vfs_lock = nvw32_lock,
    .netmist_vfs_fsync = nvw32_fsync,
    .netmist_vfs_pathconf = nvw32_pathconf,
    .netmist_vfs_close = nvw32_close,
    .netmist_vfs_wwatch = nvw32_wwatch,
    .netmist_vfs_wwait = nvw32_wwait,
};

/* XXX Incomplete; should absorb impersonation logic */
/**
 * @brief  Initialize the vfs layer for Win32.
 */
int
netmist_win32_vfs_init (int argc, char **argv,
			struct netmist_vfs_init_ret *out)
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
    out->vtable_v0 = &netmist_vfs_win32;

    return 0;
}
