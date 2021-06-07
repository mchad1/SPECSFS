/*
 * Copyright 2003-2020
 * Created: Aug 2018
 * Author: Don Capps
 *
 * This is a monitor that can be used to watch values in the
 * pdsm_log_file. It is a simple demonstration of the capabilty to 
 * monitor variables that are modified by a set of distributed clients, and 
 * transported into a unified space for monitoring. 
 *
 * In this case, Netmist is running on a bunch of clients. Netmist
 * detects if PDSM is active, and if so, it will deposit the 
 * collected statistics.  (approximately 50 statistics are collected)
 *
 * This example takes the data from the PDSM log file and sends it to 
 * a csv file for later processing.
 *
 */
#define PDSM_RO_ACTIVE

#include <stdio.h>
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
#include <../win32lib/win32_sub.h>
#include <../win32lib/win32_getopt.h>
#endif
#include <stdlib.h>
#include <errno.h>
#if !defined(WIN32)
#include <strings.h>
#endif
#include <string.h>
#include <time.h>
#include <fcntl.h>
#if defined(LITE)
#include "lite/netmist.h"
#else
#if defined(PRO)
#include "pro/netmist.h"
#else
#include "dist/netmist.h"
#endif
#endif
#include "netmist_if.h"
#include "netmist_version.c"

int interval = 2;
int phase, set_continue;
int stop_flag;
char *phasestr;
time_t my_time;
char my_pdsm_file[256];
int scan_pdsm_file (void);
char *get_timestamp (char *);
int getopt ();
int client_id, no_loop;
struct pdsm_remote_stats *pdsm_stats;
time_t dummytime;
char time_string[30];
int number_of_entries;
int number;
int bar_value;
char cret;
void usage (void);
char mystate[30];
int pdsm_file_fd;
char hostname[256];
char command[256];
char csv_filename[256];
int csv;
FILE *csvfile;
char timestr[50];
double mvalue;

int
main (int argc, char **argv)
{

    int j, ret;
    if (argc == 1)
    {
	usage ();
	exit (0);
    };
    while ((cret = getopt (argc, argv, "hkvtf:s:i:")) != EOF)
    {
	switch (cret)
	{
	case 'h':		/* Help screen */
	    usage ();
	    exit (0);
	    break;
	case 'v':		/* Help screen */
	    printf ("Version %s\n", git_version);
	    exit (0);
	    break;
	case 'f':		/* pdsm file */
	    strcpy (my_pdsm_file, optarg);
	    break;
	case 's':		/* csv filename */
	    csv++;
	    strcpy (csv_filename, optarg);
	    break;
	case 'i':		/* Interval */
	    interval = atoi (optarg);
	    break;
	case 't':		/* Don't loop */
	    no_loop++;
	    break;
	case 'k':		/* Don't loop */
	    set_continue++;
	    break;
	};
    }
    sleep (1);

    pdsm_stats =
	(struct pdsm_remote_stats *)
	malloc (sizeof (struct pdsm_remote_stats));
    if (pdsm_stats == NULL)
    {
	printf ("Malloc failed for size %d\n",
		(int) sizeof (struct pdsm_remote_stats));
	exit (1);
    }

    csvfile = fopen (csv_filename, "w");
    if (csvfile == NULL)
    {
	printf ("Unable to open csv file: %s\n", csv_filename);
	exit (1);
    }
    while (1)
    {
	printf ("Scanning collection stats\n");
	number_of_entries = scan_pdsm_file ();
	printf ("Found %d collection frames\n", number_of_entries);
	fprintf (csvfile,
		 "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
		 "Client_name,", "Client_id,", "Phase,", "Timestamp,",
		 "Requested_rate,", "Achieved_rate,", "run_time,",
		 "read_throughput,", "write_throughput,", "meta_throughput,",
		 "read_op_count,", "read_op_latency,", "readfile_op_count,",
		 "readfile_op_latency,", "readrand_op_count,",
		 "readrand_op_latency,", "write_op_count,",
		 "write_op_latency,", "writefile_op_count,",
		 "writefile_op_latency,", "writerand_op_count,",
		 "writerand_op_latency,", "rmw_op_count,", "rmw_op_latency,",
		 "mkdir_op_count,", "mkdir_op_latency,", "rmdir_op_count,",
		 "rmdir_op_latency,", "unlink_op_count,",
		 "unlink_op_latency,", "unlink2_op_count,",
		 "unlink2_op_latency,", "append_op_count,",
		 "append_op_latency,", "lock_op_count,", "lock_op_latency,",
		 "access_op_count,", "access_op_latency,", "chmod_op_count,",
		 "chmod_op_latency,", "readdir_op_count,",
		 "readdir_op_latency,", "stat_op_count,", "stat_op_latency,",
		 "neg_stat_op_count,", "neg_stat_latency,",
		 "copyfile_op_count,", "copyfile_op_latency,",
		 "rename_op_count,", "rename_op_latency,", "statfs_op_count,",
		 "statfs_op_latency,", "pathconf_op_count,",
		 "pathconf_op_latency,", "trunc_op_count,",
		 "trunc_op_latency\n");


	for (j = 0; j < number_of_entries; j++)
	{
	    lseek (pdsm_file_fd, j * sizeof (struct pdsm_remote_stats),
		   SEEK_SET);
	    ret =
		read (pdsm_file_fd, pdsm_stats,
		      sizeof (struct pdsm_remote_stats));
	    if (ret <= 0)
		break;
	    strcpy (hostname, pdsm_stats->client_name);
	    client_id = pdsm_stats->client_id;

	    if (csv && (strlen (hostname) != 0)
		&& (pdsm_stats->achieved_op_rate != 0.0))
	    {
		my_time = pdsm_stats->epoch_time;
		phase = pdsm_stats->cur_state;
		if (phase == WARM_BEAT)
		    phasestr = "WARMUP";
		if (phase == RUN_BEAT)
		    phasestr = "RUN";

		fprintf (csvfile,
			 "%20s, %10d, %10s, %lld, %10.2f, %10.2f, %10.2f, %10.2f, %10.2f, %10.2f, ",
			 pdsm_stats->client_name, pdsm_stats->client_id,
			 phasestr, (long long) my_time,
			 pdsm_stats->requested_op_rate,
			 pdsm_stats->achieved_op_rate, pdsm_stats->run_time,
			 pdsm_stats->read_throughput,
			 pdsm_stats->write_throughput,
			 pdsm_stats->meta_throughput);

		if (pdsm_stats->read_op_count)
		    mvalue =
			pdsm_stats->read_op_time / pdsm_stats->read_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->read_op_count,
			 mvalue);
		if (pdsm_stats->read_file_op_count)
		    mvalue =
			pdsm_stats->read_file_op_time /
			pdsm_stats->read_file_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->read_file_op_count, mvalue);

		if (pdsm_stats->read_rand_op_count)
		    mvalue =
			pdsm_stats->read_rand_op_time /
			pdsm_stats->read_rand_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->read_rand_op_count, mvalue);
		if (pdsm_stats->write_op_count)
		    mvalue =
			pdsm_stats->write_op_time /
			pdsm_stats->write_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->write_op_count,
			 mvalue);

		if (pdsm_stats->write_file_op_count)
		    mvalue =
			pdsm_stats->write_file_op_time /
			pdsm_stats->write_file_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->write_file_op_count, mvalue);

		if (pdsm_stats->write_rand_op_count)
		    mvalue =
			pdsm_stats->write_rand_op_time /
			pdsm_stats->write_rand_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->write_rand_op_count, mvalue);

		if (pdsm_stats->rmw_op_count)
		    mvalue =
			pdsm_stats->rmw_op_time / pdsm_stats->rmw_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->rmw_op_count,
			 mvalue);

		if (pdsm_stats->mmap_read_op_count)
		    mvalue =
			pdsm_stats->mmap_read_op_time /
			pdsm_stats->mmap_read_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->mmap_read_op_count, mvalue);

		if (pdsm_stats->mmap_write_op_count)
		    mvalue =
			pdsm_stats->mmap_write_op_time /
			pdsm_stats->mmap_write_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->mmap_write_op_count, mvalue);

		if (pdsm_stats->mkdir_op_count)
		    mvalue =
			pdsm_stats->mkdir_op_time /
			pdsm_stats->mkdir_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->mkdir_op_count,
			 mvalue);

		if (pdsm_stats->rmdir_op_count)
		    mvalue =
			pdsm_stats->rmdir_op_time /
			pdsm_stats->rmdir_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->rmdir_op_count,
			 mvalue);

		if (pdsm_stats->create_op_count)
		    mvalue =
			pdsm_stats->create_op_time /
			pdsm_stats->create_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->create_op_count, mvalue);

		if (pdsm_stats->unlink_op_count)
		    mvalue =
			pdsm_stats->unlink_op_time /
			pdsm_stats->unlink_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->unlink_op_count, mvalue);

		if (pdsm_stats->unlink2_op_count)
		    mvalue =
			pdsm_stats->unlink2_op_time /
			pdsm_stats->unlink2_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->unlink2_op_count, mvalue);

		if (pdsm_stats->append_op_count)
		    mvalue =
			pdsm_stats->append_op_time /
			pdsm_stats->append_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->append_op_count, mvalue);

		if (pdsm_stats->lock_op_count)
		    mvalue =
			pdsm_stats->lock_op_time / pdsm_stats->lock_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->lock_op_count,
			 mvalue);

		if (pdsm_stats->access_op_count)
		    mvalue =
			pdsm_stats->access_op_time /
			pdsm_stats->access_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->access_op_count, mvalue);

		if (pdsm_stats->chmod_op_count)
		    mvalue =
			pdsm_stats->chmod_op_time /
			pdsm_stats->chmod_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->chmod_op_count,
			 mvalue);

		if (pdsm_stats->readdir_op_count)
		    mvalue =
			pdsm_stats->readdir_op_time /
			pdsm_stats->readdir_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->readdir_op_count, mvalue);

		if (pdsm_stats->stat_op_count)
		    mvalue =
			pdsm_stats->stat_op_time / pdsm_stats->stat_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ", pdsm_stats->stat_op_count,
			 mvalue);

		if (pdsm_stats->neg_stat_op_count)
		    mvalue =
			pdsm_stats->neg_stat_op_time /
			pdsm_stats->neg_stat_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->neg_stat_op_count, mvalue);

		if (pdsm_stats->copyfile_op_count)
		    mvalue =
			pdsm_stats->copyfile_op_time /
			pdsm_stats->copyfile_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->copyfile_op_count, mvalue);

		if (pdsm_stats->rename_op_count)
		    mvalue =
			pdsm_stats->rename_op_time /
			pdsm_stats->rename_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->rename_op_count, mvalue);

		if (pdsm_stats->statfs_op_count)
		    mvalue =
			pdsm_stats->statfs_op_time /
			pdsm_stats->statfs_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f, ",
			 pdsm_stats->statfs_op_count, mvalue);

		if (pdsm_stats->pathconf_op_count)
		    mvalue =
			pdsm_stats->pathconf_op_time /
			pdsm_stats->pathconf_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f,",
			 pdsm_stats->pathconf_op_count, mvalue);

		if (pdsm_stats->trunc_op_count)
		    mvalue =
			pdsm_stats->trunc_op_time /
			pdsm_stats->trunc_op_count;
		else
		    mvalue = 0.0;
		fprintf (csvfile, "%ld, %10.6f", pdsm_stats->trunc_op_count,
			 mvalue);

		fprintf (csvfile, "\n");

		fflush (csvfile);
		printf
		    ("Sending stats to csv file: %s.%d.oprate  %10.2f  %lld %s",
		     hostname, client_id, pdsm_stats->achieved_op_rate,
		     (long long) my_time, ctime (&my_time));
	    }
	    if ((pdsm_stats->cur_state == STOP_BEAT) && !set_continue)
	    {
		stop_flag = 1;
		break;
	    }
	}
	if (pdsm_stats->cur_state == INIT_BEAT)
	{
	    printf ("Waiting for WARMUP or RUN phase to begin. \n");
	}
	close (pdsm_file_fd);
	sleep (interval);
	if (stop_flag && !set_continue)
	{
	    printf ("Ending collection. Found STOP frame\n");
	    if (csv && csvfile)
		fclose (csvfile);
	    exit (0);
	}
	if (no_loop)
	    break;
    }
    return (0);
}

void
usage ()
{
    printf ("pump_carbon:\n");
    printf ("\t-h............  Help screen\n");
    printf ("\t-f............  pdsm_log_file_name\n");
    printf
	("\t-k............  Ignore STOP. Used for multiple load point runs with PDSM_MODE=1\n");
    printf ("\t-s............  csv filename\n");
    printf ("\t-i............  Interval in seconds\n");
    printf
	("\t-t............  One pass flag. Use with append mode collection.\n");
    printf ("\t-v............  Display version information.\n");
    printf ("\n");
}

char *
get_timestamp (char *string)
{
    time_t run_str;

    run_str = time ((time_t *) & dummytime);
    (void) strncpy ((char *) string, (char *) ctime ((time_t *) & run_str),
		    24);
    return (string);
}

int
scan_pdsm_file ()
{
    int x;
    int count = 0;
    struct pdsm_remote_stats remote_stats;
    pdsm_file_fd = open (my_pdsm_file, O_RDONLY, 0666);
    if (pdsm_file_fd < 0)
    {
	printf ("Unable to open %s\n", my_pdsm_file);
	exit (1);
    }
    while (1)
    {
	x = read (pdsm_file_fd, &remote_stats,
		  sizeof (struct pdsm_remote_stats));
	if (x <= 0)
	{
	    return (count);
	}
	count++;
    }
}
