/*----------------------------------------------------------------------------
 * File    : sequence.c
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
 *   This file contains all the code that handles sequences.
 *
 * Functions:
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

static CONFIG_CONST char rcsid[] = "$Id: sequence.c,v 1.5 1999/10/03 23:28:29 kaz Exp $";

/*---------------------------- Global varibles ----------------------------*/
static int		seq_max = 0;
static ulong	*seq_tab = NULL;


/*----------------------------- sequence_open -----------------------------*\
 *
 * Purpose	 : Opens the sequence file.
 *
 * Parameters: db		- Pointer to db struct.
 *
 * Returns	 : -1		- Could not open sequence file.
 *			   0		- Successful.
 *
 */
int seq_open(db)
Dbentry *db;
{
    int isnew, i;
    char fname[128];

	sprintf(fname, "%ssequence.dat", db->dbfpath);

    /* if file exists then read header record, otherwise create */
    isnew = access(fname, 0);

    if( (db->seq_fh=os_open(fname, CONFIG_O_BINARY|O_RDWR|O_CREAT,CONFIG_CREATMASK)) == -1 )
    {
    	db_status = S_IOFATAL;
    	return -1;
    }

	if( db->header.sequences > seq_max )
	{
		void *ptr;
	
		if( !(ptr = (void *)realloc(seq_tab, sizeof(*seq_tab) * db->header.sequences)) )
		{
			close(db->seq_fh);
			db_status = S_NOMEM;
			return -1;
		}

		seq_tab = (ulong *)ptr;	
		seq_max = db->header.sequences;
	}

	if( isnew )
	{
		/* Initialize sequences */
		for( i=0; i<db->header.sequences; i++ )
			seq_tab[i] = db->sequence[i].start;

		write(db->seq_fh, seq_tab, sizeof(*seq_tab) * DB->header.sequences);
	}

	return 0;
}


/*----------------------------- sequence_close ----------------------------*\
 *
 * Purpose	 : Closes the sequence file.
 *
 * Parameters: db		- Pointer to db struct.
 *
 * Returns	 : 0		- Successful.
 *
 */
int seq_close(db)
Dbentry *db;
{
	close(DB->seq_fh);

	if( typhoon.dbs_open == 1 )
	{
		free(seq_tab);
		seq_tab = NULL;
		seq_max = 0;
	}
	
	return 0;
}


/*----------------------------- d_getsequence -----------------------------*\
 *
 * Purpose	 : Gets the next number in a sequence.
 *
 * Parameters: id		- Sequence id.
 *			   number	- Will contain number on return.
 *
 * Returns	 : S_NOCD	- No current database.
 *			   S_INVSEQ	- Invalid sequence id.
 *			   S_OKAY	- Succesful.
 *
 */
int d_getsequence(id, number)
Id id;
ulong *number;
{
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	if( id >= DB->header.sequences )
		RETURN_RAP(S_INVSEQ);

	ty_lock();

	lseek(DB->seq_fh, 0, SEEK_SET);
	read(DB->seq_fh, seq_tab, sizeof(*seq_tab) * DB->header.sequences);
	
	*number = seq_tab[id];
	
	if( DB->sequence[id].asc )
		seq_tab[id] += DB->sequence[id].step;
	else
		seq_tab[id] -= DB->sequence[id].step;

	lseek(DB->seq_fh, 0, SEEK_SET);
	write(DB->seq_fh, seq_tab, sizeof(*seq_tab) * DB->header.sequences);

	ty_unlock();

	RETURN S_OKAY;
}

/* end-of-file */
