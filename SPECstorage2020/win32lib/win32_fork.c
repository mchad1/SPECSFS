#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include "win32_sub.h"
extern void strip_extra_slashes(char *, char*);

/* Define some global variables */

static HANDLE hChildMutex;	/* Mutex for the child handle array */
static HANDLE *children;	/* Array of handles to the children */

/******************************************************************
 *
 * fork(void)
 *
 * WIN32 implementation of fork(void) specifically written for SFS.
 *
 * Use CreateProcess to begin another process using the current command
 * line.  CreateProcess is more like fork() and exec() rather than fork(),
 * since child execution begins at the beginning of the file, not at 
 * the point of the fork.
 *
 * Attach a 'hidden flag' to the command line which will indicate that
 * the new process is a child, as well as its child number.  After
 * initialization of the parameter space (which should result in the
 * same values as the parent) the fork command is reached.  This 
 * implementation will check for the child flag, and not perform another
 * CreateProcess if it is found.  The child is assigned its child number,
 * and continues execution.
 * 
 * The added command line information is in the form "-- child [num]".
 * The "--" is parsed by getopt as the end of the parameter list, and
 * num contains the child number according to the parent.  This information
 * is not indicated or used by the sfs application except for in this 
 * function.
 *
 * The fork() implementation uses the child_num variable from sfs_c_man.c.
 * The declaration of this variable must be given as an external 
 * definition in that file, to be seen by this function.
 *
 * A macro must also be defined for WIN32 implementations to limit the
 * use of the fork() function as defined here.  The reliance on the 
 * child_num variable results in an error when that variable is not
 * declared elsewhere (i.e. in sfs_prime or sfs_syncd, which also use
 * these subroutines).
 */

#define USE_FORK


#if defined(USE_FORK)		/* Some SFS executables do not use fork(), and the
				 * the required globals child_num and Child_num. Avoid
				 * an 'undefined variable' error.
				 */
int parent = 1;			/* Reset by main on re-entry */
			        /* When the child proc starts with a -8 option, it *must* set parent = 0 */
static int numChildren;
int child_num;			/* MUST BE SET BY THE CALLER */
				/* Child number to be passed to the new process */

int Child_num;			/* Will be set by child of fork ( option -8 )*/
				/* Child number received by this process */
/**
 * @brief  fork is the windows implementation of fork().
 *	   Note: Unlike Unix, this fork starts the child as a new proc
 *	   at main() handing it the same argument list as the parent, except
 *	   one additional arugment is passed to the new process. -8 is 
 *	   passed to tell the child it is the child.
 */

extern int
fork (void)
{
    int i, wr;
    size_t bo;
    char commandLine[4096];
    STARTUPINFO si;		/* Information about current execution */
    PROCESS_INFORMATION pi;	/* Information on newly created process */
    char temp[255];

    DWORD mutexWait;

    memset(temp,0,255);
    if (parent)			/* Only the parent does this. */
    {
	/* Put the argv[] values in to the command line, separated
	 * by spaces, then append the child flag and child number.
	 */

	/* Just in case there are extra slashes that might confuse CreateProcess */
	strip_extra_slashes(temp,__argv[0]);

	/* Start with the application command. */
	wr = snprintf(commandLine, sizeof(commandLine), "%s", temp);
	if (wr < 0 || ((size_t) wr >= sizeof(commandLine))) {
		return -1;
	}
	bo = (size_t) wr;

	if (bo + 3 >= sizeof(commandLine)) {
		return -1;
	}
	snprintf(&commandLine[bo], sizeof(commandLine) - bo, " -8");
	bo += 3;

	/* Then fill in the rest of the command line */
	for (i = 1; i < __argc; i++)	
	{
	    wr = snprintf(&commandLine[bo], sizeof(commandLine) - bo, " %s", __argv[i]);
	    if (wr < 0 || ((bo + (size_t) wr) >= sizeof(commandLine))) {
		return -1;
	    }
	    bo += (size_t) wr;
	}

	/* Get startup information for new process */

	GetStartupInfo (&si);

	/* Create the new process */
	if (!CreateProcessA (NULL, (LPSTR)commandLine,	/* Command line of application to launch */
			     NULL,	/* Default process security attributes */
			     NULL,	/* Default thread security attributes */
			     TRUE,	/* Don't inherit handles from the parent */
			     CREATE_NEW_PROCESS_GROUP,	/* Normal priority */
			     NULL,	/* Use the same environment as the parent */
			     NULL,	/* Launch in the current directory */
			     (LPSTARTUPINFOA)&si,	/* Startup Information */
			     &pi)	/* Process information stored upon return */
	    )
	{
	    /* perror("An error occurred in child creation.\n"); */
	    exit (1);
	}
	/* E-Gad, don't be freeing stack space !! */
	/* free (commandLine); */

	/* Add the child to the array of children handles. */
	/* Grab the mutex for the child handles. */
	mutexWait = WaitForSingleObject (hChildMutex, INFINITE);
	if (child_num == 0)	/* first element in array */
	    children = (HANDLE *) malloc (sizeof (HANDLE) * (child_num + 1));
	else			/* adding additional elements to array */
	    children =
		(HANDLE *) realloc (children,
				    sizeof (HANDLE) * (child_num + 1));
	children[child_num] = pi.hProcess;
	numChildren++;

	/* Release the child handle mutex */
	ReleaseMutex (hChildMutex);

	/* Return the pid of the child to the calling process */
	return pi.dwProcessId;

    }				/* End parental duties */
    else			/* Establish the child's environment */
    {
	/* Assign the child number */
	child_num = Child_num;

	/* A child should receive 0 from the fork() function */
	return 0;
    }				/* end child setup */
}

#endif /* USE_FORK */
