/*----------------------------------------------------------------------------
 * File    : vlr.c
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
 *   Contains functions for Variable Length Records.
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#	ifdef __STDC__
#		include <stdlib.h>
#	endif
#else
#	include <io.h>
#	include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"

#define VLR_VERSION	"1.00"
#define SEM_LEN 	0

#define _BLOCKSIZE		(vlr->header.blocksize)
#define _FIRSTFREE		(vlr->header.firstfree)
#define _NEXTBLOCK		(vlr->block->nextblock)
#define _RECSIZE		(vlr->block->recsize)
#define filelength(fd)	lseek(fd, 0, SEEK_END)


static CONFIG_CONST char rcsid[] = "$Id: vlr.c,v 1.8 1999/10/04 03:45:08 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/
static void get_block		PRM( (VLR *, ulong); )
static void put_block		PRM( (VLR *, ulong); )
static ulong get_nextblock	PRM( (VLR *, ulong); )
static void get_header		PRM( (VLR *); )
static void put_header		PRM( (VLR *); )



static void get_block(vlr, blockno)
VLR *vlr;
ulong blockno;
{
	lseek(vlr->fh, (off_t) (blockno * vlr->header.blocksize), SEEK_SET);
	read(vlr->fh, vlr->block, vlr->header.blocksize - SEM_LEN);
}

static void put_block(vlr, blockno)
VLR *vlr;
ulong blockno;
{
	lseek(vlr->fh, (off_t) (blockno * vlr->header.blocksize), SEEK_SET);
	write(vlr->fh, vlr->block, vlr->header.blocksize - SEM_LEN);
}


static ulong get_nextblock(vlr, blockno)
VLR *vlr;
ulong blockno;
{
	ulong nextblock;

	lseek(vlr->fh, (off_t) (blockno * vlr->header.blocksize), SEEK_SET);
	read(vlr->fh, &nextblock, sizeof nextblock);

	return nextblock;
}


/*------------------------------- get_header -------------------------------*\
 *
 * Read header from VLR file.
 *
 */

static void get_header(vlr)
VLR *vlr;
{
	lseek(vlr->fh, 0, SEEK_SET);
	read(vlr->fh, &vlr->header, sizeof vlr->header);
}


/*------------------------------- put_header -------------------------------*\
 *
 * Write header to VLR file.
 *
 */

static void put_header(vlr)
VLR *vlr;
{
	lseek(vlr->fh, 0, SEEK_SET);
	write(vlr->fh, &vlr->header, sizeof vlr->header);
}


/*------------------------------- vlr_close -------------------------------*\
 *
 * Write header to VLR file and close file.
 *
 */

void vlr_close(vlr)
VLR *vlr;
{
	free(vlr->block);
	if( vlr->fh != -1 )
		os_close(vlr->fh);
	free(vlr);
}


/*------------------------------- vlr_open --------------------------------*\
 *
 * Open VRL file. If the file does not already exist it is created.
 *
 */

VLR *vlr_open(fname, blocksize, shared)
char *fname;
unsigned blocksize;
int shared;
{
	VLR *vlr;
    int fh, isnew;

	isnew = access(fname, 0);
	if( (fh = os_open(fname, CONFIG_O_BINARY|O_CREAT|O_RDWR, CONFIG_CREATMASK)) == -1 )
	{
		db_status = S_IOFATAL;
		return NULL;
	}

	if( !(vlr = (VLR *)calloc(sizeof *vlr, 1)) )
    {
    	os_close(fh);
        db_status = S_NOMEM;
        return NULL;
    }

	vlr->fh = fh;

	if( !(vlr->block = (VLRBLOCK *)malloc(blocksize)) )
	{
		os_close(fh);
		free(vlr);
		db_status = S_NOMEM;
		return NULL;
	}

	if( isnew )
	{
		strcpy(vlr->header.version, VLR_VERSION);
		vlr->header.id[0] = 0;
		vlr->header.blocksize = blocksize;
		vlr->header.firstfree = 1;
		vlr->header.numrecords = 0;
		put_header(vlr);
		lseek(vlr->fh, (off_t) (blocksize-1L), SEEK_SET);
		write(vlr->fh, "", 1);
	}
	else
		get_header(vlr);

	vlr->datasize = blocksize - offsetof(VLRBLOCK, data[0]) - SEM_LEN;
	vlr->shared	  = shared;
	strcpy(vlr->fname, fname);

	db_status = S_OKAY;

	return vlr;
}


int vlr_dynclose(vlr)
VLR *vlr;
{
	if( vlr->fh != -1 )
	{
		close(vlr->fh);
		vlr->fh = -1;
	}

	RETURN S_OKAY;
}


int vlr_dynopen(vlr)
VLR *vlr;
{
	if( vlr->fh == -1 )
		if( (vlr->fh = os_open(vlr->fname, CONFIG_O_BINARY|O_CREAT|O_RDWR, CONFIG_CREATMASK)) == -1 )
			RETURN S_IOFATAL;

	RETURN S_OKAY;
}



/*------------------------------- vlr_add ---------------------------------*\
 *
 * Add a record to vlr file. If the delete-chain is non-empty, blocks are
 * taken from there, otherwise blocks are appended to the file.
 *
 */

int vlr_add(vlr, buf, bufsize, recno)
VLR *vlr;
void *buf;
unsigned bufsize;
ulong *recno;
{
	ulong		old_firstfree = _FIRSTFREE;
	ulong		tmp_firstfree = _FIRSTFREE;

	get_header(vlr);

	_RECSIZE = bufsize;

	while( bufsize )
	{
		unsigned copy = bufsize > vlr->datasize ? vlr->datasize : bufsize;
		memcpy(vlr->block->data, buf, copy);
		bufsize -= copy;

		/* Some day, I have to optimize this */

		if( (vlr->header.firstfree) == filelength(vlr->fh)/vlr->header.blocksize )
		{
			_NEXTBLOCK = bufsize ? filelength(vlr->fh) / _BLOCKSIZE + 1 : 0;
			put_block(vlr, _FIRSTFREE);
			_FIRSTFREE = filelength(vlr->fh)/_BLOCKSIZE;
		}
		else
		{
			tmp_firstfree = get_nextblock(vlr, _FIRSTFREE);
			_NEXTBLOCK = bufsize ? tmp_firstfree : 0;
			put_block(vlr, _FIRSTFREE);
			_FIRSTFREE = tmp_firstfree;
		}
		buf = (void *)((char *)buf + vlr->datasize);
		_RECSIZE = 0;
	}

	vlr->header.numrecords++;
	put_header(vlr);

	*recno = old_firstfree;

	return S_OKAY;
}



/*------------------------------- vlr_write -------------------------------*\
 *
 * Update a record. The record is first deleted then inserted. This is not
 * the most efficient way to do it, but it works!
 *
 */

int vlr_write(vlr, buf, bufsize, blockno)
VLR *vlr;
void *buf;
unsigned bufsize;
ulong blockno;
{
	ulong dummy;

	vlr_del(vlr, blockno);
	vlr_add(vlr, buf, bufsize, &dummy);

	RETURN S_OKAY;
}


/*------------------------------- vlr_read --------------------------------*\
 *
 * Read a record.
 *
 */

int vlr_read(vlr, buf, blockno, sizeptr)
VLR *vlr;
void *buf;
ulong blockno;
unsigned *sizeptr;
{
	unsigned size = 0;
	unsigned rest, copy;

	get_header(vlr);
	_NEXTBLOCK = blockno;

	if( (blockno+1) * _BLOCKSIZE > filelength(vlr->fh) )
		return 0;

	do
	{
		get_block(vlr, _NEXTBLOCK);

		if( _RECSIZE > 0 )
			rest = size = _RECSIZE;

		if( !size )
			break;

		copy  = rest > vlr->datasize ? vlr->datasize : rest;
		rest -= copy;

		memcpy(buf, vlr->block->data, copy);
		
		buf = (void *)((char *)buf + vlr->datasize);
	}
	while( _NEXTBLOCK );

	*sizeptr = size;

	RETURN S_OKAY;
}

/*-------------------------------- vlr_del --------------------------------*\
 *
 * Delete a record. The blocks used by the record deleted, are inserted in
 * front of the delete chain.
 *
 */

int vlr_del(vlr, blockno)
VLR *vlr;
ulong blockno;
{
	ulong tmp_firstfree = _FIRSTFREE;
	ulong cur_block = blockno;

	get_header(vlr);

	_FIRSTFREE = blockno;
	get_block(vlr, blockno);

	_RECSIZE = 0;
	put_block(vlr, blockno);

	while( _NEXTBLOCK > 0 )
	{
		cur_block = _NEXTBLOCK;
		_NEXTBLOCK = get_nextblock(vlr, _NEXTBLOCK);
	}

	_NEXTBLOCK = tmp_firstfree;
	put_block(vlr, cur_block);

	vlr->header.numrecords--;
	put_header(vlr);

	RETURN S_OKAY;
}

/* end-of-file */
