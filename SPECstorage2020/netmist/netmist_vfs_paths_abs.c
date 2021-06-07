#include "./copyright.txt"
#include "./license.txt"

/**
 * @file
 * An implementation of the VFS abstract path functionality using absolute
 * (VFS-interpreted) paths internally.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#include <WinError.h>
#define strdup _strdup
#endif

#include "netmist_vfs_paths_abs.h"

/**
 * @brief Initialize the vfs_dir root directory mechanism.
 *
 * @param rootdir : Name of the root directory.
 * @param out : Pointer to the root vfs_dir structure.
 */
int
nvpa_init_root (char *rootdir, struct netmist_vfs_dir **out)
{
    struct netmist_vfs_dir *r;

    r = malloc (sizeof (*r));
    if (r == NULL)
    {
#if defined(WIN32)
	return ERROR_OUTOFMEMORY;
#else
	return ENOMEM;
#endif
    }

    r->parent = NULL;
    r->refcount = 1;
    r->abspath = strdup (rootdir);

    if (r->abspath == NULL)
    {
	free (r);
#if defined(WIN32)
	return ERROR_OUTOFMEMORY;
#else
	return ENOMEM;
#endif
    }

    *out = r;
    return 0;
}

/**
 * @brief Walk down a directory, looking for the specified file.
 *
 * @param rock : Pointer to the file system object.
 * @param d    : Pointer to the directory object.
 * @param n    : Relative file name.
 * @param w    : Pointer to the directory object.
 */
int
nvpa_walk (struct netmist_vfs_rock *rock,
	   struct netmist_vfs_dir *d, char *n, struct netmist_vfs_dir **w)
{
    struct netmist_vfs_dir *o;
    int res;
    size_t len;

    if (*n == '\0')
    {
	d->refcount++;
	if (*w)
	    nvpa_dfree (rock, w);
	*w = d;
	return 0;
    }

    o = malloc (sizeof (*o));
    if (o == NULL)
    {
#if defined(WIN32)
	return ERROR_OUTOFMEMORY;
#else
	return ENOMEM;
#endif
    }

    /* can't use asprintf(), as that's a GNUism that's not on
     * Windows.  So, here we go; calculate the length:
     * all chars, path separator, and NUL byte */
    len = strlen (d->abspath) + strlen (n) + 1 + 1;

    o->abspath = malloc (len);
    if (o->abspath == NULL)
    {
	free (o);
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    res = snprintf (o->abspath, len,
#if defined(WIN32)
		    "%s\\%s",
#else
		    "%s/%s",
#endif
		    d->abspath, n);

    if (res < 0 || ((size_t) res >= len))
    {
	free (o->abspath);
	free (o);
#if defined(WIN32)
	return ERROR_BUFFER_OVERFLOW;
#else
	return ENAMETOOLONG;
#endif
    }

    o->refcount = 1;
    o->parent = d;
    d->refcount++;

    if (*w)
    {
	nvpa_dfree (rock, w);
    }

    *w = o;
    return 0;
}

/**
 * @brief parent directory ?
 *
 * @param rock : Pointer to the file system object.
 * @param p    : Pointer to the directory object.
 * @param w    : Pointer to the directory object.
 */
int
nvpa_pdir (struct netmist_vfs_rock *rock,
	   struct netmist_vfs_dir *p, struct netmist_vfs_dir **w)
{
    struct netmist_vfs_dir *old = *w;

    if (p->parent == NULL)
    {
	*w = NULL;
    }
    else
    {
	p->parent->refcount++;
	*w = p->parent;
    }

    if (old)
    {
	nvpa_dfree (rock, &old);
    }

    return 0;
}

/**
 * @brief Free a file object.
 *
 * @param rock : Pointer to the file system object.
 * @param p    : Pointer to the directory object.
 */
void
nvpa_dfree (struct netmist_vfs_rock *rock, struct netmist_vfs_dir **p)
{
    struct netmist_vfs_dir *d = *p;

    if (d == NULL)
	return;

    *p = NULL;

    if (d->refcount == 1)
    {
	if (d->parent != NULL)
	    nvpa_dfree (rock, &d->parent);	/* Drop parent refcount */
	free (d->abspath);
	free (d);
    }
    else
    {
	d->refcount--;
    }
}

/**
 * @brief Returns a path string for the given directory.
 *
 * @param _rock : Pointer to the file system object.
 * @param d: Pointer to the directory object.
 */
char *
nvpa_pathptr (struct netmist_vfs_rock *_rock, struct netmist_vfs_dir *d)
{
    (void) _rock;
    return d->abspath;
}
