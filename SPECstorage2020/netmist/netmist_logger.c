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
 *  
 *      Author: Udayan Bapat, NetApp Inc.
 *
 */
#if defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#include "./copyright.txt"
#include "./license.txt"

#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#if !defined(WIN32)
#include<unistd.h>
#else
#include "../win32lib/win32_sub.h"
#endif
#include<time.h>
#include<errno.h>
#if !defined(WIN32)
#include<sys/time.h>
#endif

#include "netmist_logger.h"

#define LOCKING

#ifdef LOCKING
  #if defined(WIN32)
    HANDLE hMutex;
  #else
    #include <pthread.h>
    pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
  #endif
#endif

#ifdef LOCKING
void lock_logging(void);
void unlock_logging(void);
#endif

#define PRINT_LOG_STRING  0

#if defined(WIN32)
extern int w_gettimeofday (struct timeval *, struct w_timezone *);
#endif

static FILE *log_handle = NULL;
static unsigned int log_level = LOG_MIN;
static int print_timestamp_in_log = 1;
static int print_timestamp_in_stdout = 1;
static char time_string_ms[70];
static char role[20];

#if PRINT_LOG_STRING
static const char *log_string[] = {
    "ERROR",
    "FSM",
    "EXEC ",
    "CONFIG",
    "COMM",
    "FSM_VERBOSE",
    "EXEC_VERBOSE",
    "CONFIG_VERBOSE",
    "RESULTS",
    "RESULTS_VERBOSE",
    "COMM_VERBOSE",
    "DEBUG",
    "DATABASE"
};

/**
 * @brief  Return the log string.
 *
 * @param level : Level we are searching for.
 */
static const char *
get_log_string (unsigned int level)
{
    switch (level)
    {
    case LOG_ERROR:
	return log_string[0];
    case LOG_FSM:
	return log_string[1];
    case LOG_EXEC:
	return log_string[2];
    case LOG_CONFIG:
	return log_string[3];
    case LOG_COMM:
	return log_string[4];
    case LOG_FSM_VERBOSE:
	return log_string[5];
    case LOG_EXEC_VERBOSE:
	return log_string[6];
    case LOG_CONFIG_VERBOSE:
	return log_string[7];
    case LOG_RESULTS:
	return log_string[8];
    case LOG_RESULTS_VERBOSE:
	return log_string[9];
    case LOG_COMM_VERBOSE:
	return log_string[10];
    case LOG_DEBUG:
	return log_string[11];
    default:
	return "UNDEFINED";
    }
}
#endif

/**
 * @brief  Set my role in the logging.
 *
 * @param my_role : My role in the logging.
 */
void
set_log_role (const char *my_role)
{
    memset (role, 0, sizeof (role));
    strncpy (role, my_role, sizeof (role));
    role[sizeof (role) - 1] = '\0';
}

/**
 * @brief  Enable the timestamp in the logging.
 */
void
set_timestamp_in_log_file (void)
{
    print_timestamp_in_log = 1;
}

/**
 * @brief  Disable the timestamp in the logging.
 */
void
reset_timestamp_in_log_file (void)
{
    print_timestamp_in_log = 0;
}

/**
 * @brief  Enable logging to stdout.
 */
void
set_timestamp_in_log_stdout (void)
{
    print_timestamp_in_stdout = 1;
}

/**
 * @brief  Disable logging to stdout.
 */
void
reset_timestamp_in_log_stdout (void)
{
    print_timestamp_in_stdout = 0;
}

/**
 * @brief  Set the current log level.
 * @param level : Level of logging to set.
 */
void
set_log_level (unsigned int level)
{
    log_level = level;
}

/**
 * @brief  Open/create the log file by the mode specified.
 *
 * @param filename : Name of the log file.
 * @param mode : Mode string. e.g. "w+"
 */
FILE *
init_log (char *filename, char *mode)
{
    log_handle = fopen (filename, mode);

    return log_handle;
}


/**
 * @brief Put the date and time into a string, without the 
 *        new line.
 *
 * @param string  : Pointer to string space for the time stamp,
 * 		    must be at least 64 bytes long.
 */
/*
 * __doc__
 * __doc__  Function : char * get_timestamp_ms(char *string)
 * __doc__  Arguments: char *: Pointer to string space.
 * __doc__  Returns  : Pointer to the final string.
 * __doc__  Performs : Put the date and time.ms into a string
 * __doc__
 */
static char *
get_timestamp_ms (char *string)
{
    time_t time_in_sec;
    unsigned int time_in_ms;
    struct timeval tv;
    char time_buf[64], time_buf_ms[64];
    struct tm *nowtm;

    memset (time_buf, 0, sizeof (time_buf));
    memset (time_buf_ms, 0, sizeof (time_buf_ms));

#if defined(WIN32)
    w_gettimeofday (&tv, NULL);
#else
    gettimeofday (&tv, NULL);
#endif
    time_in_sec = tv.tv_sec;
    time_in_ms = (unsigned int) (tv.tv_usec / 1000);
    nowtm = localtime (&time_in_sec);
    strftime (time_buf, sizeof (time_buf), "%Y-%m-%d %H:%M:%S", nowtm);
    (void) snprintf (string, 64, "%s.%03u", time_buf, time_in_ms);
    return (string);
}

/**
 * @brief  Log this message to stdout. Uses va_list to handle variable list.
 *
 * @param level : Level of this log message
 * @param format : Format string for the log message.
 */
void
log_stdout (unsigned int level, const char *format, ...)
{
    va_list args;

    if (!(level & log_level))
    {
	return;
    }

#ifdef LOCKING
    lock_logging();
#endif
    if (print_timestamp_in_stdout)
    {
#if PRINT_LOG_STRING
	const char *log_s;
	log_s = get_log_string (level);
	printf ("%s: %-15s: %-15s: ", get_timestamp_ms (time_string_ms),
		role, log_s);
#else
	printf ("%s: %-15s: ", get_timestamp_ms (time_string_ms), role);
#endif
    }

    va_start (args, format);
    vprintf (format, args);
    va_end (args);
    fflush (stdout);
#ifdef LOCKING
    unlock_logging();
#endif
}

/**
 * @brief Log this message to the log file, if the level matches the log's set level.
 * Uses va_list for variable number of input args.
 *
 * @param level : What level is this message.
 * @param format : Format of the string.
 */
void
log_file (unsigned int level, const char *format, ...)
{
    va_list args;

    if (!(level & log_level))
    {
	return;
    }

    if (!log_handle)
    {
	return;
    }

#ifdef LOCKING
    lock_logging();
#endif
    if (print_timestamp_in_log)
    {
#if PRINT_LOG_STRING
	const char *log_s;
	log_s = get_log_string (level);
	fprintf (log_handle, "%s: %-15s: %-15s: ",
		 get_timestamp_ms (time_string_ms), role, log_s);
#else
	fprintf (log_handle, "%s: %-15s: ",
		 get_timestamp_ms (time_string_ms), role);
#endif
    }

    va_start (args, format);
    vfprintf (log_handle, format, args);
    va_end (args);
    fflush (log_handle);
#ifdef LOCKING
    unlock_logging();
#endif
}

/**
 * @brief Send log message to the log file if the level matches log level.
 * Uses va_list for variable input arguments.
 *
 * @param level : Current message level.
 * @param format : Format of the string that follows.
 */
void
log_all (unsigned int level, const char *format, ...)
{
    if (!(level & log_level))
    {
	return;
    }

#ifdef LOCKING
    lock_logging();
#endif
    (void) get_timestamp_ms (time_string_ms);
    time_string_ms[69] = '\0';

    if (print_timestamp_in_stdout)
    {
#if PRINT_LOG_STRING
	const char *log_s;
	log_s = get_log_string (level);
	printf ("%s: %-15s: %-15s: ", time_string_ms, role, log_s);
#else
	printf ("%s: %-15s: ", time_string_ms, role);
#endif
    }

    if (print_timestamp_in_log)
    {
	if (log_handle)
	{
#if PRINT_LOG_STRING
	    const char *log_s;
	    log_s = get_log_string (level);
	    fprintf (log_handle, "%s: %-15s: %-15s: ", time_string_ms, role,
		     log_s);
#else
	    fprintf (log_handle, "%s: %-15s: ", time_string_ms, role);
#endif
	}
    }

    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
    fflush (stdout);

    if (log_handle)
    {
	va_start (args, format);
	vfprintf (log_handle, format, args);
	va_end (args);
	fflush (log_handle);
    }
#ifdef LOCKING
    unlock_logging();
#endif
}

#ifdef LOCKING
void
lock_logging(void)
{
#if defined(WIN32)
    DWORD dwWaitResult;

    if(hMutex == NULL)
        hMutex = CreateMutex( NULL, FALSE, "MutexLogger");

    /* This should have been done in init_log() */
    if(hMutex == NULL)
	ExitProcess(0);

    dwWaitResult = WaitForSingleObject(hMutex, INFINITE);
    switch( dwWaitResult ) 
    {
	case WAIT_OBJECT_0:
	    break;
    }
#else
    pthread_mutex_lock(&print_mutex);
#endif
}
#endif

#ifdef LOCKING
void
unlock_logging(void)
{
#if defined(WIN32)
    if(!ReleaseMutex(hMutex))
	ExitProcess(0);
#else
    pthread_mutex_unlock(&print_mutex);
#endif
#endif
}
