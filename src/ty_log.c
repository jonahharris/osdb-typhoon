/*----------------------------------------------------------------------------
 * File    : ty_log.c
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
 *   Contains function that write to the Replication Server log.
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_log.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_log.c,v 1.6 1999/10/04 03:45:08 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/
static int do_log		PRM( (ulong, ulong); )


/*---------------------------- Global variables ----------------------------*/
static int log_fh = -1;			/* Log file handle							*/
static int log_trans = 0;		/* True if the current transaction was		*/
								/* started during backup					*/

/*--------------------------------------------------------------------------*\
 *
 * Function  : do_log
 *
 * Purpose   : This function checks whether an operation should be logged or
 *			   not. If yes, the log file is opened if not already open.
 *			   If the backup is no longer active, the log file is closed.
 *
 * Parameters: 
 *
 * Returns   : 
 *	
 */			
static int do_log(recid, recno)
ulong recid, recno;
{
	if( !DB->shm->backup_active && !log_trans )
	{
		if( log_fh != -1 )
		{
			close(log_fh);
			log_fh = -1;
		}
		return -1;
	}

	if( recid > DB->shm->curr_recid && recno > DB->shm->curr_recno )
		return -1;
	
	if( log_fh == -1 )
	{
		log_fh = os_open(LOG_FNAME, O_RDWR|O_APPEND|O_CREAT, CONFIG_CREATMASK);
		
		if( log_fh == -1 )
		{
			puts("cannot open log");
			return -1;
		}
	}
	
	return 0;
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : Log a record update.
 *
 * Purpose   : 
 *
 * Parameters: 
 *
 * Returns   : 
 *	
 */			
int log_update(recid, recno, size, data)
ulong recid;
ulong recno;
unsigned size;
void *data;
{
	LogUpdate update;

	if( do_log(recid, recno) == -1 )
		return 0;

	os_lock(log_fh, 0, 1, 'u');

#ifdef CONFIG_RISC
	if( size & (sizeof(long)-1) )
		size += sizeof(long) - (size & (sizeof(long)-1));
#endif

	update.id		= LOG_UPDATE;
	update.len		= size + sizeof update;
	update.recid	= recid;
	update.recno	= recno;

	write(log_fh, &update, sizeof update);
	write(log_fh, data, size);

	os_unlock(log_fh, 0, 1);

	return 0;
}


int log_delete(recid, recno)
ulong recid, recno;
{
	LogDelete delete;

	if( do_log(recid, recno) == -1 )
		return 0;

	os_lock(log_fh, 0, 1, 'u');

	delete.id		= LOG_UPDATE;
	delete.len		= sizeof delete;
	delete.recid	= recid;
	delete.recno	= recno;

	write(log_fh, &delete, sizeof delete);

	os_unlock(log_fh, 0, 1);

	return 0;
}

#if UNUSED_CODE
/*--------------------------------------------------------------------------*\
 *
 * Function  : 
 *
 * Purpose   : 
 *
 * Parameters: 
 *
 * Returns   : 
 *
 */
FNCLASS int d_beginwork()
{
	if( DB->shm->backup_active )
	{
		DB->shm->num_trans_active++;
		log_trans = 1;		
	}

	return S_OKAY;
}


FNCLASS int d_commitwork()
{
	if( log_trans )
	{
		log_trans = 0;
		DB->shm->num_trans_active--;
	}

	return S_OKAY;
}

#endif


FNCLASS int d_abortwork()
{
	if( log_trans )
	{
		log_trans =0 ;
		DB->shm->num_trans_active--;
	}		

	return S_OKAY;
}

/* end-of-file */
