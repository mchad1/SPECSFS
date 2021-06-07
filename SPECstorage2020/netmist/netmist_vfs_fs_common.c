#include "./copyright.txt"
#include "./license.txt"
int netmist_get_error (void);

/**
 * @file
 * Several functions are sufficiently similar across POSIX and WIN32 APIs that
 * there's little point in duplicating them into the per-backend files.  These
 * are intended to work with the netmist_vfs_paths_abs implementation of VFS
 * paths, and so assume that they can make direct reference to the absolute
 * path within those handles.
 *
 * Since we want these to be static to the definition of the VFS plugin, this
 * file is intended to be included, not built into its own .o.
 */

/* netmist_get_error() abstraction for mixed platform types.
 * Returns errno on Unix, and the results of GetLastError on Windows.
 */
int
netmist_get_error ()
{
#if defined(WIN32)
    return GetLastError ();
#else
    return errno;
#endif
}

/**
 * @brief Common function to create a directory.
 *
 * @param _rock : Pointer to the Rock for this filesystem.
 * @param d : Pointer to the netmist_vfs_dir object.
 * @param e : Pointer to a flag used to return the error or not on existence of the dir.
 */
static int
nvc_mkdir (struct netmist_vfs_rock *_rock, struct netmist_vfs_dir *d, int *e)
{
    int error_value = 0;
    (void) _rock;
    if (mkdir (d->abspath, 0777) != 0)
    {
#if defined(WIN32)
	error_value = netmist_get_error ();
	if (error_value == ERROR_ALREADY_EXISTS)
#else
	if (error_value == EEXIST)
#endif
	{
	    if (e != NULL)
	    {
		*e = 1;
	    }
	    return 0;
	}
	return error_value;
    }
    return 0;
}

/**
 * @brief Common function to remove a directory.
 *
 * @param _rock : Pointer to the rock object for this filesystem.
 * @param d : Pointer to the netmsit_vfs_dir object for this directory.
 */
static int
nvc_rmdir (struct netmist_vfs_rock *_rock, struct netmist_vfs_dir *d)
{
    int error_value = 0;
    (void) _rock;
    if (rmdir (d->abspath) != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    return 0;
}

/**
 * @brief Common function to remove a file.
 *
 * @param _rock : Pointer to the rock object for this filesystem.
 * @param d : Pointer to the directory object.
 * @param rfn : Pointer to the relative file name.
 */
static int
nvc_unlink (struct netmist_vfs_rock *_rock,
	    struct netmist_vfs_dir *d, char *rfn)
{
    char buf[MAXNAME2];
    int error_value = 0;
    int res = 0;

    (void) _rock;

    res = snprintf (buf, sizeof (buf),
#if defined(WIN32)
		    "%s\\%s",
#else
		    "%s/%s",
#endif
		    d->abspath, rfn);

    if (res <= 0 || (size_t) res >= sizeof (buf))
    {
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    res = unlink (buf);
    if (res != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    return 0;
}

/**
 * @brief Common function to implement "access" system call.
 *
 * @param _rock : Pointer to the rock object for this filesystem.
 * @param d : Pointer to the directory object.
 * @param rfn : Pointer to the relative file name.
 */
static int
nvc_access (struct netmist_vfs_rock *_rock,
	    struct netmist_vfs_dir *d, char *rfn)
{
    char buf[MAXNAME2];
    int res = 0;
    int error_value = 0;

    (void) _rock;

    res = snprintf (buf, sizeof (buf),
#if defined(WIN32)
		    "%s\\%s",
#else
		    "%s/%s",
#endif
		    d->abspath, rfn);

    if (res <= 0 || (size_t) res >= sizeof (buf))
    {
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    res = access (buf, F_OK);
    if (res != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    return 0;
}

/** 
 * @brief  Common function that implements the chmod functionality.
 *
 * @param _rock : Pointer to the rock object for this filesystem.
 * @param d : Pointer to the directory object.
 * @param rfn : Pointer to the relative file name.
 */
static int
nvc_chmod (struct netmist_vfs_rock *_rock,
	   struct netmist_vfs_dir *d, char *rfn)
{
    char buf[MAXNAME2];
    int error_value = 0;
    int res = 0;

    (void) _rock;

    res = snprintf (buf, sizeof (buf),
#if defined(WIN32)
		    "%s\\%s",
#else
		    "%s/%s",
#endif
		    d->abspath, rfn);

    if (res <= 0 || (size_t) res >= sizeof (buf))
    {
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    res = chmod (buf, 0666);
    if (res != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }
    return 0;
}

/** 
 * @brief  Common function that implements the rename functionality.
 *
 * @param _rock : Pointer to the rock object for this filesystem.
 * @param sd : Pointer to the source directory object.
 * @param sn : Pointer to the source name.
 * @param td : Pointer to the target directory object.
 * @param tn : Pointer to the target name.
 */
static int
nvc_rename (struct netmist_vfs_rock *_rock,
	    struct netmist_vfs_dir *sd, char *sn,
	    struct netmist_vfs_dir *td, char *tn)
{
    int res;
    char sbuf[MAXNAME2];
    char tbuf[MAXNAME2];
    int error_value = 0;

    (void) _rock;

    res = snprintf (sbuf, sizeof (sbuf), "%s%s%s",
		    sd->abspath, sn == NULL ? "" :
#if defined(WIN32)
		    "\\",
#else
		    "/",
#endif
		    sn == NULL ? "" : sn);

    if (res <= 0 || (size_t) res >= sizeof (sbuf))
    {
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    res = snprintf (tbuf, sizeof (tbuf), "%s%s%s",
		    td->abspath, tn == NULL ? "" :
#if defined(WIN32)
		    "\\",
#else
		    "/",
#endif
		    tn == NULL ? "" : tn);

    if (res <= 0 || (size_t) res >= sizeof (tbuf))
    {
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    if (rename (sbuf, tbuf) != 0)
    {
	error_value = netmist_get_error ();
	return error_value;
    }

    return 0;
}
