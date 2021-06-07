#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "win32_sub.h"
/*
#include "../netmist/netmist_logger.h"
*/
extern void log_file (unsigned int level, const char *format, ...);

/* Define some global variables */

/******************************************************************
 *
 * execv(argv, argc)
 *
 */

#define USE_EXECV

#if defined(USE_EXECV)


static int e_parent = 1;

/**
 * @brief  win_exec is the windows implementation of execv().
 *
 * @param argv     : Argument list
 * @param argc     : Number of arguments in the list
 */
int
win_exec (char *argv[], int argc)
{
    int i, wr;
    size_t bo;
    char commandLine[4096];
    STARTUPINFO si;		/* Information about current execution */
    PROCESS_INFORMATION pi;	/* Information on newly created process */
    int value;

    if (e_parent)		/* Only the parent does this. */
    {
	/* Put the argv[] values in to the command line, separated
	 * by spaces, then append the child flag and child number.
	 */
	/* Start with the application command. */
	wr = snprintf(commandLine, sizeof(commandLine), "%s", argv[0]);
	if (wr < 0 || ((size_t) wr >= sizeof(commandLine))) {
		return -1;
	}
	bo = (size_t) wr;

	for (i = 1; i < argc; i++)	/* Then fill in the rest of the command line */
	{
	    wr = snprintf(&commandLine[bo], sizeof(commandLine) - bo, " %s", argv[i]);
	    if (wr < 0 || ((bo + (size_t) wr) >= sizeof(commandLine))) {
		return -1;
	    }
	    bo += (size_t) wr;
	}

	log_file (0x40, "Command line %s\n", (char *)commandLine);

	GetStartupInfo (&si);

	log_file (0x40, "Spawning proc \n");
	/* Create the new process */
	value = CreateProcessA (NULL, (LPSTR)commandLine,	/* Command line of application to launch */
				NULL,	/* Default process security attributes */
				NULL,	/* Default thread security attributes */
				TRUE,	/* Don't inherit handles from the parent */
				CREATE_NEW_PROCESS_GROUP,	/* Normal priority */
				NULL,	/* Use the same environment as the parent */
				NULL,	/* Launch in the current directory */
				(LPSTARTUPINFOA)&si,	/* Startup Information */
				&pi);	/* Process information stored upon return */
	if (value == 0)
	{

	    value = GetLastError ();
	    log_file (0x01, "Unable to Spawn proc. Error: %d\n", value);
	    /* perror("An error occurred in child creation.\n"); */
	    exit (1);
	}

    }				/* End duties */
    log_file (0x40, "win_exec returns\n");
    return (0);
}

#endif /* USE_EXECV */
