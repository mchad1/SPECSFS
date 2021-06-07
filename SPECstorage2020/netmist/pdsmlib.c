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
 *****************************************************************************/

/*
 * A simple client interface for the PDSM remote side.
 * 
 * This functionality depends on the PDSM mechanism being
 * active and the pdsm_remote client is already up and running.
 */
#include <copyright.txt>
#include <license.txt>

#if defined(_vmware_)
/**
 * @brief Returns pointer to the remote child's segment
 */
/*
 * __doc__
 * __doc__  Function : char * get_remote_zone(int child_id,int msize)
 * __doc__  Arguments: int child_id: Child ID
 * __doc__             int msize: Size of the memory segment
 * __doc__  Returns  : char *: Pointer to the segment
 * __doc__  Performs : Returns pointer to the remote child's segment
 * __doc__             
 */
char *
get_remote_zone (int child_id, int msize)
{
    return (0);
}

/**
 * @brief Returns pointer to the Prime's segment
 */
/*
 * __doc__
 * __doc__  Function : char * get_my_prime_zone(int child_id,int msize)
 * __doc__  Arguments: int child_id: Child ID
 * __doc__             int msize: Size of segment
 * __doc__  Returns  : pointer to the Prime's segment
 * __doc__  Performs : Returns pointer to the Prime's segment
 * __doc__             
 */
char *
get_my_prime_zone (int child_id, int msize)
{
    return (0);
}

#else


#ifdef WIN32
#pragma warning(disable: 4996)
/*Windows specific headers*/
#include <ws2tcpip.h>
#include <Windows.h>
#include <tchar.h>
#include <time.h>
#include <win32_sub.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef WIN32
/* UNIX specific headers*/
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

/*
 *	Interface API:  
 *
 *	pointer = (char *)get_remote_zone(client_id,sizeof(struct stats));
 *	
 *	Success: Returns pointer to shared memory segment.
 *      Failure: Returns zero. 
*/

#ifdef WIN32
/*
 * Create file on C: drive
 */
#define PDSM_R_REGISTRY "c:\\tmp\\pdsm_registry_client"
typedef unsigned int key_t;
#else
#define PDSM_R_REGISTRY "/tmp/pdsm_registry_client"
#endif

char buf[256];
int pcdebug = 0;
int shmid, mo_key, mo_size, ro_key, ro_size;
int remove_flag = 0;
int noise;
char *private_segs_ro_pointer;
char *private_segs_mo_pointer;

/*
 * Prototypes 
 */
char *get_sh_mem (key_t, size_t, int);
char *get_remote_zone (int, int);
char *get_my_prime_zone (int, int);

char *pointer;


/*------------------------------------------------*/
/**
 * @brief Returns pointer to the remote child's segment
 */
/*
 * __doc__
 * __doc__  Function : char * get_remote_zone(int child_id,int msize)
 * __doc__  Arguments: int child_id: Child ID
 * __doc__             int msize: Size of the memory segment
 * __doc__  Returns  : char *: Pointer to the segment
 * __doc__  Performs : Returns pointer to the remote child's segment
 * __doc__             
 */
char *
get_remote_zone (int child_id, int msize)
{
    FILE *reg;
    char my_reg_name[256];

    /*
     * Pick the file that has this client's shared memory key.
     */
    snprintf (my_reg_name, 256, "%s_%d", PDSM_R_REGISTRY, child_id);
    reg = fopen (my_reg_name, "r");
    if (reg == NULL)
    {
	if (pcdebug)
	    printf ("Open of registry %s failed\n", my_reg_name);
	return ((char *) 0);
    }
    if (pcdebug)
	printf ("Open of registry %s succeeded\n", my_reg_name);
    memset (buf, 0, 256);
    /*
     * Read in the shared memory key and size.
     */
    noise =
	fscanf (reg, "PDSM_MO_key %x PDSM_MO_size %d\n", &mo_key, &mo_size);
    noise =
	fscanf (reg, "PDSM_RO_key %x PDSM_RO_size %d\n", &ro_key, &ro_size);

    fclose (reg);
    if (pcdebug)
	printf ("Using RO key %x \n", ro_key);
    if (ro_key == 0)
	return (0);
    if (ro_size == 0)
	return (0);
    private_segs_ro_pointer = get_sh_mem (ro_key, ro_size, remove_flag);
    if (private_segs_ro_pointer == NULL)
    {
	return ((char *) 0);
    }
    if (msize > ro_size)
    {
	if (pcdebug)
	    printf ("Client structure is too big for the PDSM seg\n");
	return ((char *) 0);
    }
    return (private_segs_ro_pointer);
}

/**
 * @brief Returns pointer to the Prime's segment
 */
/*
 * __doc__
 * __doc__  Function : char * get_my_prime_zone(int child_id,int msize)
 * __doc__  Arguments: int child_id: Child ID
 * __doc__             int msize: Size of segment
 * __doc__  Returns  : pointer to the Prime's segment
 * __doc__  Performs : Returns pointer to the Prime's segment
 * __doc__             
 */
char *
get_my_prime_zone (int child_id, int msize)
{
    FILE *reg;
    char my_reg_name[256];

    /*
     * Pick the file that has this client's shared memory key.
     */
    snprintf (my_reg_name, 256, "%s_%d", PDSM_R_REGISTRY, child_id);
    reg = fopen (my_reg_name, "r");
    if (reg == NULL)
    {
	if (pcdebug)
	    printf ("Open of registry %s failed\n", my_reg_name);
	return ((char *) 0);
    }
    memset (buf, 0, 256);
    /*
     * Read in the shared memory key and size.
     */
    noise =
	fscanf (reg, "PDSM_MO_key %x PDSM_MO_size %d\n", &mo_key, &mo_size);
    noise =
	fscanf (reg, "PDSM_RO_key %x PDSM_RO_size %d\n", &ro_key, &ro_size);

    fclose (reg);
    if (pcdebug)
	printf ("Using RO key %x \n", ro_key);
    if (mo_key == 0)
	return (0);
    if (mo_size == 0)
	return (0);
    private_segs_mo_pointer = get_sh_mem (mo_key, mo_size, remove_flag);
    if (private_segs_mo_pointer == NULL)
    {
	return ((char *) 0);
    }
    if (msize > mo_size)
    {
	if (pcdebug)
	    printf ("Client structure is too big for the PDSM seg\n");
	return ((char *) 0);
    }
    return (private_segs_mo_pointer);
}

/*
 * This does the work of attaching to the shared memory segment.
 */
#ifdef WIN32

#define MAX_SHARE_MEMORY 32	//max number of shared memory one process can request, regardless of their size
typedef struct _SHARED_MEMORY
{
    HANDLE hFile;		//associated file handle, unused here
    HANDLE hMapFile;		//associated mapping object handle
    key_t key;			//shared memory key
    BOOL valid;
} SHARED_MEMORY;

static SHARED_MEMORY sharedMem[MAX_SHARE_MEMORY] = { 0 };

/**
 * @brief Win32 implementation using memory mapped file
 */
/*
 * __doc__
 * __doc__  Function : char * get_sh_mem(key_t key, size_t size, int flag)
 * __doc__  Arguments: key: Shared memory key
 * __doc__             size: Size of segment
 * __doc__             flag: delete share memory segment
 * __doc__  Returns  : Pointer to the memory segment
 * __doc__  Performs : Win32 implementation using memory mapped file
 * __doc__             key - share memory id, it is used to generate mapping 
 * __doc__               object
 * __doc__             size - the size of shared memory requested
 * __doc__             flag - delete shared memory if its value is non-zero
 * __doc__             
 */
char *
get_sh_mem (key_t key, size_t size, int flag)
{
    TCHAR szObjName[255];
    TCHAR szFileName[255];
    char *pMem = NULL;
    int position = -1, i = 0;	//index in shared memory array where we can save shared_memory information 
    HANDLE hFile;

    _stprintf (szObjName, TEXT ("Global\\pdsm-%x"), key);
    _stprintf (szFileName, TEXT ("SharedMemory.%x"), key);

    /*First we search the shared memory array, try to get existing one */
    for (i = 0; i < MAX_SHARE_MEMORY; i++)
    {
	if (sharedMem[i].valid && sharedMem[i].key == key)
	{
	    position = i;
	    break;
	}
	else if (position < 0 && !sharedMem[i].valid)
	    position = i;	//find a unused slot
    }
    /* if we reach here, we did not find exising shared memory */
    if (flag && sharedMem[position].valid)
    {
	/*do we need to delete memory mapped file here??? */
	hFile = CreateFile (szFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE,	//delete file on close
			    NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	    CloseHandle (hFile);
    }

    if (position < 0)
	/*the process has already allocated max-allowed shared memory, we have no choice but fail the call */
	return NULL;
    /* Now we start to create new shared memory */
    if (!sharedMem[position].valid)
    {
	sharedMem[position].hMapFile = NULL;
	/* We open or create a file to map to memory */
	if ((sharedMem[position].hFile = CreateFile (szFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS,	// create a new file if it does not exist, otherwise open the old file
						     (flag ?
						      FILE_FLAG_DELETE_ON_CLOSE
						      :
						      FILE_ATTRIBUTE_NORMAL),
						     NULL)) ==
	    (HANDLE) INVALID_HANDLE_VALUE)
	{
	    if (pcdebug >= 2)
		printf
		    ("Unable to open or create file for memory mapping:\"SharedMemory.%x\"\n",
		     key);
	}
	else
	{
	    /*Create mapping object */
	    sharedMem[position].hMapFile = CreateFileMapping (sharedMem[position].hFile,	// use paging file
							      NULL,	// default security
							      PAGE_READWRITE,	// read/write access
							      0,	// maximum object size (high-order DWORD)
							      (DWORD) size,	// maximum object size (low-order DWORD)
							      szObjName);	// name of mapping object
	    if (sharedMem[position].hMapFile == NULL)
	    {
		if (pcdebug >= 2)
		    printf
			("Unable to create mapping object: \"Global\\pdsm-%x\", error code: %d\n",
			 key, GetLastError ());
		CloseHandle (sharedMem[position].hFile);
	    }
	    else
	    {
		if (pcdebug >= 2 && GetLastError () == ERROR_ALREADY_EXISTS)
		    printf
			("CreateFileMapping returned a handle of existing mapping object\n");
		sharedMem[position].key = key;
		sharedMem[position].valid = TRUE;
	    }
	}
    }

    if (sharedMem[position].hMapFile != NULL)
    {
	//
	//now we need to mapped the view. 
	//
	pMem = (char *) MapViewOfFile (sharedMem[position].hMapFile,	// handle to map object
				       FILE_MAP_ALL_ACCESS,	// read/write permission
				       0, 0, (DWORD) size);
	if (pMem == NULL && pcdebug >= 2)
	    printf ("Unable to map file to memory, error code: %d\n",
		    GetLastError ());
    }
    return pMem;
}
#else
/**
 * @brief Win32 implementation using memory mapped file
 */
/*
 * __doc__
 * __doc__  Function : char * get_sh_mem(key_t key, size_t size, int flag)
 * __doc__  Arguments: key: Shared memory key
 * __doc__             size: Size of segment
 * __doc__             flag: delete share memory segment
 * __doc__  Returns  : Pointer to the memory segment
 * __doc__  Performs : Win32 implementation using memory mapped file
 * __doc__             key - share memory id, it is used to generate mapping 
 * __doc__               object
 * __doc__             size - the size of shared memory requested
 * __doc__             flag - delete shared memory if its value is non-zero
 * __doc__             
 */
char *
get_sh_mem (key_t key, size_t size, int flag)
{
    char *statsp;

    if (pcdebug == 1)
    {
	printf ("Trying to get shared memory segment with key %x\n",
		(unsigned int) key);
	printf ("Size %d\n", (int) size);
    }
    shmid = shmget ((key_t) key, (size_t) size, (int) (0666));
    if (shmid < 0)
    {
	if (pcdebug)
	    printf ("Shmid %d Errno %d\n", shmid, errno);
	return ((char *) 0);
    }

    if (pcdebug == 2)
	printf ("Got shared memory with key %x\n", (unsigned int) key);

    statsp = (char *) shmat (shmid, 0, 0);
    if (statsp == (char *) -1)
    {
	if (pcdebug)
	    printf ("Attach to shared memory failed\n");
	return ((char *) 0);
    }
    /* Attach ref count is now all that is holding on to this segment */
    if (flag)
	shmctl (shmid, IPC_RMID, 0);	/* cleanup */

    if (pcdebug == 2)
	printf ("Attached to shared memory with key %x\n",
		(unsigned int) key);

    if (pcdebug == 1)
	printf ("Shared memory key %x Size = %d\n", (unsigned int) key,
		(int) size);
    /*
     * Don't need to call quit, as any termination will cleanup
     * automatically.
     */

    /*quit(); */
    return ((char *) statsp);
}
#endif /* WIN32 */
#endif /*_vmware_*/
