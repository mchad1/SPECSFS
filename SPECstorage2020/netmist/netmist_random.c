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
 */
#include "./copyright.txt"
#include "./license.txt"

#if defined(WIN32)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4133)
#pragma warning(disable:4244)
#pragma warning(disable:4102)
#pragma warning(disable:4018)
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>

#include "netmist_random.h"
#include "netmist_logger.h"

extern void *my_malloc (size_t);
void print_md5 (char *, unsigned *);
struct rotor global_rotor;

long cur_seed;

#if defined(WIN32)
/*
 * Make a local version of rand() and srand() that work on
 * Windows platforms.  We need this becasue the rand() implementation
 * in Visual Studio 'C++' only returns 16 bit values.
 *
 * We looked at rand_s()... DON'T go there. It is expensive (CPU)
 * and ignores srand().  The ability to control the seed IS needed
 * by the rest of Netmist.
 */

int n_seedi = 2231;

/**
 * @brief Seed the random number generator.
 *
 * @param seed : Set the seed
 */
/*
 * __doc__
 * __doc__  Function : void n_spec_srand(int seed)
 * __doc__  Arguments: int seed
 * __doc__  Returns  : void
 * __doc__  Performs : Seed the random number generator.
 * __doc__             
 */
void
n_spec_srand (int seed)
{
    n_seedi = seed;
}

/**
 * @brief Returns a random number.
 */
/*
 * __doc__
 * __doc__  Function : int n_spec_rand(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int: random number
 * __doc__  Performs : Returns a random number.
 * __doc__
 */
int
n_spec_rand (void)
{
    (void) n_ran ();
    return (n_seedi);
}

/**
 * @brief Get the current seed value 
 */
/*
 * __doc__
 * __doc__  Function : int n_spec_gseed(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int: Current seed.
 * __doc__  Performs : Get the current seed value 
 * __doc__             
 */
int
n_spec_gseed (void)
{
    return (n_seedi);
}

#define _A_MULTIPLIER  16807L
#define _M_MODULUS     2147483647L	/* (2**31)-1 */
#define _Q_QUOTIENT    127773L	/* 2147483647 / 16807 */
#define _R_REMAINDER   2836L	/* 2147483647 % 16807 */

/**
 * @brief Generate a random number  (internal)
 */
/* 
 * __doc__
 * __doc__  Function : void n_ran(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Generate a random number  (internal)
 * __doc__             
 */

void
n_ran (void)
{
    int lo;
    int hi;
    int test;

    hi = n_seedi / _Q_QUOTIENT;
    lo = n_seedi % _Q_QUOTIENT;
    test = _A_MULTIPLIER * lo - _R_REMAINDER * hi;
    if (test > 0)
    {
	n_seedi = test;
    }
    else
    {
	n_seedi = test + _M_MODULUS;
    }
    /* return((float) n_seedi / _M_MODULUS); */
    return;
}
#endif

int name_seedi = 2231;

/**
 * @brief Seed the random number generator.
 *
 * @param seed : Set the seed
 */
/*
 * __doc__
 * __doc__  Function : void name_srand(int seed)
 * __doc__  Arguments: int seed
 * __doc__  Returns  : void
 * __doc__  Performs : Seed the random number generator.
 * __doc__             
 */
void
name_srand (int seed)
{
    name_seedi = seed;
}

/**
 * @brief Returns a random number.
 */
/*
 * __doc__
 * __doc__  Function : int name_rand(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : random number
 * __doc__  Performs : Returns a random number.
 * __doc__             
 */
int
name_rand (void)
{
    (void) name_ran ();
    return (name_seedi);
}

/**
 * @brief Return current seed value.
 */
/*
 * __doc__
 * __doc__  Function : int name_gseed(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int: Current seed
 * __doc__  Performs : Return current seed value.
 * __doc__             
 */
int
name_gseed (void)
{
    return (name_seedi);
}

#define _N_A_MULTIPLIER  16807L
#define _N_M_MODULUS     2147483647L	/* (2**31)-1 */
#define _N_Q_QUOTIENT    127773L	/* 2147483647 / 16807 */
#define _N_R_REMAINDER   2836L	/* 2147483647 % 16807 */

/**
 * @brief Generate a random number  (internal)
 */
/* 
 * __doc__
 * __doc__  Function : void name_ran(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Generate a random number  (internal)
 * __doc__             
 * Generate a random number 
 */

void
name_ran (void)
{
    int lo;
    int hi;
    int test;

    hi = name_seedi / _N_Q_QUOTIENT;
    lo = name_seedi % _N_Q_QUOTIENT;
    test = _N_A_MULTIPLIER * lo - _N_R_REMAINDER * hi;
    if (test > 0)
    {
	name_seedi = test;
    }
    else
    {
	name_seedi = test + _N_M_MODULUS;
    }
}


/**
 * @brief Rotate (circular shift) of 32 bit int. 
 *        Used to rotate some low order bits that are typically
 *        not having enough variation to be a reasonable set for
 *        modulo with low values.
 *
 * @param value : value to shift 
 * @param shift : shift amount
 */
/*
 * __doc__
 * __doc__  Function : unsigned int rotateR(unsigned int value, int shift) 
 * __doc__  Arguments: int value to shift, int shift amount
 * __doc__  Returns  : number after shift
 * __doc__  Performs : Rotate (circular shift) of 32 bit int. 
 * __doc__             Used to rotate some low order bits that are typically
 * __doc__             not having enough variation to be a reasonable set for
 * __doc__             modulo with low values.
 * __doc__             
 */
unsigned int
rotateR (unsigned int value, int shift)
{
    if ((shift &= 31) == 0)
	return value;
    return (value >> shift) | (value << (32 - shift));
}

/**
 *       Author: Don Capps (capps@iozone.org)
 *               7417 Crenshaw
 *             Plano, TX 75025
 *
 * Copyright 2015 Don Capps
 *
 * License to freely use and distribute this software is hereby granted 
 * by the author, subject to the condition that this copyright notice 
 * remains intact.  The author retains the exclusive right to publish 
 * derivative works based on this work, including, but not limited to, 
 * revised versions of this work.
 *
 *  Trademarks. This License does not grant permission to use the trade
 *  names, trademarks, service marks, or product names of the Licensor,
 *  except as required for reasonable and customary use in describing the
 *  origin of the Work and reproducing the content of the NOTICE file.
 *                                                                           
 *  Disclaimer of Warranty. Unless required by applicable law or
 *  agreed to in writing, Licensor provides the Work (and each
 *  Contributor provides its Contributions) on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 *  implied, including, without limitation, any warranties or conditions
 *  of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
 *  PARTICULAR PURPOSE. You are solely responsible for determining the
 *  appropriateness of using or redistributing the Work and assume any
 *  risks associated with Your exercise of permissions under this License.
 *                                                                           
 *  Limitation of Liability. In no event and under no legal theory,
 *  whether in tort (including negligence), contract, or otherwise,
 *  unless required by applicable law (such as deliberate and grossly
 *  negligent acts) or agreed to in writing, shall any Contributor be
 *  liable to You for damages, including any direct, indirect, special,
 *  incidental, or consequential damages of any character arising as a
 *  result of this License or out of the use or inability to use the
 *  Work (including but not limited to damages for loss of goodwill,
 *  work stoppage, computer failure or malfunction, or any and all
 *  other commercial damages or losses), even if such Contributor
 *  has been advised of the possibility of such damages.
 *
 */

/* 
 * Iozone's personal Enigma_on_steroids encrypt/decrypt routines.
 * (This is a simple flexible symetric encryption algorithm)
 *
 * Author: Don Capps
 * Location: Iozone.org
 * Date: 3/1/2015
 *
 * MT = Mersenne Twister.
 * P & M = Park and Miller Congruential random number generator.
 *
 * This is loosly based on the Enigma machine. Enigma, plus variable number of
 * rotors, and each rotor has 256 characters. The rotors are initialized from
 * a key that the user provides.  This key is used to seed two random number 
 * generators. One that shuffle the characters on the rotors, and another
 * that is used to XOR with input characters.  Each time a character is 
 * processed it is Xor'd with a changing random value (MT), and a random 
 * rotor will be re-shuffled (P & M), using either a per character partial 
 * shuffle (P & M), or a longer shuffle (P & M) at a user specified re-shuffle
 * frequency. 
 * 
 * The layers of cipher are:
 *   Xor with input character with random sequence. ( 64 bit Mersenne Twister )
 *   Substitution shift across N user defined rotors. ( Rotors randomized 
 *							with P & M )
 *   Per character partial shuffle of random rotor. ( P & M )
 *   More exhaustive shuffle of random rotor at user defined freq. (P & M)
 *
 * There are "num_rotors" rotors.  Each contains a randomized set of values
 * from 0 to 255.   Each letter is fed into the series of rotors, with
 * the input to the next rotor being the output from the previous rotor.
 *
 *  
 * Rotor # 0                         Rotor #1   ---->
 *
 * Index       _____Output____>                          __Output__>
 *             ^               |                         ^          |
 *             |               |                         |          |
 * 0   1   2   3   4   5       |     0   1   2   3   4   5          |
 * -   -   -   -   -   -       |     -   -   -   -   -   -          |
 * Input ----> 65              |__Input________________> 3          |___> 5
 *
 * The letter 'A' is decimal 65.  The first rotor converts this to 
 * the index value of 3.  The second rotor converts the input value of 3 into
 * the index of 5.  This continues through all of the randomized rotors.
 *
 * Note: The total number of rotors is configurable. (See num_rotors)
 * Currently there are 7 default rotors, each of which contains randomized
 * values.  The seed for the randomization, the number of rotors, and
 * the re-shuffle frequency are given by the user as parameters to the 
 * _start_encryption() routine. 
 *
 * ------------------------------------------------------------------------
 * Functions:
 *
 *    _start_encryption( key, num_rotors, reshuffle_freq)
 *      This starts the entire encryption mechanism;
 *
 *    _end_encryption(void)
 *       This frees all memory associated with encyption machine.
 *
 *    ret = _crypt_char( char )
 *       This encrypts a character, and returns the encrypted version.
 *
 *    ret = _decrypt_char( char )
 *       This decrypts a character, and returns the un-encrypted version.
 *
 *    ret = _crypt_string( unsigned char *dest, unsigned char *src, int count )
 *       This encrypts a string, and returns pointer to the destination 
 *
 *    ret = _decrypt_string(unsigned char *dest, unsigned char *src, int count )
 *       This decrypts a string, and returns pointer to the destination 
 *   --------------------------------------------------------------------
 *   Internal mechanisms 
 *   --------------------------------------------------------------------
 *    unsigned char _rotor_forward(int which, unsigned char input);
 *      Routine that takes which rotor, and initial character and finds
 *      the output character from this rotor, given this input character.
 *
 *    unsigned char _rotor_backward(int, unsigned char);
 *      Routine that takes which rotor, and initial character and finds
 *      the output character from the previous rotor.
 *
 *    void _sync_rotor(void)
 *      Routine that picks a rotor (based on the user's key) and randomises
 *      the contents of this rotor. It also advances a secondary key value as
 *      each character is processed. Thus some rotors are re-randomized 
 *      as characters of input are processed. (more than simply advanced
 *      as in the original Enigma machine)
 *
 *    void shuffle_two( struct rotor * );
 *      Routine that does a shuffle of two of the characterr in a rotor
 *      
 *    void _alloc_rotors(int);
 *      Allocates the user specified number of rotors.
 *
 *    struct rotor * _init_rotor( void )
 *      Initializes the rotors with values, and shuffles each rotor's contents.
 *      Returns a pointer to the rotor.
 *
 * Legal note:  
 * This is an encryption mechanism and may come under some form of 
 * distribution restrictions.  See:
 * http://en.wikipedia.org/wiki/Export_of_cryptography_from_the_United_States	
 */

static int rotor_seedi = 2231;

/* Distance, in characters, between re-shuffles */
#define COG 13
int cog = COG;

/* Default number of rotors */
#define NUM_ROT 7
int num_rotors = NUM_ROT;	/* Defaults */

int global_start_enc = 0;



struct rotor **r_array;		/* Array of all of the rotors */
unsigned long long key;		/* The seed for the srand48(); */
int syncp = 1;			/* Advances for each input character */

/**
 * @brief Re-shuffle the contents of a randomly selected rotor
 */
/* 
 * __doc__
 * __doc__  Function : void _sync_rotor(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Re-shuffle the contents of a randomly selected rotor
 * __doc__             
 */
void
_sync_rotor (void)
{
    int rd;
    int which_rotor, how_much;
    unsigned char rand1, rand2, s1, s2, tmp;
    struct rotor *rotor_ptr;


    /* Move the seed for each input character */
    syncp++;

    if (syncp % cog != 0)
    {
	/* Minor shuffle */
	which_rotor = ((_rotor_rand () >> 2) % num_rotors);
	rotor_ptr = r_array[which_rotor];
	shuffle_two (rotor_ptr);
	return;
    }

    /* Prepare for the big shuffle */

    /* Pick a rotor to re-spin */
    which_rotor = ((_rotor_rand () >> 2) % num_rotors);
    /* Shuffle ~1/2 of the total number of positions */
    how_much = ((_rotor_rand () >> 3) % NUM_POS);
    rotor_ptr = r_array[which_rotor];

    /* The big shuffle */
    for (rd = 0; rd < (how_much); rd++)
    {
	rand1 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
	rand2 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
	s1 = rotor_ptr->ch_array[(unsigned char) rand1];
	s2 = rotor_ptr->ch_array[(unsigned char) rand2];
	rotor_ptr->ch_array[(unsigned char) rand1] = s2;
	rotor_ptr->ch_array[(unsigned char) rand2] = s1;
	/* Swap the reverse lookup too */
	tmp = rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand2]];
	rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand2]] =
	    rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand1]];
	rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand1]] = tmp;
    }
}

/**
 * @brief A minor shuffle. This is called more frequently, and 
 *        only does a little shuffle of a randomly chosen rotor. 
 *
 * @param rotor_ptr : Pointer to current rotor to shuffle.
 */
/*
 * __doc__
 * __doc__  Function : void shuffle_two( struct rotor *rotor_ptr )
 * __doc__  Arguments: pointer to current rotor to shuffle.
 * __doc__  Returns  : void
 * __doc__  Performs : A minor shuffle. This is called more frequently, and 
 * __doc__             only does a little shuffle of a randomly chosen rotor. 
 * __doc__             
 */
void
shuffle_two (struct rotor *rotor_ptr)
{
    unsigned char rand1, rand2, s1, s2, tmp;

    /* Shuffle two locations */
    rand1 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
    rand2 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
    s1 = rotor_ptr->ch_array[(unsigned char) rand1];
    s2 = rotor_ptr->ch_array[(unsigned char) rand2];
    rotor_ptr->ch_array[(unsigned char) rand1] = s2;
    rotor_ptr->ch_array[(unsigned char) rand2] = s1;

    /* Swap the reverse lookup too */
    tmp = rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand2]];
    rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand2]] =
	rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand1]];
    rotor_ptr->ch_iarray[rotor_ptr->ch_array[rand1]] = tmp;
}

/**
 * @brief Initialize the rotors.
 */
/*
 * __doc__
 * __doc__  Function : struct rotor * _init_rotor(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : struct rotor pointer
 * __doc__  Performs : Initialize the rotors.
 * __doc__             
 */

struct rotor *
_init_rotor (void)
{
    int i, rd, how_much;
    unsigned char rand1, rand2, s1, s2;
    struct rotor *rotor_ptr;
    rotor_ptr = (struct rotor *) my_malloc (sizeof (struct rotor));
    how_much = NUM_POS / 2;

    for (i = 0; i < NUM_POS; i++)
    {
	global_rotor.ch_array[i] = i;
    }
    memcpy (rotor_ptr->ch_array, &global_rotor.ch_array, NUM_POS);

    /* Randomize the contents of the rotor */
    for (rd = 0; rd < (how_much); rd++)
    {
	rand1 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
	rand2 = (unsigned char) ((_rotor_rand () >> 2) % NUM_POS);
	s1 = rotor_ptr->ch_array[(unsigned char) rand1];
	s2 = rotor_ptr->ch_array[(unsigned char) rand2];
	rotor_ptr->ch_array[(unsigned char) rand1] = s2;
	rotor_ptr->ch_array[(unsigned char) rand2] = s1;

    }
    /* Create the inverted lookup */
    for (i = 0; i < NUM_POS; i++)
    {
	rotor_ptr->ch_iarray[rotor_ptr->ch_array[i]] = i;
    }
    return (rotor_ptr);
}

/**
 * @brief Move to next character from the next rotor.
 *
 * @param which_rotor : which rotor to select
 * @param input : character to input 
 */
/*
 * __doc__
 * __doc__  Function : unsigned char _rotor_forward( int which_rotor, 
 * __doc__                            unsigned char input )
 * __doc__  Arguments: int which rotor, unsigned char input character 
 * __doc__  Returns  : Next output char.
 * __doc__  Performs : Move to next character from the next rotor.
 * __doc__             
 */
unsigned char
_rotor_forward (int which_rotor, unsigned char input)
{
    struct rotor *rotor_ptr;
    char offset;
    rotor_ptr = r_array[which_rotor];
    /* Return index of this character */
    offset = rotor_ptr->ch_iarray[input];
    return (offset);
}

/**
 * @brief Move to next character from the previous rotor.
 *
 * @param which_rotor : which rotor to select
 * @param offset : offset position on rotor
 */
/*
 * __doc__
 * __doc__  Function : unsigned char _rotor_backward( int which_rotor, 
 * __doc__                            unsigned char offset )
 * __doc__  Arguments: int which rotor, unsigned char offset
 * __doc__  Returns  : Next output char.
 * __doc__  Performs : Move to next character from the previous rotor.
 * __doc__             
 */

unsigned char
_rotor_backward (int which_rotor, unsigned char offset)
{
    unsigned char current_char;
    struct rotor *rotor_ptr;
    rotor_ptr = r_array[which_rotor];
    current_char = rotor_ptr->ch_array[offset];
    return (current_char);
}

/**
 * @brief Allocate the internal rotors
 *
 * @param num_rot: number of rotors
 */
/*
 * __doc__
 * __doc__  Function : void _alloc_rotors(int num_rot)
 * __doc__  Arguments: int number of rotors
 * __doc__  Returns  : void
 * __doc__  Performs : allocate the internal rotors
 * __doc__             
 */
void
_alloc_rotors (int num_rot)
{
    /* Array of all of the pointers to the rotors */
    r_array = (struct rotor **) malloc (sizeof (struct rotor *) * num_rot);
}

/**
 * @brief Deallocate the rotors, and the memory for the pointers to 
 *          the rotors.
 *
 * @param num_rot : Number of rotors
 */
/*
 * __doc__
 * __doc__  Function : void _dealloc_rotors(int num_rot)
 * __doc__  Arguments: number of rotors
 * __doc__  Returns  : void
 * __doc__  Performs : Deallocate the rotors, and the memory for the 
 * __doc__  pointers to the rotors.
 * __doc__             
 */

void
_dealloc_rotors (int num_rot)
{
    int i;
    for (i = 0; i < num_rot; i++)
    {
	free (r_array[i]);
    }
    free (r_array);
    r_array = (struct rotor **) NULL;
}

/*
 * -------------------------  External Definitions  -------------------------
 */

/* 
 * The Park and Miller LC random number generator.
 */

/**
 * @brief Seed the random number generator.
 *
 * @param  seed : input seed value
 */
/*
 * __doc__
 * __doc__  Function : void _rotor_srand(int seed)
 * __doc__  Arguments: input seed
 * __doc__  Returns  : void
 * __doc__  Performs : Seed the random number generator.
 * __doc__             
 */
void
_rotor_srand (int seed)
{
    rotor_seedi = seed;
}

/**
 * @brief Returns a random number.
 */
/*
 * __doc__
 * __doc__  Function : int _rotor_rand(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : int
 * __doc__  Performs : Returns a random number.
 * __doc__             
 */
int
_rotor_rand (void)
{
    (void) _rotor_ran ();
    return (rotor_seedi);
}

/*
 * Get the current seed value 
 */

/*  Not defined until needed..... 
int
_rotor_gseed(void)
{
    return(rotor_seedi);
}
*/

/**
 * @brief Compute the next random number.
 */
/*
 * __doc__
 * __doc__  Function : double _rotor_ran(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : double random number
 * __doc__  Performs : Compute the next random number.
 * __doc__             
 */
double
_rotor_ran (void)
/***************************************************************/
/* See "Random Number Generators: Good Ones Are Hard To Find", */
/*     Park & Miller, CACM 31#10 October 1988 pages 1192-1201. */
/***************************************************************/
/* THIS IMPLEMENTATION REQUIRES AT LEAST 32 BIT INTEGERS !     */
/***************************************************************/
#if defined(WIN32)
#define int32_t int
#endif
#define _ROTOR_A_MULTIPLIER  16807L
#define _ROTOR_M_MODULUS     2147483647L	/* (2**31)-1 */
#define _ROTOR_Q_QUOTIENT    127773L	/* 2147483647 / 16807 */
#define _ROTOR_R_REMAINDER   2836L	/* 2147483647 % 16807 */
{
    int32_t lo;
    int32_t hi;
    int32_t test;

    hi = rotor_seedi / _ROTOR_Q_QUOTIENT;
    lo = rotor_seedi % _ROTOR_Q_QUOTIENT;
    test = _ROTOR_A_MULTIPLIER * lo - _ROTOR_R_REMAINDER * hi;
    if (test > 0)
    {
	rotor_seedi = test;
    }
    else
    {
	rotor_seedi = test + _ROTOR_M_MODULUS;
    }
    return ((float) rotor_seedi / _ROTOR_M_MODULUS);
}

/**
 * @brief Allocate the rotors 
 *
 * @param num_rotors : int number of rotors
 */
/* 
 * __doc__
 * __doc__  Function : void _allocate_all_rotors(int num_rotors)
 * __doc__  Arguments: int number of rotors
 * __doc__  Returns  : void
 * __doc__  Performs : Allocate the rotors 
 * __doc__             
 * */
void
_allocate_all_rotors (int num_rotors)
{
    int rotors;
    /* Allocate rotors */
    _alloc_rotors (num_rotors);

    /* Initalize the rotors */
    for (rotors = 0; rotors < num_rotors; rotors++)
    {
	r_array[rotors] = _init_rotor ();
    }
}

/**
 * @brief Encrypt a char 
 *
 * @param buffer : char to input.
 */
/* 
 * __doc__
 * __doc__  Function : unsigned char _crypt_char( unsigned char buffer )
 * __doc__  Arguments: pointer to char buffer.
 * __doc__  Returns  : encrypted char.
 * __doc__  Performs : Encrypt a char 
 * __doc__             
 */
unsigned char
_crypt_char (unsigned char buffer)
{
    unsigned char val;
    int spin;
    val = buffer ^ ((rotor_genrand64_int64 () >> 2) & 0xff);
    for (spin = 0; spin < num_rotors; spin++)
    {
	val = _rotor_forward (spin, val);
    }
    _sync_rotor ();
    return (val);
}

/**
 * @brief Encrypt a string 
 *
 * @param dest : Pointer to dest string
 * @param src : Pointer to src string
 * @param count : Count.
 */
/* 
 * __doc__
 * __doc__  Function : unsigned char * _crypt_string( unsigned char *dest,
 * __doc__                                 unsigned char *src, int count )
 * __doc__  Arguments: pointer to src string, pointer to dest string, count.
 * __doc__  Returns  : pointer to resulting string.
 * __doc__  Performs : Encrypt a string 
 * __doc__             
 */
unsigned char *
_crypt_string (unsigned char *dest, unsigned char *src, int count)
{
    int j;
    for (j = 0; j < count; j++)
	dest[j] = _crypt_char (src[j]);
    return (dest);
}

/**
 * @brief Decrypt a string 
 *
 * @param dest : Pointer to dest 
 * @param src : Pointer to src 
 * @param count : Count
 */
/* 
 * __doc__
 * __doc__  Function : unsigned char * _decrypt_string( unsigned char *dest, 
 * __doc__                                unsigned char *src, int count )
 * __doc__  Arguments: Pointer to dest, pointer to src, count.
 * __doc__  Returns  : pointer to dest.
 * __doc__  Performs : Decrypt a string 
 * __doc__             
 */
unsigned char *
_decrypt_string (unsigned char *dest, unsigned char *src, int count)
{
    int j;
    for (j = 0; j < count; j++)
	dest[j] = _decrypt_char (src[j]);
    return (dest);
}

/**
 * @brief Decrypt a char 
 *
 * @param buffer : Pointer to char buffer.
 */
/* 
 * __doc__
 * __doc__  Function unsigned char _decrypt_char(unsigned char buffer)
 * __doc__  Arguments: pointer to char buffer.
 * __doc__  Returns  : decrypted char.
 * __doc__  Performs : Decrypt a char 
 * __doc__             
 */
unsigned char
_decrypt_char (unsigned char buffer)
{
    unsigned char nval;
    int spin;
    nval = buffer;
    for (spin = num_rotors - 1; spin >= 0; spin--)
    {
	nval = _rotor_backward (spin, nval);
    }
    nval = nval ^ ((rotor_genrand64_int64 () >> 2) & 0xff);
    _sync_rotor ();
    return (nval);
}

/**
 * @brief Used to start the Enigma machine.
 *
 * @param pkey : pkey
 * @param num_rot : number_of_rotors
 * @param reshuffle_freq : shuffle frequency
 */
 /*
  * __doc__
  * __doc__  Function : void _start_encryption( unsigned long long pkey, 
  * __doc__                      int num_rot, int reshuffle_freq)
  * __doc__  Arguments: long long key, int number_of_rotors, shuffle freq
  * __doc__  Returns  : void
  * __doc__  Performs : Used to start the Enigma machine.
  * __doc__             
  */
void
_start_encryption (unsigned long long pkey, int num_rot, int reshuffle_freq)
{
    if (global_start_enc == 1)
	return;
    syncp = 1;
    key = pkey;
    cog = reshuffle_freq;
    num_rotors = num_rot;
    _rotor_srand ((int) (key + (syncp * 17)));
    rotor_init_genrand64 ((unsigned long long) (key + syncp * 17));
    _allocate_all_rotors (num_rotors);
    global_start_enc = 1;
}

/**
 * @brief Used to stop the Enigma machine.
 */
/*
 * __doc__
 * __doc__  Function : void _end_encryption(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Used to stop the Enigma machine.
 * __doc__             
 */
void
_end_encryption (void)
{
    if (global_start_enc == 0)
	return;
    _dealloc_rotors (num_rotors);
    global_start_enc = 0;
}

/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using rotor_init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/



#define ROTOR_NN 312
#define ROTOR_MM 156
#define ROTOR_MATRIX_A 0xB5026F5AA96619E9ULL
#define ROTOR_UM 0xFFFFFFFF80000000ULL	/* Most significant 33 bits */
#define ROTOR_LM 0x7FFFFFFFULL	/* Least significant 31 bits */


/* The array for the state vector */
static unsigned long long rotor_mt[ROTOR_NN];
/* rotor_mti==ROTOR_NN+1 means rotor_mt[ROTOR_NN] is not initialized */
static int rotor_mti = ROTOR_NN + 1;

/**
 * @brief Initializes rotor_mt[ROTOR_NN] with a seed 
 *
 * @param seed : initial seed value
 */
/* 
 * __doc__
 * __doc__  Function : void rotor_init_genrand64(unsigned long long seed)
 * __doc__  Arguments: long long seed
 * __doc__  Returns  : void
 * __doc__  Performs : Initializes rotor_mt[ROTOR_NN] with a seed 
 * __doc__             
 */
void
rotor_init_genrand64 (unsigned long long seed)
{
    rotor_mt[0] = seed;
    for (rotor_mti = 1; rotor_mti < ROTOR_NN; rotor_mti++)
	rotor_mt[rotor_mti] =
	    (6364136223846793005ULL *
	     (rotor_mt[rotor_mti - 1] ^ (rotor_mt[rotor_mti - 1] >> 62)) +
	     rotor_mti);
}


/**
 * @brief Generates a random number on [0, 2^64-1]-interval 
 */
/* 
 * __doc__
 * __doc__  Function : unsigned long long rotor_genrand64_int64(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : unsigned long long random number
 * __doc__  Performs : Generates a random number on [0, 2^64-1]-interval 
 * __doc__             
 */
unsigned long long
rotor_genrand64_int64 (void)
{
    int i;
    unsigned long long x;
    static unsigned long long mag01[2] = { 0ULL, ROTOR_MATRIX_A };

    if (rotor_mti >= ROTOR_NN)
    {				/* generate ROTOR_NN words at one time */

	/* if rotor_init_genrand64() has not been called, */
	/* a default initial seed is used     */
	if (rotor_mti == ROTOR_NN + 1)
	    rotor_init_genrand64 (5489ULL);

	for (i = 0; i < ROTOR_NN - ROTOR_MM; i++)
	{
	    x = (rotor_mt[i] & ROTOR_UM) | (rotor_mt[i + 1] & ROTOR_LM);
	    rotor_mt[i] =
		rotor_mt[i + ROTOR_MM] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];
	}
	for (; i < ROTOR_NN - 1; i++)
	{
	    x = (rotor_mt[i] & ROTOR_UM) | (rotor_mt[i + 1] & ROTOR_LM);
	    rotor_mt[i] =
		rotor_mt[i +
			 (ROTOR_MM -
			  ROTOR_NN)] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];
	}
	x = (rotor_mt[ROTOR_NN - 1] & ROTOR_UM) | (rotor_mt[0] & ROTOR_LM);
	rotor_mt[ROTOR_NN - 1] =
	    rotor_mt[ROTOR_MM - 1] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];

	rotor_mti = 0;
    }

    x = rotor_mt[rotor_mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

unsigned long long init[4] =
    { 0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL };

unsigned long long length = 4;

/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/



#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL	/* Most significant 33 bits */
#define LM 0x7FFFFFFFULL	/* Least significant 31 bits */


/* The array for the state vector */
static unsigned long long mt[NN];
/* mti==NN+1 means mt[NN] is not initialized */
static int mti = NN + 1;

/**
 * @brief Initializes mt[NN] with a seed 
 *
 * @param seed : Initial seed
 */
/* 
 * __doc__
 * __doc__  Function : void init_genrand64(unsigned long long seed)
 * __doc__  Arguments: unsigned long long seed.
 * __doc__  Returns  : void
 * __doc__  Performs : initializes mt[NN] with a seed 
 * __doc__             
 * */
void
init_genrand64 (unsigned long long seed)
{
    mt[0] = seed;
    for (mti = 1; mti < NN; mti++)
	mt[mti] =
	    (6364136223846793005ULL * (mt[mti - 1] ^ (mt[mti - 1] >> 62)) +
	     mti);
}

/**
 * @brief Initialize by an array with array-length 
 *        init_key is the array for initializing keys 
 *        key_length is its length 
 *
 * @param init_key : Initial key value
 * @param key_length : Length of the key
 */
/* 
 * __doc__
 * __doc__  Function : void init_by_array64(unsigned long long init_key[],
 * __doc__ 		     unsigned long long key_length)
 * __doc__  Arguments: initial key, key_length.
 * __doc__  Returns  : void
 * __doc__  Performs : initialize by an array with array-length 
 * __doc__             init_key is the array for initializing keys 
 * __doc__             key_length is its length 
 * __doc__
 */
void
init_by_array64 (unsigned long long init_key[], unsigned long long key_length)
{
    unsigned long long i, j, k;
    init_genrand64 (19650218ULL);
    i = 1;
    j = 0;
    k = (NN > key_length ? NN : key_length);
    for (; k; k--)
    {
	mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 3935559000370003845ULL)) + init_key[j] + j;	/* non linear */
	i++;
	j++;
	if (i >= NN)
	{
	    mt[0] = mt[NN - 1];
	    i = 1;
	}
	if (j >= key_length)
	    j = 0;
    }
    for (k = NN - 1; k; k--)
    {
	mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 62)) * 2862933555777941757ULL)) - i;	/* non linear */
	i++;
	if (i >= NN)
	{
	    mt[0] = mt[NN - 1];
	    i = 1;
	}
    }

    mt[0] = 1ULL << 63;		/* MSB is 1; assuring non-zero initial array */
}

/**
 * @brief Generates a random number on [0, 2^64-1]-interval 
 */
/* 
 * __doc__
 * __doc__  Function : unsigned long long genrand64_int64(void)
 * __doc__  Arguments: void
 * __doc__  Returns  :  unsigned long long  random number.
 * __doc__  Performs : generates a random number on [0, 2^64-1]-interval 
 * __doc__            
 */
unsigned long long
genrand64_int64 (void)
{
    int i;
    unsigned long long x;
    static unsigned long long mag01[2] = { 0ULL, MATRIX_A };

    if (mti >= NN)
    {				/* generate NN words at one time */

	/* if init_genrand64() has not been called, */
	/* a default initial seed is used     */
	if (mti == NN + 1)
	    init_genrand64 (5489ULL);

	for (i = 0; i < NN - MM; i++)
	{
	    x = (mt[i] & UM) | (mt[i + 1] & LM);
	    mt[i] = mt[i + MM] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];
	}
	for (; i < NN - 1; i++)
	{
	    x = (mt[i] & UM) | (mt[i + 1] & LM);
	    mt[i] = mt[i + (MM - NN)] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];
	}
	x = (mt[NN - 1] & UM) | (mt[0] & LM);
	mt[NN - 1] = mt[MM - 1] ^ (x >> 1) ^ mag01[(int) (x & 1ULL)];

	mti = 0;
    }

    x = mt[mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

/**
 * @brief Generates a random number on [0, 2^63-1]-interval 
 */
/* 
 * __doc__
 * __doc__  Function : long long genrand64_int63(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : long long: random number
 * __doc__  Performs : Generates a random number on [0, 2^63-1]-interval 
 * __doc__
 */
long long
genrand64_int63 (void)
{
    return (long long) (genrand64_int64 () >> 1);
}

/**
 * @brief Generates a random number on [0,1]-real-interval 
 */
/* 
 * __doc__
 * __doc__  Function : double genrand64_real1(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : double random value.
 * __doc__  Performs : Generates a random number on [0,1]-real-interval 
 * __doc__             
 * */
double
genrand64_real1 (void)
{
    return (genrand64_int64 () >> 11) * (1.0 / 9007199254740991.0);
}

/**
 * @brief Generates a random number on [0,1)-real-interval 
 */
/* 
 * __doc__
 * __doc__  Function : double genrand64_real2(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : double random value
 * __doc__  Performs : Generates a random number on [0,1)-real-interval 
 * __doc__             
 */
double
genrand64_real2 (void)
{
    return (genrand64_int64 () >> 11) * (1.0 / 9007199254740992.0);
}

/**
 * @brief Generates a random number on (0,1)-real-interval 
 */
/* 
 * __doc__
 * __doc__  Function : double genrand64_real3(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : double random value
 * __doc__  Performs : Generates a random number on (0,1)-real-interval 
 * __doc__
 * */
double
genrand64_real3 (void)
{
    return ((genrand64_int64 () >> 12) + 0.5) * (1.0 / 4503599627370496.0);
}

/**
 * @brief Set the seed in the random number generator 
 *
 * @param seed : Set the seed value.
 */
/*
 * __doc__
 * __doc__  Function : void netmist_srand(int seed)
 * __doc__  Arguments: int seed: Set the seed value.
 * __doc__  Returns  : void
 * __doc__  Performs : Set the seed in the random number generator 
 * __doc__
 */
void
netmist_srand (int seed)
{
#if defined(WIN32)
    cur_seed = seed;
    n_spec_srand (seed);
#else
    cur_seed = seed;
    srand (seed);
#endif
}

/**
 * @brief Return the current seed of the random number generator.
 */
/*
 * __doc__
 * __doc__  Function : int netmist_gseed(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : void
 * __doc__  Performs : Return the current seed of the random number generator.
 * __doc__
 */
int
netmist_gseed (void)
{
    return (cur_seed);
}

/**
 * @brief Return a random number.
 */
/*
 * __doc__
 * __doc__  Function : int netmist_rand(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : a random number.
 * __doc__  Performs : Return a random number.
 * __doc__
 */
int
netmist_rand (void)
{
    int ret;
#if defined(WIN32)
    cur_seed = n_spec_rand ();
#else
    cur_seed = rand ();
#endif
    ret = (int) rotateR ((unsigned int) cur_seed, 4);
    return (ret & 0x7fffffff);
}

/**
 * @brief Return a 64 bit random number 
 */
/*
 * __doc__
 * __doc__  Function : long long netmsit_llrand(void)
 * __doc__  Arguments: void
 * __doc__  Returns  : long long random number.
 * __doc__  Performs : Return a 64 bit random number 
 * __doc__
 */
unsigned long long
netmist_llrand (void)
{
    long long ret;
#ifdef MERSENNE
    /* 
     * Use the Mersenne Twister. This is a much better random number
     * generator. The standard LCA (Linear Congruential Algorithm) 
     * generators stink. The low order bits are not changing frequently.
     */
    ret = genrand64_int64 ();
#else
    ret = lrand48 ();
    ret = ret << 31;
    ret |= lrand48 ();
#endif
    return (ret);
}

/**********************************************************
 * MD5 functions
 **********************************************************
*/
typedef union uwb
{
    unsigned w;
    unsigned char b[4];
} WBunion;

typedef unsigned Digest[4];

unsigned
f0 (unsigned abcd[])
{
    return (abcd[1] & abcd[2]) | (~abcd[1] & abcd[3]);
}

unsigned
f1 (unsigned abcd[])
{
    return (abcd[3] & abcd[1]) | (~abcd[3] & abcd[2]);
}

unsigned
f2 (unsigned abcd[])
{
    return abcd[1] ^ abcd[2] ^ abcd[3];
}

unsigned
f3 (unsigned abcd[])
{
    return abcd[2] ^ (abcd[1] | ~abcd[3]);
}

typedef unsigned (*DgstFctn) (unsigned a[]);

unsigned *
calcKs (unsigned *k)
{
    double s, pwr;
    int i;

    pwr = pow (2, 32);
    for (i = 0; i < 64; i++)
    {
	s = fabs (sin (1 + i));
	k[i] = (unsigned) (s * pwr);
    }
    return k;
}

/* ROtate v Left by amt bits */
unsigned
rol (unsigned v, short amt)
{
    unsigned msk1 = (1 << amt) - 1;
    return ((v >> (32 - amt)) & msk1) | ((v << amt) & ~msk1);
}

unsigned *
md5 (const char *msg, int mlen)
{
    static Digest h0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
    static DgstFctn ff[] = { &f0, &f1, &f2, &f3 };
    static short M[] = { 1, 5, 3, 7 };
    static short O[] = { 0, 1, 5, 0 };
    static short rot0[] = { 7, 12, 17, 22 };
    static short rot1[] = { 5, 9, 14, 20 };
    static short rot2[] = { 4, 11, 16, 23 };
    static short rot3[] = { 6, 10, 15, 21 };
    static short *rots[] = { rot0, rot1, rot2, rot3 };
    static unsigned kspace[64];
    static unsigned *k;

    static Digest h;
    Digest abcd;
    DgstFctn fctn;
    short m, o, g;
    unsigned f;
    short *rotn;
    union
    {
	unsigned w[16];
	char b[64];
    } mm;
    int os = 0;
    int grp, grps, q, p;
    unsigned char *msg2;

    if (k == NULL)
	k = calcKs (kspace);

    for (q = 0; q < 4; q++)
	h[q] = h0[q];		/* initialize */

    {
	grps = 1 + (mlen + 8) / 64;
	msg2 = malloc (64 * grps);
	memcpy (msg2, msg, mlen);
	msg2[mlen] = (unsigned char) 0x80;
	q = mlen + 1;
	while (q < 64 * grps)
	{
	    msg2[q] = 0;
	    q++;
	}
	{
	    WBunion u;
	    u.w = 8 * mlen;
	    q -= 8;
	    memcpy (msg2 + q, &u.w, 4);
	}
    }

    for (grp = 0; grp < grps; grp++)
    {
	memcpy (mm.b, msg2 + os, 64);
	for (q = 0; q < 4; q++)
	    abcd[q] = h[q];
	for (p = 0; p < 4; p++)
	{
	    fctn = ff[p];
	    rotn = rots[p];
	    m = M[p];
	    o = O[p];
	    for (q = 0; q < 16; q++)
	    {
		g = (m * q + o) % 16;
		f = abcd[1] + rol (abcd[0] + fctn (abcd) + k[q + 16 * p] +
				   mm.w[g], rotn[q % 4]);

		abcd[0] = abcd[3];
		abcd[3] = abcd[2];
		abcd[2] = abcd[1];
		abcd[1] = f;
	    }
	}
	for (p = 0; p < 4; p++)
	    h[p] += abcd[p];
	os += 64;
    }

    if (msg2)
	free (msg2);

    return h;
}

void
print_md5 (char *name, unsigned *d)
{
    int j, k;
    WBunion u;
    char output[256];
    char temp[256];

    memset (output, 0, 50);
    snprintf (output, sizeof (output), "%-20s  0x", name);
    for (j = 0; j < 4; j++)
    {
	u.w = d[j];
	for (k = 0; k < 4; k++)
	{
	    memset (temp, 0, 256);
	    snprintf (temp, sizeof (temp), "%02x", u.b[k]);
	    strncat (output, temp, strlen (temp));
	}
    }
    log_all (LOG_RESULTS, "\t%s\n", output);
}
