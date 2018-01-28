/*----------------------------------------------------------------------------
 * File    : os.c
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
 *   Contains functions that may vary between different operating systems
 *   such as opening and locking files.
 *
 * Functions:
 *   os_lock		Get exclusive access to the database.
 *   os_unlock		Release the lock.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#ifdef _MSC_VER
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/locking.h>
#endif
#ifdef __BORLANDC__
#  include <sys\locking.h>
#  include <io.h>
#endif
#ifdef __IBMC__
#include <io.h>
#include <share.h>
#endif
#include <fcntl.h>
#include <stdio.h>

#include "ty_type.h"
#include "ty_prot.h"

static CONFIG_CONST char rcsid[] = "$Id: os.c,v 1.7 1999/10/04 03:45:07 kaz Exp $";


/*--------------------------------- os_lock --------------------------------*\
 *
 * Purpose	 : Lock a region of a file.
 *
 * Parameters: fh		- File handle.
 *			   offset	- Offset from start of file.
 *			   bytes	- Number of bytes to lock.
 *			   type		- Lock type. 't'=test and lock, 'w'=wait and lock
 *
 *
 * Returns	 : -1 		- The region could not be locked.
 *			   0		- Region locked.
 *
 */

int os_lock(fh, offset, bytes, type)
int fh, type;
long offset;
unsigned bytes;
{
#ifdef CONFIG_USE_FLOCK
	struct flock flk;
#endif
	lseek(fh, offset, SEEK_SET);
#ifdef CONFIG_CONFIG_DOS
	/* Microsoft C */
#	ifdef _MSC_VER
    if( locking(fh, type == 't' ? LK_NBLCK : LK_LOCK, bytes) == -1 )
    	return -1;
#	endif
#endif
#ifdef CONFIG_UNIX
#ifdef CONFIG_USE_FLOCK
    flk.l_type = F_WRLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start = 0;
    flk.l_len = bytes;

    if (fcntl(fh, F_SETLK, &flk) == -1)
		puts("fcntl() F_SETLK failed");
#else
	if( lockf(fh, type == 't' ? F_TLOCK : F_LOCK, bytes) == -1 )
		puts("lockf failed");
#endif
#endif
#ifdef CONFIG_OS2
#	ifdef __BORLANDC__
    if( locking(fh, type == 't' ? LK_NBLCK : LK_LOCK, bytes) == -1 )
    	return -1;
#	endif
#endif

	return 0;
}

/*-------------------------------- os_unlock -------------------------------*\
 *
 * Purpose	 : Unlock a region of a file.
 *
 * Parameters: fh		- File handle.
 *			   offset	- Offset from start of file.
 *			   bytes	- Number of bytes to lock.
 *			   type		- Lock type. 't'=test and lock, 'w'=wait and lock
 *
 *
 * Returns	 : -1 		- The region could not be unlocked.
 *			   0		- Region unlocked.
 *
 */

int os_unlock(fh, offset, bytes)
int fh;
long offset;
unsigned bytes;
{
	lseek(fh, offset, SEEK_SET);
#ifdef CONFIG_DOS
	/* Microsoft C */
#	ifdef _MSC_VER
	return locking(fh, LK_UNLCK, bytes);
#	endif
#endif
#ifdef CONFIG_UNIX
	return lockf(fh, F_ULOCK, bytes);
#endif
#ifdef CONFIG_OS2
#	ifdef __BORLANDC__
    return locking(fh, LK_UNLCK, bytes);
#	endif
#endif
}



int os_open(fname, flags, creatflags)
char *fname;
int flags, creatflags;
{

#ifdef __IBMC__
	return sopen(fname, flags, SH_DENYNO, creatflags);
#else
	return open(fname, flags, creatflags);
#endif
}



int os_close(fh)
int fh;
{
	return close(fh);
}


int os_access(fname, mode)
char *fname;
int mode;
{
#ifdef __IBMC__
	return _access(fname, mode);
#else
	return access(fname, mode);
#endif
}

/* end-of-file */
