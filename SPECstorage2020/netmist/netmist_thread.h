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

#ifndef __NETMIST_THREAD_H__
#define __NETMIST_THREAD_H__


extern int netmist_start_threads (void *(*keepalive) (void *),
				  void *(*listen) (void *));
const char *netmist_vfs_errstr (int);

#endif
