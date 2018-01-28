/*----------------------------------------------------------------------------
 * File    : bt_io
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
 *   This file contains the layer between the low-level B-tree and record
 *   routines and the Typhoon API functions. The low-level routines are never
 *   called directly from the API functions.
 *
 * +-----------------------------+
 * |    Typhoon API functions    |
 * +-----------------------------+
 * |  Dynamic open files layer   |	 <----- this layer
 * +----------+------------------+
 * |  B-tree  |  Record  |  VLR  |
 * +----------+------------------+
 * |   C language read/write     |
 * +-----------------------------+
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_io.c,v 1.9 1999/10/04 03:45:08 kaz Exp $";

#define PTRTORECID(rec)		((((rec) - DB->record)+1) * REC_FACTOR)

/*-------------------------- Function prototypes ---------------------------*/
static int	checkfile				PRM( (Id); )

/*---------------------------- Global variables ----------------------------*/
static ulong seqno = 1;				/* Current sequence number (always > 0)	*/




int ty_closeafile()
{
	ulong lowest_seqno = seqno;
	int			dbs, fhs;			/* Used for scanning tables				*/
	Dbentry		*db =typhoon.dbtab;	/* Used for scanning tables				*/
	Fh			*fh;				/* Used for scanning tables				*/
	Fh			*foundfh = NULL;	/* The filehandle found to be closed	*/

	/* Find the file least recently accessed open file */
	dbs = DB_MAX;

	while( dbs-- )
	{
		if( db->clients )
		{
			fh	= db->fh;
			fhs	= db->header.files;

			while( fhs-- )
			{
				if( fh->any && fh->any->fh != -1 )
				{
					if( lowest_seqno > fh->any->seqno )
					{
						lowest_seqno = fh->any->seqno;
						foundfh		 = fh;
					}
				}
				fh++;
			}	
		}
		db++;
	}

	if( foundfh == NULL )
	{
		printf("\a*** Could not close a file **");
		return -1;
	}

	/* If the file is already closed, we just return */
	if( foundfh->any->fh == -1 )
		return S_OKAY;

	switch( foundfh->any->type )
	{
		case 'k':
		case 'r':
			btree_dynclose(foundfh->key); 
			break;
		case 'd': 
			rec_dynclose(foundfh->rec);	
			break;
        case 'v': 
			vlr_dynclose(foundfh->vlr);	
			break;
	}

	/* Mark the file as closed */
	typhoon.cur_open--;

  	return 0;
}




static int checkfile(fileid)
Id fileid;
{
	Fh *fh;
	int rc;

	fh = &DB->fh[fileid];

	/* If the file is open we set the file's sequence number to seqno to
	 * indicate that it is the most recently accessed file.
	 */
	if( fh->any->fh != -1 )
	{
		fh->any->seqno = seqno++;
		return S_OKAY;
	}

	if( typhoon.cur_open == typhoon.max_open )
	{
		if( ty_closeafile() == -1 )
		{
			puts("checkfile: could not find a file to close");
			RETURN S_IOFATAL;
		}
	}

	/* Now <cur_open> should be less than <max_open> so we can call
	 * ty_openfile() to reopen the file. If ty_openfile() cannot open the
	 * file the API function will return S_IOFATAL.
	 */

	switch( fh->any->type )
	{
		case 'k':
		case 'r':
			rc = btree_dynopen(fh->key); 
			break;
		case 'd': 
			rc = rec_dynopen(fh->rec);	
			break;
        case 'v': 
			rc = vlr_dynopen(fh->vlr);	
			break;
	}

	fh->any->seqno = seqno++;
	typhoon.cur_open++;

	return rc;
}


/*------------------------------ ty_openfile -------------------------------*\
 *
 * Purpose	 : Opens a database file.
 *
 * Parameters: fp   	- Pointer to file definition table entry.
 *			   fh		- Pointer to file handle table entry.
 *			   shared	- Open in shared mode?
 *
 * Returns	 : db_status from d_keyopen(), d_recopen() or vlr_open().
 *
 */

int ty_openfile(fp, fh, shared)
File *fp;
Fh *fh;
int shared;
{
	char fname[255];
	Key *key;
	CMPFUNC cmp;

	/* If the maximum number of open files has been reached we return the
	 * CLOSEDPTR to indicate to the other ty_.. functions that the file
	 * descriptor points to a closed file and must be reopened.
	 */
	if( typhoon.cur_open == typhoon.max_open )
	{
		if( ty_closeafile() == -1 )
		{
			puts("ty_openfile: could not find a file to close");
			RETURN S_IOFATAL;
		}
	}

	/* File is located in <DB->dbfpath> */
	sprintf(fname, "%s%s", DB->dbfpath, fp->name);

	switch( fp->type )
	{
		case 'r':
			fh->key = btree_open(fname, sizeof(REF_ENTRY), fp->pagesize, (CMPFUNC)refentrycmp, 0, shared);
			break;
		case 'k':
			key = DB->key + fp->id;

			/* If the key has multiple fields or is sorted in descending order
			 * we use the compoundkeycmp function for key value comparisons.
			 */
			if( key->fields > 1 || !DB->keyfield[key->first_keyfield ].asc )
				cmp = compoundkeycmp;
			else
			{
				Field *fld = &DB->field[ DB->keyfield[key->first_keyfield].field ];

				cmp = keycmp[ fld->type & (FT_BASIC|FT_UNSIGNED) ];
			}

			fh->key = btree_open(fname, key->size, fp->pagesize, cmp,
            					(key->type & KT_UNIQUE) ? 0 : 1, shared);
            break;
		case 'd':
			/* Add the preamble to the size of the record */
			fh->rec = rec_open(fname, (unsigned) (DB->record[fp->id].size +
										DB->record[fp->id].preamble), shared);
			break;
        case 'v':
			fh->vlr = vlr_open(fname, fp->pagesize, shared);
            break;
	}

	if( db_status == S_OKAY )
	{
		fh->any->type  = fp->type;
		fh->any->seqno = seqno++;
		typhoon.cur_open++;
	}
/*
	else
		printf("cannot open '%s' (db_status %d, errno %d)\n", fname, db_status, errno);
*/
	return db_status;
}


/*------------------------------- d_closefile ------------------------------*\
 *
 * Purpose	 : Closes a database file.
 *
 * Parameters: fh	- Pointer to file handle table entry.
 *
 * Returns	 : db_status from d_keyclose(), d_recclose() or vlr_close().
 *
 */

int ty_closefile(fh)
Fh *fh;
{
	/* If the file is already closed, we just return */
	if( fh->any->fh != -1 )
		typhoon.cur_open--;

	switch( fh->any->type )
	{
		case 'k':
		case 'r':
			btree_close(fh->key); 
			break;
		case 'd': 
			rec_close(fh->rec);	
			break;
        case 'v': 
			vlr_close(fh->vlr);	
			break;
	}

	return db_status;
}



int ty_keyadd(key, value, ref)
Key *key;
void *value;
ulong ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_add(idx, value, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}


int ty_keyfind(key, value, ref)
Key *key;
void *value;
ulong *ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_find(idx, value, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}



int ty_keyfrst(key, ref)
Key *key;
ulong *ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_frst(idx, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}


int ty_keylast(key, ref)
Key *key;
ulong *ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_last(idx, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}


int ty_keyprev(key, ref)
Key *key;
ulong *ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_prev(idx, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}


int ty_keynext(key, ref)
Key *key;
ulong *ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_next(idx, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}

			   
int ty_keydel(key, value, ref)
Key *key;
void *value;
ulong ref;
{
	INDEX *idx;
	int rc;

	if( (rc = checkfile(key->fileid)) != S_OKAY )
		return rc;

	idx = DB->fh[key->fileid].key;
	rc = btree_del(idx, value, ref);
	btree_keyread(idx, CURR_KEYBUF);

	return rc;
}


int ty_recadd(rec, buf, recno)
Record *rec;
void *buf;
ulong *recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_add(DB->fh[rec->fileid].rec, buf, recno);
}


int ty_recwrite(rec, buf, recno)
Record *rec;
void *buf;
ulong recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_write(DB->fh[rec->fileid].rec, buf, recno);
}


int ty_recread(rec, buf, recno)
Record *rec;
void *buf;
ulong recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_read(DB->fh[rec->fileid].rec, buf, recno);
}


int ty_recdelete(rec, recno)
Record *rec;
ulong recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_delete(DB->fh[rec->fileid].rec, recno);
}


int ty_recfrst(rec, buf)
Record *rec;
void *buf;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_frst(DB->fh[rec->fileid].rec, buf);
}


int ty_reclast(rec, buf)
Record *rec;
void *buf;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_last(DB->fh[rec->fileid].rec, buf);
}


int ty_recnext(rec, buf)
Record *rec;
void *buf;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_next(DB->fh[rec->fileid].rec, buf);
}

int ty_recprev(rec, buf)
Record *rec;
void *buf;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_prev(DB->fh[rec->fileid].rec, buf);
}

ulong ty_reccount(rec, count)
Record *rec;
ulong *count;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_numrecords(DB->fh[rec->fileid].rec, count);
}

int ty_reccurr(rec, recno)
Record *rec;
ulong *recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return rec_curr(DB->fh[rec->fileid].rec, recno);	
}

int ty_vlradd(rec, buf, size, recno)
Record *rec;
void *buf;
unsigned size;
ulong *recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return vlr_add(DB->fh[rec->fileid].vlr, buf, size + rec->preamble, recno);
}

int ty_vlrwrite(rec, buf, size, recno)
Record *rec;
void *buf;
unsigned size;
ulong recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return vlr_write(DB->fh[rec->fileid].vlr, buf, size + rec->preamble, recno);
}


unsigned ty_vlrread(rec, buf, recno, size)
Record *rec;
void *buf;
ulong recno;
unsigned *size;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return vlr_read(DB->fh[rec->fileid].vlr, buf, recno, size);
}

int ty_vlrdel(rec, recno)
Record *rec;
ulong recno;
{
	int rc;

	if( (rc = checkfile(rec->fileid)) != S_OKAY )
		return rc;

	return vlr_del(DB->fh[rec->fileid].vlr, recno);
}

/* end-of-file */
