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
#ifndef __NETMIST_RANDOM_H__
#define __NETMIST_RANDOM_H__

/* Default number of characters on each rotor */
#define NUM_POS 256
struct rotor
{
    unsigned char ch_array[NUM_POS];	/* The character */
    unsigned char ch_iarray[NUM_POS];	/* index of character */
};
/* global_rotor;*/


void name_srand (int);
int name_rand (void);
int name_gseed (void);
void name_ran (void);
unsigned int rotateR (unsigned int, int);
void _allocate_all_rotors (int);
void _dealloc_rotors (int);
unsigned char _rotor_forward (int, unsigned char);
unsigned char _rotor_backward (int, unsigned char);
void _sync_rotor (void);
void _alloc_rotors (int);
double _rotor_ran (void);
int _rotor_rand (void);
void _rotor_srand (int);
void _start_encryption (unsigned long long, int, int);
void _end_encryption (void);
unsigned char _crypt_char (unsigned char);
unsigned char _decrypt_char (unsigned char);
unsigned char *_crypt_string (unsigned char *, unsigned char *, int);
unsigned char *_decrypt_string (unsigned char *, unsigned char *, int);
void shuffle_two (struct rotor *);
unsigned long long rotor_genrand64_int64 (void);
void rotor_init_genrand64 (unsigned long long);
int netmist_rand (void);

#define MERSENNE

#ifdef MERSENNE

extern unsigned long long init[4];

extern unsigned long long length;

void init_genrand64 (unsigned long long);
void init_by_array64 (unsigned long long *, unsigned long long);
unsigned long long genrand64_int64 (void);
long long genrand64_int63 (void);
double genrand64_real1 (void);
double genrand64_real2 (void);
double genrand64_real3 (void);
#endif
#if defined(WIN32)
void n_spec_srand (int);
int n_spec_rand (void);
int n_spec_gseed (void);
void n_ran (void);
#endif

#endif
