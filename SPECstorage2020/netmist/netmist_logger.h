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
 *  Author: Udayan Bapat, NetApp Inc.
 *
 */
#include "./copyright.txt"
#include "./license.txt"

#ifndef __NETMIST_LOGGER_H__
#define __NETMIST_LOGGER_H__

#define LOG_ERROR               0x0001
#define LOG_FSM                 0x0002
#define LOG_EXEC                0x0004
#define LOG_CONFIG              0x0008
#define LOG_COMM                0x0010
#define LOG_FSM_VERBOSE         0x0020
#define LOG_EXEC_VERBOSE        0x0040
#define LOG_CONFIG_VERBOSE      0x0080
#define LOG_RESULTS             0x0100
#define LOG_RESULTS_VERBOSE     0x0200
#define LOG_COMM_VERBOSE        0x0400
#define LOG_DEBUG               0x0800
#define LOG_DATABASE            0x1000
#define LOG_MSG                 0x2000
#define LOG_MEMORY              0x4000
#define LOG_THREAD              0x8000

#define LOG_DEFAULT   (LOG_RESULTS | LOG_COMM | LOG_CONFIG | LOG_EXEC | LOG_FSM | LOG_ERROR | LOG_FSM_VERBOSE | LOG_DATABASE)

#define LOG_ALL  (LOG_EXEC_VERBOSE | LOG_FSM_VERBOSE | LOG_COMM | LOG_CONFIG | LOG_EXEC | LOG_FSM | LOG_ERROR | LOG_COMM_VERBOSE | LOG_RESULTS)

#define LOG_MIN  (LOG_RESULTS | LOG_EXEC | LOG_ERROR)

#define LOG_FSM_ONLY  (LOG_FSM_VERBOSE | LOG_FSM | LOG_ERROR)

#define LOG_FSM_MSG_ONLY  (LOG_FSM_VERBOSE | LOG_FSM | LOG_MSG | LOG_ERROR)

extern void set_log_role (const char *my_role);
extern void set_timestamp_in_log_file (void);
extern void reset_timestamp_in_log_file (void);
extern void set_timestamp_in_log_stdout (void);
extern void reset_timestamp_in_log_stdout (void);
extern void set_log_level (unsigned int level);
extern FILE *init_log (char *filename, char *mode);
extern void log_stdout (unsigned int level, const char *format, ...);
extern void log_file (unsigned int level, const char *format, ...);
extern void log_all (unsigned int level, const char *format, ...);

#endif
