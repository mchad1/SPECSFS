/*
 * A very simple netmist vfs test driver program.  Compile with, e.g.,
 *

   gcc -I./netmist -D_GNU_SOURCE -Wall -Wextra -g -Og -I./netmist/dist_pro \
     -DSPEC_DIST -ldl \
     ./netmist/netmist_vfs_test_driver.c -o ./bin/netmist_vfs_test_driver

 *
 * Expects the name of the VFS plugin as the first argument; everything else
 * will be passed down to the VFS plugin.  You must pass the mandatory
 * arguments specified in netmist_vfs.h, and may specify any optional arguments
 * as expected by the VFS.  As an example,
 *

   ./bin/netmist_vfs_test_runner \
      ./build/dist/linux/unknown/netmist_vfs_fs_posix.so \
      -R /tmp/netmist-workdir

 *
 */

#include "./copyright.txt"
#include "./license.txt"

#include <stdio.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "netmist_vfs.h"

void *vfs_so_handle = NULL;
static struct netmist_vfs_init_ret vfs;

#define VFS(fn, ...) (vfs.vtable_v0->netmist_vfs_ ## fn(vfs.rock, __VA_ARGS__))
/* Special case: just rock */
static inline struct netmist_vfs_dir *
netmist_vfs_root (void)
{
    return vfs.vtable_v0->netmist_vfs_root (vfs.rock);
}

static inline uint64_t
netmist_vfs_direct_size (void)
{
    return vfs.vtable_v0->netmist_vfs_direct_size (vfs.rock);
}

/* Special case: no arguments, even rock */
static inline const char *
netmist_vfs_errstr (int e)
{
    return vfs.vtable_v0->netmist_vfs_errstr (e);
}

int
main (int argc, char **argv)
{
#define VFSDO(codep, what, fn, ...) \
    *codep = VFS(fn, __VA_ARGS__); \
    if (*codep != 0) { \
	fprintf(stderr, "Could not " what ": %d == %s\n", \
			*codep, netmist_vfs_errstr(*codep)); \
	return 1; \
    } else { printf("OK: " what "\n"); }

    netmist_vfs_init_t initfun;
    struct netmist_vfs_dir *rootdir;
    int code;

    if (argc < 2)
    {
	fprintf (stderr, "%s vfs.so [vfs args*]\n", argv[0]);
	return 1;
    }

#if !defined(WIN32)
    vfs_so_handle = dlopen (argv[1], RTLD_LOCAL | RTLD_NOW);
    if (vfs_so_handle == NULL)
    {
	fprintf (stderr, "Cannot find VFS shared object '%s': %s\n",
		 argv[1], dlerror ());
	return 1;
    }

    initfun = (netmist_vfs_init_t) dlsym (vfs_so_handle, "netmist_vfs_init");
    if (initfun == NULL)
    {
	fprintf (stderr, "Cannot find netmist_vfs_init in '%s'\n", argv[1]);
	return 1;
    }
#else
#error Do not yet know how to dlopen() on Windows, sorry.
#endif

    code = initfun (argc - 1, &argv[1], &vfs);
    if (code != 0)
    {
	fprintf (stderr, "Cannot initialize VFS; error code %d\n", code);
	return 1;
    }

    if (vfs.vtable_v0 == NULL)
    {
	printf ("FAIL: VFS plugin '%s' did not initialize vtable v0\n",
		argv[1]);
	return 1;
    }

    printf ("OK: VFS plugin '%s' (which calls itself '%s') initialized.\n",
	    argv[1], vfs.vtable_v0->netmist_vfs_name);

    /* Static functions and such */
    rootdir = netmist_vfs_root ();

    printf ("  Root dir is '%s', direct size is %" PRIu64 ".\n",
	    VFS (pathptr, rootdir), netmist_vfs_direct_size ());

    /* Test some directory and walk ops */
    {
	struct netmist_vfs_dir *td1 = NULL;
	struct netmist_vfs_dir *td2 = NULL;
	int e;

	e = 0;
	VFSDO (&code, "create root directory", mkdir, rootdir, &e);
	printf ("  In fact, %s root directory\n",
		e == 0 ? "Created" : "Found");

	VFSDO (&code, "walk to subdir", walk, rootdir, "testdir1", &td1);
	e = 0;
	VFSDO (&code, "create subdirectory 1", mkdir, td1, &e);

	VFSDO (&code, "walk up from subdirectory", pdir, td1, &td2);
	if (strcmp (VFS (pathptr, rootdir), VFS (pathptr, td2)) != 0)
	{
	    fprintf (stderr, "Walking back up from subdir does not give root?"
		     " '%s' != '%s'\n",
		     VFS (pathptr, rootdir), VFS (pathptr, td2));
	    return 1;
	}

	VFSDO (&code, "walk to subdirectory 2", walk, td2, "testdir2", &td2);
	code = VFS (rmdir, td2);
	printf ("OK: %s subdirectory 2\n",
		code == 0 ? "Removed" : "Nonexistent");

	VFSDO (&code, "rename subdirectories N+N", rename, td1, NULL, td2,
	       NULL);
	VFSDO (&code, "rename subdirectories N+r", rename, td2, NULL, rootdir,
	       "testdir1");
	VFSDO (&code, "rename subdirectories r+N", rename, rootdir,
	       "testdir1", td2, NULL);
	VFSDO (&code, "rename subdirectories r+r", rename, rootdir,
	       "testdir2", rootdir, "testdir1");

	VFSDO (&code, "remove renamed subdirectory", rmdir, td1);

	VFS (dfree, &td1);
	VFS (dfree, &td2);
    }

    /* Re-create a directory, create a file, write, read, verify, remove */
    {
	struct netmist_vfs_dir *td = NULL;
	struct netmist_vfs_object *ohr = NULL;
	struct netmist_vfs_object *ohw = NULL;
	struct netmist_vfs_stat stbuf;
	char buf1[1024];
	char buf2[sizeof buf1];
	int e;
	size_t ix;
	uint64_t ol;

	VFSDO (&code, "walk to subdir", walk, rootdir, "testdir1", &td);
	e = 0;
	VFSDO (&code, "create subdirectory", mkdir, td, &e);
	if (e != 0)
	{
	    fprintf (stderr, "Subdirectory already exists?\n");
	    return 1;
	}

	VFSDO (&code, "create file", open, td, "file1",
	       NETMIST_OPEN_CREATE, &ohw);

	VFSDO (&code, "open file for reading", open, td, "file1", 0, &ohr);

	for (ix = 0; ix < sizeof buf1; ix++)
	{
	    buf1[ix] = (char) ix;
	}

	VFSDO (&code, "write data to file", write, ohw, buf1, sizeof buf1,
	       &ol);
	if (ol != sizeof buf1)
	{
	    fprintf (stderr, "Failed to write all data to file?"
		     " %" PRIu64 " != %" PRIu64 "\n",
		     ol, (uint64_t) (sizeof buf1));
	    return 1;
	}

	VFSDO (&code, "read data back", read, ohr, buf2, sizeof buf2, &ol);
	if (ol != sizeof buf1)
	{
	    fprintf (stderr, "Failed to read all data from file?"
		     " %" PRIu64 " != %zd\n", ol, sizeof buf1);
	    return 1;
	}

	if (memcmp (buf1, buf2, sizeof buf1) != 0)
	{
	    fprintf (stderr, "Data mismatch after readback\n");
	    return 1;
	}
	printf ("OK: data match after readback\n");

	VFSDO (&code, "stat file", stat, td, "file1", &stbuf);
	if (stbuf.netmist_st_size != sizeof buf1)
	{
	    fprintf (stderr, "VFS reports unexpected file size: "
		     "%" PRIu64 " != %zd\n",
		     stbuf.netmist_st_size, sizeof buf1);
	    return 1;
	}

	VFSDO (&code, "access check", access, td, "file1");

	VFSDO (&code, "seek read handle", seek, ohr, 128,
	       NETMIST_SEEK_SET, &ol);
	if (ol != 128)
	{
	    fprintf (stderr, "Seek to strange place? "
		     "%" PRIu64 " != %d", ol, 128);
	    return -1;
	}

	VFSDO (&code, "read data again", read, ohr, buf2, sizeof buf2, &ol);
	if (ol != (sizeof buf2) - 128)
	{
	    fprintf (stderr, "Failed to read all data from file?"
		     " %" PRIu64 " != %zd\n", ol, (sizeof buf2) - 128);
	    return 1;
	}
	if (memcmp (buf1 + 128, buf2, (sizeof buf2) - 128) != 0)
	{
	    fprintf (stderr, "Data mismatch after second readback\n");
	    return 1;
	}
	printf ("OK: data match after second readback\n");

	VFS (close, &ohw);
	VFS (close, &ohr);

	VFSDO (&code, "unlink file", unlink, td, "file1");
	VFSDO (&code, "rmdir", rmdir, td);

	VFS (dfree, &td);
    }

    printf ("OK: Finished testing\n");

    return 0;

#undef VFSERR
}
