/**
 *  @copyright
 *  Copyright (c) 2002-2021 by Iozone.org
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
 *  Author: Udayan Bapat, Cohesity Inc.
 *
 */
#include "./copyright.txt"
#include "./license.txt"


#ifndef __NETMIST_HASHTABLE_H__
#define __NETMIST_HASHTABLE_H__

#define HASH_FAILURE 0
#define HASH_SUCCESS 1

void *create_hash_table(int max_buckets);
int add_hash_entry (void *table, int entry_id, void *data);
void *lookup_hash_entry (void *table, int entry_id);

#endif