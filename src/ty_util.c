/*----------------------------------------------------------------------------
 * File    : ty_util.c
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
#include "ty_glob.h"
#include "ty_prot.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_util.c,v 1.5 1999/10/03 23:28:29 kaz Exp $";


/*------------------------------ d_getkeysize ------------------------------*\
 *
 * Purpose	 : Return the size of a key.
 *
 * Parameters: id		- Field ID or compound key ID.
 *			   size		- Pointer to var in which size will be returned.
 *
 * Returns	 : S_NOTKEY	- The ID is not a key ID.
 *			   S_NOCD	- No current database.
 *
 */

FNCLASS int d_getkeysize(id, size)
Id id;
unsigned *size;
{
	Field *fld;
	int rc;

	/* Make sure that a database is open */
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	/* Determine whether this id is a key id or a compound key id */
	if( id < REC_FACTOR )
	{
		if( id >= DB->header.keys )
			RETURN_RAP(S_NOTKEY);

		*size = DB->key[id].size;
	}
	else
	{
		if( (rc = set_recfld(id, NULL, &fld)) != S_OKAY )
			return rc;

		if( !(fld->type & FT_KEY) )
			RETURN_RAP(S_NOTKEY);

		*size = DB->key[fld->keyid].size;
	}

	RETURN S_OKAY;
}



FNCLASS int d_getfieldtype(id, type)
Id id;
unsigned *type;
{
	Field *fld;
	int rc;

	/* Make sure that a database is open */
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	/* Determine whether this id is a key id or a compound key id */
	if( id < REC_FACTOR )
	{
		if( id >= DB->header.keys )
			RETURN_RAP(S_NOTKEY);
	}
	else
	{
		if( (rc = set_recfld(id, NULL, &fld)) != S_OKAY )
			return rc;

		id = fld->keyid;
	}

	*type = DB->field[ DB->keyfield[ DB->key[id].first_keyfield ].field ].type;

	RETURN S_OKAY;
}




/*------------------------------ d_getrecsize ------------------------------*\
 *
 * Purpose	 : Return the size of a record.
 *
 * Parameters: recid	- Record ID.
 *			   size		- Pointer to var in which size will be returned.
 *
 * Returns	 : S_NOTKEY	- The ID is not a record ID.
 *			   S_NOCD	- No current database.
 *
 */

FNCLASS int d_getrecsize(recid, size)
Id recid;
unsigned *size;
{
	Record *rec;
	int rc;

	/* Make sure that a database is open */
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	if( (rc = set_recfld(recid, &rec, NULL)) != S_OKAY )
		return rc;
	
	*size = rec->size;

	RETURN S_OKAY;
}




FNCLASS int d_makekey(id, recbuf, keybuf)
Id id;
void *recbuf, *keybuf;
{
	KeyField *keyfld;
	Key *key;
	int n, rc;

	/* Make sure that a database is open */
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	if( (rc=aux_getkey(id, &key)) != S_OKAY )
		return rc;

	keyfld = DB->keyfield + key->first_keyfield;
	n = key->fields;

	/* Build compound key from record */
	while( n-- )
	{
		memcpy((char *)keybuf + keyfld->offset,
			   (char *)recbuf + DB->field[ keyfld->field ].offset,
						        DB->field[ keyfld->field ].size);
		keyfld++;
	}
	
	RETURN S_OKAY;
}


FNCLASS int d_getkeyid(recid, keyid)
Id recid, *keyid;
{
	Record *rec;
	int rc;
	
	if( (rc = set_recfld(recid, &rec, NULL)) != S_OKAY )
		return rc;

	*keyid = rec->first_key;

	RETURN S_OKAY;
}



FNCLASS int d_getforeignkeyid(recid, parent_table, keyid)
Id recid, parent_table, *keyid;
{
	Record *rec;
	Key *key;
	int rc, n;

	if( (rc = set_recfld(recid, &rec, NULL)) != S_OKAY )
		return rc;

	parent_table= RECID_TO_INTERN(parent_table);
	n			= rec->keys;
	key			= DB->key + rec->first_key;
	
	while( n-- )
	{
		if( KEY_ISFOREIGN(key) && key->parent == parent_table )
		{
			*keyid = key - DB->key;
			RETURN S_OKAY;
		}
		key++;
	}
	
	RETURN S_NOTFOUND;
}



/* end-of-file */
