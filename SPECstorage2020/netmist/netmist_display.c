/*
 * Copyright 2018 - 2020
 * Created: Aug 2018
 * Author: Don Capps
 *
 * This is a monitor that can be used to watch values in the
 * pdsm_log_file. It is a simple demonstration of the capabilty to 
 * monitor variables that are modified by a set of distributed clients, and 
 * transported into a unified space for monitoring. 
 *
 * In this case, Netmist is running on a bunch of clients. Netmist
 * detects if PDSM is present, and if so, it will deposit the 
 * current requested ops/sec and the currently achieved ops/sec
 * from every client into the PDSM LOG file. 
 */
#define PDSM_RO_ACTIVE

#include <curses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "dist/netmist.h"
#define GEN_SZ (4096)
#define INBUFSZ (GEN_SZ)
#define UPDATE_TIME 1
#define THISVERSION "        $Revision: 1.3 $"

/* Duplicated from netmist_if.h */
#define INIT_BEAT 1
#define WARM_BEAT 2
#define RUN_BEAT 3
#define PRE_WARM_BEAT 4
#define CLEAN_BEAT 5
#define VAL_BEAT 6
#define STOP_BEAT 7

#define INDENT 7
#define WINTOP 1
#define WINLEFT 1
#define WINWIDE 71
#define ENDOFW (INDENT+WINWIDE)
#define WINTALL 21
#define WINBOT (WINTOP+WINTALL)

char version[] = THISVERSION;
char my_pdsm_file[256];
int scan_pdsm_file (void);
char *get_timestamp (char *);
int getopt ();
void loop (void);
void re_calc (void);
struct pdsm_remote_stats *pdsm_stats;
time_t dummytime;
char time_string[30];
int number_of_private_segs;
int number;
int *bar_value;
int *bar_svalue;
char cret;
void usage (void);
char mystate[30];
WINDOW *cur_win, *wind1;
int pdsm_file_fd;

int
main (int argc, char **argv)
{

    int j, x, output;
    char *myseg;

    if (argc == 1)
    {
	usage ();
	exit (0);
    };
    while ((cret = getopt (argc, argv, "hvf:")) != EOF)
    {
	switch (cret)
	{
	case 'h':		/* Help screen */
	    usage ();
	    exit (0);
	    break;
	case 'v':		/* Help screen */
	    printf ("Version %s\n", version);
	    exit (0);
	    break;
	case 'f':		/* pdsm file */
	    strcpy (my_pdsm_file, optarg);

	};
    }

    /* 
     * Display the RO slots
     */
    number_of_private_segs = scan_pdsm_file ();
    close (pdsm_file_fd);
    number = number_of_private_segs;
    bar_value = malloc (sizeof (int) * number);
    bar_svalue = malloc (sizeof (double) * number);
    cur_win = initscr ();
    wind1 = newwin (WINBOT, ENDOFW, WINTOP, WINLEFT);
    wstandout (wind1);
    box (wind1, '|', '-');
    wstandend (wind1);
    wrefresh (wind1);

    pdsm_stats =
	(struct pdsm_remote_stats *)
	malloc (sizeof (struct pdsm_remote_stats));

    while (1)
    {
	number_of_private_segs = scan_pdsm_file ();
	for (j = 0; j < number_of_private_segs; j++)
	{
	    output = 0;
#if defined(WIN32)
	    _lseek (pdsm_file_fd, j * sizeof (struct pdsm_remote_stats),
		    SEEK_SET);
	    x = _read (pdsm_file_fd, pdsm_stats,
		       sizeof (struct pdsm_remote_stats));
#else
	    lseek (pdsm_file_fd, j * sizeof (struct pdsm_remote_stats),
		   SEEK_SET);
	    x = read (pdsm_file_fd, pdsm_stats,
		      sizeof (struct pdsm_remote_stats));
#endif
	    switch (pdsm_stats->cur_state)
	    {
	    case INIT_BEAT:
		strcpy (mystate, "INIT");
		break;
	    case WARM_BEAT:
		strcpy (mystate, "WARM");
		break;
	    case PRE_WARM_BEAT:
		strcpy (mystate, "PRE-WARM");
		break;
	    case RUN_BEAT:
		strcpy (mystate, "RUN ");
		break;
	    case CLEAN_BEAT:
		strcpy (mystate, "CLEANUP ");
		break;
	    };

	    bar_value[j] = pdsm_stats->achieved_op_rate;
	}
	loop ();
	close (pdsm_file_fd);
    }
}

void
usage ()
{
    printf ("netmist_display:\n");
    printf ("\t-h............  Help screen\n");
    printf ("\t-f............  pdsm_log_file_name\n");
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

int scalev = 1;
int step, maxy;

void
loop (void)
{
    int i, j, k, hh;
    char buf[20];
    char buf2[30];

    for (i = 1; i < 21; i++)
	mvwaddstr (wind1, i, INDENT, "|");
    for (j = INDENT; j < ENDOFW - 2; j++)
	mvwaddstr (wind1, WINBOT - 2, j, "-");
    step = setupbars (number);

    for (k = 0; k < number; k++)
    {
	bar_svalue[k] = calcy (bar_value[k]);
	snprintf (buf, 20, "%d", bar_value[k]);
	mvwaddstr (wind1, (WINBOT - 3) - bar_svalue[k],
		   INDENT + 2 + (k * step), buf);
    }
    snprintf (buf2, 30, "Ops");
    mvwaddstr (wind1, 10, 2, buf2);
    snprintf (buf2, 30, "per");
    mvwaddstr (wind1, 11, 2, buf2);
    snprintf (buf2, 30, "sec");
    mvwaddstr (wind1, 12, 2, buf2);
    snprintf (buf2, 30, "Netmist dynamic monitor");
    mvwaddstr (wind1, 2, 28, buf2);
    snprintf (buf2, 30, "State: %s Max ops/sec = %d", mystate, maxy);
    mvwaddstr (wind1, 3, 25, buf2);
    mvwaddstr (wind1, WINTALL - 1, 35, "- Procs -");
    wrefresh (wind1);
    sleep (UPDATE_TIME);
    werase (wind1);
    wstandout (wind1);
    box (wind1, '|', '-');
    wstandend (wind1);
    re_calc ();
}

int
setupbars (int number)
{
    int xstep;
    if (number <= 0)
	number = 1;
    xstep = (ENDOFW - INDENT - 2) / (number);
    return (xstep);
}

int
calcy (int value)
{
    int ret;
    if (scalev <= 0)
	scalev = 1;
    if (value > maxy)
	maxy = value;
    if (value / scalev > WINTALL - 3)
	newscale (value);
    ret = value / scalev;
    return (ret);
}

int
newscale (int value)
{
    int j;
    scalev = value / ((WINTALL / 3));
    if (scalev > 0)
    {
	for (j = 0; j < number; j++)
	    bar_svalue[j] = bar_value[j] / scalev;
    }
    wrefresh (wind1);
}

/* Re-calculate the graph scaling, for everything. This 
 * compensates for spurious high values that go away, but
 * left the scale jacked up.
 */
void
re_calc (void)
{
    int i;
    int max = 0;
    for (i = 0; i < number; i++)
    {
	if (bar_value[i] > max)
	    max = bar_value[i];
    }
    if (max > 0)
    {
	newscale (max);
	maxy = 0;
	calcy (max);
    }
}

int
scan_pdsm_file ()
{
    int x;
    int count = 0;
    struct pdsm_remote_stats remote_stats;
#if defined(WIN32)
    pdsm_file_fd = _open (my_pdsm_file, O_RDONLY, 0666);
#else
    pdsm_file_fd = open (my_pdsm_file, O_RDONLY, 0666);
#endif
    if (pdsm_file_fd < 0)
	exit (1);
/*
printf("size of seg %d\n",sizeof(struct pdsm_remote_stats));
exit(0);
*/
    while (1)
    {
#if defined(WIN32)
	x = _read (pdsm_file_fd, &remote_stats,
		   sizeof (struct pdsm_remote_stats));
#else
	x = read (pdsm_file_fd, &remote_stats,
		  sizeof (struct pdsm_remote_stats));
#endif
	if (x <= 0)
	{
	    return (count);
	}
	count++;
    }
}
