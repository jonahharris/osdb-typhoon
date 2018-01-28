/*----------------------------------------------------------------------------
 * File    : ty_ins.c
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
 *   Contains API functions.
 *
 * Functions:
 *   report_err	   	- Report an error to the user.
 *   d_keyread	   	- Read the value of the last retrieved key.
 *   d_keyfind	   	- Find a key.
 *   d_keymove	   	- Perform a d_keyfrst(), d_keylast(), d_keyprev() or
 *				   	  d_keynext().
 *   d_recmove	   	- Perform a d_recfrst(), d_reclast(), d_recprev() or
 *				   	  d_recnext().
 *   d_crread	   	- Read the value of a field of the current record.
 *   d_recwrite		- Update a record.
 *   d_recread		- Read the current record.
 *   d_fillnew		- Add a new record to the database.
 *   d_delete		- Delete the current record.
 *   d_crget		- Get the database address of the current record.
 *   d_crset		- Set the database address of the current record.
 *   d_records		- Return the number of records in a file.
 *   d_getkeysize	- Return the size of a key.
 *   d_getrecsize	- Return the size of a record.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#define DEFINE_GLOBALS
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_log.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_ins.c,v 1.7 1999/10/03 23:28:29 kaz Exp $";

int report_err(v)
int v;
{
	if( typhoon.ty_errfn )
		typhoon.ty_errfn(db_status, db_subcode);
	else
	{
#ifdef CONFIG_UNIX
		printf("** pid %d - db_status = %d **\n", getpid(), db_status = v);
#endif
#ifdef CONFIG_OS2
		printf("** db_status = %d **\n", db_status = v);
#endif
	}

	return v;
}



/*--------------------------------- d_block ---------------------------------*\
 *
 * Purpose	 : Requests exclusive access to the API. The second time another
 *             thread in a program calls this function it will block, until
 *             d_unblock() is called.
 *
 * Parameters: None.
 *
 * Returns	 : S_OKAY	- Ok.
 *
 */

FNCLASS int d_block()
{
#ifdef CONFIG_OS2
	os2_block();
#endif

	RETURN S_OKAY;
}



FNCLASS int d_unblock()
{
#ifdef CONFIG_OS2
	os2_unblock();
#endif

	RETURN S_OKAY;
}


/*------------------------------- d_recwrite -------------------------------*\
 *
 * Purpose	 : Updates the contents of the current record.
 *
 * Parameters: buf			- Buffer containing current record.
 *
 * Returns	 : S_OKAY		- Operation performed successfully.
 *			   S_DUPLICATE	- The record contained a duplicate key. db_subcode
 *							  contains the id of the conflicting field or key.
 *			   S_NOCD		- No current database.
 *			   S_NOCR		- No current record.
 *             S_RECSIZE    - Invalid record size (if variable size). 
 *						      db_subcode contains the ID of the size field.
 *			   S_FOREIGN	- A foreign key was not found (db_subcode holds
 *							  the foreing key ID).
 *			   S_RESTRICT	- The primary key was updated, but a dependent
 *							  table with restrict rule had a record which 
 *							  referenced the record to be deleted. 
 *							  (db_subcode holds the foreign key ID).
 *
 */
FNCLASS int d_recwrite(buf)
void *buf;
{
    Record *rec;
	Key *key;
	Key *keyptr[RECKEYS_MAX];
	int keys_changed=0, rc;
	int n;
	ulong ref;

	/* Set pointers to current record and first field */
	if( (rc = set_recfld((Id) -1, &rec, NULL)) != S_OKAY )
	   	return rc;

	ty_lock();
    if( (rc = update_recbuf()) != S_OKAY )
    {
    	ty_unlock();
    	return rc;
    }

	/* Check foreign keys (if any) */
	if( (rc = check_foreign_keys(rec, buf, 0)) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}

	/* Check dependent tables (if any) */
	if( (rc = check_dependent_tables(rec, buf, 'u')) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}

	/* Have any keys changed? */
	key = DB->key + rec->first_key;

	/* Find out which keys have changed and must be updated. An optional key
	 * that has changed from null to not null, or not null to null, or 
	 * has changed its value is regarded as changed.
	 */

	for( n = rec->keys; n-- && KT_GETBASIC(key->type) != KT_FOREIGN; key++ )
	{
		if( reckeycmp(key, buf, DB->recbuf) )
		{
			keyptr[keys_changed++] = key;

			if( key->type & KT_UNIQUE )
			{
				if( (key->type & KT_OPTIONAL) && null_indicator(key, buf) )
					continue;
			
				if( keyfind(key, buf, &ref) == S_OKAY )
				{
					set_subcode(key);
					ty_unlock();
					RETURN S_DUPLICATE;
				}
	        }
		}
    }

	for( n=0; n<keys_changed; n++ )
	{
		key = keyptr[n];

		if( reckeycmp(key, buf, DB->recbuf) )
		{
			/* Don't remove null keys */
			if( !(key->type & KT_OPTIONAL) || !null_indicator(key, DB->recbuf) )
				keydel(key, DB->recbuf, CURR_REC);

			/* Don't insert null keys */
			if( (key->type & KT_OPTIONAL) && null_indicator(key, buf) )
				continue;

			if( (rc = keyadd(key, buf, CURR_REC)) != S_OKAY )
			{
				set_subcode(key);
				ty_unlock();
				RETURN rc;
			}
		}
	}

	if( rec->is_vlr )
	{
		unsigned size;
	
		if( (rc = compress_vlr(COMPRESS, rec, DB->recbuf, buf, &size)) != S_OKAY )
		{
			ty_unlock();
			return rc;
		}

		ty_vlrwrite(rec, DB->real_recbuf, size, CURR_REC);
	}
	else
	{
		memcpy(DB->recbuf, buf, rec->size);

		ty_recwrite(rec, DB->real_recbuf, CURR_REC);
	}

	/* Store changed references to parent records */
	update_foreign_keys(rec, 0);

#ifdef CONFIG_UNIX
	if( DB->logging )
		ty_log('u');

	log_update(CURR_RECID, CURR_REC, rec->size, buf);
#endif

	ty_unlock();

	RETURN S_OKAY;
}



/*-------------------------------- d_recread -------------------------------*\
 *
 * Purpose	 : Read contents of the current record.
 *
 * Parameters: buf			- Buffer containing current record.
 *
 * Returns	 : S_OKAY		- Operation performed successfully.
 *			   S_DELETED	- Record has been deleted since last accessed.
 *			   S_NOCD		- No current database.
 *			   S_NOCR		- No current record.
 *
 */

FNCLASS int d_recread(buf)
void *buf;
{
	Record *rec;
	int rc;

	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	if( CURR_REC == 0 )
		RETURN_RAP(S_NOCR);

	ty_lock();
	rec = DB->record + CURR_RECID;

	if( (rc = update_recbuf()) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}
	
	if( rec->is_vlr )
		rc = compress_vlr(UNCOMPRESS, rec, buf, DB->recbuf, NULL);
	else
	{
		memcpy(buf, DB->recbuf, rec->size);
		rc = S_OKAY;
	}

	ty_unlock();

	RETURN rc;
}



/*------------------------------- d_fillnew --------------------------------*\
 *
 * Purpose	 : Adds a new record to the current database and updates indexes.
 *
 * Parameters: record		- Record id.
 *			   buf			- Pointer to record buffer.
 *
 * Returns	 : S_OKAY		- Ok.
 *			   S_NOCD		- No current database.
 *			   S_INVREC 	- Invalid record id.
 *             S_DUPLICATE  - The record contained a duplicate key. The id
 *                            of the field or compound key is stored in
 *                            db_subcode.
 *             S_RECSIZE    - Invalid record size (if variable size). 
 *						      db_subcode contains the ID of the size field.
 *			   S_FOREIGN	- A foreign key was not found (db_subcode holds
 *							  the foreing key ID.
 *
 */

FNCLASS int d_fillnew(record, buf)
Id record;
void *buf;
{
	Record *rec;
	Field *fld;
    Key *key;
	ulong ref;
	int rc, n;

	if( (rc = set_recfld(record, &rec, &fld)) != S_OKAY )
		return rc;

	/* So far we have no current record */
	CURR_REC = 0;

	ty_lock();

	/* Set pointer to actual data */
	DB->recbuf = DB->real_recbuf + rec->preamble;

	/* Check foreign keys (if any) */
	if( (rc = check_foreign_keys(rec, buf, 1)) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}

    /* Make sure that there are no duplicate keys in this record */
    key = DB->key + rec->first_key;

	for( n = rec->keys; n-- && !KEY_ISFOREIGN(key); key++ )
	{
        if( key->type & KT_UNIQUE )
		{
			if( KEY_ISOPTIONAL(key) && null_indicator(key, buf) )
				continue;

			if( keyfind(key, buf, &ref) == S_OKAY )
			{
				set_subcode(key);
				ty_unlock();
				RETURN S_DUPLICATE;
			}
        }
    }

	/* Update data and index files */
	if( rec->is_vlr )
	{
		unsigned size;
	
		if( (rc = compress_vlr(COMPRESS, rec, DB->recbuf, buf, &size)) != S_OKAY )
		{
			ty_unlock();
			return rc;
		}

		if( (rc = ty_vlradd(rec, DB->real_recbuf, size, &CURR_REC)) != S_OKAY )
		{
			ty_unlock();
			return rc;
		}

		/* The record in DB->recbuf is still compressed */
	}
	else
	{
		memcpy(DB->recbuf, buf, rec->size);
		if( (rc=ty_recadd(rec, DB->real_recbuf, &CURR_REC)) != S_OKAY )
		{
			ty_unlock();
			return rc;
		}
	}

	CURR_RECID	= rec - DB->record;
	n			= rec->keys;
	key 		= DB->key + rec->first_key;

	for( n=rec->keys; n-- && !KEY_ISFOREIGN(key); key++ )
	{
		/* Don't store null keys */
		if( KEY_ISOPTIONAL(key) && null_indicator(key, buf) )
			continue;

		if( (rc = keyadd(key, buf, CURR_REC)) != S_OKAY )
		{
			ty_unlock();
			RETURN rc;
		}
	}

	/* Store references to parent records */
	update_foreign_keys(rec, 1);

#ifdef CONFIG_UNIX
	if( DB->logging )
		ty_log('u');

	log_update(CURR_RECID, CURR_REC, rec->size, buf);
#endif

	ty_unlock();

	RETURN S_OKAY;
}


/*-------------------------------- d_delete --------------------------------*\
 *
 * Purpose	 : Delete the current record.
 *
 * Parameters: None.
 *
 * Returns	 : S_OKAY		- The was successfully deleted.
 *			   S_NOCD		- No current database.
 *			   S_NOCR		- No current record.
 *			   S_RESTRICT	- A dependent table had a foreign key which
 *							  referenced the primary key of the record to
 *							  to be deleted. db_subcode holds the foreign
 *							  key ID.
 *
 */

FNCLASS int d_delete()
{
    Record *rec;
	Key *key;
	int n, rc;

	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

    if( CURR_REC == 0 )
        RETURN_RAP(S_NOCR);

	/* We must update recbuf in order to access the record's keys */
	ty_lock();

	rec = DB->record + CURR_RECID;
	DB->recbuf = DB->real_recbuf + rec->preamble;

	if( (rc = update_recbuf()) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}

	/* Check dependent tables (if any) */
	if( (rc = check_dependent_tables(rec, DB->recbuf, 'd')) != S_OKAY )
	{
		ty_unlock();
		return rc;
	}

	key = DB->key + rec->first_key;

	if( DB->fh[rec->fileid].any->type == 'd' )
		rc = ty_recdelete(rec, CURR_REC);
	else
		rc = ty_vlrdel(rec, CURR_REC);

	if( rc != S_OKAY )
	{
		ty_unlock();
		RETURN rc;
	}

	for( n=rec->keys; n-- && !KEY_ISFOREIGN(key); key++ )
	{
		if( KEY_ISOPTIONAL(key) && null_indicator(key, DB->recbuf) )
			continue;
	
		if( (rc = keydel(key, DB->recbuf, CURR_REC)) != S_OKAY )
		{
			printf("typhoon: could not delete key %s.%s (db_status %d)\n",
				rec->name, key->name, rc);
			ty_unlock();
			RETURN rc;
		}
	}

	delete_foreign_keys(rec);

#ifdef CONFIG_UNIX
	if( DB->logging )
		ty_log('d');

	log_delete(CURR_RECID, CURR_REC);
#endif

	CURR_REC = 0;

	ty_unlock();

	RETURN S_OKAY;
}

/* end-of-file */
