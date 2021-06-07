/*
 * Example parser to display the contents of the PDSM file.
 */

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

#define PDSM_RO_ACTIVE
#include <time.h>
#include "netmist_if.h"
#if defined(LITE)
#include "lite/netmist.h"
#else
#if defined(PRO)
#include "pro/netmist.h"
#else
#include "dist/netmist.h"
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !defined(WIN32)
#include <unistd.h>
#endif
#if defined(WIN32)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4133)
#pragma warning(disable:4244)
#pragma warning(disable:4102)
#pragma warning(disable:4018)
#include "../win32lib/win32_getopt.h"
#include "../win32lib/win32_sub.h"
#endif

char *time_buf;
int count;
extern char *optarg;
int out, cret, no_loop, keep_going;
char my_pdsm_file[256];
void usage (void);

struct pdsm_remote_stats pdsm_stat;
int
main (int argc, char **argv)
{
    FILE *input;
    int x = 0;
    while ((cret = getopt (argc, argv, "htkf:")) != EOF)
    {
	switch (cret)
	{
	case 'h':		/* Help screen */
	    usage ();
	    exit (0);
	    break;
	case 'f':		/* pdsm file */
	    strcpy (my_pdsm_file, optarg);
	    break;
	case 't':		/* one time flag */
	    no_loop++;
	    break;
	case 'k':		/* Keep going. Ignore the stop flag */
	    keep_going++;
	    break;
	};
    }

    input = fopen (my_pdsm_file, "r");
    if (input == NULL)
    {
	printf ("Can't open the file %s\n", my_pdsm_file);
	usage ();
	exit (1);
    }
    while (1)
    {
	x = (int) fread (&pdsm_stat, sizeof (struct pdsm_remote_stats), 1,
			 input);
	if (x < 1)
	{
	    rewind (input);
	    sleep (1);
	    fclose (input);
	    input = fopen (my_pdsm_file, "r");
	    if (input == NULL)
	    {
		printf ("Can't open the file %s\n", my_pdsm_file);
		exit (1);
	    }
	    count = 0;
	    if (no_loop)
	    {
		printf ("\n");
		exit (0);
	    }
	    else
		continue;
	}

	printf ("\n");

	/* Ignore empty slots */
	if (strlen (pdsm_stat.client_name) == 0)
	    continue;
	printf ("Child name = %-20s  ", pdsm_stat.client_name);
	printf ("Child id = %-5d  ", pdsm_stat.client_id);
	printf ("Workload = %-10s  ", pdsm_stat.workload_name);
	switch (pdsm_stat.cur_state)
	{
	case INIT_BEAT:
	    printf ("State: INIT Phase\n");
	    break;
	case WARM_BEAT:
	    printf ("State: WARMUP Phase\n");
	    break;
	case RUN_BEAT:
	    printf ("State: RUN Phase\n");
	    break;
	case CLEAN_BEAT:
	    printf ("State: CLEAN Phase\n");
	    break;
	case VAL_BEAT:
	    printf ("State: VALIDATE Phase\n");
	    break;
	    /* Time to exit... collection has stopped */
	case STOP_BEAT:
	    if (!keep_going)
	    {
		printf ("State: STOP Phase\n");
		exit (0);
	    }
	    break;
	}
	printf ("Requested rate = %-6.2f ", pdsm_stat.requested_op_rate);
	printf ("  Current rate  = %-6.2f ", pdsm_stat.achieved_op_rate);
	time_buf = ctime (&pdsm_stat.epoch_time);
	if (time_buf[strlen (time_buf) - 1] == '\n')
	    time_buf[strlen (time_buf) - 1] = 0;
	printf ("  Run time = %-6.2f Sample time %s\n", pdsm_stat.run_time,
		time_buf);
	printf ("Read  throughput  = %10.2f KiB  ",
		pdsm_stat.read_throughput);
	printf ("Write throughput  = %10.2f KiB  ",
		pdsm_stat.write_throughput);
	printf ("Meta  throughput  = %10.2f KiB\n",
		pdsm_stat.meta_throughput);


	printf ("%17s ", pdsm_stat.read_string);
	if (pdsm_stat.read_op_time && pdsm_stat.read_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ", pdsm_stat.read_op_count,
		    pdsm_stat.read_op_time / pdsm_stat.read_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}
	printf ("%17s ", pdsm_stat.read_file_string);
	if (pdsm_stat.read_file_op_time && pdsm_stat.read_file_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.read_file_op_count,
		    (pdsm_stat.read_file_op_time /
		     pdsm_stat.read_file_op_count));
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.read_rand_string);
	if (pdsm_stat.read_rand_op_time && pdsm_stat.read_rand_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.read_rand_op_count,
		    pdsm_stat.read_rand_op_time /
		    pdsm_stat.read_rand_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.write_string);
	if (pdsm_stat.write_op_time && pdsm_stat.write_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.write_op_count,
		    pdsm_stat.write_op_time / pdsm_stat.write_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.write_file_string);
	if (pdsm_stat.write_file_op_time && pdsm_stat.write_file_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.write_file_op_count,
		    pdsm_stat.write_file_op_time /
		    pdsm_stat.write_file_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.mmap_write_string);
	if (pdsm_stat.mmap_write_op_time && pdsm_stat.mmap_write_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.mmap_write_op_count,
		    pdsm_stat.mmap_write_op_time /
		    pdsm_stat.mmap_write_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.mmap_read_string);
	if (pdsm_stat.mmap_read_op_time && pdsm_stat.mmap_read_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.mmap_read_op_count,
		    pdsm_stat.mmap_read_op_time /
		    pdsm_stat.mmap_read_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.write_rand_string);
	if (pdsm_stat.write_rand_op_time && pdsm_stat.write_rand_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.write_rand_op_count,
		    pdsm_stat.write_rand_op_time /
		    pdsm_stat.write_rand_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.rmw_string);
	if (pdsm_stat.rmw_op_time && pdsm_stat.rmw_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ", pdsm_stat.rmw_op_count,
		    pdsm_stat.rmw_op_time / pdsm_stat.rmw_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.mkdir_string);
	if (pdsm_stat.mkdir_op_time && pdsm_stat.mkdir_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.mkdir_op_count,
		    pdsm_stat.mkdir_op_time / pdsm_stat.mkdir_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.rmdir_string);
	if (pdsm_stat.rmdir_op_time && pdsm_stat.rmdir_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.rmdir_op_count,
		    pdsm_stat.rmdir_op_time / pdsm_stat.rmdir_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.unlink_string);
	if (pdsm_stat.unlink_op_time && pdsm_stat.unlink_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.unlink_op_count,
		    pdsm_stat.unlink_op_time / pdsm_stat.unlink_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.unlink2_string);
	if (pdsm_stat.unlink2_op_time && pdsm_stat.unlink2_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.unlink2_op_count,
		    pdsm_stat.unlink2_op_time / pdsm_stat.unlink2_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.create_string);
	if (pdsm_stat.create_op_time && pdsm_stat.create_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.create_op_count,
		    pdsm_stat.create_op_time / pdsm_stat.create_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.append_string);
	if (pdsm_stat.append_op_time && pdsm_stat.append_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.append_op_count,
		    pdsm_stat.append_op_time / pdsm_stat.append_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.locking_string);
	if (pdsm_stat.lock_op_time && pdsm_stat.lock_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n", pdsm_stat.lock_op_count,
		    pdsm_stat.lock_op_time / pdsm_stat.lock_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.access_string);
	if (pdsm_stat.access_op_time && pdsm_stat.access_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.access_op_count,
		    pdsm_stat.access_op_time / pdsm_stat.access_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.chmod_string);
	if (pdsm_stat.chmod_op_time && pdsm_stat.chmod_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.chmod_op_count,
		    pdsm_stat.chmod_op_time / pdsm_stat.chmod_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.readdir_string);
	if (pdsm_stat.readdir_op_time && pdsm_stat.readdir_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.readdir_op_count,
		    pdsm_stat.readdir_op_time / pdsm_stat.readdir_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.stat_string);
	if (pdsm_stat.stat_op_time && pdsm_stat.stat_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n", pdsm_stat.stat_op_count,
		    pdsm_stat.stat_op_time / pdsm_stat.stat_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.copyfile_string);
	if (pdsm_stat.copyfile_op_time && pdsm_stat.copyfile_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.copyfile_op_count,
		    pdsm_stat.copyfile_op_time / pdsm_stat.copyfile_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.rename_string);
	if (pdsm_stat.rename_op_time && pdsm_stat.rename_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.rename_op_count,
		    pdsm_stat.rename_op_time / pdsm_stat.rename_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.statfs_string);
	if (pdsm_stat.statfs_op_time && pdsm_stat.statfs_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f  ",
		    pdsm_stat.statfs_op_count,
		    pdsm_stat.statfs_op_time / pdsm_stat.statfs_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f  ", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.pathconf_string);
	if (pdsm_stat.pathconf_op_time && pdsm_stat.pathconf_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.pathconf_op_count,
		    pdsm_stat.pathconf_op_time / pdsm_stat.pathconf_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}

	printf ("%17s ", pdsm_stat.trunc_string);
	if (pdsm_stat.trunc_op_time && pdsm_stat.trunc_op_count)
	{
	    printf ("Count %-5ld   Latency %10.6f\n",
		    pdsm_stat.trunc_op_count,
		    pdsm_stat.trunc_op_time / pdsm_stat.trunc_op_count);
	}
	else
	{
	    printf ("Count %-5ld   Latency %10.6f\n", 0L, 0.0);
	}
    }
}

void
usage (void)
{
    printf ("Usage:  netmist_monitor [-f filename] <-t>\n");
    printf ("          -f filename. PDSM_LOG filename.\n");
    printf
	("          -k Ignore STOP. Used for multiple load point runs with PDSM_MODE=1.\n");
    printf
	("          -t Optional one pass flag. Use with PDSM_MODE=1 collection.\n");
}
