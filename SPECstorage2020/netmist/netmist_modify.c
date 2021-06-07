/*
 * Example Tool to dynamically modify a Netmist procs workload and or op_rate
 * without stopping the testing.
 * Note: This version only modifies the op rate :-)
 * ALSO, This is just an example. It will not scale up in client procs !!!
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
#include <time.h>
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

int count;
int child = -1;
int op_rate;
extern char *optarg;
int out, cret;
char my_pdsm_file[256];
void usage (void);
int count;

struct pdsm_remote_control *pdsm_control[10000];
struct pdsm_remote_control pdsm_control_dummy;
int yy;
int change_workload;
int percent_direct, set_percent_direct;
int percent_compress, set_percent_compress;
int percent_dedup, set_percent_dedup;

int
main (int argc, char **argv)
{
    FILE *input;
    int x = 0;
    while ((cret = getopt (argc, argv, "hf:c:o:O:C:D:")) != EOF)
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
	case 'c':
	    child = atoi (optarg);
	    break;
	case 'o':
	    op_rate = atoi (optarg);
	    break;
	case 'O':
	    percent_direct = atoi (optarg);
	    set_percent_direct = 1;
	    change_workload++;
	    break;
	case 'C':
	    percent_compress = atoi (optarg);
	    set_percent_compress = 1;
	    change_workload++;
	    break;
	case 'D':
	    percent_dedup = atoi (optarg);
	    set_percent_dedup = 1;
	    change_workload++;
	    break;
	};
    }

    input = fopen (my_pdsm_file, "r+");
    if (input == NULL)
    {
	printf ("Can't open the file %s\n", my_pdsm_file);
	usage ();
	exit (1);
    }
    while (1)
    {
	x = (int) fread (&pdsm_control_dummy,
			 sizeof (struct pdsm_remote_control), 1, input);
	if (x < 1)
	{
	    break;
	}
	pdsm_control[count] =
	    (struct pdsm_remote_control *)
	    malloc (sizeof (struct pdsm_remote_control));
	count++;
    }
    if (count == 0)
    {
	printf ("No entries found in the control file: %s\n", my_pdsm_file);
	usage ();
	exit (1);
    }
    fseek (input, 0, SEEK_SET);
    /* Show the list that you found */
    while (yy < count)
    {
	x = (int) fread (pdsm_control[yy],
			 sizeof (struct pdsm_remote_control), 1, input);
	printf ("Child name   = %-20s  ", pdsm_control[yy]->hostname);
	printf ("Child id  = %-5d  ", pdsm_control[yy]->client_id);
	printf ("op_rate  = %-6.2f ", pdsm_control[yy]->op_rate);
	printf ("P_direct = %-5d ",
		pdsm_control[yy]->work_load_out.percent_direct);
	printf ("P_compress = %-5d ",
		pdsm_control[yy]->work_load_out.percent_compress);
	printf ("P_dedup = %-5d ",
		pdsm_control[yy]->work_load_out.percent_dedup);
	printf ("\n");
	yy++;

    }
    if (child != -1)
    {
	printf ("Child %d Seek to %d offset and update control entry.\n",
		child, (int) (child * sizeof (struct pdsm_remote_control)));
	if (op_rate)
	{
	    pdsm_control[child]->client_id = child;
	    pdsm_control[child]->set_op_rate = 1;
	    pdsm_control[child]->op_rate = (double) op_rate;
	}
	if (change_workload)
	{
	    pdsm_control[child]->client_id = child;
	    pdsm_control[child]->set_workload = 1;
	    if (set_percent_direct)
		pdsm_control[child]->work_load_out.percent_direct =
		    percent_direct;
	    if (set_percent_compress)
		pdsm_control[child]->work_load_out.percent_compress =
		    percent_compress;
	    if (set_percent_dedup)
		pdsm_control[child]->work_load_out.percent_dedup =
		    percent_dedup;
	}
	/* Update the pdsm_control structure */
	fseek (input, (child * sizeof (struct pdsm_remote_control)),
	       SEEK_SET);
	fwrite (pdsm_control[child], sizeof (struct pdsm_remote_control), 1,
		input);
    }
    return (0);
}

void
usage (void)
{
    printf ("Usage:  netmist_modify \n");
    printf ("-f filename \n");
    printf ("-c child_id \n");
    printf ("-o Op_rate \n");
    printf ("-O Percent_direct \n");
    printf ("-C Percent_compress \n");
    printf ("-D Percent_dedup \n");
    printf
	("  (if only -f is specifed, then show the possible client ids) \n");

}
