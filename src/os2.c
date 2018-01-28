/*----------------------------------------------------------------------------
 * File    : os2.c
 * Library : typhoon
 * OS      : UNIX, OS/2, DOS
 * Author  : Thomas B. Pedersen
 *
 * Copyright (c) 1994 Thomas B. Pedersen.  All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the above
 * copyright notice and the following two  paragraphs appear (1) in all 
 * source copies of this software and (2) in accompanying documentation
 * wherever the programatic interface of this software, or any derivative
 * of it, is described.
 *
 * IN NO EVENT SHALL THOMAS B. PEDERSEN BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF
 * THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF HE HAS BEEN 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THOMAS B. PEDERSEN SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" 
 * BASIS, AND THOMAS B. PEDERSEN HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Description:
 *   Contains functions specific to OS/2.
 *
 * Functions:
 *   ty_openlock	- Create/open the locking resource.
 *   ty_closelock	- Close the locking resource.
 *   ty_lock		- Obtain the lock.
 *   ty_unlock		- Release the lock.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"

static char rcsid[] = "$Id: os2.c,v 1.3 1999/10/04 03:45:07 kaz Exp $";

#ifdef CONFIG_OS2

#define INCL_NOPMAPI
#define INCL_DOSSEMAPHORES
#include <os2.h>

#define TIMEOUT		-1			/* Wait maximum 30 seconds for excl. access	*/

/*---------------------------- Global variables ----------------------------*/
static HMTX block_sem;			/* Blocking semaphore used by d_block()		*/
static HMTX	ty_sem;		 		/* API function semaphore					*/


#ifdef __BORLANDC__
ULONG _dllmain(ULONG termflag, HMODULE modhandle)
#endif
#ifdef __IBMC__
void _CRT_init(void);
ULONG _System _DLL_InitTerm(ULONG modhandle, ULONG termflag)
#endif
{
	static char fname[] = "\\SEM32\\TYPHOON";

	switch( termflag )
	{
		case 0:
#ifdef __IBMC__
			_CRT_init();
#endif
			if( DosOpenMutexSem(fname, &ty_sem) )
				if( DosCreateMutexSem(fname, &ty_sem, 0, 0) )
					return 0;

			if( DosCreateMutexSem(NULL, &block_sem, 0, 0) )
				return 0;
			break;

		case 1:
			/* Never release the ty_sem semaphore */
			DosCloseMutexSem(ty_sem);
			DosCloseMutexSem(block_sem);
			break;
	}

	return 1;
}

/*------------------------------ ty_openlock  ------------------------------*\
 *
 * Purpose	 : This function ensures that only one instance of a Typhoon
 *             function is active at the time, at least in its critical
 *             section.
 *
 * Parameters: None.
 *
 * Returns	 : -1		- Semaphore could not be created.
 *			   0		- Successful.
 *
 */

ty_openlock()
{
/*
	static char fname[] = "\\SEM32\\TYPHOON";

	if( DosOpenMutexSem(fname, &ty_sem) )
		if( DosCreateMutexSem(fname, &ty_sem, 0, 0) )
			return -1;

	locked = 0;
*/
	return 0;
}


ty_closelock()
{
/*	DosCloseMutexSem(ty_sem);*/
	return 0;
}


ty_lock()
{
	if( DosRequestMutexSem(ty_sem, TIMEOUT) )
		return -1;

	return 0;
}


ty_unlock()
{
	DosReleaseMutexSem(ty_sem);
	return 0;
}


void os2_block()
{
	DosRequestMutexSem(block_sem, -1L);
}


void os2_unblock()
{
	DosReleaseMutexSem(block_sem);
}

#endif

/* end-of-file */
