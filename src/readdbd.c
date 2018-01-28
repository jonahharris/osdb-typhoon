/*----------------------------------------------------------------------------
 * File    : readdbd.c
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
 *   Contains a function that reads the dbd-file.
 *
 * Functions:
 *   read_dbdfile	- Read the database description file.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#  include <unistd.h>
#  include <stdio.h>
#  ifdef __STDC__
#    include <stdlib.h>
#  endif
#else
#  include <stdlib.h>
#  include <io.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"

static CONFIG_CONST char rcsid[] = "$Id: readdbd.c,v 1.9 1999/10/04 03:45:07 kaz Exp $";


/*------------------------------ read_dbdfile ------------------------------*\
 *
 * Purpose	 : Reads the dbd-file into a available database slot provided
 *			   by the calling function. The table pointers in the slot are
 *			   also set up.
 *
 * Parameters: _db			- Pointer to database entry slot.
 *			   fname		- The name of the dbd-file.
 *
 * Returns	 : S_OKAY		- dbd-file successfully read.
 *			   S_NOMEM		- Not enough memory to allocate tables.
 *			   S_INVDB		- Cannot find dbd-file.
 *			   S_IOFATAL	- The dbd-file is corrupted.
 *
 */

int read_dbdfile(_db, fname)
Dbentry *_db;
char *fname;
{
	int dbdfile, size;

	if( (dbdfile = os_open(fname, O_RDONLY|CONFIG_O_BINARY, 0)) == -1 )
		RETURN S_INVDB;

	/*
	 * Get the size of the dbd-file, read the header and expand to buffer
	 * to hold a table of file handles.
	 */
	size = lseek(dbdfile, 0, SEEK_END);
	lseek(dbdfile, 0, SEEK_SET);

	if( read(dbdfile, &_db->header, sizeof _db->header) < sizeof(_db->header) )
		RETURN S_IOFATAL;

	if( strcmp(_db->header.version, DBD_VERSION) )
		RETURN S_VERSION;

	size -= sizeof _db->header;

	if( !(_db->dbd = (void *)malloc(size + _db->header.files * sizeof(Fh))) )
	{
		close(dbdfile);
		RETURN S_NOMEM;
	}

	read(dbdfile, _db->dbd, (size_t) size);
	close(dbdfile);

	/* Set up pointers to tables */
	_db->file		= (File     *)_db->dbd;
	_db->key		= (Key      *)(_db->file	 + _db->header.files);
	_db->keyfield	= (KeyField *)(_db->key		 + _db->header.keys);
	_db->record 	= (Record   *)(_db->keyfield + _db->header.keyfields);
	_db->field		= (Field    *)(_db->record	 + _db->header.records);
	_db->structdef	= (Structdef*)(_db->field	 + _db->header.fields);
	_db->sequence	= (Sequence *)(_db->structdef+ _db->header.structdefs);
	_db->fh 		= (Fh       *)(_db->sequence + _db->header.sequences);

	return S_OKAY;
}


/* end-of-file */
