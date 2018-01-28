/*----------------------------------------------------------------------------
 * File    : ty_open.c
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
 *
 * Functions:
 *   Contains functions for opening and closing the database.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#  include <unistd.h>
#  include <stdio.h>
#  ifdef __STDC__
#    include <stdlib.h>
#  endif
#  define DIR_SWITCH	'/'
#else
#  include <stdlib.h>
#  include <io.h>
#  define DIR_SWITCH	'\\'
#endif
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_log.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_open.c,v 1.8 1999/10/04 03:45:08 kaz Exp $";

/*--------------------------- Function prototypes --------------------------*/
static void	fixpath			PRM( (char *, char *); )
	   int  read_dbdfile	PRM( (Dbentry *, char *); )
static int  perform_rebuild PRM( (unsigned); )


static void (*rebuildverbose_fn) PRM((char *, ulong, ulong);)


static void fixpath(path, s)
char *path, *s;
{
	int len = strlen(s);

	if( len > 0 )
	{
		strcpy(path, s);

		if( path[len-1] != DIR_SWITCH )
		{
			path[len++] = DIR_SWITCH;
			path[len] = 0;
		}
	}
	else
	{
		path[0] = '.';
		path[1] = DIR_SWITCH;
		path[2] = 0;
	}
}



/*-------------------------------- d_dbdpath -------------------------------*\
 *
 * Purpose	 : Set the path of the dbd-file. This function is called prior
 *			   d_open().
 *
 * Parameters: path		- The path of the dbd-file.
 *
 * Returns	 : S_OKAY	- Successful.
 *
 */


FNCLASS int d_dbdpath(path)
char *path;
{
	fixpath(typhoon.dbdpath, path);

	return S_OKAY;
}



/*-------------------------------- d_dbfpath -------------------------------*\
 *
 * Purpose	 : Set the path of the database files. This function is called
 *			   prior to d_open().
 *
 * Parameters: path		- The path of the database files.
 *
 * Returns	 : S_OKAY	- Successful.
 *
 */


FNCLASS int d_dbfpath(path)
char *path;
{
	fixpath(typhoon.dbfpath, path);

	return S_OKAY;
}



/*--------------------------------- d_dbget --------------------------------*\
 *
 * Purpose	 : Gets the current database ID.
 *
 * Parameters: id		- Pointer to database ID.
 *
 * Returns	 : S_OKAY	- The database ID is stored in <id>.
 *			   S_NOCD	- There is no current database.
 *
 */

FNCLASS int d_dbget(id)
int *id;
{
	if( CURR_DB == -1 )
		RETURN S_NOCD;

	*id = CURR_DB;

	RETURN S_OKAY;
}



/*--------------------------------- d_dbset --------------------------------*\
 *
 * Purpose	 : Sets the current database ID.
 *
 * Parameters: id		- Database ID obtained by call to d_dbget().
 *
 * Returns	 : S_OKAY	- The database ID is stored in <id>.
 *			   S_INVDB	- Invalid database ID.
 *
 */

FNCLASS int d_dbset(id)
int id;
{
	/* Ensure that the id is valid */
	if( id < 0 || id >= DB_MAX )
		RETURN S_INVDB;

	/* Ensure that the database is actually open */
	if( typhoon.dbtab[id].clients == 0 )
		RETURN S_INVDB;

	DB->db_status = db_status;

	typhoon.curr_db = id;
	typhoon.db = typhoon.dbtab + id;

	db_status = DB->db_status;

	RETURN S_OKAY;
}


/*------------------------------- d_setfiles -------------------------------*\
 *
 * Purpose	 : Set the maximum number of open files.
 *
 * Parameters: maxfiles	- The maximum number of open files allowed.
 *
 * Returns	 : S_OKAY	- Ok.
 * 			   S_INVPARM- The maximum is invalid.
 *
 */
FNCLASS int d_setfiles(maxfiles)
int maxfiles;
{
	if( maxfiles < 2 )
		RETURN S_INVPARM;

	if( maxfiles < typhoon.max_open )
	{
		/* maxfiles is less than max_open, so we need minimize the number
		 * of open files right away.
		 */
		int diff = typhoon.max_open - maxfiles;

		while( typhoon.cur_open > maxfiles && diff-- )
			ty_closeafile();

		/* If it was not possible to closed the required number of files
		 * anyway, an error code is returned
		 */
		if( typhoon.cur_open > maxfiles )
			RETURN S_INVPARM;
	}

	typhoon.max_open = maxfiles;
	
	RETURN S_OKAY;
}



FNCLASS int d_keybuild(fn)
void (*fn)PRM((char *, ulong, ulong);)
{
	typhoon.do_rebuild = 1;
	rebuildverbose_fn = fn;
	return S_OKAY;
}


static int perform_rebuild(biggest_rec)
unsigned biggest_rec;
{
	ulong		recno;
	ulong		recid;
	ulong		datasize;
	ulong		records;
	char		*buf;
	int 		fh, i;
	Record		*rec;
	RECORD		filehd;
	char		fname[128];
	int			preamble;
	int			foreign_keys;
	
	if( (buf = (void *)malloc(biggest_rec)) == NULL )
		RETURN S_NOMEM;

	for( i=0, rec=DB->record; i<DB->header.records; i++, rec++ )
	{
		sprintf(fname, "%s%c%s_tmp", DB->dbfpath, DIR_SWITCH, 
			DB->file[rec->fileid].name);

		fh = os_open(fname, O_RDWR|CONFIG_O_BINARY, 0);
		read(fh, &filehd.H, sizeof filehd.H);

		recno 	= (sizeof(filehd.H) + filehd.H.recsize - 1) / filehd.H.recsize;
		recid 	= INTERN_TO_RECID(i);
		records	= lseek(fh, 0, SEEK_END) / filehd.H.recsize;

		rebuildverbose_fn(rec->name, records, 0);

		if( rec->first_foreign == -1 )
			foreign_keys = 0;
		else
			foreign_keys = rec->keys - (rec->first_foreign - rec->first_key);
		preamble= sizeof(long) * foreign_keys + offsetof(RECORDHEAD, data[0]);
		datasize= filehd.H.recsize - preamble;

		for( ;; )
		{
			lseek(fh, (off_t) (filehd.H.recsize * recno + preamble), SEEK_SET);
			if( read(fh, buf, datasize) != datasize )
				break;
			
			if( d_fillnew(recid, buf) != S_OKAY )
				printf("%s: d_fillnew failed\n", rec->name);

			recno++;

			rebuildverbose_fn(rec->name, records, recno);
		}
		
		close(fh);
		unlink(fname);

		rebuildverbose_fn(rec->name, records, records);
	}		
	free(buf);

	RETURN S_OKAY;
}



/*--------------------------------- d_open ---------------------------------*\
 *
 * Purpose	 : Opens a database of the name <dbname>. The dbd-file is
 *			   expected to be in <dbdpath> and the data and index file are
 *			   expected to be in (or will be created in) <dbfpath>, set by
 *			   d_dbdpath() and d_dbfpath().
 *
 *			   If the database is opened in exclusive or one-user mode the
 *			   first file is locked. This will make future calls to d_open()
 *			   return S_UNAVAIL.
 *
 * Parameters: dbname		- Database name.
 *			   mode			- [s]hared, e[x]clusive or [o]ne user mode.
 *
 * Returns	 : S_OKAY		- Database successfully opened.
 *			   S_NOTAVAIL	- Database is currently not available.
 *			   S_NOMEM		- Not enough memory to open database.
 *			   S_INVDB		- Invalid database name.
 *			   S_BADTYPE	- The mode parmeter was invalid.
 *
 */

FNCLASS int d_open(dbname, mode)
char *dbname, *mode;
{
	char fname[129];
	Record *rec;
	int i, n;
	unsigned biggest_rec = 0;
	Dbentry	*_db;

	db_status = S_OKAY;

	/* Validate the mode parameter */
    if( *mode != 's' && *mode != 'x' && *mode != 'o' )
    	RETURN_RAP(S_BADTYPE);

	/* Initialize locking resource */
	if( ty_openlock() == -1 )
		RETURN S_FATAL;

	ty_lock();

	/* Find an available database slot */
	for( i=0, _db=typhoon.dbtab; i<DB_MAX; i++, _db++ )
		if( !_db->clients )
			break;

	if( i == DB_MAX )
	{
		ty_unlock();
		RETURN S_NOTAVAIL;
	}

	DB = _db;
    DB->mode	 = *mode;
    strcpy(DB->name, dbname);
	strcpy(DB->dbfpath, typhoon.dbfpath);

	/* dbd-file is located in <dbdpath> */
	sprintf(fname, "%s%s.dbd", typhoon.dbdpath, dbname);
	if( read_dbdfile(DB, fname) != S_OKAY )
    {
    	ty_unlock();
    	return db_status;
    }

	/* Open sequence file */
	if( seq_open(_db) == -1 )
	{
		ty_unlock();
		return db_status;
	}

	/* Allocate 'current record' buffer (add room for record numbers of
	 * parent records (one per foreign key)
	 */
	for( i=0, rec=DB->record; i<DB->header.records; i++, rec++ )
	{
		if( rec->first_foreign == -1 )
			n = rec->size;
		else
			n = rec->size + (rec->keys - (rec->first_foreign - rec->first_key))
            			  * sizeof(ulong);

		if( n > biggest_rec )
			biggest_rec = n;
	}

	if( (DB->real_recbuf = (char *)malloc(biggest_rec)) == NULL )
	{
		seq_close(DB);
		ty_unlock();
		free(DB->dbd);
		RETURN S_NOMEM;
	}

#ifdef CONFIG_UNIX
	if( shm_alloc(DB) == -1 )
	{
		seq_close(DB);
		ty_unlock();
		free(DB->dbd);
		RETURN S_NOMEM;
	}

	/* The database cannot be opened during a restore (except for rebuild) */
	if( DB->shm->restore_active && !typhoon.do_rebuild )
	{
		seq_close(DB);
		ty_unlock();
		free(DB->dbd);
		shm_free(DB);
		RETURN S_NOTAVAIL;
	}
#endif

	DB->recbuf = DB->real_recbuf;
    DB->clients++;

	/* If the indices should be rebuilt we'll remove them first */
	if( typhoon.do_rebuild )
	{
		char fname[256];
		char new_fname[256];

		for( i=0; i<DB->header.files; i++ )
			if( DB->file[i].type == 'k' )
			{
				sprintf(fname, "%s%c%s", typhoon.dbfpath, DIR_SWITCH, DB->file[i].name);
				unlink(fname);
			}
			else
			{
				sprintf(fname, "%s%c%s", typhoon.dbfpath, DIR_SWITCH, DB->file[i].name);
				sprintf(new_fname, "%s_tmp", fname);
				unlink(new_fname);
#ifndef	CONFIG_UNIX
				rename(fname, new_fname);
#else
				if (link(fname, new_fname) == 0)
					if (unlink(fname) != 0)
						unlink(new_fname);
#endif
			}
	}

	/* Before opening the database we mark all file handles are closed */
	for( i=0; i<DB->header.files; i++ )
		DB->fh[i].any = NULL;

	/* Open all the database files */
	for( i=0; i<DB->header.files && db_status == S_OKAY; i++ )
		ty_openfile(DB->file + i, DB->fh + i, *mode == 's');

    /* Roll back if a file could not be opened */
    if( db_status != S_OKAY )
    {
		i--;
    	while( i-- )
			ty_closefile(DB->fh + i);

		DB->clients--;
		CURR_DB = -1;
#ifdef CONFIG_UNIX
		shm_free(DB);
#endif
		free(DB->real_recbuf);
		free(DB->dbd);
		seq_close(DB);
		ty_unlock();
		RETURN db_status;
    }

    CURR_DB  		= _db - typhoon.dbtab;
    CURR_REC		= 0;
	CURR_RECID		= 0;
	CURR_BUFREC		= 0;
	CURR_BUFRECID	= 0;

	if( typhoon.do_rebuild )
	{
		perform_rebuild(biggest_rec);
		typhoon.do_rebuild = 0;
	}

	typhoon.dbs_open++;
	ty_unlock();

	/* Return the status of the last db_open command */
	return db_status;
}



/*--------------------------------- d_close --------------------------------*\
 *
 * Purpose	 : Close the current database.
 *
 * Parameters: None.
 *
 * Returns	 : S_NOCD		- No current database.
 *			   S_OKAY		- Database successfully closed.
 *
 */

FNCLASS int d_close()
{
	int i;

    if( CURR_DB == -1 )
        RETURN_RAP(S_NOCD);

	ty_lock();

	DB->clients--;

	for( i=0; i<DB->header.files; i++ )
		ty_closefile(DB->fh + i);

	seq_close(DB);

	/* If there is currently a transaction active the transaction counter
	 * in shared memory must be decremented.
	 */
#ifdef CONFIG_UNIX
	d_abortwork();
	shm_free(DB);
#endif
	FREE(DB->dbd);
	FREE(DB->real_recbuf);

	CURR_DB  = -1;
    CURR_REC = 0;

	ty_unlock();

	if( !--typhoon.dbs_open )
		ty_closelock();

	RETURN S_OKAY;
}



/*-------------------------------- d_destroy -------------------------------*\
 *
 * Purpose	 : Destroy a database.
 *
 * Parameters: dbname		- Database name.
 *
 * Returns	 : S_NOMEM		- Not enough memory to open database and destroy.
 *			   S_INVDB		- Invalid database name.
 *			   S_OKAY		- Database successfully closed.
 *			   S_NOTAVAIL	- The database is already opened in non-shared
 *							  mode.
 *
 *
 *        INSERT LOCK CHECK HERE!!!!!
 *
 *
 */

FNCLASS int d_destroy(dbname)
char *dbname;
{
	int i, dbdfile;
    char fname[80];
    Header   header;
    File    *file;
    Dbentry *_db = typhoon.dbtab;

	ty_lock();

    for( i=0; i<DB_MAX; i++, _db++ )        /* Find database name in table  */
        if( !strcmp(_db->name, dbname) )
            break;

	if( i == DB_MAX )
	{										/* Database currently not open	*/
		sprintf(fname, "%s%s.dbd", typhoon.dbdpath, dbname);

        if( (dbdfile = open(fname, CONFIG_O_BINARY|O_RDONLY)) == -1 )
		{
			ty_unlock();
            RETURN S_INVDB;                 /* Database name not found      */
		}
        read(dbdfile, &header, sizeof header);

		if( !(file = (File *)malloc(sizeof(File) * header.files)) )
		{
			close(dbdfile);
			ty_unlock();
			RETURN S_NOMEM;
		}

		read(dbdfile, file, sizeof(File) * header.files);
        close(dbdfile);

        for( i=0; i < header.files; i++ )   /* Now remove all files         */
            unlink(file[i].name);

		ty_unlock();
        RETURN S_OKAY;
    }

    /*---------- Destroy current database ----------------------------------*/
	for( i=0; i<DB->header.files; i++ )
    {
		ty_closefile(DB->fh + i);           /* Close all database files     */
        unlink(DB->file[i].name);           /* and remove them              */
    }

	FREE(DB->dbd);

	_db->clients = 0;
    CURR_DB  = -1;
    CURR_REC = 0;

	ty_unlock();
	RETURN S_OKAY;
}

/* end-of-file */
