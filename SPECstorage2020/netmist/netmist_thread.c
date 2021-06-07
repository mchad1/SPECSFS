/*
 *  @copyright
 *  Copyright (c) 2003-2020 by Iozone.org
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


#include "./copyright.txt"
#include "./license.txt"
#include <stdio.h>
#include <time.h>
#include <signal.h>
#if !defined(WIN32)
#include <unistd.h>
#include <sys/time.h>
#endif
#include <string.h>
#include <errno.h>

#if !defined(WIN32)
#include <pthread.h>
#else
#include <WinSock2.h>
#include "../win32lib/win32_sub.h"
extern TCHAR *win32_strerror (int code);
#endif

#include "netmist_thread.h"
#include "netmist_logger.h"
#include "netmist_utils.h"

#if !defined(WIN32)
static pthread_t netmist_listen_thread;
static pthread_t netmist_keepalive_thread;
#else
static LPDWORD netmist_listen_thread;
static LPDWORD netmist_keepalive_thread;
#endif

/**
 * @brief  Start two threads. One sends keep-alive messages, while the other
 * listens for keep-alive messages.
 * @param  thread_keepalive : Pointer to thread function to send keep alives.
 * @param  thread_listen : Pointer to thread function that listens for keep-alives.
 */
int
netmist_start_threads (void *(*thread_keepalive) (void *),
		       void *(*thread_listen) (void *))
{
#if defined(WIN32)
    HANDLE rc;
    int err = 0;
    char *str = NULL;
#else
    int rc;
#endif

    if (thread_keepalive != NULL)
    {
#if !defined(WIN32)
	rc = pthread_create (&netmist_keepalive_thread,
			     NULL, thread_keepalive, (void *) NULL);

	if (rc != 0)
	{
	    return rc;
	}
#else
	rc = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) thread_keepalive,
			   NULL, 0, (LPDWORD) & netmist_keepalive_thread);
	if (rc == 0)
	{
	    err = GetLastError ();
	    str = win32_strerror (err);
	    log_file (LOG_ERROR, "CreateThread error: %s\n", str);
	    return -1;
	}
#endif
    }

    if (thread_listen != NULL)
    {
#if !defined(WIN32)
	rc = pthread_create (&netmist_listen_thread,
			     NULL, thread_listen, (void *) NULL);
	if (rc != 0)
	{
	    return rc;
	}
#else
	rc = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) thread_listen,
			   NULL, 0, (LPDWORD) & netmist_listen_thread);
	if (rc == 0)
	{
	    err = GetLastError ();
	    str = win32_strerror (err);
	    log_file (LOG_ERROR, "CreateThread error: %s\n", str);
	    return -1;
	}
#endif

    }

    return 0;
}
