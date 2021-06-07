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

#include "netmist_hashtable.h"
#include "netmist_logger.h"
#include "netmist_utils.h"

struct bucket_ {
    int key;              // key
    void *value;          // value (user manages memory for this pointer)
    struct bucket_ *next; // linked-list 'next' pointer
};

typedef struct bucket_ BUCKET;

struct buckets_ {
    int total_entries; // book keeping to measure collisions
    BUCKET *head;      // linked-list 'head' pointer
};

typedef struct buckets_ HASH_BUCKETS;

struct hastable_ {
    int max_buckets;       // HASH key
    HASH_BUCKETS *buckets; // bucket array for lookups
};

typedef struct hastable_ HASHTABLE;

static BUCKET *create_bucket (int entry_id, void *data) {
    BUCKET *bucket = (BUCKET *) my_malloc(sizeof(BUCKET));

    if (!bucket) {
        log_file (LOG_ERROR, "Failed allocate bucket in %s\n", __FUNCTION__);
        return NULL;
    }

    bucket->key = entry_id;
    bucket->value = data;
    bucket->next = NULL;

    return bucket;
}

static void *find_bucket (BUCKET *head, int entry_id) {
    if (!head) {
        return NULL;
    }

    BUCKET *curr = head;

    while (curr) {
        if (curr->key == entry_id) {
            return curr;
        }

        curr = curr->next;
    }

    return NULL;
}

void *create_hash_table (int max_buckets) {
    HASHTABLE *table = (HASHTABLE *) my_malloc(sizeof(HASHTABLE));

    if (!table) {
        log_file (LOG_ERROR, "Failed allocate hashtable in %s\n", __FUNCTION__);
        return NULL;
    }

    table->max_buckets = max_buckets;

    table->buckets = (HASH_BUCKETS *) my_malloc(max_buckets * sizeof(HASH_BUCKETS));

    if (!table->buckets) {
        log_file (LOG_ERROR, "Failed allocate hashtable buckets in %s\n", __FUNCTION__);
        return NULL;
    }

    return (void *) table;
}

int add_hash_entry (void *table, int entry_id, void *data) {
    if (!table || !data) {
        log_file (LOG_ERROR, "Invalid entry addition in %s\n", __FUNCTION__);
    }

    HASHTABLE *t = (HASHTABLE *) table;

    int total_buckets = t->max_buckets;

    int bucket_id = (entry_id *4099 ) % total_buckets;

    BUCKET *bucket = create_bucket(entry_id, data);

    if (!bucket) {
        log_file (LOG_ERROR, "bucket creation failed in %s\n", __FUNCTION__);
        return HASH_FAILURE;
    }

    BUCKET *head = t->buckets[bucket_id].head;

    if (head == NULL) {
        t->buckets[bucket_id].total_entries++;
        t->buckets[bucket_id].head = bucket;
        return HASH_SUCCESS;
    }

    if (find_bucket(head, entry_id) != NULL) {
        log_file (LOG_ERROR, "can not add duplicate entry %d in %s\n", 
                    entry_id, __FUNCTION__);
        free (bucket);
        return HASH_FAILURE;
    }

    t->buckets[bucket_id].total_entries++;

    /* Add at the front */
    bucket->next = head;
    t->buckets[bucket_id].head = bucket;
    return HASH_SUCCESS;
}

void *lookup_hash_entry (void *table, int entry_id) {
   if (!table) {
        log_file (LOG_ERROR, "Invalid tanle pointer in %s\n", __FUNCTION__);
    }

    HASHTABLE *t = (HASHTABLE *) table;

    int total_buckets = t->max_buckets;
    int bucket_id = (entry_id * 4099) % total_buckets;

    BUCKET *head = t->buckets[bucket_id].head;

    if (head == NULL) {
        log_file (LOG_ERROR, "entry %d not found in %s\n", entry_id, __FUNCTION__);
        return NULL;
    }

    BUCKET *bucket = find_bucket(head, entry_id);

    if (bucket == NULL) {
        log_file (LOG_ERROR, "entry %d not found in %s\n", entry_id, __FUNCTION__);
        return NULL;
    }

    return bucket->value;
}
