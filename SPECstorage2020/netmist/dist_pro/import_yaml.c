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
#include "netmist.h"
int yaml_debug = 0;
int component;			/* Total number of workload components */
int benchmarks;			/* Total number of benchmark */
int warm_time, run_time;
extern struct mix_table my_mix_table[MAX_WORK_OBJ];
extern _CMD_LINE cmdline;
#define MY_YAML_MAGIC 2
int yaml_magic = MY_YAML_MAGIC;

/**
 * @brief This imports the workloads from the yaml configuration file.
 *
 * @param iname : The file name of the yaml file.
 */
int
import_yaml (char *iname)
{
    int sequence = 0;
    int mapping = 0;
    int total, j, k, totalr, totalw, totalf;
    FILE *fh = fopen (iname, "r");
    int next = 0;
    char temp_string[20];
    yaml_parser_t parser;
    yaml_event_t event;		/* New variable */

    /* Initialize parser */
    if (!yaml_parser_initialize (&parser))
	fputs ("Failed to initialize parser!\n", stderr);
    if (fh == NULL)
	printf ("Failed to open file! %s\n", iname);
    /*    fputs ("Failed to open file!\n", stderr); */

    /* Set input file */
    yaml_parser_set_input_file (&parser, fh);

    /* START new code */
    do
    {
	if (!yaml_parser_parse (&parser, &event))
	{
	    printf ("Parser error %d\n", parser.error);
	    exit (250);
	}
	memset (temp_string, 0, 20);
	switch (event.type)
	{
	case YAML_NO_EVENT:
	    if (yaml_debug)
		puts ("No event!");
	    break;
	    /* Stream start/end */
	case YAML_STREAM_START_EVENT:
	    if (yaml_debug)
		puts ("STREAM START");
	    break;
	case YAML_STREAM_END_EVENT:
	    if (yaml_debug)
		puts ("STREAM END");
	    break;
	    /* Block delimeters */
	case YAML_DOCUMENT_START_EVENT:
	    if (yaml_debug)
		puts ("<b>Start Document</b>");
	    break;
	case YAML_DOCUMENT_END_EVENT:
	    if (yaml_debug)
		puts ("<b>End Document</b>");
	    break;
	case YAML_SEQUENCE_START_EVENT:
	    if (yaml_debug)
		puts ("<b>Start Sequence </b>");
	    sequence++;
	    break;
	case YAML_SEQUENCE_END_EVENT:
	    if (yaml_debug)
		puts ("<b>End Sequence </b>");
	    sequence--;
	    break;
	case YAML_MAPPING_START_EVENT:
	    if (yaml_debug)
		puts ("<b>Start Mapping </b>");
	    mapping++;
	    break;
	case YAML_MAPPING_END_EVENT:
	    if (yaml_debug)
		puts ("<b>End Mapping </b>");
	    mapping--;
	    break;
	    /* Data */
	case YAML_ALIAS_EVENT:
	    printf ("Got alias (anchor %s)\n",
		    (char *) event.data.alias.anchor);
	    break;
	case YAML_SCALAR_EVENT:
	    if (yaml_debug)
		printf ("Got scalar (value %s) Sequence %d Mapping %d \n",
			(char *) event.data.scalar.value, sequence, mapping);
	    switch (next)
	    {
	    case 0:
		next = 0;
		break;
	    case 1:		/* Global Workload name */
		next = 0;
		break;
	    case 2:		/* Run_time */
		if (run_time == 0)
		    (void) sscanf ((char *) (char *) event.data.scalar.value,
				   "%d", &run_time);
		cmdline.yaml_run_time = run_time;
		cmdline.run_time_flag = 1;
		next = 0;
		break;
	    case 3:		/* Warmup time */
		if (warm_time == 0)
		    (void) sscanf ((char *) (char *) event.data.scalar.value,
				   "%d", &warm_time);
		/* printf("Loading warmup time %d for component %d\n",warm_time, component); */
		/* Replicate for all components in this workload */
		my_mix_table[component].warm_time = warm_time;
		next = 0;
		break;
	    case 4:		/* Proc_oprate_threshold */
		next = 0;
		break;
	    case 5:		/* Global_oprate_threshold */
		next = 0;
		break;
	    case 6:		/* Workload_variance */
		next = 0;
		break;
	    case 7:		/* Component_name */
		component++;
		strcpy (my_mix_table[component - 1].workload_name,
			(char *) (char *) event.data.scalar.value);
		/*   printf("Component %d name %s \n",component-1,my_mix_table[component-1].workload_name); */
		if (warm_time)
		    my_mix_table[component - 1].warm_time = warm_time;
		next = 0;
		break;
	    case 8:		/* Op_rate */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%f", &my_mix_table[component - 1].op_rate);
		next = 0;
		break;
	    case 9:		/* Instances */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d", &my_mix_table[component - 1].instances);
		next = 0;
		break;
	    case 10:		/* File_size */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%s", temp_string);
		my_mix_table[component - 1].file_size = (atoi (temp_string));
		if (temp_string[strlen (temp_string) - 1] == 'k'
		    || temp_string[strlen (temp_string) - 1] == 'K')
		{
		    temp_string[strlen (temp_string) - 1] = 0;
		    my_mix_table[component - 1].file_size =
			(atoi (temp_string));
		}
		if (temp_string[strlen (temp_string) - 1] == 'm'
		    || temp_string[strlen (temp_string) - 1] == 'M')
		{
		    temp_string[strlen (temp_string) - 1] = 0;
		    my_mix_table[component - 1].file_size =
			(atoi (temp_string)) * 1024;
		}
		if (temp_string[strlen (temp_string) - 1] == 'g'
		    || temp_string[strlen (temp_string) - 1] == 'G')
		{
		    temp_string[strlen (temp_string) - 1] = 0;
		    my_mix_table[component - 1].file_size =
			(atoi (temp_string)) * 1024 * 1024;
		}
		next = 0;
		break;
	    case 11:		/* Dir_count */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d", &my_mix_table[component - 1].dir_count);
		next = 0;
		break;
	    case 12:		/* Files_per_dir */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d",
			       &my_mix_table[component - 1].files_per_dir);
		next = 0;
		break;
	    case 13:		/* Extra_dir_levels */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d",
			       &my_mix_table[component - 1].extra_dir_levels);
		next = 0;
		break;
	    case 14:		/* Chaff_count */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].chaff_count);
		next = 0;
		break;
	    case 15:		/* Shared_buckets */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].shared_buckets);
		next = 0;
		break;
	    case 16:		/* Read_string */
		strcpy (my_mix_table[component - 1].read_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 17:		/* Read percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_read);
		next = 0;
		break;
	    case 18:		/* Read_file_string */
		strcpy (my_mix_table[component - 1].read_file_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 19:		/* Read_file percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_read_file);
		next = 0;
		break;
	    case 20:		/* Mmap_read_string */
		strcpy (my_mix_table[component - 1].mmap_read_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 21:		/* Mmap_read percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_mmap_read);
		next = 0;
		break;
	    case 22:		/* Random_read_string */
		strcpy (my_mix_table[component - 1].read_rand_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 23:		/* Random_read percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_read_rand);
		next = 0;
		break;
	    case 24:		/* Write_string */
		strcpy (my_mix_table[component - 1].write_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 25:		/* Write percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_write);
		next = 0;
		break;
	    case 26:		/* Write_file_string */
		strcpy (my_mix_table[component - 1].write_file_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 27:		/* Write_file percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_write_file);
		next = 0;
		break;
	    case 28:		/* Mmap_write_string */
		strcpy (my_mix_table[component - 1].mmap_write_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 29:		/* Mmap_write percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_mmap_write);
		next = 0;
		break;
	    case 30:		/* Random_write_string */
		strcpy (my_mix_table[component - 1].write_rand_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 31:		/* Random_write percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_write_rand);
		next = 0;
		break;
	    case 32:		/* Read_modify_write_string */
		strcpy (my_mix_table[component - 1].rmw_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 33:		/* Read_modify_write percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_rmw);
		next = 0;
		break;
	    case 34:		/* Mkdir_string  */
		strcpy (my_mix_table[component - 1].mkdir_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 35:		/* Mkdir percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_mkdir);
		next = 0;
		break;
	    case 36:		/* Rmdir_string */
		strcpy (my_mix_table[component - 1].rmdir_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 37:		/* Rmdir percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_rmdir);
		next = 0;
		break;
	    case 38:		/* Unlink_string */
		strcpy (my_mix_table[component - 1].unlink_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 39:		/* Unlink percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_unlink);
		next = 0;
		break;
	    case 40:		/* Unlink2_string */
		strcpy (my_mix_table[component - 1].unlink2_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 41:		/* Unlink2 percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_unlink2);
		next = 0;
		break;
	    case 42:		/* Create_string */
		strcpy (my_mix_table[component - 1].create_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 43:		/* Create percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_create);
		next = 0;
		break;
	    case 44:		/* Append_string */
		strcpy (my_mix_table[component - 1].append_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 45:		/* Append percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_append);
		next = 0;
		break;
	    case 46:		/* Lock_string */
		strcpy (my_mix_table[component - 1].locking_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 47:		/* Lock percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_locking);
		next = 0;
		break;
	    case 48:		/* Access_string */
		strcpy (my_mix_table[component - 1].access_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 49:		/* Access percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_access);
		next = 0;
		break;
	    case 50:		/* Stat_string */
		strcpy (my_mix_table[component - 1].stat_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 51:		/* Stat percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_stat);
		next = 0;
		break;
	    case 52:		/* Neg_stat_string */
		next = 0;
		strcpy (my_mix_table[component - 1].neg_stat_string,
			(char *) event.data.scalar.value);
		break;
	    case 53:		/* Neg_stat percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_neg_stat);
		next = 0;
		break;
	    case 54:		/* Chmod_string */
		strcpy (my_mix_table[component - 1].chmod_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 55:		/* Chmod percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_chmod);
		next = 0;
		break;
	    case 56:		/* Readdir_string */
		strcpy (my_mix_table[component - 1].readdir_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 57:		/* Readdir percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_readdir);
		next = 0;
		break;
	    case 58:		/* Copyfile_string */
		strcpy (my_mix_table[component - 1].copyfile_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 59:		/* Copyfile percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_copyfile);
		next = 0;
		break;
	    case 60:		/* Rename_string */
		strcpy (my_mix_table[component - 1].rename_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 61:		/* Rename percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_rename);
		next = 0;
		break;
	    case 62:		/* Statfs_string */
		strcpy (my_mix_table[component - 1].statfs_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 63:		/* Statfs percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_statfs);
		next = 0;
		break;
	    case 64:		/* Pathconf_string */
		strcpy (my_mix_table[component - 1].pathconf_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 65:		/* Pathconf percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_pathconf);
		next = 0;
		break;
	    case 66:		/* Trunc_string */
		strcpy (my_mix_table[component - 1].trunc_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 67:		/* Trunc percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_trunc);
		next = 0;
		break;
	    case 68:		/* Custom1_string */
		strcpy (my_mix_table[component - 1].custom1_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 69:		/* Custom1 percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_custom1);
		next = 0;
		break;
	    case 70:		/* Custom2_string */
		strcpy (my_mix_table[component - 1].custom2_string,
			(char *) event.data.scalar.value);
		next = 0;
		break;
	    case 71:		/* Custom2 percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_custom2);
		next = 0;
		break;
	    case 72:		/* Read_elem_0_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[0].size_min);
		next = 0;
		break;
	    case 73:		/* Read_elem_0_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[0].size_max);
		next = 0;
		break;
	    case 74:		/* Read_elem_0_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[0].percent);
		next = 0;
		break;
	    case 75:		/* Read_elem_1_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[1].size_min);
		next = 0;
		break;
	    case 76:		/* Read_elem_1_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[1].size_max);
		next = 0;
		break;
	    case 77:		/* Read_elem_1_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[1].percent);
		next = 0;
		break;
	    case 78:		/* Read_elem_2_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[2].size_min);
		next = 0;
		break;
	    case 79:		/* Read_elem_2_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[2].size_max);
		next = 0;
		break;
	    case 80:		/* Read_elem_2_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[2].percent);
		next = 0;
		break;
	    case 81:		/* Read_elem_3_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[3].size_min);
		next = 0;
		break;
	    case 82:		/* Read_elem_3_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[3].size_max);
		next = 0;
		break;
	    case 83:		/* Read_elem_3_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[3].percent);
		next = 0;
		break;
	    case 84:		/* Read_elem_4_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[4].size_min);
		next = 0;
		break;
	    case 85:		/* Read_elem_4_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[4].size_max);
		next = 0;
		break;
	    case 86:		/* Read_elem_4_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[4].percent);
		next = 0;
		break;
	    case 87:		/* Read_elem_5_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[5].size_min);
		next = 0;
		break;
	    case 88:		/* Read_elem_5_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[5].size_max);
		next = 0;
		break;
	    case 89:		/* Read_elem_5_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[5].percent);
		next = 0;
		break;
	    case 90:		/* Read_elem_6_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[6].size_min);
		next = 0;
		break;
	    case 91:		/* Read_elem_6_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[6].size_max);
		next = 0;
		break;
	    case 92:		/* Read_elem_6_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[6].percent);
		next = 0;
		break;
	    case 93:		/* Read_elem_7_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[7].size_min);
		next = 0;
		break;
	    case 94:		/* Read_elem_7_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[7].size_max);
		next = 0;
		break;
	    case 95:		/* Read_elem_7_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[7].percent);
		next = 0;
		break;
	    case 96:		/* Read_elem_8_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[8].size_min);
		next = 0;
		break;
	    case 97:		/* Read_elem_8_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[8].size_max);
		next = 0;
		break;
	    case 98:		/* Read_elem_8_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[8].percent);
		next = 0;
		break;
	    case 99:		/* Read_elem_9_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[9].size_min);
		next = 0;
		break;
	    case 100:		/* Read_elem_9_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[9].size_max);
		next = 0;
		break;
	    case 101:		/* Read_elem_9_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[9].percent);
		next = 0;
		break;
	    case 102:		/* Read_elem_10_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[10].size_min);
		next = 0;
		break;
	    case 103:		/* Read_elem_10_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[10].size_max);
		next = 0;
		break;
	    case 104:		/* Read_elem_10_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[10].percent);
		next = 0;
		break;
	    case 105:		/* Read_elem_11_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[11].size_min);
		next = 0;
		break;
	    case 106:		/* Read_elem_11_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[11].size_max);
		next = 0;
		break;
	    case 107:		/* Read_elem_11_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[11].percent);
		next = 0;
		break;
	    case 108:		/* Read_elem_12_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[12].size_min);
		next = 0;
		break;
	    case 109:		/* Read_elem_12_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[12].size_max);
		next = 0;
		break;
	    case 110:		/* Read_elem_12_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[12].percent);
		next = 0;
		break;
	    case 111:		/* Read_elem_13_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[13].size_min);
		next = 0;
		break;
	    case 112:		/* Read_elem_13_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[13].size_max);
		next = 0;
		break;
	    case 113:		/* Read_elem_13_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[13].percent);
		next = 0;
		break;
	    case 114:		/* Read_elem_14_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[14].size_min);
		next = 0;
		break;
	    case 115:		/* Read_elem_14_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[14].size_max);
		next = 0;
		break;
	    case 116:		/* Read_elem_14_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[14].percent);
		next = 0;
		break;
	    case 117:		/* Read_elem_15_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[15].size_min);
		next = 0;
		break;
	    case 118:		/* Read_elem_15_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[15].size_max);
		next = 0;
		break;
	    case 119:		/* Read_elem_15_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].read_dist[15].percent);
		next = 0;
		break;
	    case 120:		/* Write_elem_0_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[0].size_min);
		next = 0;
		break;
	    case 121:		/* Write_elem_0_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[0].size_max);
		next = 0;
		break;
	    case 122:		/* Write_elem_0_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[0].percent);
		next = 0;
		break;
	    case 123:		/* Write_elem_1_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[1].size_min);
		next = 0;
		break;
	    case 124:		/* Write_elem_1_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[1].size_max);
		next = 0;
		break;
	    case 125:		/* Write_elem_1_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[1].percent);
		next = 0;
		break;
	    case 126:		/* Write_elem_2_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[2].size_min);
		next = 0;
		break;
	    case 127:		/* Write_elem_2_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[2].size_max);
		next = 0;
		break;
	    case 128:		/* Write_elem_2_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[2].percent);
		next = 0;
		break;
	    case 129:		/* Write_elem_3_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[3].size_min);
		next = 0;
		break;
	    case 130:		/* Write_elem_3_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[3].size_max);
		next = 0;
		break;
	    case 131:		/* Write_elem_3_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[3].percent);
		next = 0;
		break;
	    case 132:		/* Write_elem_4_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[4].size_min);
		next = 0;
		break;
	    case 133:		/* Write_elem_4_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[4].size_max);
		next = 0;
		break;
	    case 134:		/* Write_elem_4_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[4].percent);
		next = 0;
		break;
	    case 135:		/* Write_elem_5_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[5].size_min);
		next = 0;
		break;
	    case 136:		/* Write_elem_5_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[5].size_max);
		next = 0;
		break;
	    case 137:		/* Write_elem_5_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[5].percent);
		next = 0;
		break;
	    case 138:		/* Write_elem_6_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[6].size_min);
		next = 0;
		break;
	    case 139:		/* Write_elem_6_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[6].size_max);
		next = 0;
		break;
	    case 140:		/* Write_elem_6_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[6].percent);
		next = 0;
		break;
	    case 141:		/* Write_elem_7_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[7].size_min);
		next = 0;
		break;
	    case 142:		/* Write_elem_7_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[7].size_max);
		next = 0;
		break;
	    case 143:		/* Write_elem_7_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[7].percent);
		next = 0;
		break;
	    case 144:		/* Write_elem_8_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[8].size_min);
		next = 0;
		break;
	    case 145:		/* Write_elem_8_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[8].size_max);
		next = 0;
		break;
	    case 146:		/* Write_elem_8_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[8].percent);
		next = 0;
		break;
	    case 147:		/* Write_elem_9_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[9].size_min);
		next = 0;
		break;
	    case 148:		/* Write_elem_9_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[9].size_max);
		next = 0;
		break;
	    case 149:		/* Write_elem_9_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[9].percent);
		next = 0;
		break;
	    case 150:		/* Write_elem_10_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[10].size_min);
		next = 0;
		break;
	    case 151:		/* Write_elem_10_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[10].size_max);
		next = 0;
		break;
	    case 152:		/* Write_elem_10_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[10].percent);
		next = 0;
		break;
	    case 153:		/* Write_elem_11_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[11].size_min);
		next = 0;
		break;
	    case 154:		/* Write_elem_11_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[11].size_max);
		next = 0;
		break;
	    case 155:		/* Write_elem_11_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[11].percent);
		next = 0;
		break;
	    case 156:		/* Write_elem_12_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[12].size_min);
		next = 0;
		break;
	    case 157:		/* Write_elem_12_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[12].size_max);
		next = 0;
		break;
	    case 158:		/* Write_elem_12_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[12].percent);
		next = 0;
		break;
	    case 159:		/* Write_elem_13_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[13].size_min);
		next = 0;
		break;
	    case 160:		/* Write_elem_13_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[13].size_max);
		next = 0;
		break;
	    case 161:		/* Write_elem_13_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[13].percent);
		next = 0;
		break;
	    case 162:		/* Write_elem_14_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[14].size_min);
		next = 0;
		break;
	    case 163:		/* Write_elem_14_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[14].size_max);
		next = 0;
		break;
	    case 164:		/* Write_elem_14_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[14].percent);
		next = 0;
		break;
	    case 165:		/* Write_elem_15_xfer_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[15].size_min);
		next = 0;
		break;
	    case 166:		/* Write_elem_15_xfer_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[15].size_max);
		next = 0;
		break;
	    case 167:		/* Write_elem_15_xfer_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].write_dist[15].percent);
		next = 0;
		break;
	    case 168:		/* File_size_elem_0_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[0].size_min);
		next = 0;
		break;
	    case 169:		/* File_size_elem_0_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[0].size_max);
		next = 0;
		break;
	    case 170:		/* File_size_elem_0_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[0].percent);
		next = 0;
		break;
	    case 171:		/* File_size_elem_1_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[1].size_min);
		next = 0;
		break;
	    case 172:		/* File_size_elem_1_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[1].size_max);
		next = 0;
		break;
	    case 173:		/* File_size_elem_1_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[1].percent);
		next = 0;
		break;
	    case 174:		/* File_size_elem_2_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[2].size_min);
		next = 0;
		break;
	    case 175:		/* File_size_elem_2_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[2].size_max);
		next = 0;
		break;
	    case 176:		/* File_size_elem_2_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[2].percent);
		next = 0;
		break;
	    case 177:		/* File_size_elem_3_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[3].size_min);
		next = 0;
		break;
	    case 178:		/* File_size_elem_3_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[3].size_max);
		next = 0;
		break;
	    case 179:		/* File_size_elem_3_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[3].percent);
		next = 0;
		break;
	    case 180:		/* File_size_elem_4_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[4].size_min);
		next = 0;
		break;
	    case 181:		/* File_size_elem_4_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[4].size_max);
		next = 0;
		break;
	    case 182:		/* File_size_elem_4_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[4].percent);
		next = 0;
		break;
	    case 183:		/* File_size_elem_5_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[5].size_min);
		next = 0;
		break;
	    case 184:		/* File_size_elem_5_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[5].size_max);
		next = 0;
		break;
	    case 185:		/* File_size_elem_5_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[5].percent);
		next = 0;
		break;
	    case 186:		/* File_size_elem_6_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[6].size_min);
		next = 0;
		break;
	    case 187:		/* File_size_elem_6_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[6].size_max);
		next = 0;
		break;
	    case 188:		/* File_size_elem_6_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[6].percent);
		next = 0;
		break;
	    case 189:		/* File_size_elem_7_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[7].size_min);
		next = 0;
		break;
	    case 190:		/* File_size_elem_7_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[7].size_max);
		next = 0;
		break;
	    case 191:		/* File_size_elem_7_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[7].percent);
		next = 0;
		break;
	    case 192:		/* File_size_elem_8_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[8].size_min);
		next = 0;
		break;
	    case 193:		/* File_size_elem_8_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[8].size_max);
		next = 0;
		break;
	    case 194:		/* File_size_elem_8_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[8].percent);
		next = 0;
		break;
	    case 195:		/* File_size_elem_9_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[9].size_min);
		next = 0;
		break;
	    case 196:		/* File_size_elem_9_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[9].size_max);
		next = 0;
		break;
	    case 197:		/* File_size_elem_9_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[9].percent);
		next = 0;
		break;
	    case 198:		/* File_size_elem_10_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[10].size_min);
		next = 0;
		break;
	    case 199:		/* File_size_elem_10_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[10].size_max);
		next = 0;
		break;
	    case 200:		/* File_size_elem_10_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[10].percent);
		next = 0;
		break;
	    case 201:		/* File_size_elem_11_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[11].size_min);
		next = 0;
		break;
	    case 202:		/* File_size_elem_11_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[11].size_max);
		next = 0;
		break;
	    case 203:		/* File_size_elem_11_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[11].percent);
		next = 0;
		break;
	    case 204:		/* File_size_elem_12_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[12].size_min);
		next = 0;
		break;
	    case 205:		/* File_size_elem_12_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[12].size_max);
		next = 0;
		break;
	    case 206:		/* File_size_elem_12_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[12].percent);
		next = 0;
		break;
	    case 207:		/* File_size_elem_13_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[13].size_min);
		next = 0;
		break;
	    case 208:		/* File_size_elem_13_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[13].size_max);
		next = 0;
		break;
	    case 209:		/* File_size_elem_13_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[13].percent);
		next = 0;
		break;
	    case 210:		/* File_size_elem_14_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[14].size_min);
		next = 0;
		break;
	    case 211:		/* File_size_elem_14_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[14].size_max);
		next = 0;
		break;
	    case 212:		/* File_size_elem_14_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[14].percent);
		next = 0;
		break;
	    case 213:		/* File_size_elem_15_min_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[15].size_min);
		next = 0;
		break;
	    case 214:		/* File_size_elem_15_max_size */
		(void) sscanf ((char *) event.data.scalar.value, "%lld",
			       &my_mix_table[component -
					     1].file_size_dist[15].size_max);
		next = 0;
		break;
	    case 215:		/* File_size_elem_15_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].file_size_dist[15].percent);
		next = 0;
		break;
	    case 216:		/* Min_pre_name_length */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].min_pre_name_length);
		next = 0;
		break;
	    case 217:		/* Max_pre_name_length */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].max_pre_name_length);
		next = 0;
		break;
	    case 218:		/* Min_post_name_length */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].min_post_name_length);
		next = 0;
		break;
	    case 219:		/* Max_post_name_length */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].max_post_name_length);
		next = 0;
		break;
	    case 220:		/* Percent_write_commit */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_commit);
		next = 0;
		break;
	    case 221:		/* Percent_direct */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_direct);
		next = 0;
		break;
	    case 222:		/* Align */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].align);
		next = 0;
		break;
	    case 223:		/* Percent_osync */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_osync);
		next = 0;
		break;
	    case 224:		/* Percent_geometric */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_geometric);
		next = 0;
		break;
	    case 225:		/* Percent_compress */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_compress);
		next = 0;
		break;
	    case 226:		/* Percent_dedup */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_dedup);
		next = 0;
		break;
	    case 227:		/* Percent_dedup_within */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_dedup_within);
		next = 0;
		break;
	    case 228:		/* Percent_dedup_across */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_dedup_across);
		next = 0;
		break;
	    case 229:		/* Dedupe_group_count */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].dedup_group_count);
		next = 0;
		break;
	    case 230:		/* Percent_per_spot */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_per_spot);
		next = 0;
		break;
	    case 231:		/* Min_acc_per_spot */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].min_acc_per_spot);
		next = 0;
		break;
	    case 232:		/* Acc_mult_spot */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].acc_mult_spot);
		next = 0;
		break;
	    case 233:		/* Percent_affinity */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].percent_affinity);
		next = 0;
		break;
	    case 234:		/* Spot_shape */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].spot_shape);
		next = 0;
		break;
	    case 235:		/* Dedup_Granule_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].dedup_granule_size);
		next = 0;
		break;
	    case 236:		/* Dedup_gran_rep_limit */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].dedup_gran_rep_limit);
		next = 0;
		break;
	    case 237:		/* Use_file_size_dist */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].use_file_size_dist);
		next = 0;
		break;
	    case 238:		/* Comp_Granule_size */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].comp_granule_size);
		next = 0;
		break;
	    case 239:		/* Background */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].background);
		next = 0;
		break;
	    case 240:		/* Sharemode */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].sharemode);
		next = 0;
		break;
	    case 241:		/* Uniform_size_dist */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].uniform_file_size_dist);
		next = 0;
		break;
	    case 242:		/* Rand_dist_behavior */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].rand_dist_behavior);
		next = 0;
		break;
	    case 243:		/* Cipher_behavior */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].cipher);
		next = 0;
		break;
	    case 244:		/* Notification_percent */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].notify);
		next = 0;
		break;
	    case 245:		/* LRU */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].lru_on);
		next = 0;
		break;
	    case 246:		/* Pattern_version */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].patt_vers);
		next = 0;
		break;
	    case 247:		/* Init_rate enable */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].init_rate_enable);
		next = 0;
		break;
	    case 248:		/* Init_read_flag */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].init_read);
		next = 0;
		break;
	    case 249:		/* Release_version */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component - 1].rel_version);
		next = 0;
		break;
	    case 250:		/* FS_type */
		(void) sscanf ((char *) event.data.scalar.value, "%s",
			       my_mix_table[component - 1].fs_type);
		next = 0;
		break;
	    case 251:		/* Proc_latency_threshold */
		next = 0;
		break;
	    case 252:		/* Global_latency_thresholad */
		next = 0;
		break;
	    case 253:		/* Unlink2_no_recreate */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].unlink2_no_recreate);
		next = 0;
		break;
	    case 254:		/* Do_op_validate */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d", &cmdline.do_validate);
		next = 0;
		break;
	    case 255:		/* Init_rate_speed */
		(void) sscanf ((char *) event.data.scalar.value, "%f",
			       &my_mix_table[component - 1].init_rate_speed);
		next = 0;
		break;
	    case 256:		/* Dedicated_subdirectory. SfsManager uses this at the Workload object level */
		next = 0;
		break;
	    case 257:		/* Business_metric. SfsManager uses this at the Benchmark object level */
		next = 0;
		break;
	    case 258:		/* Skip_init. */
		(void) sscanf ((char *) (char *) event.data.scalar.value,
			       "%d", &cmdline.yaml_skip_init);
		cmdline.skip_init_flag = 1;
		next = 0;
		break;
	    case 259:		/* Percent_fadvise_seq */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_fadvise_seq);
		next = 0;
		break;
	    case 260:		/* Percent_fadvise_rand */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_fadvise_rand);
		next = 0;
		break;
	    case 261:		/* Percent_fadvise_dont_need */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_fadvise_dont_need);
		next = 0;
		break;
	    case 262:		/* Percent_madvise_seq */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_madvise_seq);
		next = 0;
		break;
	    case 263:		/* Percent_madvise_rand */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_madvise_rand);
		next = 0;
		break;
	    case 264:		/* Percent_madvise_dont_need */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &my_mix_table[component -
					     1].percent_madvise_dont_need);
		next = 0;
		break;
	    case 265:		/* YAML_magic */
		(void) sscanf ((char *) event.data.scalar.value, "%d",
			       &yaml_magic);
		if (yaml_magic != MY_YAML_MAGIC)
		{
		    printf
			("YAML magic number mismatch. Please use the current YAML format with YAML_magic = %d\n",
			 MY_YAML_MAGIC);
		    exit (1);
		}
		break;
	    case 266:		/* Platform type. Used so that component def can tell SM the neede type */
		(void) sscanf ((char *) event.data.scalar.value, "%s",
			       my_mix_table[component - 1].platform_type);
		next = 0;
		break;
	    default:
		printf ("Error in parsing YAML file. Unexpected value %s \n",
			(char *) event.data.scalar.value);
		exit (3);
		break;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Benchmarks") == 0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Globals") == 0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Names") == 0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percentages") == 0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Transfer_sizes") ==
		0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Misc") == 0)
	    {
		next = 0;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Benchmark_name") ==
		0)
	    {
		benchmarks++;
		warm_time = 0;
		next = 1;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Run_time") == 0)
	    {
		next = 2;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Warmup_time") == 0)
	    {
		next = 3;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Proc_oprate_threshold") == 0)
	    {
		next = 4;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Global_oprate_threshold") == 0)
	    {
		next = 5;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Workload_variance")
		== 0)
	    {
		next = 6;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Components") == 0)
	    {
		next = 7;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Op_rate") == 0)
	    {
		next = 8;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Instances") == 0)
	    {
		next = 9;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "File_size") == 0)
	    {
		next = 10;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Dir_count") == 0)
	    {
		next = 11;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Files_per_dir") ==
		0)
	    {
		next = 12;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Extra_dir_levels")
		== 0)
	    {
		next = 13;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Chaff_count") == 0)
	    {
		next = 14;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Shared_buckets") ==
		0)
	    {
		next = 15;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "System_call_distribution") == 0)
	    {
		next = 0;
	    }
	  /*----Read ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Read_string") == 0)
	    {
		next = 16;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Read_percent") ==
		0)
	    {
		next = 17;
	    }

	  /*----Read_file ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Read_file_string")
		== 0)
	    {
		next = 18;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Read_file_percent")
		== 0)
	    {
		next = 19;
	    }

	  /*---- Mmap_read ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Mmap_read_string")
		== 0)
	    {
		next = 20;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Mmap_read_percent")
		== 0)
	    {
		next = 21;
	    }

	  /*---- Random_read ----*/
	    if (strcmp
		((char *) event.data.scalar.value, "Random_read_string") == 0)
	    {
		next = 22;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Random_read_percent") == 0)
	    {
		next = 23;
	    }

	  /*---- Write ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Write_string") ==
		0)
	    {
		next = 24;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Write_percent") ==
		0)
	    {
		next = 25;
	    }

	  /*---- Write_file ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Write_file_string")
		== 0)
	    {
		next = 26;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Write_file_percent") == 0)
	    {
		next = 27;
	    }

	  /*---- Mmap_write ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Mmap_write_string")
		== 0)
	    {
		next = 28;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Mmap_write_percent") == 0)
	    {
		next = 29;
	    }

	  /*---- Random_write ----*/
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Random_write_string") == 0)
	    {
		next = 30;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Random_write_percent") == 0)
	    {
		next = 31;
	    }

	  /*---- Read_modify_write ----*/
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_modify_write_string") == 0)
	    {
		next = 32;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_modify_write_percent") == 0)
	    {
		next = 33;
	    }

	  /*---- Mkdir ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Mkdir_string") ==
		0)
	    {
		next = 34;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Mkdir_percent") ==
		0)
	    {
		next = 35;
	    }

	  /*---- Rmdir ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Rmdir_string") ==
		0)
	    {
		next = 36;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Rmdir_percent") ==
		0)
	    {
		next = 37;
	    }

	  /*---- Unlink ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Unlink_string") ==
		0)
	    {
		next = 38;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Unlink_percent") ==
		0)
	    {
		next = 39;
	    }

	  /*---- Unlink2 ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Unlink2_string") ==
		0)
	    {
		next = 40;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Unlink2_percent")
		== 0)
	    {
		next = 41;
	    }

	  /*---- Create ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Create_string") ==
		0)
	    {
		next = 42;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Create_percent") ==
		0)
	    {
		next = 43;
	    }

	  /*---- Append ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Append_string") ==
		0)
	    {
		next = 44;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Append_percent") ==
		0)
	    {
		next = 45;
	    }

	  /*---- Lock ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Lock_string") == 0)
	    {
		next = 46;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Lock_percent") ==
		0)
	    {
		next = 47;
	    }

	  /*---- Access ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Access_string") ==
		0)
	    {
		next = 48;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Access_percent") ==
		0)
	    {
		next = 49;
	    }

	  /*---- Stat ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Stat_string") == 0)
	    {
		next = 50;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Stat_percent") ==
		0)
	    {
		next = 51;
	    }

	  /*---- Neg_stat ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Neg_stat_string")
		== 0)
	    {
		next = 52;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Neg_stat_percent")
		== 0)
	    {
		next = 53;
	    }

	  /*---- Chmod ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Chmod_string") ==
		0)
	    {
		next = 54;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Chmod_percent") ==
		0)
	    {
		next = 55;
	    }

	  /*---- Readdir ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Readdir_string") ==
		0)
	    {
		next = 56;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Readdir_percent")
		== 0)
	    {
		next = 57;
	    }

	  /*---- Copyfile ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Copyfile_string")
		== 0)
	    {
		next = 58;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Copyfile_percent")
		== 0)
	    {
		next = 59;
	    }

	  /*---- Rename ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Rename_string") ==
		0)
	    {
		next = 60;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Rename_percent") ==
		0)
	    {
		next = 61;
	    }

	  /*---- Statfs ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Statfs_string") ==
		0)
	    {
		next = 62;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Statfs_percent") ==
		0)
	    {
		next = 63;
	    }

	  /*---- Pathconf ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Pathconf_string")
		== 0)
	    {
		next = 64;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Pathconf_percent")
		== 0)
	    {
		next = 65;
	    }

	  /*---- Trunc ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Trunc_string") ==
		0)
	    {
		next = 66;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Trunc_percent") ==
		0)
	    {
		next = 67;
	    }

	  /*---- Custom1 ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Custom1_string") ==
		0)
	    {
		next = 68;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Custom1_percent")
		== 0)
	    {
		next = 69;
	    }

	  /*---- Custom2 ----*/
	    if (strcmp ((char *) event.data.scalar.value, "Custom2_string") ==
		0)
	    {
		next = 70;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Custom2_percent")
		== 0)
	    {
		next = 71;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Read_xfer_elements") == 0)
	    {
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_0_xfer_min_size") == 0)
	    {
		next = 72;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_0_xfer_max_size") == 0)
	    {
		next = 73;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_0_xfer_percent") == 0)
	    {
		next = 74;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_1_xfer_min_size") == 0)
	    {
		next = 75;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_1_xfer_max_size") == 0)
	    {
		next = 76;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_1_xfer_percent") == 0)
	    {
		next = 77;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_2_xfer_min_size") == 0)
	    {
		next = 78;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_2_xfer_max_size") == 0)
	    {
		next = 79;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_2_xfer_percent") == 0)
	    {
		next = 80;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_3_xfer_min_size") == 0)
	    {
		next = 81;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_3_xfer_max_size") == 0)
	    {
		next = 82;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_3_xfer_percent") == 0)
	    {
		next = 83;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_4_xfer_min_size") == 0)
	    {
		next = 84;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_4_xfer_max_size") == 0)
	    {
		next = 85;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_4_xfer_percent") == 0)
	    {
		next = 86;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_5_xfer_min_size") == 0)
	    {
		next = 87;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_5_xfer_max_size") == 0)
	    {
		next = 88;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_5_xfer_percent") == 0)
	    {
		next = 89;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_6_xfer_min_size") == 0)
	    {
		next = 90;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_6_xfer_max_size") == 0)
	    {
		next = 91;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_6_xfer_percent") == 0)
	    {
		next = 92;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_7_xfer_min_size") == 0)
	    {
		next = 93;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_7_xfer_max_size") == 0)
	    {
		next = 94;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_7_xfer_percent") == 0)
	    {
		next = 95;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_8_xfer_min_size") == 0)
	    {
		next = 96;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_8_xfer_max_size") == 0)
	    {
		next = 97;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_8_xfer_percent") == 0)
	    {
		next = 98;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_9_xfer_min_size") == 0)
	    {
		next = 99;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_9_xfer_max_size") == 0)
	    {
		next = 100;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_9_xfer_percent") == 0)
	    {
		next = 101;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_10_xfer_min_size") == 0)
	    {
		next = 102;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_10_xfer_max_size") == 0)
	    {
		next = 103;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_10_xfer_percent") == 0)
	    {
		next = 104;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_11_xfer_min_size") == 0)
	    {
		next = 105;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_11_xfer_max_size") == 0)
	    {
		next = 106;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_11_xfer_percent") == 0)
	    {
		next = 107;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_12_xfer_min_size") == 0)
	    {
		next = 108;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_12_xfer_max_size") == 0)
	    {
		next = 109;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_12_xfer_percent") == 0)
	    {
		next = 110;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_13_xfer_min_size") == 0)
	    {
		next = 111;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_13_xfer_max_size") == 0)
	    {
		next = 112;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_13_xfer_percent") == 0)
	    {
		next = 113;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_14_xfer_min_size") == 0)
	    {
		next = 114;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_14_xfer_max_size") == 0)
	    {
		next = 115;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_14_xfer_percent") == 0)
	    {
		next = 116;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_15_xfer_min_size") == 0)
	    {
		next = 117;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_15_xfer_max_size") == 0)
	    {
		next = 118;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Read_elem_15_xfer_percent") == 0)
	    {
		next = 119;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_xfer_elements") == 0)
	    {
		next = 0;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_0_xfer_min_size") == 0)
	    {
		next = 120;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_0_xfer_max_size") == 0)
	    {
		next = 121;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_0_xfer_percent") == 0)
	    {
		next = 122;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_1_xfer_min_size") == 0)
	    {
		next = 123;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_1_xfer_max_size") == 0)
	    {
		next = 124;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_1_xfer_percent") == 0)
	    {
		next = 125;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_2_xfer_min_size") == 0)
	    {
		next = 126;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_2_xfer_max_size") == 0)
	    {
		next = 127;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_2_xfer_percent") == 0)
	    {
		next = 128;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_3_xfer_min_size") == 0)
	    {
		next = 129;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_3_xfer_max_size") == 0)
	    {
		next = 130;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_3_xfer_percent") == 0)
	    {
		next = 131;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_4_xfer_min_size") == 0)
	    {
		next = 132;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_4_xfer_max_size") == 0)
	    {
		next = 133;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_4_xfer_percent") == 0)
	    {
		next = 134;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_5_xfer_min_size") == 0)
	    {
		next = 135;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_5_xfer_max_size") == 0)
	    {
		next = 136;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_5_xfer_percent") == 0)
	    {
		next = 137;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_6_xfer_min_size") == 0)
	    {
		next = 138;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_6_xfer_max_size") == 0)
	    {
		next = 139;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_6_xfer_percent") == 0)
	    {
		next = 140;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_7_xfer_min_size") == 0)
	    {
		next = 141;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_7_xfer_max_size") == 0)
	    {
		next = 142;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_7_xfer_percent") == 0)
	    {
		next = 143;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_8_xfer_min_size") == 0)
	    {
		next = 144;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_8_xfer_max_size") == 0)
	    {
		next = 145;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_8_xfer_percent") == 0)
	    {
		next = 146;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_9_xfer_min_size") == 0)
	    {
		next = 147;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_9_xfer_max_size") == 0)
	    {
		next = 148;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_9_xfer_percent") == 0)
	    {
		next = 149;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_10_xfer_min_size") == 0)
	    {
		next = 150;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_10_xfer_max_size") == 0)
	    {
		next = 151;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_10_xfer_percent") == 0)
	    {
		next = 152;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_11_xfer_min_size") == 0)
	    {
		next = 153;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_11_xfer_max_size") == 0)
	    {
		next = 154;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_11_xfer_percent") == 0)
	    {
		next = 155;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_12_xfer_min_size") == 0)
	    {
		next = 156;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_12_xfer_max_size") == 0)
	    {
		next = 157;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_12_xfer_percent") == 0)
	    {
		next = 158;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_13_xfer_min_size") == 0)
	    {
		next = 159;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_13_xfer_max_size") == 0)
	    {
		next = 160;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_13_xfer_percent") == 0)
	    {
		next = 161;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_14_xfer_min_size") == 0)
	    {
		next = 162;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_14_xfer_max_size") == 0)
	    {
		next = 163;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_14_xfer_percent") == 0)
	    {
		next = 164;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_15_xfer_min_size") == 0)
	    {
		next = 165;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_15_xfer_max_size") == 0)
	    {
		next = 166;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Write_elem_15_xfer_percent") == 0)
	    {
		next = 167;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "File_size_elements") == 0)
	    {
		next = 0;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_0_min_size") == 0)
	    {
		next = 168;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_0_max_size") == 0)
	    {
		next = 169;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_0_percent") == 0)
	    {
		next = 170;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_1_min_size") == 0)
	    {
		next = 171;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_1_max_size") == 0)
	    {
		next = 172;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_1_percent") == 0)
	    {
		next = 173;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_2_min_size") == 0)
	    {
		next = 174;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_2_max_size") == 0)
	    {
		next = 175;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_2_percent") == 0)
	    {
		next = 176;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_3_min_size") == 0)
	    {
		next = 177;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_3_max_size") == 0)
	    {
		next = 178;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_3_percent") == 0)
	    {
		next = 179;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_4_min_size") == 0)
	    {
		next = 180;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_4_max_size") == 0)
	    {
		next = 181;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_4_percent") == 0)
	    {
		next = 182;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_5_min_size") == 0)
	    {
		next = 183;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_5_max_size") == 0)
	    {
		next = 184;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_5_percent") == 0)
	    {
		next = 185;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_6_min_size") == 0)
	    {
		next = 186;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_6_max_size") == 0)
	    {
		next = 187;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_6_percent") == 0)
	    {
		next = 188;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_7_min_size") == 0)
	    {
		next = 189;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_7_max_size") == 0)
	    {
		next = 190;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_7_percent") == 0)
	    {
		next = 191;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_8_min_size") == 0)
	    {
		next = 192;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_8_max_size") == 0)
	    {
		next = 193;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_8_percent") == 0)
	    {
		next = 194;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_9_min_size") == 0)
	    {
		next = 195;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_9_max_size") == 0)
	    {
		next = 196;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_9_percent") == 0)
	    {
		next = 197;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_10_min_size") == 0)
	    {
		next = 198;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_10_max_size") == 0)
	    {
		next = 199;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_10_percent") == 0)
	    {
		next = 200;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_11_min_size") == 0)
	    {
		next = 201;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_11_max_size") == 0)
	    {
		next = 202;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_11_percent") == 0)
	    {
		next = 203;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_12_min_size") == 0)
	    {
		next = 204;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_12_max_size") == 0)
	    {
		next = 205;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_12_percent") == 0)
	    {
		next = 206;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_13_min_size") == 0)
	    {
		next = 207;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_13_max_size") == 0)
	    {
		next = 208;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_13_percent") == 0)
	    {
		next = 209;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_14_min_size") == 0)
	    {
		next = 210;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_14_max_size") == 0)
	    {
		next = 211;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_14_percent") == 0)
	    {
		next = 212;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_15_min_size") == 0)
	    {
		next = 213;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_15_max_size") == 0)
	    {
		next = 214;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "File_size_elem_15_percent") == 0)
	    {
		next = 215;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Min_pre_name_length") == 0)
	    {
		next = 216;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Max_pre_name_length") == 0)
	    {
		next = 217;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Min_post_name_length") == 0)
	    {
		next = 218;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Max_post_name_length") == 0)
	    {
		next = 219;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_write_commit") == 0)
	    {
		next = 220;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_direct") ==
		0)
	    {
		next = 221;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Align") == 0)
	    {
		next = 222;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_osync") ==
		0)
	    {
		next = 223;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_geometric")
		== 0)
	    {
		next = 224;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_compress")
		== 0)
	    {
		next = 225;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_dedup") ==
		0)
	    {
		next = 226;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_dedup_within") == 0)
	    {
		next = 227;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_dedup_across") == 0)
	    {
		next = 228;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Dedupe_group_count") == 0)
	    {
		next = 229;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_per_spot")
		== 0)
	    {
		next = 230;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Min_acc_per_spot")
		== 0)
	    {
		next = 231;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Acc_mult_spot") ==
		0)
	    {
		next = 232;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Percent_affinity")
		== 0)
	    {
		next = 233;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Spot_shape") == 0)
	    {
		next = 234;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Dedup_Granule_size") == 0)
	    {
		next = 235;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Dedup_gran_rep_limit") == 0)
	    {
		next = 236;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Use_file_size_dist") == 0)
	    {
		next = 237;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Comp_Granule_size")
		== 0)
	    {
		next = 238;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Background") == 0)
	    {
		next = 239;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Sharemode") == 0)
	    {
		next = 240;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Uniform_file_size_dist") == 0)
	    {
		next = 241;
	    }
	    if (strcmp
		((char *) event.data.scalar.value, "Rand_dist_behavior") == 0)
	    {
		next = 242;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Cipher_behavior")
		== 0)
	    {
		next = 243;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Notification_percent") == 0)
	    {
		next = 244;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "LRU") == 0)
	    {
		next = 245;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Pattern_version")
		== 0)
	    {
		next = 246;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Init_rate_enable")
		== 0)
	    {
		next = 247;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Init_read_flag") ==
		0)
	    {
		next = 248;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Release_version")
		== 0)
	    {
		next = 249;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "FS_type") == 0)
	    {
		next = 250;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Proc_latency_threshold") == 0)
	    {
		next = 251;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Global_latency_threshold") == 0)
	    {
		next = 252;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Unlink2_no_recreate") == 0)
	    {
		next = 253;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Do_op_validate") ==
		0)
	    {
		next = 254;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Init_rate_speed")
		== 0)
	    {
		next = 255;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Dedicated_subdirectory") == 0)
	    {
		next = 256;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Business_metric")
		== 0)
	    {
		next = 257;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Skip_init") == 0)
	    {
		next = 258;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_fadvise_seq") == 0)
	    {
		next = 259;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_fadvise_rand") == 0)
	    {
		next = 260;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_fadvise_dont_need") == 0)
	    {
		next = 261;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_madvise_seq") == 0)
	    {
		next = 262;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_madvise_rand") == 0)
	    {
		next = 263;
	    }
	    if (strcmp
		((char *) event.data.scalar.value,
		 "Percent_madvise_dont_need") == 0)
	    {
		next = 264;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "YAML_magic") == 0)
	    {
		next = 265;
	    }
	    if (strcmp ((char *) event.data.scalar.value, "Platform_type") ==
		0)
	    {
		next = 266;
	    }
	}
	if (event.type != YAML_STREAM_END_EVENT)
	    yaml_event_delete (&event);
    }
    while (event.type != YAML_STREAM_END_EVENT);
    yaml_event_delete (&event);
    /* END new code */

    /* Cleanup */
    yaml_parser_delete (&parser);
    fclose (fh);
    cmdline.workload_count = component;
    cmdline.benchmark_count = benchmarks;

    total = 0;
    for (j = 0; j < component; j++)
    {
	/* Sanity check that the total percents from all op types adds up to 100% */
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
	total = totalr = totalw = totalf = 0;
	/* Sanity check that the total percents from all read, write, and file_size percents add up to 100% */
	for (k = 0; k < MAX_DIST_ELEM; k++)
	{
	    totalr += my_mix_table[j].read_dist[k].percent;
	    totalw += my_mix_table[j].write_dist[k].percent;
	    totalf += my_mix_table[j].file_size_dist[k].percent;
	}
	if (totalr != 100)
	{
	    printf
		("Bad read size distribution. Workload %s Read size distribution percent != 100 -> Found (%d)\n",
		 (char *) &my_mix_table[j].workload_name, totalr);
	    exit (3);
	}
	if (totalw != 100)
	{
	    printf
		("Bad write size distribution. Workload %s Write size distribution percent != 100 -> Found (%d)\n",
		 (char *) &my_mix_table[j].workload_name, totalw);
	    exit (3);
	}
	if ((my_mix_table[j].use_file_size_dist) && (totalf != 100))
	{
	    printf
		("Bad file size distribution. Workload %s File size distribution percent != 100 -> Found (%d)\n",
		 (char *) &my_mix_table[j].workload_name, totalf);
	    exit (3);
	}
    }
    return 0;
}
