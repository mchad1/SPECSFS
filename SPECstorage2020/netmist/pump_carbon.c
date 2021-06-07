/*
 * Copyright 2002-2020
 * Created: Aug 2018
 * Author: Don Capps
 * Location: Iozone.org 
 *
 * This is a monitor that can be used to watch values in the
 * pdsm_log_file. It is a simple demonstration of the capabilty to 
 * monitor variables that are modified by a set of distributed clients, and 
 * transported into a unified space for monitoring. 
 *
 * In this case, Netmist is running on a bunch of clients. Netmist
 * detects if PDSM is active, and if so, it will deposit the 
 * current requested ops/sec and the currently achieved ops/sec
 * from every client into the PDSM LOG file. 
 *
 * This example takes the data from the PDSM log file and sends it to 
 * a "Carbon" server for graphing.
 *
 * The format for the Carbon server is:
 *
 *     clientname.clientid.workloadname.variable_name value timestamp
 *
 * One can examine the Carbon results via a web browser
 *    http://10.0.0.120/render?target=centos7.*.*.oprate&height=600&width=800&&lineWidth=2&graphType=line&lineMode=connected
 */
#define PDSM_RO_ACTIVE

#include <stdio.h>
#if !defined(WIN32)
#include <unistd.h>
#include <strings.h>
#endif
#include <stdlib.h>
#include <errno.h>
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
#include "netmist_version.c"

int interval = 2;
int stop_flag;
time_t my_time;
char my_pdsm_file[256];
int scan_pdsm_file (void);
char *get_timestamp (char *);
int getopt ();
int client_id, no_loop, set_continue;
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
char workload_name[256];
char command[256];
int carbon;
char carbonhost[256];
FILE *carbonfile;
char timestr[50];

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
	case 's':		/* carbon server */
	    carbon++;
	    strcpy (carbonhost, optarg);
	    break;
	case 'i':		/* Interval */
	    interval = atoi (optarg);
	    break;
	case 't':		/* Don't loop */
	    no_loop++;
	    break;
	case 'k':		/* loop */
	    set_continue++;
	    break;
	};
    }

    snprintf (command, sizeof (command), "nc %s 2003", carbonhost);
    carbonfile = popen (command, "w");
    if (carbonfile == 0)
    {
	printf ("popen failed %d\n", errno);
	exit (1);
    }
    printf ("Carbon server: %s\n", carbonhost);
    pclose (carbonfile);
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

    if (carbon)
    {
	snprintf (command, sizeof (command), "nc %s 2003", carbonhost);
	carbonfile = popen (command, "w");
	if (carbonfile == 0)
	{
	    printf ("popen failed %d\n", errno);
	    exit (1);
	}
    }
    while (1)
    {
	printf ("Scanning collection frames\n");
	number_of_entries = scan_pdsm_file ();
	printf ("Found %d collection frames\n", number_of_entries);
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
	    strcpy (workload_name, pdsm_stats->workload_name);
	    client_id = pdsm_stats->client_id;

	    if ( carbon && (strlen (hostname) != 0) )
	    {
		my_time = pdsm_stats->epoch_time;
		fprintf (carbonfile, "%s.%d.%s.oprate %10.2f %lld\n",
			 hostname, client_id, workload_name,
			 pdsm_stats->achieved_op_rate, (long long) my_time);
		if (pdsm_stats->read_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.read_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->read_op_time /
			     pdsm_stats->read_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.read_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->read_file_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.read_file_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->read_file_op_time /
			     pdsm_stats->read_file_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.read_file_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->read_rand_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.read_rand_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->read_rand_op_time /
			     pdsm_stats->read_rand_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.read_rand_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->mmap_read_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.mmap_read_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->mmap_read_op_time /
			     pdsm_stats->mmap_read_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.mmap_read_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->mmap_write_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.mmap_write_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->mmap_write_op_time /
			     pdsm_stats->mmap_write_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.mmap_write_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);

		if (pdsm_stats->write_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.write_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->write_op_time /
			     pdsm_stats->write_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.write_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->write_file_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.write_file_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->write_file_op_time /
			     pdsm_stats->write_file_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.write_file_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->write_rand_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.write_rand_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->write_rand_op_time /
			     pdsm_stats->write_rand_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.write_rand_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->rmw_op_count)
		    fprintf (carbonfile, "%s.%d.%s.rmw_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->rmw_op_time /
			     pdsm_stats->rmw_op_count, (long long) my_time);
		else
		    fprintf (carbonfile, "%s.%d.%s.rmw_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->mkdir_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.mkdir_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->mkdir_op_time /
			     pdsm_stats->mkdir_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.mkdir_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->rmdir_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.rmdir_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->rmdir_op_time /
			     pdsm_stats->rmdir_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.rmdir_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->create_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.create_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->create_op_time /
			     pdsm_stats->create_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.create_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->unlink_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.unlink_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->unlink_op_time /
			     pdsm_stats->unlink_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.unlink_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->unlink2_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.unlink2_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->unlink2_op_time /
			     pdsm_stats->unlink2_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.unlink2_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->append_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.append_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->append_op_time /
			     pdsm_stats->append_op_count,
			     (long long) (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.append_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->lock_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.lock_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->lock_op_time /
			     pdsm_stats->lock_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.lock_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->access_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.access_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->access_op_time /
			     pdsm_stats->access_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.access_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->chmod_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.chmod_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->chmod_op_time /
			     pdsm_stats->chmod_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.chmod_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->readdir_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.readdir_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->readdir_op_time /
			     pdsm_stats->readdir_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.readdir_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->stat_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.stat_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->stat_op_time /
			     pdsm_stats->stat_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.stat_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->neg_stat_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.neg_stat_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->neg_stat_op_time /
			     pdsm_stats->neg_stat_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.neg_stat_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->copyfile_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.copyfile_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->copyfile_op_time /
			     pdsm_stats->copyfile_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.copyfile_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->rename_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.rename_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->rename_op_time /
			     pdsm_stats->rename_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.rename_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->statfs_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.statfs_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->statfs_op_time /
			     pdsm_stats->statfs_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.statfs_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->pathconf_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.pathconf_latency %10.6f %lld\n",
			     hostname, client_id, workload_name,
			     pdsm_stats->pathconf_op_time /
			     pdsm_stats->pathconf_op_count,
			     (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.pathconf_latency %10.6f %lld\n",
			     hostname, client_id, workload_name, 0.0,
			     (long long) my_time);
		if (pdsm_stats->trunc_op_count)
		    fprintf (carbonfile,
			     "%s.%d.%s.trunc_latency %10.6f %lld\n", hostname,
			     client_id, workload_name,
			     pdsm_stats->trunc_op_time /
			     pdsm_stats->trunc_op_count, (long long) my_time);
		else
		    fprintf (carbonfile,
			     "%s.%d.%s.trunc_latency %10.6f %lld\n", hostname,
			     client_id, workload_name, 0.0,
			     (long long) my_time);

		printf
		    ("Sending stats to Carbon: %s.%d.%s.oprate  %10.2f  %lld %s",
		     hostname, client_id, workload_name,
		     pdsm_stats->achieved_op_rate, (long long) my_time,
		     ctime (&my_time));
	    }
	    if ((pdsm_stats->cur_state == STOP_BEAT) && !set_continue)
		stop_flag = 1;
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
	    if (carbon && carbonfile)
		pclose (carbonfile);
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
    printf ("\t-s............  Carbon server\n");
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
