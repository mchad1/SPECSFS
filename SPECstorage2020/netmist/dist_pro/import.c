/*****************************************************************************
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
 *****************************************************************************/

#include "./copyright.txt"
#include "./license.txt"
#if !defined(WIN32)
#include <strings.h>
#endif

int is_empty (char *);
void import_table (char *);
int find_count (char *);
int number_of_workloads;
int grab_line (FILE *, char *, int);

extern struct mix_table my_mix_table[MAX_WORK_OBJ];

/**
 * @brief Imports a custom workload 
 */
/*
 * __doc__ Function  : void import_table (char *iname)
 * __doc__ Arguments : char iname: Buffer space for import info
 * __doc__ Returns   : void
 * __doc__ Performs  : Imports a custom workload 
 * __doc__
 */
#define LBUFSZ 1024
void
import_table (char *iname)
{
    FILE *fd;
    int total = 0;
    int i, j, z, dummy;
    int rtotal = 0;
    int wtotal = 0;
    int stotal = 0;
    int num;
    char buf[LBUFSZ];
    char tmpbuf[500];
    extern _CMD_LINE cmdline;

    number_of_workloads = find_count (iname);
    if (number_of_workloads > MAX_WORK_OBJ)
    {
	printf ("Too many workload definitions. Limit is currently %d",
		MAX_WORK_OBJ);
	exit (-1);
    }

    /* 
     * Save the workload_count information for distribution to NM and clients.
     */
    cmdline.workload_count = number_of_workloads;
    fd = fopen (iname, "r");
    if (fd == NULL)
    {
	printf ("Unable to open the mix file %s\n", iname);
	exit (3);
    }
    for (j = 0; j < number_of_workloads; j++)
    {
	/* workload name */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Workload name %s\n",
		    (char *) &my_mix_table[j].workload_name);
	if (num == EOF)
	{
	    printf ("EOF while importing workload. Object number %d\n", j);
	    exit (-1);
	}
	if (num != 1)
	{
	    printf ("Error importing workload name for object number %d\n",
		    j);
	    exit (-1);
	}
#if defined(DEBUG)
	printf ("Importing %s\n", my_mix_table[j].workload_name);
#endif

	/* percent read */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].read_string, my_mix_table[0].read_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].read_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_read);
	if (num != 1)
	{
	    printf
		("Error importing Percent read for object number %d Num %d\n",
		 j, num);
	    printf ("Line: %s My_mix_table[%d].read_string = %s\n", buf, j,
		    my_mix_table[j].read_string);
	    exit (-1);
	}

	/* percent readfile */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].read_file_string,
		my_mix_table[0].read_file_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].read_file_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_read_file);
	if (num != 1)
	{
	    printf ("Error importing Percent read for object number %d\n", j);
	    exit (-1);
	}

	/* percent mmap */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].mmap_read_string,
		my_mix_table[0].mmap_read_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].mmap_read_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_mmap_read);
	if (num != 1)
	{
	    printf
		("Error importing Percent mmap read for object number %d %d\n",
		 j, num);
	    exit (-1);
	}

	/* percent readrand */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].read_rand_string,
		my_mix_table[0].read_rand_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].read_rand_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_read_rand);
	if (num != 1)
	{
	    printf
		("Error importing Percent rand read for object number %d\n",
		 j);
	    exit (-1);
	}

	/* write */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].write_string, my_mix_table[0].write_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].write_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_write);
	if (num != 1)
	{
	    printf ("Error importing Percent write for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Write file */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].write_file_string,
		my_mix_table[0].write_file_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].write_file_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_write_file);
	if (num != 1)
	{
	    printf ("Error importing Percent write for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Mmap write */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].mmap_write_string,
		my_mix_table[0].mmap_write_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].mmap_write_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_mmap_write);
	if (num != 1)
	{
	    printf
		("Error importing Percent mmap write for object number %d\n",
		 j);
	    exit (-1);
	}

	/* write_rand */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].write_rand_string,
		my_mix_table[0].write_rand_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].write_rand_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_write_rand);
	if (num != 1)
	{
	    printf
		("Error importing Percent rand write for object number %d\n",
		 j);
	    exit (-1);
	}

	/* RMW */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].rmw_string, my_mix_table[0].rmw_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].rmw_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_rmw);
	if (num != 1)
	{
	    printf ("Error importing Percent rmw for object number %d\n", j);
	    exit (-1);
	}

	/* Mkdir */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].mkdir_string, my_mix_table[0].mkdir_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].mkdir_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_mkdir);
	if (num != 1)
	{
	    printf ("Error importing Percent mkdir for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Rmdir */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].rmdir_string, my_mix_table[0].rmdir_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].rmdir_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_rmdir);
	if (num != 1)
	{
	    printf ("Error importing Percent rmdir for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Unlink */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].unlink_string, my_mix_table[0].unlink_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].unlink_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_unlink);
	if (num != 1)
	{
	    printf ("Error importing Percent unlink for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Unlink2 */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].unlink2_string,
		my_mix_table[0].unlink2_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].unlink2_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_unlink2);
	if (num != 1)
	{
	    printf ("Error importing Percent unlink2 for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Create */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].create_string, my_mix_table[0].create_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].create_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_create);
	if (num != 1)
	{
	    printf ("Error importing Percent create for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Append */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].append_string, my_mix_table[0].append_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].append_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_append);
	if (num != 1)
	{
	    printf ("Error importing Percent append for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Locking */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].locking_string,
		my_mix_table[0].locking_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].locking_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_locking);
	if (num != 1)
	{
	    printf ("Error importing Percent locking for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Access */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].access_string, my_mix_table[0].access_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].access_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_access);
	if (num != 1)
	{
	    printf ("Error importing Percent access for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Stat */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].stat_string, my_mix_table[0].stat_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].stat_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_stat);
	if (num != 1)
	{
	    printf ("Error importing Percent stat for object number %d\n", j);
	    exit (-1);
	}

	/* Neg_Stat */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].neg_stat_string,
		my_mix_table[0].neg_stat_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].neg_stat_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_neg_stat);
	if (num != 1)
	{
	    printf ("Error importing Percent neg_stat for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Chmod */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].chmod_string, my_mix_table[0].chmod_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].chmod_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_chmod);
	if (num != 1)
	{
	    printf ("Error importing Percent chmod for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Readdir */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].readdir_string,
		my_mix_table[0].readdir_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].readdir_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_readdir);
	if (num != 1)
	{
	    printf ("Error importing Percent readdir for object number %d\n",
		    j);
	    exit (-1);
	}

	/* CopyFile */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].copyfile_string,
		my_mix_table[0].copyfile_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].copyfile_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_copyfile);
	if (num != 1)
	{
	    printf ("Error importing Percent copyfile for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Rename */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].rename_string, my_mix_table[0].rename_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].rename_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_rename);
	if (num != 1)
	{
	    printf ("Error importing Percent rename for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Statfs */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].statfs_string, my_mix_table[0].statfs_string);
	sprintf (tmpbuf, "Percent %-35s %s \n", my_mix_table[j].statfs_string,
		 "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_statfs);
	if (num != 1)
	{
	    printf ("Error importing Percent statfs for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Pathconf */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].pathconf_string,
		my_mix_table[0].pathconf_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].pathconf_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_pathconf);
	if (num != 1)
	{
	    printf ("Error importing Percent pathconf for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Truncate */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].trunc_string, my_mix_table[0].trunc_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].trunc_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_trunc);
	if (num != 1)
	{
	    printf ("Error importing Percent trunc for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Custom1 */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].custom1_string,
		my_mix_table[0].custom1_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].custom1_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_custom1);
	if (num != 1)
	{
	    printf ("Error importing Percent custom1 for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Custom2 */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	memset (tmpbuf, 0, 500);
	strcpy (my_mix_table[j].custom2_string,
		my_mix_table[0].custom2_string);
	sprintf (tmpbuf, "Percent %-35s %s \n",
		 my_mix_table[j].custom2_string, "%d");
	num = sscanf (buf, tmpbuf, &my_mix_table[j].percent_custom2);
	if (num != 1)
	{
	    printf ("Error importing Percent custom2 for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Read size dist table */
	rtotal = 0;
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Read elem %d xfer min size %d\n", &dummy,
			  &my_mix_table[j].read_dist[i].size_min);
	    if (num != 2)
	    {
		printf
		    ("Error importing read elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }
	    if (my_mix_table[j].read_dist[i].size_min == 0)
		my_mix_table[j].read_dist[i].size_min = 1;

	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Read elem %d xfer max size %d\n", &dummy,
			  &my_mix_table[j].read_dist[i].size_max);
	    if (num != 2)
	    {
		printf
		    ("Error importing read elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }

	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Read elem %d xfer percent %d\n", &dummy,
			  &my_mix_table[j].read_dist[i].percent);
	    if (num != 2)
	    {
		printf
		    ("Error importing read elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }
	    rtotal += my_mix_table[j].read_dist[i].percent;
	}
	if (rtotal != 100)
	{
	    printf ("Bad mix file. Workload %s Read dist percent != 100.\n",
		    (char *) &my_mix_table[j].workload_name);
	    exit (3);
	}

	/* Write size dist table */
	wtotal = 0;
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Write elem %d xfer min size %d\n", &dummy,
			  &my_mix_table[j].write_dist[i].size_min);
	    if (num != 2)
	    {
		printf
		    ("Error importing write elem %d for object number %d\n",
		     i, j);
		exit (-1);
	    }
	    if (my_mix_table[j].write_dist[i].size_min == 0)
		my_mix_table[j].write_dist[i].size_min = 1;


	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Write elem %d xfer max size %d\n", &dummy,
			  &my_mix_table[j].write_dist[i].size_max);
	    if (num != 2)
	    {
		printf
		    ("Error importing write elem %d for object number %d\n",
		     i, j);
		exit (-1);
	    }

	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "Write elem %d xfer percent %d\n", &dummy,
			  &my_mix_table[j].write_dist[i].percent);
	    if (num != 2)
	    {
		printf
		    ("Error importing write elem %d for object number %d\n",
		     i, j);
		exit (-1);
	    }
	    wtotal += my_mix_table[j].write_dist[i].percent;
	}
	if (wtotal != 100)
	{
	    printf ("Bad mix file. Workload %s Write dist percent != 100.\n",
		    (char *) &my_mix_table[j].workload_name);
	    exit (3);
	}

	/* File size dist table */
	stotal = 0;
	for (i = 0; i < MAX_DIST_ELEM; i++)
	{
	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "File size elem %d min size %lld\n", &dummy,
			  &my_mix_table[j].file_size_dist[i].size_min);
	    if (num != 2)
	    {
		printf
		    ("Error importing file size elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }
	    if (my_mix_table[j].file_size_dist[i].size_min == 0LL)
		my_mix_table[j].file_size_dist[i].size_min = 1LL;

	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "File size elem %d max size %lld\n", &dummy,
			  &my_mix_table[j].file_size_dist[i].size_max);
	    if (num != 2)
	    {
		printf
		    ("Error importing file size elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }

	    if (grab_line (fd, buf, LBUFSZ) == 0)
		break;
	    num = sscanf (buf, "File size elem %d percent %d\n", &dummy,
			  &my_mix_table[j].file_size_dist[i].percent);
	    if (num != 2)
	    {
		printf
		    ("Error importing file size elem %d for object number %d num=%d\n",
		     i, j, num);
		exit (-1);
	    }
	    stotal += my_mix_table[j].file_size_dist[i].percent;
	}
	if (stotal != 100)
	{
	    printf
		("Bad mix file. Workload %s File size dist percent != 100.\n",
		 (char *) &my_mix_table[j].workload_name);
	    exit (3);
	}

	/* Min_pre_name_length */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Min_pre_name_length %d\n",
		    &my_mix_table[j].min_pre_name_length);
	if (num != 1)
	{
	    printf
		("Error importing min_pre_name_lengthfor object number %d\n",
		 j);
	    exit (-1);
	}

	/* Max_pre_name_length */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Max_pre_name_length %d\n",
		    &my_mix_table[j].max_pre_name_length);
	if (num != 1)
	{
	    printf
		("Error importing max_pre_name_lengthfor object number %d\n",
		 j);
	    exit (-1);
	}

	/* Min_post_name_length */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Min_post_name_length %d\n",
		    &my_mix_table[j].min_post_name_length);
	if (num != 1)
	{
	    printf
		("Error importing min_post_name_lengthfor object number %d\n",
		 j);
	    exit (-1);
	}

	/* Max_post_name_length */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Max_post_name_length %d\n",
		    &my_mix_table[j].max_post_name_length);
	if (num != 1)
	{
	    printf
		("Error importing max_post_name_lengthfor object number %d\n",
		 j);
	    exit (-1);
	}

	/* Percent commit */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent write commit %d\n",
		    &my_mix_table[j].percent_commit);
	if (num != 1)
	{
	    printf
		("Error importing percent write commit for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Percent direct */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent direct %d\n",
		    &my_mix_table[j].percent_direct);
	if (num != 1)
	{
	    printf ("Error importing percent direct for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Align */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Align %d\n", &my_mix_table[j].align);
	if (num != 1)
	{
	    printf ("Error importing align for object number %d\n", j);
	    exit (-1);
	}

	/* Percent O_SYNC */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent osync %d\n",
		    &my_mix_table[j].percent_osync);
	if (num != 1)
	{
	    printf ("Error importing percent osync for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Geometric */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent geometric %d\n",
		    &my_mix_table[j].percent_geometric);
	if (num != 1)
	{
	    printf
		("Error importing percent geometric for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Percent compress */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent compress %d\n",
		    &my_mix_table[j].percent_compress);
	if (num != 1)
	{
	    printf ("Error importing percent compress for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Percent dedup */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent dedup %d\n",
		    &my_mix_table[j].percent_dedup);
	if (num != 1)
	{
	    printf ("Error importing percent dedup for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Percent dedup within */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent dedup_within %d\n",
		    &my_mix_table[j].percent_dedup_within);
	if (num != 1)
	{
	    printf
		("Error importing percent dedup_within for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Percent dedup across */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent dedup_across %d\n",
		    &my_mix_table[j].percent_dedup_across);
	if (num != 1)
	{
	    printf
		("Error importing percent dedup_across for object number %d\n",
		 j);
	    exit (-1);
	}

	/* dedup group count */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Dedupe group count %d\n",
		    &my_mix_table[j].dedup_group_count);
	if (num != 1)
	{
	    printf
		("Error importing dedupe group count for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Percent spot */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent per_spot %d\n",
		    &my_mix_table[j].percent_per_spot);
	if (num != 1)
	{
	    printf ("Error importing percent per spot for object number %d\n",
		    j);
	    exit (-1);
	}

	/* min access per spot */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Min acc_per_spot %d\n",
		    &my_mix_table[j].min_acc_per_spot);
	if (num != 1)
	{
	    printf ("Error importing min_acc_per_spot for object number %d\n",
		    j);
	    exit (-1);
	}

	/* access multiplier per spot */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Acc mult_spot %d\n",
		    &my_mix_table[j].acc_mult_spot);
	if (num != 1)
	{
	    printf ("Error importing acc_mult_spot for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Percent affinity */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Percent affinity %d\n",
		    &my_mix_table[j].percent_affinity);
	if (num != 1)
	{
	    printf ("Error importing percent affinity for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Spot shape */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Spot shape %d\n", &my_mix_table[j].spot_shape);
	if (num != 1)
	{
	    printf ("Error importing spot shape for object number %d\n", j);
	    exit (-1);
	}

	/* Dedup granule size */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Dedup Granule size %d\n",
		    &my_mix_table[j].dedup_granule_size);
	if (num != 1)
	{
	    printf
		("Error importing dedup granule size for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Dedup granule rep limit */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Dedup gran rep limit %d\n",
		    &my_mix_table[j].dedup_gran_rep_limit);
	if (num != 1)
	{
	    printf
		("Error importing dedup gran rep limit for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Use file size dist */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Use file size dist %d\n",
		    &my_mix_table[j].use_file_size_dist);
	if (num != 1)
	{
	    printf
		("Error importing use file size dist for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Compression granule size */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Comp Granule size %d\n",
		    &my_mix_table[j].comp_granule_size);
	if (num != 1)
	{
	    printf
		("Error importing compression granule size for object number %d\n",
		 j);
	    exit (-1);
	}

	/* Background flag */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Background %d\n", &my_mix_table[j].background);
	if (num != 1)
	{
	    printf ("Error importing background flag for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Sharemode flag */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Sharemode %d\n", &my_mix_table[j].sharemode);
	if (num != 1)
	{
	    printf ("Error importing sharemode flag for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Uniform file size flag */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Uniform size dist %d\n",
		    &my_mix_table[j].uniform_file_size_dist);
	if (num != 1)
	{
	    printf
		("Error importing uniform size dist value for object number %d Value %d\n",
		 j, num);
	    exit (-1);
	}

	/* Random dist behavior value */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Rand dist behavior %d\n",
		    &my_mix_table[j].rand_dist_behavior);
	if (num != 1)
	{
	    printf
		("Error importing rand dist behavior value for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Cipher flag */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Cipher behavior %d\n", &my_mix_table[j].cipher);
	if (num != 1)
	{
	    printf
		("Error importing cipher behavior value for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Get the notifications param */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Notification percent %d\n",
		    &my_mix_table[j].notify);
	if (num != 1)
	{
	    printf
		("Error importing notification percent value for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Get the lru_on param */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "LRU %d\n", &my_mix_table[j].lru_on);
	if (num != 1)
	{
	    printf ("Error importing LRU value for object number %d val %d\n",
		    j, num);
	    exit (-1);
	}

	/* Get the pattern layout version param */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Pattern version %d\n", &my_mix_table[j].patt_vers);
	if (num != 1)
	{
	    printf
		("Error importing Pattern version value for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Get the INIT_RATE throttle */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Init rate throttle %d\n",
		    &my_mix_table[j].init_rate_enable);
	if (num != 1)
	{
	    printf
		("Error importing init rate throttle for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Get the INIT_read flag */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Init read flag %d\n", &my_mix_table[j].init_read);
	if (num != 1)
	{
	    printf
		("Error importing init read flag for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	/* Extra Dir levels */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Extra dir levels %d\n",
		    &my_mix_table[j].extra_dir_levels);
	if (num != 1)
	{
	    printf ("Error importing extra dir levels for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Chaff count */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "Chaff count %d\n", &my_mix_table[j].chaff_count);
	if (num != 1)
	{
	    printf ("Error importing chaff_count for object number %d\n", j);
	    exit (-1);
	}

	/* Shared_buckets */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Shared buckets %d\n",
		    &my_mix_table[j].shared_buckets);
	if (num != 1)
	{
	    printf ("Error importing Shared buckets for object number %d\n",
		    j);
	    exit (-1);
	}

	/* Get the release version */
	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num =
	    sscanf (buf, "Release version %d\n",
		    &my_mix_table[j].rel_version);
	if (num != 1)
	{
	    printf
		("Error importing release version number for object number %d val %d\n",
		 j, num);
	    exit (-1);
	}

	if (grab_line (fd, buf, LBUFSZ) == 0)
	    break;
	num = sscanf (buf, "FS type %s\n", my_mix_table[j].fs_type);
	if (num != 1)
	{
	    printf ("Error importing FS type string for object number %d\n",
		    j);
	    exit (-1);
	}
	total =
	    my_mix_table[j].percent_read + my_mix_table[j].percent_read_rand +
	    my_mix_table[j].percent_write +
	    my_mix_table[j].percent_write_rand +
	    my_mix_table[j].percent_mmap_write +
	    my_mix_table[j].percent_mmap_read + my_mix_table[j].percent_rmw +
	    my_mix_table[j].percent_mkdir + my_mix_table[j].percent_rmdir +
	    my_mix_table[j].percent_unlink + my_mix_table[j].percent_append +
	    my_mix_table[j].percent_unlink2 +
	    my_mix_table[j].percent_locking + my_mix_table[j].percent_access +
	    my_mix_table[j].percent_stat + my_mix_table[j].percent_chmod +
	    my_mix_table[j].percent_neg_stat +
	    my_mix_table[j].percent_readdir +
	    my_mix_table[j].percent_copyfile +
	    my_mix_table[j].percent_rename + my_mix_table[j].percent_statfs +
	    my_mix_table[j].percent_pathconf +
	    my_mix_table[j].percent_trunc +
	    my_mix_table[j].percent_custom1 +
	    my_mix_table[j].percent_custom2 +
	    my_mix_table[j].percent_read_file +
	    my_mix_table[j].percent_write_file +
	    my_mix_table[j].percent_create;
	if (total != 100)
	{
	    printf
		("Bad mix file. Workload %s Total Op percent != 100 (%d)\n",
		 (char *) &my_mix_table[j].workload_name, total);
	    exit (3);
	}
	total = 0;
    }
    /* Zap any workload objects that are not initialized in the workload file */
    if (j < MAX_WORK_OBJ)
    {
	for (z = j; z < MAX_WORK_OBJ; z++)
	    memset ((char *) &my_mix_table[z].workload_name, 0, MAXHNAME);
    }
    fclose (fd);
}

int
grab_line (FILE * fd, char *buffer, int size)
{
    char *cret;
    while (1)
    {
	memset (buffer, 0, size);
	cret = fgets (buffer, size, fd);
	if (cret == (char *) 0)
	    return 0;
	/*We also ignore empty line */
	if (buffer[0] == '#' || is_empty (buffer))
	    continue;
	if (buffer[strlen (buffer) - 1] == '\n')
	    buffer[strlen (buffer) - 1] = 0;;
	return 1;
    }
}

/* 
 * Ignore empty lines... Return 1 if it was empty.
 */


/**
 * @brief Check for empty lines
 */
/*
 * __doc__ Function  : int is_empty (char *buf)
 * __doc__ Arguments : char buf: Input buffer to examine
 * __doc__ Returns   : int: True or false
 * __doc__ Performs  : Check for empty lines
 * __doc__
 */
int
is_empty (char *buf)
{
    while (buf != NULL && *buf != '\0')
    {
	if (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == '\r')
	{
	    buf++;
	}
	else
	    return 0;
    }
    return 1;
}

/**
 * @brief Find the number of workloads in the workload file
 */
/*
 * __doc__ Function  : int find_count(). Returns number of workloads
 * __doc__ Arguments : char *name: Name of the workload file
 * __doc__ Returns   : int: Number of entries
 * __doc__
 */
int
find_count (char *name)
{
    FILE *fd;
    int num;
    char buf[1024], temp_buf[256];
    char *cret;
    int k = 0;

    fd = fopen (name, "r");
    if (fd == NULL)
    {
	printf ("Unable to open the mix file %s\n", name);
	exit (3);
    }
    while (1)
    {
	memset (buf, 0, 1024);
	cret = fgets (buf, 1024, fd);
	if (cret == (char *) 0)
	    break;
	/*We also ignore empty line */
	if (buf[0] == '#' || is_empty (buf))
	    continue;
	if (buf[strlen (buf) - 1] == '\n')
	    buf[strlen (buf) - 1] = 0;;
	num = sscanf (buf, "Workload name %s\n", (char *) temp_buf);
	if (num == EOF)
	    break;
	if (num == 1)
	    k++;
    }
    fclose (fd);
    return (k);
}
