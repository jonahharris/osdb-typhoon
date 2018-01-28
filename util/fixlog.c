/*----------------------------------------------------------------------------
 * File    : fixlog.c
 * Program : tyrestore
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
 *   Contains functions used to applying the log to the database after a
 *   restore.
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

static char rcsid[] = "$Id: fixlog.c,v 1.2 1999/10/03 23:36:49 kaz Exp $";

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <setjmp.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_log.h"



/*------------------------------- Constants --------------------------------*/
#define LOGBUF_SIZE		(64 * 1024)
#define RECHEAD_SIZE	offsetof(RECORDHEAD, data[0])
#define FILES_MAX		512


/*-------------------------- Function prototypes ---------------------------*/
static void		*ReadLog				PRM( (ulong); )
static void		insert_in_lru			PRM( (ulong); )
static void		delete_from_lru			PRM( (ulong); )
static void		update_in_lru			PRM( (ulong); )

/*---------------------------- Global variables ----------------------------*/
static char		*logbuf;				/* Log buffer						*/
static ulong	logbytes = 0;			/* Number of bytes in buffer		*/
static ulong	logreadpos = 0;			/* Read position in log buffer		*/
static int		log_fh;					/* Log file handle					*/
static Dbentry	dbd;					/* DBD-anchor						*/
extern jmp_buf	err_jmpbuf;
extern int		verbose;
extern char		*datapath;

static struct file_t {
	int			fh;
	unsigned	recsize;
	unsigned	blocksize;
	unsigned	preamble;
	int			prev_in_lru;
	int			next_in_lru;
} file[FILES_MAX];
static int		mru_file = -1;
static int		lru_file = -1;
static int		r_cur_open = 0;
static int		r_max_open = 40;


/*--------------------------------------------------------------------------*\
 *
 * Function  : ReadLog
 *
 * Purpose   : Returns a pointer to the next x bytes in the log.
 *
 * Parameters: bytes	- Number of bytes requested.
 *
 * Returns   : Nothing.
 *
 */
static void *ReadLog(bytes)
ulong bytes;
{
	ulong	numread;
	char	*p;

	if( bytes > logbytes - logreadpos )
	{
		memmove(logbuf, logbuf+logreadpos, logbytes - logreadpos);
		logbytes -= logreadpos;
		logreadpos = 0;

		numread = read(log_fh, logbuf + logbytes, LOGBUF_SIZE - logbytes);
		
		if( numread <= 0 )
			return NULL;
		
		logbytes += numread;
	}

	p = logbuf + logreadpos;
	logreadpos += bytes;

	return p;
}


static void insert_in_lru(id)
ulong id;
{
	struct file_t *f = file + id;

	f->prev_in_lru = -1;
	
	if( mru_file != -1 )
	{
		f->next_in_lru				= mru_file;
		file[mru_file].prev_in_lru	= id;
	}
	else
	{
		lru_file		= id;
		f->prev_in_lru	= -1;
	}
	
	mru_file = id;
}

static void delete_from_lru(id)
ulong id;
{
	struct file_t *f = file + id;

    if( f->next_in_lru != -1 )
		file[f->next_in_lru].prev_in_lru = f->prev_in_lru;
                    
	if( f->prev_in_lru != -1 )
		file[f->prev_in_lru].next_in_lru = f->next_in_lru;
                                     
	if( mru_file == id )
		mru_file = f->next_in_lru;
                                                      
	if( lru_file == id )
		lru_file = f->prev_in_lru;
}


static void update_in_lru(id)
ulong id;
{
	delete_from_lru(id);
	insert_in_lru(id);
}



static void CheckFile(fileid)
ulong fileid;
{
	char fname[128];

	if( file[fileid].fh == -1 )
	{
		if( r_cur_open == r_max_open )
		{
			/* Close least recently used file */
			close(file[lru_file].fh);
			file[lru_file].fh = -1;
			
			delete_from_lru(lru_file);
			r_cur_open--;
		}

		sprintf(fname, "%s/%s", datapath, dbd.file[fileid].name);		
		if( (file[fileid].fh = os_open(fname, O_RDWR, 0)) == -1 )
		{
			printf("Cannot open '%s' (errno %d)\n", dbd.file[fileid].name, errno);
			longjmp(err_jmpbuf, 1);
		}
		
		insert_in_lru(fileid);
		r_cur_open++;
	}
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : FixLog
 *
 * Purpose   : Copies all the updates from the log to the database tables.
 *
 * Parameters: dbname		- Database name.
 *
 * Returns   : Nothing.
 *
 */
void FixLog(dbname)
char *dbname;
{
	ushort		*logblock;
	char		dbdname[DBNAME_LEN+5];
	unsigned	i, fileid;
	RECORDHEAD	recordhd = { 0 };
	char		fname[128];

	/* Open log file */
	if( (log_fh = open(LOG_FNAME, O_RDONLY)) == -1 )
	{
		if( verbose )
			puts("No log");
		return;
	}

	if( verbose )
		printf("Fixing log");

	/* Read dbd-file */
	sprintf(dbdname, "%s.dbd", dbname);
	if( read_dbdfile(&dbd, dbdname) != S_OKAY )
	{
		printf("Cannot open '%s'\n", dbdname);
		longjmp(err_jmpbuf, 1);
	}

	/* Allocate log buffer */	
	if( !(logbuf = (char *)malloc(LOGBUF_SIZE)) )
	{
		puts("out of memory");
		longjmp(err_jmpbuf, 1);
	}

	for( i=0; i<dbd.header.records; i++ )
	{
		ulong foreign_keys, preamble, datasize;
		Record *rec = dbd.record + i;

		if( rec->first_foreign == -1 )
			foreign_keys = 0;
		else
			foreign_keys = rec->keys - (rec->first_foreign - rec->first_key);
		preamble= sizeof(long) * foreign_keys;

		fileid = dbd.record[i].fileid;
		file[fileid].fh				= -1;
		file[fileid].next_in_lru	= -1;
		file[fileid].recsize		= dbd.record[i].size;
		file[fileid].blocksize		= dbd.record[i].size + RECHEAD_SIZE + 
									  preamble;
		file[fileid].preamble		= preamble;
	}

	while( logblock = (ushort *)ReadLog(sizeof *logblock) )
	{
		switch( *logblock )
		{
			case LOG_UPDATE: {
				LogUpdate	*update = (void *)logblock;

				/* Skip rest of LogUpdate structure and record */
				ReadLog(update->len - sizeof(update->id));

				fileid = dbd.record[update->recid].fileid;
				CheckFile(fileid);

				recordhd.flags = 0;

				lseek(file[fileid].fh, update->recno * file[fileid].blocksize, SEEK_SET);
				write(file[fileid].fh, &recordhd, RECHEAD_SIZE);
				
				lseek(file[fileid].fh, file[fileid].preamble, SEEK_CUR);
				write(file[fileid].fh, update+1, file[fileid].recsize);
				}
				break;
			
			case LOG_DELETE: {
				LogDelete	*delete = (void *)logblock;

				/* Skip rest of LogDelete structure */
				ReadLog(delete->len - sizeof(delete->id));

				fileid = dbd.record[delete->recid].fileid;
				CheckFile(fileid);

				recordhd.flags = BIT_DELETED;

				lseek(file[fileid].fh, delete->recno * file[fileid].blocksize, SEEK_SET);
				write(file[fileid].fh, &recordhd, RECHEAD_SIZE);
				}
				break;
		}	
	}

	close(log_fh);
	free(logbuf);
	free(dbd.dbd);

	if( verbose )
		puts("");
}

/* end-of-file */
