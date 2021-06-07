/*
 *  Copyright (c) 2008 by Standard Performance Evaluation
 *  Corporation
 *
 *  All rights reserved.
 *      Standard Performance Evaluation Corporation (SPEC)
 *      6585 Merchant Place, Suite 100
 *      Warrenton, VA 20187
 *
 *  This product contains benchmarks acquired from several sources who
 *  understand and agree with SPEC's goal of creating fair and objective
 *  benchmarks to measure computer performance.
 *
 *  This copyright notice is placed here only to protect SPEC in the
 *  event the source is misused in any manner that is contrary to the
 *  spirit, the goals and the intent of SPEC.
 *
 *  The source code is provided to the user or company under the license
 *  agreement for the SPEC Benchmark Suite for this product.
 */

/*
 * win32_sub.c
 *
 * Implementation of wrappers and unix functions not defined
 * in WIN32. All functions written specifically for SFS use.
 * Modification for use in other applications may be possible
 * for some functions.
 *
 * Define W32DBG compile flag to print extra information (debugging).
 */


/* Include win32_sub.h header file, which includes other needed headers */
#pragma warning(disable:4996)
#include "win32_sub.h"
#include <crtdbg.h>
#include <stdio.h>
#include <process.h>
#include <Dsrole.h>
#include <Dsgetdc.h>
#include <Lm.h>
void disable_parameter_checking (void);

/******************************************************************
 *
 * Begin implementation of functions
 *
 ******************************************************************/


/**
 * @brief WIN32 implementation of bzero. Put zeros into a memory location.
 *        Deprecated - use memset with zero as the set character.
 *
 * @param s : Pointer to memory space to zero
 * @param n : Number of characters to zero
 */
/******************************************************************
 *
 * __doc__
 * __doc__ Function    : bzero(void *s, int n)
 * __doc__ Arguments   : Void *s : Memory space to zero
 * __doc__             : int n: Number of characters to zero
 * __doc__ Returns     : void
 * __doc__ Description : WIN32 implementation of bzero. Put zeros into a memory location.
 * __doc__               Deprecated - use memset with zero as the set character.
 * __doc__
 */

extern void
bzero (void *s, int n)
{
    memset (s, 0, n);
}

/******************************************************************
 *
 * __doc__
 * __doc__ Function    : int close(int fd)
 * __doc__ Arguments   : int fd: File descriptor
 * __doc__ Returns     : int: Success or failure.
 * __doc__ Description : WIN32 implementation of close(fd), function to 
 * __doc__               close a file descriptor. Unix versions can close 
 * __doc__               both a socket or a standard file. WIN32 differentiates 
 * __doc__               between the two descriptors.
 * __doc__
 *
 * close(int fd)
 */

/**
 * @brief  WIN32 implementation of close(fd), function to close a file 
 *               descriptor. Unix versions can close both a socket or a 
 *               standard file. WIN32 differentiates between the two 
 *               descriptors.
 * 		 First call closesocket(...) on the file descriptor. If the 
 * 		 descriptor is not a socket, a WSAENOTSOCK error is returned. 
 * 		 If this error is received, then use _close(...) to close the 
 * 		 file descriptor.
 *
 * @param fd : File descriptor to close
 */

extern int
close (int fd)
{
    int result;

    result = closesocket (fd);
    if (result == SOCKET_ERROR)
    {
	if (WSAGetLastError () == WSAENOTSOCK)	/* fd is not a socket. */
	    result = _close (fd);	/* Close using an alternate method. */
	else
	    result = -1;	/* closesocket(fd) failed on a socket. */
    }
    else
    {
	result = 0;		/* closesocket(fd) succeeded. */
    }
    return result;
}

/**
 * @brief Truncate this file to the specified length.
 *
 * @param path   : Path to file
 * @param length : New length for the file.
 */
/*
 * __doc__
 * __doc__ Function    : int truncate(const char *path, off_t length)
 * __doc__ Arguments   : char *path: Path to file
 * __doc__             : off_t length: Truncate to this size.
 * __doc__ Returns     : Success or failure.
 * __doc__ Description : Truncate this file to the specified length.
 * __doc__
 */
int
truncate (const char *path, off_t length)
{
    HANDLE handle;
    BOOL ret;
    FILE_END_OF_FILE_INFO fileattr;

    if ((handle = CreateFileA (path,
			       GENERIC_READ,
			       FILE_SHARE_READ,
			       NULL,
			       OPEN_EXISTING,
			       FILE_FLAG_NO_BUFFERING,
			       NULL)) == INVALID_HANDLE_VALUE)
    {
	printf ("open existing file %s failed", path);
	return FALSE;
    }
    fileattr.EndOfFile.QuadPart = length;
    ret =
	SetFileInformationByHandle (handle, FileEndOfFileInfo, &fileattr,
				    sizeof (fileattr));
    CloseHandle (handle);
    if (ret)
	return 0;
    else
	return -1;
}

/**
 * @brief Return the pid of this process.
 */
/*
 * __doc__
 * __doc__ Function    : int getpid(void)
 * __doc__ Arguments   : void
 * __doc__ Returns     : Pid of process or error.
 * __doc__ Description : Return the pid of this process.
 * __doc__
 */
int
getpid ()
{
    return GetCurrentProcessId ();
}


/**
 * @brief Return the current time of day.
 * 
 * @param tv  : Pointer to timeval structure
 * @param tz  : Pointer to timezone structure
 */
/******************************************************************
 *
 * __doc__
 * __doc__ Function    : int gettimeofday(struct timeval *tv, struct timezone *tz)
 * __doc__ Arguments   : struct timeval *tv: Pointer to timeval struct
 * __doc__               struct timezone *tz: Pointer to timezone struct
 * __doc__ Returns     : Success or failure
 * __doc__ Description : Return the current time of day.
 * __doc__
 *
 * gettimeofday(struct timeval *tv, struct timezone *tz)
 *
 * WIN32 implementation of gettimeofday(...) written specifically for SFS.
 *
 * The following code is taken from the time_so_far1() function in
 * sfs_c_man.c. Code from Iozone with permission from author (Don Capps).
 * Use QueryPerformanceCounter and QueryPerformanceFrequency to determine
 * elapsed time.
 *
 * The code has been slightly adjusted for simplicity, and to handle the fact
 * that counter.LowPart / freq.LowPart sometimes results in a value of
 * 1 (second) or higher (more than 1000000 microseconds).  Need to put
 * the seconds into the tv_sec variable, and the remainder into tv_usec.
 */

extern int
gettimeofday (struct timeval *tv, struct timezone *tz)
{
    LARGE_INTEGER freq, counter;
    double bigcounter;
    double msec;
    int int_part;

    UNREFERENCED_PARAMETER (tz);

    if (tv == NULL)
    {
	errno = EINVAL;
	return -1;
    }
    else
    {
	QueryPerformanceFrequency (&freq);
	QueryPerformanceCounter (&counter);

	bigcounter = (double) counter.HighPart * (double) 0xffffffff;
	tv->tv_sec = (int) (bigcounter / (double) freq.LowPart);	/* Grab the initial seconds value */

	bigcounter = (double) counter.LowPart;
	msec = (double) (bigcounter / (double) freq.LowPart);	/* May contain an integer part */
	int_part = (int) msec;	/* Take the integer part (additional seconds) */
	msec -= int_part;	/* Remove the integer part from the msec value */

	tv->tv_sec += int_part;	/* Add any additional seconds */
	tv->tv_usec = (int) (msec * 1000000);	/* convert msec from fraction to integer */

	return 0;
    }
}

/**
 * @brief Sleep for specifed number of seconds
 *
 * @param seconds : Number of seconds to sleep.
 */
/******************************************************************
 *
 * __doc__
 * __doc__ Function    : sleep(uint_t seconds)
 * __doc__ Arguments   : Time in seconds to sleep
 * __doc__ Returns     : Return the number of seconds left after waking. 
 * __doc__               If we slept the full time, then return 0.
 * __doc__ Description : Sleep for specifed number of seconds
 * __doc__
 *
 * sleep(uint_t seconds)
 *
 * WIN32 implementation of sleep(...) written specifically for SFS.
 *
 * Use SleepEx() function, which implements a sleep call using milliseconds.
 * SleepEx also takes a second boolean parameter, which is an alertable
 * flag.  If the function is alertable, then an APC call can wake the
 * process from sleeping before the time has elapsed.  This will be used
 * for waking the sleep call due to a signal interrupt.
 *
 * Because we are using a separate thread for signal handling, a race
 * condition occurs in SFS. SFS code uses a signal to indicate to children
 * that it is time to begin execution, then the parent sleeps for a
 * specified number of seconds, until it receives a signal from the prime
 * client to end the test. However, due to the separate threads, sometimes
 * the parent's signal to the children is not caught by the parent signal
 * thread until after the parent main thread is sleeping, thereby causing
 * the parent main thread to wake almost immediately and end the execution.
 * Relinquishing the parent main thread time slice does not solve the problem,
 * since there is no way to guarantee that the parent's signal thread will
 * be the next to run. As more processes are used on a single machine, more
 * threads are running, reducing the chance that relinquishing the time slice
 * would help. The workaround is to put the parent thread to sleep for a short
 * time period (0.5 seconds) without possibility of waking. This in effect
 * forces any sleep call to last at least 1/2 second, but should not affect
 * total execution. See code comments below for specific details.
 */

extern uint_t
sleep (uint_t seconds)
{
    struct timeval now;
    int start, remaining;

    /* If input time < 0, sleep for 0 */
    /*
       if (seconds < 0)
       seconds = 0;
     */
    /* Mark when we start to sleep */
    gettimeofday (&now, 0);
    start = now.tv_sec;

    /* Make a very short call to Sleep (0.5 seconds) which
     * does not wake on an APC call. This allows the signal
     * thread time to get through the signal handling just
     * before the sleep call (so the signal sent just before
     * sleeping doesn't accidentally cause the sleep to wake
     * immediately.
     */
    Sleep (500);

    /* Make an initial SleepEx call with a very short time
     * to clear any APC calls that may have previously queued.
     */
    SleepEx (100, TRUE);

    /* Then make the required SleepEx call which will wake if
     * an APC call is made by a signal. Since the above two
     * calls can possibly take up to a second, sleep for one
     * less second here.
     */
    if (seconds < 1)
	seconds = 0;
    else
	seconds--;
    SleepEx (seconds * 1000, TRUE);

    /* Return the number of seconds left after waking. If we
     * slept the full time, then return 0.
     */
    gettimeofday (&now, 0);
    remaining = seconds - (now.tv_sec - start);
    if (remaining > 0)
	return remaining;
    else
	return 0;
}


/**
 * @brief Return zero, and don't fork. Not available.
 */
/*
 * __doc__
 * __doc__ Function    : int fork(void)
 * __doc__ Arguments   : void
 * __doc__ Returns     : returns zero, this doesn't exist in Windows
 * __doc__ Description : Return zero, and don't fork. Not available.
 * __doc__
 */
extern int
nofork (void)
{
    return 0;
}



/**
 * @brief WIN32 initialization of Windows Sockets and 
 *        signal handling routines and threads specifically 
 *        written for SFS. This code initializes information 
 *        specific to SFS, and cannot be directly used for 
 *        other application ports.
 */
/******************************************************************
 *
 *
 * __doc__
 * __doc__ Function    : void win32_init(void)
 * __doc__ Arguments   : void
 * __doc__ Returns     : void
 * __doc__ Description : WIN32 initialization of Windows Sockets and 
 * __doc__               signal handling routines and threads specifically 
 * __doc__               written for SFS. This code initializes information 
 * __doc__               specific to SFS, and cannot be directly used for 
 * __doc__               other application ports.
 * __doc__
 * 
 * win32_init()
 *
 * WIN32 initialization of Windows Sockets and signal handling
 * routines and threads specifically written for SFS. This code
 * initializes information specific to SFS, and cannot be directly
 * used for other application ports.
 *
 * This function must be called by the main function before any
 * network or socket calls are made, and before any signal
 * setup occurs. Easiest way is to place it as first command in main().
 *
 * First the sockets initialization occurs. Then establish the parent
 * process flag.  Initialize the number of children. Establish the
 * thread ID of the parent.  Establish the signal handling
 * events (except for SIGCHLD). Start the threads to watch for events.
 * All processes start a single thread, and the parent starts an additional
 * thread to watch for SIGCHLD (child exit signals).
 */

extern void
win32_init ()
{
    WSADATA wsaData;
    int iResult;

    /* Initialize Windows Sockets */
    iResult = WSAStartup (MAKEWORD (2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
	/* If Windows Sockets does not initialize correctly
	 * we have trouble.
	 */
	perror ("WSAStartup");
	exit (1);
    }
}

/**
 * @brief Win32 cleanup and get out of town.
 */
/*
 * __doc__
 * __doc__ Function    : void win32_close(void)
 * __doc__ Arguments   : void
 * __doc__ Returns     : void
 * __doc__ Description : Win32 cleanup and get out of town.
 * __doc__
 */
extern void
win32_close ()
{
    WSACleanup ();
}


/**
 * @brief Send a signal to a process.
 *
 * @param pid : Pid of process to receive signal
 * @param sig : Signal value to send. ( Only SIGINT works )
 */
/******************************************************************
 *
 * __doc__
 * __doc__ Function    : kill(int pid, int sig)
 * __doc__ Arguments   : int pid: Process to send signal
 * __doc__               int sig: Signal to send
 * __doc__ Returns     : returns zero
 * __doc__ Description : Send a signal to a process.
 * __doc__
 * 
 * kill(int pid, int sig)
 *
 * also kill itself, revise later...
 * and only send ctrl+c now
 */

extern int
kill (int pid, int sig)
{
	int ret;
	DWORD err;
	char *str;
    UNREFERENCED_PARAMETER (sig);
	/* Don't free the console if the caller is me. Last ref issues */
	if(pid != getpid())
	{
    	FreeConsole ();
		ret = (int)AttachConsole(pid);

    	if(ret == 0)
    	{
			err = GetLastError();
	    	str = (char *)win32_strerror (err);
	    	printf("Error in AttachConsole: %s\n",str);
	    	printf("Kill signal will NOT work.\n");
    	}
	}
    /*printf ("send ctr break to process %d \n", pid);*/
    GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, 0);

    return 0;
}


/**
 * @brief WIN32 implementation of write(...). Write to a file 
 *        descriptor.
 *
 * @param  fd    : File descriptor
 * @param  buf   : Pointer to buffer to write
 * @param  count : Number of bytes to write
 */
/******************************************************************
 *
 *
 * __doc__
 * __doc__ Function    : win32_write(int fd, const void *buf, size_t count)
 * __doc__ Arguments   : int fd: File descriptor
 * __doc__               void *buf : Buffer to write
 * __doc__               size_t count: Number of bytes to write
 * __doc__ Returns     : Number of bytes written
 * __doc__ Description : WIN32 implementation of write(...). Write to a file 
 * __doc__               descriptor.
 * __doc__
 * 
 * win32_write(int fd, const void *buf, size_t count)
 *
 * WIN32 implementation of write(...). Write to a file descriptor.
 *
 * Unix implementation of write(...) allows the file descriptor to be
 * a socket or a regular file. WIN32 differentiates between the
 * two types of descriptors.
 *
 * First call send(...) on the file descriptor. If the file descriptor
 * is not a socket, a WSAENOTSOCK error is returned. If this error
 * is received, then use _write(...) to send the information to the
 * file descriptor.
 */

extern int
win32_write (int fd, const void *buf, size_t count)
{
    int result;

    result = send (fd, (const char *) buf, (int) count, 0);
    if (result == SOCKET_ERROR)
    {
	if (WSAGetLastError () == WSAENOTSOCK)	/* fd is not a socket. */
	    result = _write (fd, buf, (unsigned int) count);	/* Write using an alternate method. */
	else
	    result = -1;	/* send() failed on a socket. */
    }
    return result;
}


/**
 * @brief Windows version of read().
 *
 * @param  fd    : File descriptor
 * @param  buf   : Buffer to read into 
 * @param  count : Number of bytes to read
 */
/******************************************************************
 *
 *
 * __doc__
 * __doc__ Function    : win32_read(int fd, void *buf, size_t count)
 * __doc__ Arguments   : int fd: File descriptor
 * __doc__             : void *buf: Buffer to read into 
 * __doc__             : size_t count: Number of bytes to read
 * __doc__ Returns     : 
 * __doc__ Description : WIN32 implementation of read(...). Write to a 
 * __doc__               file descriptor.
 * __doc__
 * 
 * win32_read(int fd, void *buf, size_t count)
 *
 * WIN32 implementation of read(...). Write to a file descriptor.
 *
 * Unix implementation of read(...) allows the file descriptor to be
 * a socket or a regular file. WIN32 differentiates between the
 * two types of descriptors.
 *
 * First call recv(...) on the file descriptor. If the file descriptor
 * is not a socket, a WSAENOTSOCK error is returned. If this error
 * is received, then use _read(...) to read the information from the
 * file descriptor.
 */

extern int
win32_read (int fd, void *buf, size_t count)
{
    int result;

    result = recv (fd, (char *) buf, (int) count, 0);
    if (result == SOCKET_ERROR)
    {
	if (WSAGetLastError () == WSAENOTSOCK)	/* fd is not a socket. */
	    result = _read (fd, buf, (unsigned int) count);	/* Read using an alternate method. */
	else
	    result = -1;	/* recv() failed on a socket. */
    }
    return result;
}



/**
 * @brief In Visual Studio 2005, we need to define a handler for the
 *        invalid parameter checking. We then call disable_parameter_checking()
 *        to activate the handler (which does nothing). This was necessary
 *        to keep _close(fd) from crashing when it gets used with an invalid
 *        fd, which happens under normal conditions.
 */
/*
 * __doc__
 * __doc__ Function    : void myInvalidParameterHandler(const wchar_t* expression,
 * __doc__                       const wchar_t* function,
 * __doc__                       const wchar_t* file,
 * __doc__                       unsigned int line,
 * __doc__                       uintptr_t pReserved)
 * __doc__ Arguments   : 
 * __doc__ Returns     : void
 * __doc__ Description : In Visual Studio 2005, we need to define a handler for the
 * __doc__               invalid parameter checking. We then call disable_parameter_checking()
 * __doc__               to activate the handler (which does nothing). This was necessary
 * __doc__               to keep _close(fd) from crashing when it gets used with an invalid
 * __doc__               fd, which happens under normal conditions.
 * __doc__
 */

#ifdef VS2005

void
myInvalidParameterHandler (const wchar_t * expression,
			   const wchar_t * function,
			   const wchar_t * file,
			   unsigned int line, uintptr_t pReserved)
{
    return;
}

/**
 * @brief Disable parameter checking.
 */
/*
 * __doc__
 * __doc__ Function    : void disable_parameter_checking(void) 
 * __doc__ Arguments   : void
 * __doc__ Returns     : void
 * __doc__ Description : Disable parameter checking.
 * __doc__
 */
void
disable_parameter_checking (void)
{

    _invalid_parameter_handler oldHandler, newHandler;
    newHandler = myInvalidParameterHandler;
    oldHandler = _set_invalid_parameter_handler (newHandler);

    // Disable the message box for assertions.
    _CrtSetReportMode (_CRT_ASSERT, 0);
}

#else

/**
 * @brief Disable parameter checking.
 */
void
disable_parameter_checking (void)
{
    return;
}

#endif

/* thread unsafe
   max length of windows path name is 260 chars

*/
/**
 * @brief Windows map to network path. Returns UNC path.
 *
 * @param server : Server name
 * @param localpath: Path to a file
 */
/*
 * __doc__
 * __doc__ Function    : char *map_to_network_path(const char *server, const char *localpath)
 * __doc__ Arguments   : char *server: Server name
 * __doc__             : char *localpath: Path to a file
 * __doc__ Returns     : char *name in a UNC formatted string
 * __doc__ Description : Map a file to network path.
 * __doc__
 */
char *
map_to_network_path (const char *server, const char *localpath)
{
    static char ret[MAX_PATH];
    char networkpath[MAX_PATH];

    if (server == NULL || localpath == NULL)
	return NULL;

    strncpy (networkpath, localpath, sizeof(networkpath));
    networkpath[1] = '$';

    sprintf (ret, "\\\\%s\\%s", server, networkpath);

    return ret;
}

/**
 * @brief Return path to parents directory
 *
 * @param path : path to examine
 */
/*
 * __doc__
 * __doc__ Function    : char* GetParentPath(const char* path)
 * __doc__ Arguments   : char *path to examine
 * __doc__ Returns     : char *path to parent directory
 * __doc__ Description : Return path to parents directory
 * __doc__
 */
char *
GetParentPath (const char *path)
{
    static char new_path[MAX_PATH];
    int len = 0;
    char *p = 0;

    if (path == NULL)
	return NULL;
    strcpy (new_path, path);
    len = (int) strlen (path);
    p = &new_path[len];
    while (len > 0)
    {
	if ((p[0] == '\\' || p[0] == '/') && p[1] != '\0')
	{
	    p[0] = '\0';
	    return new_path;
	}
	p--;
	len--;
    }

    return NULL;
}

/**
 * @brief Find the drive letter or UNC share root path from 
 *        file name return:
 *
 * @param  filename : Pointer to file name
 * @param  root     : Pointer to location to store the UNC path name.
 */
/*
 * __doc__
 * __doc__ Function    : BOOL GetRootPathName(__in const char* filename, __out char* root)
 * __doc__ Arguments   : char *filename
 * __doc__               char *root path
 * __doc__ Returns     : True or false
 * __doc__ Description : Find the drive letter or UNC share root path from 
 * __doc__               file name return:
 * __doc__
 * 
 * Find the drive letter or UNC share root path from file name
 * return:
 *		TRUE - if the given file name is recognized as either containing driver letter or UNC path
 *		FALSE - if the given file name is invalid
 */
BOOL
GetRootPathName (__in const char *filename, __out char *root)
{
    char filename_local[MAX_PATH], seps[] = "\\/";	/*seperator is '\' and/or '/' */
    char *p = NULL, *token = NULL;
    int count = 0;

    if (root != 0)		/* Visual Studio static code analysis */
	root[0] = 0;

    if (!filename || filename[0] == '\0' || filename[1] == '\0'
	|| (root == 0))
	return FALSE;
    if (filename[1] == ':')
    {
	/*treat this as drive letter */
	root[0] = filename[0];
	strcat (root, ":\\\0");
	return TRUE;
    }
    else
    {
	if (filename[0] == '\\' && filename[1] == '\\')
	{
	    /*
	     * This is UNC Path in \\netoworkname\share\... format,
	     * always assume \\netoworkname\share is the root of the share
	     */
	    /*make a copy, as the strtok will modify original file name */
	    strcpy (filename_local, filename);
	    strcat (root, "\\\\\0");
	    p = &filename_local[2];
	    token = strtok (p, seps);
	    while (token != NULL)
	    {
		strcat (root, token);
		strcat (root, "\\");	/*put ending "\" */
		if (++count == 2)
		    return TRUE;
		token = strtok (NULL, seps);
	    }
	}
    }
    return FALSE;
}

/**
 * @brief Returns the page size
 */
/*
 * __doc__
 * __doc__ Function    : int getpagesize(void)
 * __doc__ Arguments   : void
 * __doc__ Returns     : int: page size
 * __doc__ Description : Return page size
 * __doc__
 */
int
getpagesize (void)
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo (&systemInfo);
    return systemInfo.dwPageSize;
}

/**
 * @brief Return a string that describes the error
 */
/*
 * __doc__
 * __doc__ Function    : TCHAR* win32_strerror_old(int code)
 * __doc__ Arguments   : int code : error code input
 * __doc__ Returns     : String describing error
 * __doc__ Description : Return a string that describes the error
 * __doc__
 * 
 *	Works with code returned by GetLastError()
 *	Attention: this function is NOT thread safe!
 *
 * 	NOTE !!!! THIS DOESN'T WORK RELIABLY !!!!!!!!!!!!
 *	Just try calling with error code of 71 (decimal) and watch
 * 	it return an error string of "N" !!!
 * 	Others have complained about this.. just Google for this mess.
 */

TCHAR *
win32_strerror_old (int code)
{
    static TCHAR msgBuf[4000];
    bzero (msgBuf, 4000);
    /* Search system message table          */
    if (0 == FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,	/* message source, ignored              */
			    code,	/* code from GetLastError()             */
			    MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	/*Language   */
			    (LPTSTR) msgBuf,	/* message buffer                       */
			    4000,	/* buffer size                          */
			    NULL))	/* arguments, none here                 */
    {
	_tcscpy (msgBuf, _T ("Unable to retrieve system error message"));
    }
    return msgBuf;
}


/**
 * @brief Logon and impersonate given user.
 *
 * @param  username : Pointer to username
 * @param  password : Pointer to password
 * @param  domain   : Pointer to domain
 */
/*
 * __doc__
 * __doc__ Function    : DWORD LogonAndImpersonateUser( __in PCSTR username, 
 * __doc__                       __in PCSTR password, __in_opt PCSTR domain)
 * __doc__ Arguments   : username, password, domain
 * __doc__ Returns     : Success or failure
 * __doc__ Description : Logon and impersonate given user.
 * __doc__
 * 
 * Logon and impersonate given user.
 *
 * Arguments:
 *
 *  username    - name of the user
 *  password    - password used during logon
 *  domain      - user's domain (can be NULL)
 *
 * Return value: error code
 */
DWORD
LogonAndImpersonateUser (__in PCSTR username, __in PCSTR password,
			 __in_opt PCSTR domain)
{
    HANDLE userToken = (HANDLE) (INT_PTR) 0xAABBCCDD;
    DWORD ret = ERROR_SUCCESS;

    //check input parameters
    if (NULL == username || _T ('\0') == username[0])
    {
	ret = ERROR_INVALID_PARAMETER;
    }

    if (ERROR_SUCCESS == ret)
    {
	if (!LogonUserA (username,	//username
			 domain,	//domain
			 password,	//password
			 LOGON32_LOGON_INTERACTIVE,	//logon type
			 LOGON32_PROVIDER_DEFAULT,	//logon provider
			 &userToken))	//token (output)
	{
	    ret = GetLastError ();
	}
    }

    if (ERROR_SUCCESS == ret)
    {
	if (!ImpersonateLoggedOnUser (userToken))
	{
	    ret = GetLastError ();
	}
    }
    return ret;
}


/**
 * @brief Get the current domain name
 *
 * @param domainName : Pointer to buffer to place domainname
 * @param length     : Length of buffer
 */
/*
 * __doc__
 * __doc__ Function    : DWORD GetCurrentDomainName(PSTR domainName, PULONG length)
 * __doc__ Arguments   : domainName - [output] save the current domain name
 * __doc__               length - the length of the domain name including 
 * __doc__                 terminating "null"
 * __doc__ Returns     : success or error
 * __doc__ Description : Get the current domain name
 * __doc__
 * 
 *	Get the current domain name
 *
 *	Parameters:
 *
 *	domainName - [output] save the current domain name
 *	length - the length of the domain name including terminating "null"
 *
 *	Return:
 *
 *	ERROR_SUCCESS: if the domain name is found. Note that if length is 0, it indicates that the machine is not domain joined.
 *	ERROR_INSUFFICIENT_BUFFER - the given buffer "domainName" is too small for the name
 *	ERROR_INVALID_PARAMETER - other errors
 *
*/
extern DWORD
GetCurrentDomainName (__out PSTR domainName, __inout PULONG length)
{
    DWORD ret = ERROR_SUCCESS;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC dsInfo = NULL;
    DWORD domainLen = 0;
    size_t convertedCnt = 0;

    if (domainName != 0)
	domainName[0] = 0;

    if (NULL == length)
	return ERROR_INVALID_PARAMETER;

    if ((ret = DsRoleGetPrimaryDomainInformation (NULL,
						  DsRolePrimaryDomainInfoBasic,
						  (PBYTE *) & dsInfo)) ==
	ERROR_SUCCESS)
    {
	if (dsInfo->DomainNameFlat == NULL ||
	    dsInfo->DomainNameFlat[0] == L'\0')
	    /*non-domain joined computer */
	    domainLen = 0;
	else
	    domainLen = (DWORD) wcslen (dsInfo->DomainNameFlat);
    }

    if (ERROR_SUCCESS == ret)
    {
	if (*length < domainLen * sizeof (wchar_t) || domainName == NULL)
	{
	    *length = domainLen * sizeof (wchar_t) + 1;
	    ret = ERROR_INSUFFICIENT_BUFFER;
	}
    }

    if (domainName != 0)
	domainName[0] = 0;
    if (ERROR_SUCCESS == ret)
    {
	if (domainLen > 0)
	{
	    RtlZeroMemory (domainName, *length);
	    if (wcstombs_s
		(&convertedCnt, domainName, *length, dsInfo->DomainNameFlat,
		 domainLen))
		ret = ERROR_INVALID_PARAMETER;
	    else
	    {
		if (length != 0)
		    *length = (ULONG) convertedCnt;
	    }
	}
	else
	{
	    if (length != 0)
		*length = 0;
	    if (domainName != 0)
		domainName[0] = 0;
	}
    }
    else
    {
	if (length != 0)
	    *length = 0;
	if (domainName != 0)
	    domainName[0] = 0;
    }
    DsRoleFreeMemory ((PVOID) dsInfo);
    return ret;
}

/**
 * @brief Return the domain controllers name
 * 
 * @param domainName : Current domain name
 * @param dcName     : Pointer to buffer to store domain controller name
 * @param length     : Length of buffer
 */
/*
 * __doc__
 * __doc__ Function    : DWORD GetDCName(__in LPCSTR domainName, __out LPSTR dcName, __out PULONG length)
 * __doc__ Arguments   : domain name, domain name controller's name
 * __doc__ Returns     : success or error
 * __doc__ Description : Return the domain controllers name
 * __doc__
 */
extern DWORD
GetDCName (__in LPCSTR domainName, __out LPSTR dcName, __out PULONG length)
{
    DWORD ret = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFOA pDcInfo = NULL;
    DWORD len = 0;

    if (length == NULL || domainName == NULL)
    {
	ret = ERROR_INVALID_PARAMETER;
	return ret;
    }
    if (dcName != 0)		/* For VC static analysis */
	dcName[0] = 0;

    if (ERROR_SUCCESS == ret)
    {
	ret = DsGetDcNameA (NULL,	//local computer
			    domainName,
			    NULL, NULL, DS_PDC_REQUIRED, &pDcInfo);
    }

    if (ERROR_SUCCESS == ret)
    {
	if (pDcInfo->DomainControllerName)
	{
	    if (pDcInfo->DomainControllerName[0] == _T ('\\') &&
		pDcInfo->DomainControllerName[1] == _T ('\\'))
		len = (DWORD) strlen (pDcInfo->DomainControllerName) - 1;
	    else
		len = (DWORD) strlen (pDcInfo->DomainControllerName) + 1;
	    if ((length != 0) || dcName == NULL)
	    {
		/*not enough buffer */
		ret = ERROR_INSUFFICIENT_BUFFER;
		*length = len;
	    }
	    else
	    {
		if (len == 0)
		{
		    ret = ERROR_INVALID_DOMAINNAME;
		    if (length != 0)
			*length = 0;
		}
	    }
	}
	else
	{
	    if (length != 0)
		*length = 0;
	    ret = ERROR_INVALID_DOMAINNAME;
	}
    }
    if (ERROR_SUCCESS == ret)
    {
	RtlZeroMemory (dcName, *length);
	if (pDcInfo->DomainControllerName[0] == _T ('\\') &&
	    pDcInfo->DomainControllerName[1] == _T ('\\'))
	    strncpy (dcName, &pDcInfo->DomainControllerName[2], *length);
	else
	    strncpy (dcName, pDcInfo->DomainControllerName, *length);
	dcName[strlen (dcName)] = 0;
	if (length != 0)
	    *length = len;
    }
    else
    {
	if (length != 0)
	    *length = 0;
    }
    NetApiBufferFree (pDcInfo);
    return ret;
}


/**
 * @brief Add a user to a domain.
 * @param server    : Server name
 * @param username  : username
 * @param password  : User password
 */
/*
 * __doc__
 * __doc__ Function    : DWORD AddUser( __in PCWSTR server, __in PCWSTR username, __in_opt PCWSTR password) 
 * __doc__ Arguments   : server to add name to, username, and password
 * __doc__ Returns     : success or error
 * __doc__ Description : Add a user to a domain.
 * __doc__
 */
extern DWORD
AddUser (__in PCWSTR server, __in PCWSTR username, __in_opt PCWSTR password)
{

    USER_INFO_1 ui;		//level one user info structure
    NET_API_STATUS nStatus;	//NetUserAdd return value
    DWORD result = ERROR_SUCCESS;	//index of the first member of ui structure


    //check parameters
    if (NULL == username)
    {
	return ERROR_INVALID_PARAMETER;
    }
    if (password == 0)
    {
	return ERROR_INVALID_PARAMETER;
    }

    //prepare user information structure
    memset (&ui, 0, sizeof (USER_INFO_1));

    ui.usri1_name = _wcsdup (username);
    ui.usri1_password = _wcsdup (password);
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;	//required for LAN Manager 2.0
    //and Windows NT and later
    //add user
    nStatus = NetUserAdd (server, 1, (LPBYTE) & ui, NULL);

    switch (nStatus)
    {
    case NERR_Success:
	break;
    case ERROR_ACCESS_DENIED:
	result = ERROR_ACCESS_DENIED;
	break;
    case NERR_InvalidComputer:
	result = ERROR_INVALID_COMPUTERNAME;
	break;
    case NERR_NotPrimary:
	/*Not sure what to map */
	result = ERROR_DS_GENERIC_ERROR;
	break;
    case NERR_GroupExists:
	result = ERROR_GROUP_EXISTS;
	break;
    case NERR_UserExists:

	result = ERROR_USER_EXISTS;
	break;
    case NERR_PasswordTooShort:
	result = ERROR_PASSWORD_RESTRICTION;
	break;
    default:
	result = ERROR_DS_GENERIC_ERROR;
    }

    if (NULL != ui.usri1_name)
    {
	free (ui.usri1_name);
	ui.usri1_name = NULL;
    }

    if (NULL != ui.usri1_password)
    {
	free (ui.usri1_password);
	ui.usri1_password = NULL;
    }
    return result;
}

/**
 * @brief Return string that describes error
 *
 * @param code : Error code
 */
/*
 *
 * __doc__
 * __doc__ Function    : TCHAR* win32_strerror(int code)
 * __doc__ Arguments   : int code: Error code
 * __doc__ Returns     : String that describes error
 * __doc__ Description : Return string that describes error
 * __doc__
 * 
 * This might seem strange.  Why not call FormatMessage() ????
 * The answer is simple.  FormatMessage() doesn't work reliably !!!!
 * For some errors it just returns a garbage string of "N" or "A" as 
 * the error string.  It is unpredictable, unreliable, junk !!! 
 * So... we simply pull the error message mapping from the M$ website
 * and hardcode it. And Eureka !!! ... this works !!!
 */

TCHAR *
win32_strerror (int code)
{
    static char buf[4000];
    bzero (buf, 4000);
    switch (code)
    {

    case 0:
	strncpy (buf, "The operation completed successfully.", 4000);
	break;
    case 1:
	strncpy (buf, "Incorrect function.", 4000);
	break;
    case 10:
	strncpy (buf, "The environment is incorrect.", 4000);
	break;
    case 100:
	strncpy (buf, "Cannot create another system semaphore.", 4000);
	break;
    case 101:
	strncpy (buf, "The exclusive semaphore is owned by another process.",
		 4000);
	break;
    case 102:
	strncpy (buf, "The semaphore is set and cannot be closed.", 4000);
	break;
    case 103:
	strncpy (buf, "The semaphore cannot be set again.", 4000);
	break;
    case 104:
	strncpy (buf,
		 "Cannot request exclusive semaphores at interrupt time.",
		 4000);
	break;
    case 105:
	strncpy (buf, "The previous ownership of this semaphore has ended.",
		 4000);
	break;
    case 106:
	strncpy (buf, "Insert the diskette for drive X.", 4000);
	break;
    case 107:
	strncpy (buf,
		 "The program stopped because an alternate diskette was not inserted.",
		 4000);
	break;
    case 108:
	strncpy (buf, "The disk is in use or locked by another process.",
		 4000);
	break;
    case 109:
	strncpy (buf, "The pipe has been ended.", 4000);
	break;
    case 11:
	strncpy (buf,
		 "An attempt was made to load a program with an incorrect format.",
		 4000);
	break;
    case 110:
	strncpy (buf, "The system cannot open the device or file specified.",
		 4000);
	break;
    case 111:
	strncpy (buf, "The file name is too long.", 4000);
	break;
    case 112:
	strncpy (buf, "There is not enough space on the disk.", 4000);
	break;
    case 113:
	strncpy (buf, "No more internal file identifiers available.", 4000);
	break;
    case 114:
	strncpy (buf, "The target internal file identifier is incorrect.",
		 4000);
	break;
    case 117:
	strncpy (buf,
		 "The IOCTL call made by the application program is not correct.",
		 4000);
	break;
    case 118:
	strncpy (buf,
		 "The verify-on-write switch parameter value is not correct.",
		 4000);
	break;
    case 119:
	strncpy (buf, "The system does not support the command requested.",
		 4000);
	break;
    case 12:
	strncpy (buf, "The access code is invalid.", 4000);
	break;
    case 120:
	strncpy (buf, "This function is not supported on this system.", 4000);
	break;
    case 121:
	strncpy (buf, "The semaphore timeout period has expired.", 4000);
	break;
    case 122:
	strncpy (buf, "The data area passed to a system call is too small.",
		 4000);
	break;
    case 123:
	strncpy (buf,
		 "The filename, directory name, or volume label syntax is incorrect.",
		 4000);
	break;
    case 124:
	strncpy (buf, "The system call level is not correct.", 4000);
	break;
    case 125:
	strncpy (buf, "The disk has no volume label.", 4000);
	break;
    case 126:
	strncpy (buf, "The specified module could not be found.", 4000);
	break;
    case 127:
	strncpy (buf, "The specified procedure could not be found.", 4000);
	break;
    case 128:
	strncpy (buf, "There are no child processes to wait for.", 4000);
	break;
    case 129:
	strncpy (buf, "The evil application cannot be run in Win32 mode.",
		 4000);
	break;
    case 13:
	strncpy (buf, "The data is invalid.", 4000);
	break;
    case 130:
	strncpy (buf,
		 "Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.",
		 4000);
	break;
    case 131:
	strncpy (buf,
		 "An attempt was made to move the file pointer before the beginning of the file.",
		 4000);
	break;
    case 132:
	strncpy (buf,
		 "The file pointer cannot be set on the specified device or file.",
		 4000);
	break;
    case 133:
	strncpy (buf,
		 "A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.",
		 4000);
	break;
    case 134:
	strncpy (buf,
		 "An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.",
		 4000);
	break;
    case 135:
	strncpy (buf,
		 "An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.",
		 4000);
	break;
    case 136:
	strncpy (buf,
		 "The system tried to delete the JOIN of a drive that is not joined.",
		 4000);
	break;
    case 137:
	strncpy (buf,
		 "The system tried to delete the substitution of a drive that is not substituted.",
		 4000);
	break;
    case 138:
	strncpy (buf,
		 "The system tried to join a drive to a directory on a joined drive.",
		 4000);
	break;
    case 139:
	strncpy (buf,
		 "The system tried to substitute a drive to a directory on a substituted drive.",
		 4000);
	break;
    case 14:
	strncpy (buf,
		 "Not enough storage is available to complete this operation.",
		 4000);
	break;
    case 140:
	strncpy (buf,
		 "The system tried to join a drive to a directory on a substituted drive.",
		 4000);
	break;
    case 141:
	strncpy (buf,
		 "The system tried to SUBST a drive to a directory on a joined drive.",
		 4000);
	break;
    case 142:
	strncpy (buf,
		 "The system cannot perform a JOIN or SUBST at this time.",
		 4000);
	break;
    case 143:
	strncpy (buf,
		 "The system cannot join or substitute a drive to or for a directory on the same drive.",
		 4000);
	break;
    case 144:
	strncpy (buf,
		 "The directory is not a subdirectory of the root directory.",
		 4000);
	break;
    case 145:
	strncpy (buf, "The directory is not empty.", 4000);
	break;
    case 146:
	strncpy (buf, "The path specified is being used in a substitute.",
		 4000);
	break;
    case 147:
	strncpy (buf,
		 "Not enough resources are available to process this command.",
		 4000);
	break;
    case 148:
	strncpy (buf, "The path specified cannot be used at this time.",
		 4000);
	break;
    case 149:
	strncpy (buf,
		 "An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.",
		 4000);
	break;
    case 15:
	strncpy (buf, "The system cannot find the drive specified.", 4000);
	break;
    case 150:
	strncpy (buf,
		 "System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.",
		 4000);
	break;
    case 151:
	strncpy (buf,
		 "The number of specified semaphore events for DosMuxSemWait is not correct.",
		 4000);
	break;
    case 152:
	strncpy (buf,
		 "DosMuxSemWait did not execute; too many semaphores are already set.",
		 4000);
	break;
    case 153:
	strncpy (buf, "The DosMuxSemWait list is not correct.", 4000);
	break;
    case 154:
	strncpy (buf,
		 "The volume label you entered exceeds the label character limit of the target file system.",
		 4000);
	break;
    case 155:
	strncpy (buf, "Cannot create another thread.", 4000);
	break;
    case 156:
	strncpy (buf, "The recipient process has refused the signal.", 4000);
	break;
    case 157:
	strncpy (buf,
		 "The segment is already discarded and cannot be locked.",
		 4000);
	break;
    case 158:
	strncpy (buf, "The segment is already unlocked.", 4000);
	break;
    case 159:
	strncpy (buf, "The address for the thread ID is not correct.", 4000);
	break;
    case 16:
	strncpy (buf, "The directory cannot be removed.", 4000);
	break;
    case 160:
	strncpy (buf, "One or more arguments are not correct.", 4000);
	break;
    case 161:
	strncpy (buf, "The specified path is invalid.", 4000);
	break;
    case 162:
	strncpy (buf, "A signal is already pending.", 4000);
	break;
    case 164:
	strncpy (buf, "No more threads can be created in the system.", 4000);
	break;
    case 167:
	strncpy (buf, "Unable to lock a region of a file.", 4000);
	break;
    case 17:
	strncpy (buf,
		 "The system cannot move the file to a different disk drive.",
		 4000);
	break;
    case 170:
	strncpy (buf, "The requested resource is in use.", 4000);
	break;
    case 173:
	strncpy (buf,
		 "A lock request was not outstanding for the supplied cancel region.",
		 4000);
	break;
    case 174:
	strncpy (buf,
		 "The file system does not support atomic changes to the lock type.",
		 4000);
	break;
    case 18:
	strncpy (buf, "There are no more files.", 4000);
	break;
    case 180:
	strncpy (buf,
		 "The system detected a segment number that was not correct.",
		 4000);
	break;
    case 182:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 183:
	strncpy (buf, "Cannot create a file when that file already exists.",
		 4000);
	break;
    case 186:
	strncpy (buf, "The flag passed is not correct.", 4000);
	break;
    case 187:
	strncpy (buf, "The specified system semaphore name was not found.",
		 4000);
	break;
    case 188:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 189:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 19:
	strncpy (buf, "The media is write protected.", 4000);
	break;
    case 190:
	strncpy (buf, "The operating system cannot run this", 4000);
	break;
    case 191:
	strncpy (buf, "Cannot run this in Win32 mode.", 4000);
	break;
    case 192:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 193:
	strncpy (buf, "is not a valid Win32 application this", 4000);
	break;
    case 194:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 195:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 196:
	strncpy (buf,
		 "The operating system cannot run this application program.",
		 4000);
	break;
    case 197:
	strncpy (buf,
		 "The operating system is not presently configured to run this application.",
		 4000);
	break;
    case 198:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 199:
	strncpy (buf,
		 "The operating system cannot run this application program.",
		 4000);
	break;
    case 2:
	strncpy (buf, "The system cannot find the file specified.", 4000);
	break;
    case 20:
	strncpy (buf, "The system cannot find the device specified.", 4000);
	break;
    case 200:
	strncpy (buf,
		 "The code segment cannot be greater than or equal to 64K.",
		 4000);
	break;
    case 201:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 202:
	strncpy (buf, "The operating system cannot run this.", 4000);
	break;
    case 203:
	strncpy (buf,
		 "The system could not find the environment option that was entered.",
		 4000);
	break;
    case 205:
	strncpy (buf,
		 "No process in the command subtree has a signal handler.",
		 4000);
	break;
    case 206:
	strncpy (buf, "The filename or extension is too long.", 4000);
	break;
    case 207:
	strncpy (buf, "The ring 2 stack is in use.", 4000);
	break;
    case 208:
	strncpy (buf,
		 "The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.",
		 4000);
	break;
    case 209:
	strncpy (buf, "The signal being posted is not correct.", 4000);
	break;
    case 21:
	strncpy (buf, "The device is not ready.", 4000);
	break;
    case 210:
	strncpy (buf, "The signal handler cannot be set.", 4000);
	break;
    case 212:
	strncpy (buf, "The segment is locked and cannot be reallocated.",
		 4000);
	break;
    case 214:
	strncpy (buf,
		 "Too many dynamic-link modules are attached to this program or dynamic-link module.",
		 4000);
	break;
    case 215:
	strncpy (buf, "Cannot nest calls to LoadModule.", 4000);
	break;
    case 216:
	strncpy (buf,
		 "The version of this is not compatible with the version you're running. Check your computer's system information to see whether you need a x86 ; or x64 ; version of the program, and then contact the software publisher.",
		 4000);
	break;
    case 217:
	strncpy (buf, "The image file is signed, unable to modify.", 4000);
	break;
    case 218:
	strncpy (buf, "The image file is strong signed, unable to modify.",
		 4000);
	break;
    case 22:
	strncpy (buf, "The device does not recognize the command.", 4000);
	break;
    case 220:
	strncpy (buf,
		 "This file is checked out or locked for editing by another user.",
		 4000);
	break;
    case 221:
	strncpy (buf, "The file must be checked out before saving changes.",
		 4000);
	break;
    case 222:
	strncpy (buf,
		 "The file type being saved or retrieved has been blocked.",
		 4000);
	break;
    case 223:
	strncpy (buf,
		 "The file size exceeds the limit allowed and cannot be saved.",
		 4000);
	break;
    case 224:
	strncpy (buf,
		 "Access Denied. Before opening files in this location, you must first add the web site to your trusted sites list, browse to the web site, and select the option to login automatically.",
		 4000);
	break;
    case 225:
	strncpy (buf,
		 "Operation did not complete successfully because the file contains a virus.",
		 4000);
	break;
    case 226:
	strncpy (buf,
		 "This file contains a virus and cannot be opened. Due to the nature of this virus, the file has been removed from this location.",
		 4000);
	break;
    case 229:
	strncpy (buf, "The pipe is local.", 4000);
	break;
    case 23:
	strncpy (buf, "Data error ;.", 4000);
	break;
    case 230:
	strncpy (buf, "The pipe state is invalid.", 4000);
	break;
    case 231:
	strncpy (buf, "All pipe instances are busy.", 4000);
	break;
    case 232:
	strncpy (buf, "The pipe is being closed.", 4000);
	break;
    case 233:
	strncpy (buf, "No process is on the other end of the pipe.", 4000);
	break;
    case 234:
	strncpy (buf, "More data is available.", 4000);
	break;
    case 24:
	strncpy (buf,
		 "The program issued a command but the command length is incorrect.",
		 4000);
	break;
    case 240:
	strncpy (buf, "The session was canceled.", 4000);
	break;
    case 25:
	strncpy (buf,
		 "The drive cannot locate a specific area or track on the disk.",
		 4000);
	break;
    case 254:
	strncpy (buf, "The specified extended attribute name was invalid.",
		 4000);
	break;
    case 255:
	strncpy (buf, "The extended attributes are inconsistent.", 4000);
	break;
    case 258:
	strncpy (buf, "The wait operation timed out.", 4000);
	break;
    case 259:
	strncpy (buf, "No more data is available.", 4000);
	break;
    case 26:
	strncpy (buf, "The specified disk or diskette cannot be accessed.",
		 4000);
	break;
    case 266:
	strncpy (buf, "The copy functions cannot be used.", 4000);
	break;
    case 267:
	strncpy (buf, "The directory name is invalid.", 4000);
	break;
    case 27:
	strncpy (buf, "The drive cannot find the sector requested.", 4000);
	break;
    case 275:
	strncpy (buf, "The extended attributes did not fit in the buffer.",
		 4000);
	break;
    case 276:
	strncpy (buf,
		 "The extended attribute file on the mounted file system is corrupt.",
		 4000);
	break;
    case 277:
	strncpy (buf, "The extended attribute table file is full.", 4000);
	break;
    case 278:
	strncpy (buf, "The specified extended attribute handle is invalid.",
		 4000);
	break;
    case 28:
	strncpy (buf, "The printer is out of paper.", 4000);
	break;
    case 282:
	strncpy (buf,
		 "The mounted file system does not support extended attributes.",
		 4000);
	break;
    case 288:
	strncpy (buf, "Attempt to release mutex not owned by caller.", 4000);
	break;
    case 29:
	strncpy (buf, "The system cannot write to the specified device.",
		 4000);
	break;
    case 298:
	strncpy (buf, "Too many posts were made to a semaphore.", 4000);
	break;
    case 299:
	strncpy (buf,
		 "Only part of a ReadProcessMemory or WriteProcessMemory request was completed.",
		 4000);
	break;
    case 3:
	strncpy (buf, "The system cannot find the path specified.", 4000);
	break;
    case 30:
	strncpy (buf, "The system cannot read from the specified device.",
		 4000);
	break;
    case 300:
	strncpy (buf, "The oplock request is denied.", 4000);
	break;
    case 301:
	strncpy (buf,
		 "An invalid oplock acknowledgment was received by the system.",
		 4000);
	break;
    case 302:
	strncpy (buf,
		 "The volume is too fragmented to complete this operation.",
		 4000);
	break;
    case 303:
	strncpy (buf,
		 "The file cannot be opened because it is in the process of being deleted.",
		 4000);
	break;
    case 304:
	strncpy (buf,
		 "Short name settings may not be changed on this volume due to the global registry setting.",
		 4000);
	break;
    case 305:
	strncpy (buf, "Short names are not enabled on this volume.", 4000);
	break;
    case 306:
	strncpy (buf,
		 "The security stream for the given volume is in an inconsistent state. Please run CHKDSK on the volume.",
		 4000);
	break;
    case 307:
	strncpy (buf,
		 "A requested file lock operation cannot be processed due to an invalid byte range.",
		 4000);
	break;
    case 308:
	strncpy (buf,
		 "The subsystem needed to support the image type is not present.",
		 4000);
	break;
    case 309:
	strncpy (buf,
		 "The specified file already has a notification GUID associated with it.",
		 4000);
	break;
    case 31:
	strncpy (buf, "A device attached to the system is not functioning.",
		 4000);
	break;
    case 317:
	strncpy (buf,
		 "The system cannot find message text for message number this in the message file for that.",
		 4000);
	break;
    case 318:
	strncpy (buf, "The scope specified was not found.", 4000);
	break;
    case 32:
	strncpy (buf,
		 "The process cannot access the file because it is being used by another process.",
		 4000);
	break;
    case 33:
	strncpy (buf,
		 "The process cannot access the file because another process has locked a portion of the file.",
		 4000);
	break;
    case 34:
	strncpy (buf,
		 "The wrong diskette is in the drive. Insert ham sandwich ; into drive A:",
		 4000);
	break;
    case 350:
	strncpy (buf, "No action was taken as a system reboot is required.",
		 4000);
	break;
    case 351:
	strncpy (buf, "The shutdown operation failed.", 4000);
	break;
    case 352:
	strncpy (buf, "The restart operation failed.", 4000);
	break;
    case 353:
	strncpy (buf, "The maximum number of sessions has been reached.",
		 4000);
	break;
    case 36:
	strncpy (buf, "Too many files opened for sharing.", 4000);
	break;
    case 38:
	strncpy (buf, "Reached the end of the file.", 4000);
	break;
    case 39:
	strncpy (buf, "The disk is full.", 4000);
	break;
    case 4:
	strncpy (buf, "The system cannot open the file.", 4000);
	break;
    case 400:
	strncpy (buf, "The thread is already in background processing mode.",
		 4000);
	break;
    case 401:
	strncpy (buf, "The thread is not in background processing mode.",
		 4000);
	break;
    case 402:
	strncpy (buf, "The process is already in background processing mode.",
		 4000);
	break;
    case 403:
	strncpy (buf, "The process is not in background processing mode.",
		 4000);
	break;
    case 487:
	strncpy (buf, "Attempt to access invalid address.", 4000);
	break;
    case 5:
	strncpy (buf, "Access is denied.", 4000);
	break;
    case 50:
	strncpy (buf, "The request is not supported.", 4000);
	break;
    case 51:
	strncpy (buf,
		 "Windows cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If Windows still cannot find the network path, contact your network administrator.",
		 4000);
	break;
    case 52:
	strncpy (buf,
		 "You were not connected because a duplicate name exists on the network. If joining a domain, go to System in Control Panel to change the computer name and try again. If joining a workgroup, choose another workgroup name.",
		 4000);
	break;
    case 53:
	strncpy (buf, "The network path was not found.", 4000);
	break;
    case 54:
	strncpy (buf, "The network is busy.", 4000);
	break;
    case 55:
	strncpy (buf,
		 "The specified network resource or device is no longer available.",
		 4000);
	break;
    case 56:
	strncpy (buf, "The network BIOS command limit has been reached.",
		 4000);
	break;
    case 57:
	strncpy (buf, "A network adapter hardware error occurred.", 4000);
	break;
    case 58:
	strncpy (buf,
		 "The specified server cannot perform the requested operation.",
		 4000);
	break;
    case 59:
	strncpy (buf, "An unexpected network error occurred.", 4000);
	break;
    case 6:
	strncpy (buf, "The handle is invalid.", 4000);
	break;
    case 60:
	strncpy (buf, "The remote adapter is not compatible.", 4000);
	break;
    case 61:
	strncpy (buf, "The printer queue is full.", 4000);
	break;
    case 62:
	strncpy (buf,
		 "Space to store the file waiting to be printed is not available on the server.",
		 4000);
	break;
    case 63:
	strncpy (buf, "Your file waiting to be printed was deleted.", 4000);
	break;
    case 64:
	strncpy (buf, "The specified network name is no longer available.",
		 4000);
	break;
    case 65:
	strncpy (buf, "Network access is denied.", 4000);
	break;
    case 66:
	strncpy (buf, "The network resource type is not correct.", 4000);
	break;
    case 67:
	strncpy (buf, "The network name cannot be found.", 4000);
	break;
    case 68:
	strncpy (buf,
		 "The name limit for the local computer network adapter card was exceeded.",
		 4000);
	break;
    case 69:
	strncpy (buf, "The network BIOS session limit was exceeded.", 4000);
	break;
    case 7:
	strncpy (buf, "The storage control blocks were destroyed.", 4000);
	break;
    case 70:
	strncpy (buf,
		 "The remote server has been paused or is in the process of being started.",
		 4000);
	break;
    case 71:
	strncpy (buf,
		 "No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.",
		 4000);
	break;
    case 72:
	strncpy (buf, "The specified printer or disk device has been paused.",
		 4000);
	break;
    case 8:
	strncpy (buf,
		 "Not enough storage is available to process this command.",
		 4000);
	break;
    case 80:
	strncpy (buf, "The file exists.", 4000);
	break;
    case 82:
	strncpy (buf, "The directory or file cannot be created.", 4000);
	break;
    case 83:
	strncpy (buf, "Fail on INT 24.", 4000);
	break;
    case 84:
	strncpy (buf, "Storage to process this request is not available.",
		 4000);
	break;
    case 85:
	strncpy (buf, "The local device name is already in use.", 4000);
	break;
    case 86:
	strncpy (buf, "The specified network password is not correct.", 4000);
	break;
    case 87:
	strncpy (buf, "The parameter is incorrect.", 4000);
	break;
    case 88:
	strncpy (buf, "A write fault occurred on the network.", 4000);
	break;
    case 89:
	strncpy (buf, "The system cannot start another process at this time.",
		 4000);
	break;
    case 9:
	strncpy (buf, "The storage control block address is invalid.", 4000);
	break;
    case 1450:
	strncpy (buf,
		 "Insufficient system resources exist to complete the requested service.",
		 4000);
	break;
    case 1451:
	strncpy (buf,
		 "Insufficient system resources exist to complete the requested service.",
		 4000);
	break;
    case 1452:
	strncpy (buf,
		 "Insufficient system resources exist to complete the requested service.",
		 4000);
	break;
    default:
	strncpy (buf,
		 "Unknown error value. See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms681381(v=vs.85).aspx",
		 4000);
    }
    return ((TCHAR *) buf);
}

TCHAR *
win32_WSAstrerror (int code)
{
    static char buf[4000];
    bzero (buf, 4000);
    switch (code)
    {
	case 6: 
	     strncpy(buf,"Specified event object handle is invalid.",4000);
	     break;
	case 8: 
	     strncpy(buf,"Insufficient memory available.",4000);
	     break;
	case 87:
	     strncpy(buf,"One or more parameters are invalid.",4000);
	     break;
	case 995:
	     strncpy(buf,"Overlapped operation aborted.",4000);
	     break;
	case 996:
	     strncpy(buf,"Overlapped I/O event object not in signaled state.",4000);
	     break;
	case 997:
	     strncpy(buf,"Overlapped operations will complete later.",4000);
	     break;
	case 10004:
	     strncpy(buf,"Interrupted function call.",4000);
	     break;
	case 10009:
	     strncpy(buf,"File handle is not valid.",4000);
	     break;
	case 10013:
	     strncpy(buf,"Permission denied.",4000);
	     break;
	case 10014:
	     strncpy(buf,"Bad address.",4000);
	     break;
	case 10022:
	     strncpy(buf,"Invalid argument.",4000);
	     break;
	case 10024:
	     strncpy(buf,"Too many open files.",4000);
	     break;
	case 10035:
	     strncpy(buf,"Resource temporarily unavailable.",4000);
	     break;
	case 10036:
	     strncpy(buf,"Operation now in progress.",4000);
	     break;
	case 10037:
	     strncpy(buf,"Operation already in progress.",4000);
	     break;
	case 10038:
	     strncpy(buf,"Socket operation on nonsocket.",4000);
	     break;
	case 10039:
	     strncpy(buf,"Destination address required.",4000);
	     break;
	case 10040:
	     strncpy(buf,"Message too long.",4000);
	     break;
	case 10041:
	     strncpy(buf,"Protocol wrong type for socket.",4000);
	     break;
	case 10042:
	     strncpy(buf,"Bad protocol option.",4000);
	     break;
	case 10043:
	     strncpy(buf,"Protocol not supported.",4000);
	     break;
	case 10044:
	     strncpy(buf,"Socket type not supported.",4000);
	     break;
	case 10045:
	     strncpy(buf,"Operation not supported.",4000);
	     break;
	case 10046:
	     strncpy(buf,"Protocol family not supported.",4000);
	     break;
	case 10047:
	     strncpy(buf,"Address family not supported by protocol family.",4000);
	     break;
	case 10048:
	     strncpy(buf,"Address already in use.",4000);
	     break;
	case 10049:
	     strncpy(buf,"Cannot assign requested address.",4000);
	     break;
	case 10050:
	     strncpy(buf,"Network is down.",4000);
	     break;
	case 10051:
	     strncpy(buf,"Network is unreachable.",4000);
	     break;
	case 10052:
	     strncpy(buf,"Network dropped connection on reset.",4000);
	     break;
	case 10053:
	     strncpy(buf,"Software caused connection abort.",4000);
	     break;
	case 10054:
	     strncpy(buf,"Connection reset by peer.",4000);
	     break;
	case 10055:
	     strncpy(buf,"No buffer space available.",4000);
	     break;
	case 10056:
	     strncpy(buf,"Socket is already connected.",4000);
	     break;
	case 10057:
	     strncpy(buf,"Socket is not connected.",4000);
	     break;
	case 10058:
	     strncpy(buf,"Cannot send after socket shutdown.",4000);
	     break;
	case 10059:
	     strncpy(buf,"Too many references.",4000);
	     break;
	case 10060:
	     strncpy(buf,"Connection timed out.",4000);
	     break;
	case 10061:
	     strncpy(buf,"Connection refused.",4000);
	     break;
	case 10062:
	     strncpy(buf,"Cannot translate name.",4000);
	     break;
	case 10063:
	     strncpy(buf,"Name too long.",4000);
	     break;
	case 10064:
	     strncpy(buf,"Host is down.",4000);
	     break;
	case 10065:
	     strncpy(buf,"No route to host.",4000);
	     break;
	case 10066:
	     strncpy(buf,"Directory not empty.",4000);
	     break;
	case 10067:
	     strncpy(buf,"Too many processes.",4000);
	     break;
	case 10068:
	     strncpy(buf,"User quota exceeded.",4000);
	     break;
	case 10069:
	     strncpy(buf,"Disk quota exceeded.",4000);
	     break;
	case 10070:
	     strncpy(buf,"Stale file handle reference.",4000);
	     break;
	case 10071:
	     strncpy(buf,"The item is not available locally.",4000);
	     break;
	case 10091:
	     strncpy(buf,"Network subsystem is unavailable",4000);
	     break;
	case 10092:
	     strncpy(buf,"Winsock.dll version out of range.",4000);
	     break;
	case 10093:
	     strncpy(buf,"Successful WSAStartup not yet performed.",4000);
	     break;
	case 10101:
	     strncpy(buf,"Graceful shutdown in progress.",4000);
	     break;
	case 10103:
	     strncpy(buf,"Call has been canceled.",4000);
	     break;
	case 10104:
	     strncpy(buf,"Procedure call table is invalid.",4000);
	     break;
	case 10105:
	     strncpy(buf,"Service provider is invalid.",4000);
	     break;
	case 10106:
	     strncpy(buf,"Service provider failed to initialize.",4000);
	     break;
	case 10107:
	     strncpy(buf,"System call failure.",4000);
	     break;
	case 10108:
	     strncpy(buf,"Service not found.",4000);
	     break;
	case 10109:
	     strncpy(buf,"Class type not found.",4000);
	     break;
	case 10110:
	     strncpy(buf,"No more results.",4000);
	     break;
	case 10111:
	     strncpy(buf,"Call was canceled.",4000);
	     break;
	case 10112:
	     strncpy(buf,"Database query was refused.",4000);
	     break;
	case 11001:
	     strncpy(buf,"Host not found.",4000);
	     break;
	case 11002:
	     strncpy(buf,"Nonauthoritative host not found.",4000);
	     break;
	case 11003:
	     strncpy(buf,"This is a nonrecoverable error.",4000);
	     break;
	case 11004:
	     strncpy(buf,"Valid name, no data record of requested type.",4000);
	     break;
	case 11005:
	     strncpy(buf,"QoS receivers.",4000);
	     break;
	case 11006:
	     strncpy(buf,"QoS senders.",4000);
	     break;
	case 11007:
	     strncpy(buf,"No QoS senders.",4000);
	     break;
	case 11008:
	     strncpy(buf,"QoS no receivers.",4000);
	     break;
	case 11009:
	     strncpy(buf,"QoS request confirmed.",4000);
	     break;
	case 11010:
	     strncpy(buf,"QoS admission error.",4000);
	     break;
	case 11011:
	     strncpy(buf,"QoS policy failure.",4000);
	     break;
	case 11012:
	     strncpy(buf,"QoS bad style.",4000);
	     break;
	case 11013:
	     strncpy(buf,"QoS bad object.",4000);
	     break;
	case 11014:
	     strncpy(buf,"QoS traffic control error.",4000);
	     break;
	case 11015:
	     strncpy(buf,"QoS generic error.",4000);
	     break;
	case 11016:
	     strncpy(buf,"QoS service type error.",4000);
	     break;
	case 11017:
	     strncpy(buf,"QoS flowspec error.",4000);
	     break;
	case 11018:
	     strncpy(buf,"Invalid QoS provider buffer.",4000);
	     break;
	case 11019:
	     strncpy(buf,"Invalid QoS filter style.",4000);
	     break;
	case 11020:
	     strncpy(buf,"Invalid QoS filter type.",4000);
	     break;
	case 11021:
	     strncpy(buf,"Incorrect QoS filter count.",4000);
	     break;
	case 11022:
	     strncpy(buf,"Invalid QoS object length.",4000);
	     break;
	case 11023:
	     strncpy(buf,"Incorrect QoS flow count.",4000);
	     break;
	case 11024:
	     strncpy(buf,"Unrecognized QoS object.",4000);
	     break;
	case 11025:
	     strncpy(buf,"Invalid QoS policy object.",4000);
	     break;
	case 11026:
	     strncpy(buf,"Invalid QoS flow descriptor.",4000);
	     break;
	case 11027:
	     strncpy(buf,"Invalid QoS provider-specific flowspec.",4000);
	     break;
	case 11028:
	     strncpy(buf,"Invalid QoS provider-specific filterspec.",4000);
	     break;
	case 11029:
	     strncpy(buf,"Invalid QoS shape discard mode object.",4000);
	     break;
	case 11030:
	     strncpy(buf,"Invalid QoS shaping rate object.",4000);
	     break;
	case 11031:
	     strncpy(buf,"Reserved policy QoS element type.",4000);
	     break;
        default:
	     strncpy (buf,"Unknown error value. See: http://msdn.microsoft.com",4000);
    }
    return ((TCHAR *) buf);
}
