/*****************************************************************************
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
 *****************************************************************************/

/*****************************************************************************/
/* Pool memory allocator, for systems that do not                            */
/* provide malloc()                                                          */
/*****************************************************************************/

#include "./copyright.txt"
#include "./license.txt"

#include <stdio.h>
/*
 * Set the pool to 100 Meg
 */
#define MAXPOOL (100*1024*1024)

char pool[MAXPOOL];
void *current = &pool[0];;
long offset = 0;

/**
 * @brief Allocates memory for the pool.
 * @param size : Size of memory pool to create
 */

/*
 * __doc__
 * __doc__  Function : void * pool_alloc( long size )
 * __doc__  Arguments: size
 * __doc__  Returns  : void
 * __doc__  Performs : Allocates memory for the pool.
 * __doc__             
 */
void *
pool_alloc (long size)
{
    if ((long) offset + (long) size > (long) MAXPOOL)
    {
	printf ("Pool empty\n");
	return ((void *) 0);
    }
    current = (void *) &pool[offset];
    offset = offset + size;
    return (current);
}
