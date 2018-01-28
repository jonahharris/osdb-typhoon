/*----------------------------------------------------------------------------
 * File    : record.c
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
 *   Contains record file functions.
 *
 * Functions:
 *   rec_open		- Open a record file.
 *   rec_close		- Close a record file.
 *   rec_add		- Add a record to a file.
 *   rec_write		- Write a record to a file.
 *   rec_delete		- Delete a record.
 *   rec_read		- Read a record.
 *   rec_frst		- Read the first record in a file.
 *   rec_last		- Read the last record in a file.
 *   rec_next		- Read the next record in a file.
 *   rec_prev		- Read the previous record in a file.
 *   rec_numrecords	- Return the number of records in a file.
 *   rec_reccurr	- Return the record number of the current record.
 *
 *--------------------------------------------------------------------------*/

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "environ.h"
#ifdef CONFIG_UNIX
#   include <unistd.h>
#	ifdef __STDC__
#		include <stdlib.h>
#	endif
#else
#   include <stdlib.h>
#   include <io.h>
#   include <sys\stat.h>
#	include <stddef.h>
#endif

#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"

static CONFIG_CONST char rcsid[] = "$Id: record.c,v 1.8 1999/10/04 03:45:07 kaz Exp $";


/*--------------------------- Function prototypes --------------------------*/
static void putheader   PRM( (RECORD *);            )
static void getheader   PRM( (RECORD *);            )

/*--------------------------------- Macros ---------------------------------*/
#define recseek(R,pos)  lseek(R->fh, R->H.recsize * (pos), SEEK_SET)
#define RECVERSION_ID   "RecMan120"
#define RECVERSION_NUM  120


static void putheader(R)
RECORD *R;
{
    recseek(R, 0L);
    write(R->fh, &R->H, sizeof(R->H));
}


static void getheader(R)
RECORD *R;
{
    recseek(R, 0L);
    read(R->fh, &R->H, sizeof(R->H));
}


/*-------------------------------- rec_open ----=---------------------------*\
 *
 * Purpose	 : Opens a record file.
 *
 * Parameters: fname	- File name.
 *			   recsize	- Record size.
 *			   shared	- Open file in shared mode?
 *
 * Returns	 : NULL		- File could not be opened. db_status contains reason.
 *			   else		- Pointer to record file descriptor.
 *
 */

RECORD *rec_open(fname, recsize, shared)
char *fname;
unsigned recsize;
int shared;
{
    RECORD *R;
    int isnew, fh;

    /* if file exists then read header record, otherwise create */
    isnew = access(fname, 0);

    if( (fh=os_open(fname, CONFIG_O_BINARY|O_RDWR|O_CREAT,CONFIG_CREATMASK)) == -1 )
    {
		db_status = S_IOFATAL;
        return NULL;
    }

	/* Lock the file if it is not opened in shared mode */
	if( !shared )
		if( os_lock(fh, 0L, 1, 't') == -1 )
        {
        	db_status = S_NOTAVAIL;
            return NULL;
       }

    if( (R=(RECORD *)calloc(sizeof(RECORD)+recsize, 1)) == NULL )
	{
    	os_close(fh);
		db_status = S_NOMEM;
        return NULL;
	}

	R->fh = fh;
    R->recno = 0;

    if( isnew )
    {
    	unsigned headersize;

        R->H.datasize       = recsize;
        R->H.recsize        = recsize + (int)offsetof(RECORDHEAD, data[0]);
        R->H.first_deleted  = 0;
        R->H.first          = 0;
        R->H.last           = 0;
        R->H.numrecords     = 0;
        R->H.version        = RECVERSION_NUM;
        strcpy(R->H.id, RECVERSION_ID);

		R->first_possible_rec = (sizeof(R->H) + R->H.recsize - 1) / R->H.recsize;
   
		/* Beware that the record can be smaller than the header */
        if( R->H.recsize < sizeof(R->H) )
        	headersize = R->first_possible_rec * R->H.recsize;
        else
        	headersize = R->H.recsize;

        recseek(R, 0L);
        write(fh, &R->H, headersize);
    }
    else
    {
	    read(R->fh, &R->H, sizeof(R->H));

		R->first_possible_rec = (sizeof(R->H) + R->H.recsize - 1) / R->H.recsize;

		if( R->H.version != RECVERSION_NUM )
		{
			db_status = S_VERSION;
			os_close(fh);
			free(R);
			return NULL;
		}
	}

	strcpy(R->fname, fname);

    db_status = S_OKAY;

    return R;
}


int rec_close(R)
RECORD *R;
{
	if( R->fh != -1 )
	    os_close(R->fh);
    free(R);
    R = NULL;

    RETURN S_OKAY;
}


int rec_dynclose(R)
RECORD *R;
{
	if( R->fh != -1 )
	{
		close(R->fh);
		R->fh = -1;
	}

	RETURN S_OKAY;
}

int rec_dynopen(R)
RECORD *R;
{
	if( R->fh == -1 )
	    if( (R->fh=os_open(R->fname, CONFIG_O_BINARY|O_RDWR|O_CREAT,CONFIG_CREATMASK)) == -1 )
			RETURN S_IOFATAL;

	RETURN S_OKAY;
}


int rec_add(R,data,rec)
RECORD *R;                  /* record file                                  */
void *data;
ulong *rec;
{
	long recno;

    getheader(R);

	if( R->H.first_deleted )
	{
		recno = R->H.first_deleted;

		/* Get recno of next deleted */
		lseek(R->fh, recno * R->H.recsize + (long)offsetof(RECORDHEAD, next), SEEK_SET);
		read(R->fh, &R->H.first_deleted, sizeof(R->H.first_deleted));
	}
	else
		recno = (lseek(R->fh, 0L, SEEK_END) + R->H.recsize - 1) / R->H.recsize;

	if( R->H.numrecords )
	{
		long pos; 

		/* Adjust next-pointer of last record */
		pos = R->H.last * R->H.recsize;
		pos += offsetof(RECORDHEAD, next);

		lseek(R->fh, pos, SEEK_SET);
		write(R->fh, &recno, sizeof recno);

		/* Set prev-pointer of new record */
		R->rec.prev = R->H.last;		
	}
	else
	{
		R->H.first = recno;			/* Set new first pointer				*/
		R->rec.prev = 0;			/* There's no previous record			*/
	}

	R->rec.next = 0;				/* No next record since at end of chain	*/

	R->H.last = recno;				/* Set last-pointer to new record		*/
	R->H.numrecords++;

	R->rec.flags = 0;
    memcpy(R->rec.data, data, R->H.datasize);	/* Copy data to buffer		*/
	lseek(R->fh, recno * R->H.recsize, SEEK_SET);
	if( write(R->fh, &R->rec, R->H.recsize) != R->H.recsize )		/* Write chain and record	*/
		RETURN S_IOFATAL;
	
    putheader(R);
	*rec = recno;

	return S_OKAY;
}


int rec_write(R, data, recno)
RECORD *R;
void *data;
ulong recno;
{
	if( recno < R->first_possible_rec )
		RETURN S_INVADDR;

	lseek(R->fh, (off_t) (R->H.recsize * recno + (long)offsetof(RECORDHEAD, data[0])), SEEK_SET);
	write(R->fh, data, R->H.datasize);

    RETURN S_OKAY;
}

 
int rec_delete(R, recno)
RECORD *R;
ulong recno;
{
    getheader(R);

	/* Get previous and next pointers of record to be deleted */
	lseek(R->fh, (off_t) (R->H.recsize * recno), SEEK_SET);
	read(R->fh, &R->rec, sizeof R->rec);

	if( R->rec.flags & BIT_DELETED )
		RETURN S_DELETED;

	/* Adjust previous pointer */
	if( R->H.first == recno )
		R->H.first = R->rec.next;
	else
	{
		lseek(R->fh, (off_t) (R->H.recsize * R->rec.prev + (long)offsetof(RECORDHEAD, next)), SEEK_SET);
		write(R->fh, &R->rec.next, sizeof R->rec.next);
	}
	
	/* Adjust next pointer */
	if( R->H.last == recno )
		R->H.last = R->rec.prev;
	else
	{
		lseek(R->fh, (off_t) (R->H.recsize * R->rec.next + (long)offsetof(RECORDHEAD, prev)), SEEK_SET);
		write(R->fh, &R->rec.prev, sizeof R->rec.prev);
	}	

	/* Delete-mark the record and insert it in delete chain */
	R->rec.flags |= BIT_DELETED;
	R->rec.next = R->H.first_deleted;
	R->rec.prev = 0;

	lseek(R->fh, (off_t) (R->H.recsize * recno), SEEK_SET);
	write(R->fh, &R->rec, sizeof R->rec);
	R->H.first_deleted = recno;
	R->H.numrecords--;

    putheader(R);

    RETURN S_OKAY;
}


int rec_read(R,data,recno)
RECORD *R;
void *data;
ulong recno;
{
	if( recno < R->first_possible_rec )
		RETURN S_INVADDR;

    recseek(R, (off_t) recno);
    if( read(R->fh, &R->rec, R->H.recsize) < R->H.recsize )
    	RETURN S_NOTFOUND;

    if( R->rec.flags & BIT_DELETED )
    	RETURN S_DELETED;    
   
    memcpy(data, R->rec.data, R->H.datasize);
    R->recno = recno;

    RETURN S_OKAY;
}


int rec_frst(R, data)
RECORD *R;
void *data;
{
    getheader(R);

    return rec_read(R, data, R->H.first);
}


int rec_last(R, data)
RECORD *R;
void *data;
{
    getheader(R);

    return rec_read(R, data, R->H.last);
}


int rec_next(R, data)
RECORD *R;
void *data;
{
	if( CURR_REC )
		return rec_read(R, data, R->rec.next);
	else
		return rec_frst(R, data);
}


int rec_prev(R, data)
RECORD *R;
void *data;
{
	if( CURR_REC )
		return rec_read(R, data, R->rec.prev);
	else
		return rec_last(R, data);
}


ulong rec_numrecords(R, number)
RECORD *R;
ulong *number;
{
    getheader(R);

    if( number )
        *number = R->H.numrecords;

    return R->H.numrecords;
}


int rec_curr(R, recno)
RECORD *R;
ulong *recno;
{
    if( recno )
        *recno = R->recno;

    RETURN R->recno ? S_OKAY : S_NOCR;
}

/* end-of-file */


