/*----------------------------------------------------------------------------
 * File    : bt_refin
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
 *   This file contains function for handling referential integrity.
 *
 * Functions:
 *   update_foreign_keys
 *   check_foreign_keys
 *   delete_foreign_keys
 *   check_dependent_tables
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_refin.c,v 1.5 1999/10/03 23:28:29 kaz Exp $";

/*--------------------------- Function prototypes --------------------------*/

/*---------------------------- Global variables ----------------------------*/
/* The ca[] table is a Communications Area used to pass information between
   check_foreign_keys() and modify_refentries(). Because foreign keys
   are modified in two steps, this table is necessary
*/
struct {
	ulong		ref_file;		/* Id of reference file. 0=skip this entry	*/
    ulong		del_parent;		/* If a foreign key has been changed this	*/
    							/* entry contains the recno of the old 		*/
    							/* parent. This should be used to remove 	*/
                                /* the old. REFENTRY. 0=no parent to delete	*/
	uchar		null;			/* Determines whether a reference should be	*/
								/* inserted, e.g. the key is not null		*/
} ca[RECKEYS_MAX];




/*--------------------------- update_foreign_keys --------------------------*\
 *
 * Purpose	 : This function is called after check_foreign_keys has set up 
 *			   the communications area (ca) with the foreign keys that should
 *			   be updated, removed or added.
 *
 * Parameters: rec			- Pointer to record.
 *			   is_new		- Called by d_fillnew() or d_recwrite().
 *
 * Returns	 : Nothing.
 *
 */
void update_foreign_keys(rec, is_new)
Record *rec;
int is_new;
{
	int n;
	Key key;
	REF_ENTRY refentry;

	/* If the record has no foreign keys we'll just return now */
	if( rec->first_foreign == -1 )
		return;

	key.size = sizeof(REF_ENTRY);
	n		 = rec->keys - (rec->first_foreign - rec->first_key);

	refentry.dependent.recid = CURR_RECID;
	refentry.dependent.recno = CURR_REC;

	while( n-- )
	{
		if( ca[n].ref_file )
		{
			key.fileid = ca[n].ref_file;

            if( !is_new )
            {
            	if( ( refentry.parent = ca[n].del_parent ) )
					ty_keydel(&key, &refentry, CURR_REC);
            }

			if( !ca[n].null )
			{
				refentry.parent = ((ulong *)DB->real_recbuf)[n];
				ty_keyadd(&key, &refentry, CURR_REC);
			}
		}
	}

}



/*--------------------------- check_foreign_keys ---------------------------*\
 *
 * Purpose	 : This function checks whether the foreign keys of a record
 *			   exist in its parent tables.
 *
 *			   First the existence of all foreign keys are checked. Only
 *			   the necessary checks are performed. If is_new all foreign
 *			   keys are checked, otherwise only foreign keys which have
 *			   changed are checked. Null keys are never checked.
 *
 *             The record numbers of the parents record are stored in the
 *			   preamble. For each entry in the preamble, the corresponding
 *			   entries in save_ref[] and old_preamble[] contains the file id
 *			   of the parent's reference file and the old preamble value,
 *			   respectively.
 *
 * Parameters: rec			- Pointer to record.
 *			   buf			- Buffer of dependent record.
 *			   new			- Is this record being created? This is set to 1
 *							  d_fillnew().
 *
 * Returns	 : S_OKAY		- All foreign keys existed.
 *			   S_NOTFOUND	- A foreign did not exist. db_subcode contains the
 *							  ID of the parent table.
 */
int check_foreign_keys(rec, buf, is_new)
Record *rec;
void *buf;
int is_new;
{
	Key *key;
	int n;
	int foreign_keys;
	ulong ref;

	/* If the record has no foreign keys we'll just return now */
	if( rec->first_foreign == -1 )
		return S_OKAY;

	key			 = DB->key + rec->first_foreign;
	foreign_keys = rec->keys - (rec->first_foreign - rec->first_key);

	/* If this is a new record the preamble must be cleared */
	if( is_new )
		memset(DB->real_recbuf, 0, foreign_keys * sizeof(ulong));

	for( n=0; KEY_ISFOREIGN(key) && n < foreign_keys; n++, key++ )
	{
		Key *primary_key = DB->key + DB->record[key->parent].first_key;

		ca[n].null = 0;

		if( is_new || reckeycmp(key, buf, DB->recbuf) )
		{
			if( KEY_ISOPTIONAL(key) )
			{
				ca[n].ref_file = DB->record[key->parent].ref_file;

				/* If the record is being created or the old value was null
				 * no key need to be deleted
				 */
				if( is_new || null_indicator(key, DB->recbuf) )
					ca[n].del_parent = 0;
				else
					ca[n].del_parent = ((ulong *)DB->real_recbuf)[n];
					
				if( null_indicator(key, buf) )
				{
					ca[n].null = 1;
					continue;
				}
			}

		    CURR_KEY = primary_key - DB->key;

			if( ty_keyfind(primary_key, set_keyptr(key, buf), &ref) != S_OKAY )
			{
            	db_subcode = (key->parent+1) * REC_FACTOR;
				RETURN S_FOREIGN;
			}

			ca[n].del_parent = ((ulong *)DB->real_recbuf)[n];
            ca[n].ref_file   = DB->record[key->parent].ref_file;

			/* Store the reference to the parent record in the preamble */
			((ulong *)DB->real_recbuf)[n] = ref;
		}
		else
			ca[n].ref_file = 0;
	}

	return S_OKAY;
}



/*-------------------------- delete_foreign_keys ---------------------------*\
 *
 * Purpose	 : This function removes all refentries from a dependent record's
 *			   parent reference files.
 *
 * Parameters: rec			- Pointer to record.
 *
 * Returns	 : Nothing.
 *
 */
void delete_foreign_keys(rec)
Record *rec;
{
	Key *key, refkey;
    REF_ENTRY refentry;
	int n;
	int foreign_keys;

	/* If the record has no foreign keys we'll just return now */
	if( rec->first_foreign == -1 )
		return;

	key			 = DB->key + rec->first_foreign;
	foreign_keys = rec->keys - (rec->first_foreign - rec->first_key);
	refkey.size	 = sizeof(REF_ENTRY);

	refentry.dependent.recid	= CURR_RECID;
    refentry.dependent.recno	= CURR_REC;

	for( n=0; KEY_ISFOREIGN(key) && n < foreign_keys; n++, key++ )
	{
		if( !(key->type & KT_OPTIONAL) || !null_indicator(key, DB->recbuf) )
		{
	    	refentry.parent = ((ulong *)DB->real_recbuf)[n];
			refkey.fileid   = DB->record[key->parent].ref_file;

			ty_keydel(&refkey, &refentry, CURR_REC);
		}
	}
}



/*------------------------- check_dependent_tables -------------------------*\
 *
 * Purpose	 : This function is called by d_fillnew() and d_delete() to check
 *			   whether this update/delete operation will have any effect on
 *			   records in dependent tables.
 *
 * Parameters: parent		- Pointer to parent table's record pointer.
 *			   buf			- Buffer of dependent record.
 *			   for_action	- 'd'=delete, 'u'=update
 *
 * Returns	 : S_OKAY		- No conflicts.
 *			   S_RESTRICT	- A dependent table with restrict rule has rows
 *                            which referenced the parent table. db_subcode
 *						      contains the ID of the dependent table.
 *
 */
int check_dependent_tables(parent, buf, for_action)
Record *parent;
void *buf;
int for_action;
{
	Key *primary_key, refkey;
	REF_ENTRY refentry;
	ulong ref;
	int rc;

	/* Does this table have any dependent tables? */
    if( !parent->dependents )
    	return S_OKAY;

    /* If the table has dependent tables, it follows that it must also have
     * a primary key, i.e. no need to check for that.
     */
	primary_key = DB->key + parent->first_key;

	/* Only perform check if the primary key has changed (if update) */
	if( for_action == 'u' )
		if( !reckeycmp(primary_key, buf, DB->recbuf) )
			return S_OKAY;

	refentry.parent = CURR_REC;
	refentry.dependent.recid = 0;
	refentry.dependent.recno = 0;

	refkey.size	  = sizeof(REF_ENTRY);
    refkey.fileid = parent->ref_file;

	if( (rc = ty_keyfind(&refkey, &refentry, &ref)) != S_OKAY )
		rc = ty_keynext(&refkey, &ref);

    if( rc != S_OKAY )
    	return S_OKAY;

	refentry = *(REF_ENTRY *)CURR_KEYBUF;

    if( refentry.parent == CURR_REC )
    {
    	/* Set db_subcode to the ID of the dependent table */
    	db_subcode = (refentry.dependent.recid+1) * REC_FACTOR;
    	RETURN S_RESTRICT;
	}

	return S_OKAY;
}

/* end-of-file */
