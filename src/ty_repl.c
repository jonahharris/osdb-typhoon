/*----------------------------------------------------------------------------
 * File    : ty_repl.c
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
 *   Contains functions for logging updates and deletions.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#else
#	include <sys\types.h>
#	include <sys\stat.h>
#	include <io.h>
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_repif.h"
#include "catalog.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_repl.c,v 1.6 1999/10/03 23:28:29 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/
static int	read_distables		PRM( (void); )
static void	add_recid			PRM( (Id); )
static int	get_recid			PRM( (Id); )
static void write_logentry		PRM( (LOGENTRY *, unsigned); )

/*---------------------------- Global variables ----------------------------*/
static int	 dis_dbid = -1;				/* Distributed database ID			*/
static int 	 dis_records = 0;			/* Number of entries in dis_record[]*/
static Id	 dis_record[100];


static int get_recid(id)
Id id;
{
	int i;
	
	for( i=0; i<dis_records; i++ )
		if( dis_record[i] == id )
			return i;

	return -1;
}


static void add_recid(id)
Id id;
{
	if( get_recid(id) == -1 )
		dis_record[dis_records++] = id;
}


static int read_distables()
{
	struct sys_distab table;
	int old_dbid;
	int rc;
	
	d_dbget(&old_dbid);

	if( d_open("catalog", "s") != S_OKAY )
	{
		d_dbset(old_dbid);
		return -1;
	}

	dis_records = 0;
	for( rc = d_recfrst(SYS_DISTAB); rc == S_OKAY; rc = d_recnext(SYS_DISTAB) )	
	{
		d_recread(&table);
		add_recid((Id) table.id);
	}

	d_close();
	d_dbset(old_dbid);

	return 0;
}


/*---------------------------- d_replicationlog ----------------------------*\
 *
 * Purpose	 : Turn the replication log on/off.
 *
 * Parameters: on		- 1=On, 0=Off
 *
 * Returns	 : S_OKAY	- The ID is not a key ID.
 *
 */
FNCLASS int d_replicationlog(on)
int on;
{
	DB->logging	= on;

	if( on )
	{
		if( read_distables() == -1 )
			RETURN S_IOFATAL;
		dis_dbid = CURR_DB;
	}
	
	RETURN S_OKAY;
}


FNCLASS int d_addsite(id)
ulong id;
{
	LOGENTRY entry;
	
	entry.action	= ACTION_NEWSITE;
	entry.u.site_id	= id;

	write_logentry(&entry, offsetof(LOGENTRY, u) + sizeof entry.u.site_id);
	
	RETURN S_OKAY;
}


FNCLASS int d_delsite(id)
ulong id;
{
	LOGENTRY entry;
	
	entry.action	= ACTION_DELSITE;
	entry.u.site_id	= id;

	write_logentry(&entry, offsetof(LOGENTRY, u) + sizeof entry.u.site_id);
	
	RETURN S_OKAY;
}


FNCLASS int d_deltable(site_id, table_id)
ulong site_id, table_id;
{
	LOGENTRY entry;
	
	entry.action	= ACTION_DELTABLE;
	entry.recid		= table_id;
	entry.u.site_id	= site_id;

	write_logentry(&entry, offsetof(LOGENTRY, u) + sizeof entry.u.site_id);
	
	RETURN S_OKAY;
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : ty_log
 *
 * Purpose   : Add an entry to the Replication Server log. If site_id == -1
 *			   ty_log is called to add or delete a site.
 *
 * Parameters: action	- 'u'=Update, 'd'=delete.
 *
 * Returns   : Nothing.
 *
 */
void ty_log(action)
int action;
{
	LOGENTRY entry;
	Id recid = INTERN_TO_RECID(CURR_RECID);
	ushort size, keysize;
	Record *rec = &DB->record[RECID_TO_INTERN(recid)];

	/* Return if the current database is not distributed */
	if( CURR_DB != dis_dbid )
		return;

	/* Return here if the record is not distributed */
	if( get_recid(recid) == -1 )
		return;
	
	entry.action= action;
	entry.recid	= recid;
	size		= offsetof(LOGENTRY, u);

	switch( action )
	{
		case ACTION_UPDATE:
			/* Store the database address of the modified record */
			size += sizeof(entry.u.addr);
			d_crget(&entry.u.addr);
			break;
		case ACTION_DELETE:
			/* Store the primary key of the deleted record */
			keysize = DB->key[ rec->first_key ].size;
			
			/* Copy the record's first key to <entry.buf> */
			memcpy(entry.u.key,
				   set_keyptr(&DB->key[ rec->first_key ], DB->recbuf),
				   keysize);
			size += keysize;
			break;
	}

	write_logentry(&entry, size);
}



static void write_logentry(entry, size)
LOGENTRY *entry;
unsigned size;
{
	int fh;
	short shsize;

	if( (fh=open(REPLLOG_NAME, CONFIG_O_BINARY|O_CREAT|O_RDWR|O_APPEND, CONFIG_CREATMASK)) == -1 )
	{
#ifdef CONFIG_UNIX
		printf("PANIC: cannot open '%s' (pid %d)\n\a", REPLLOG_NAME, getpid());
#else
		printf("PANIC: cannot open '%s'\n\a", REPLLOG_NAME);
#endif
		return;
	}

	shsize = size;
	write(fh, &shsize, sizeof shsize);
	write(fh, entry, size);

	close(fh);
}

/* end-of-file */
