/*----------------------------------------------------------------------------
 * File    : ty_find.c
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
#include "ty_glob.h"
#include "ty_prot.h"

static CONFIG_CONST char rcsid[] = "$Id: ty_find.c,v 1.9 1999/10/04 04:39:42 kaz Exp $";

/*--------------------------- Function prototypes --------------------------*/
static int	d_keymove		PRM( (Id, int); )
static int	d_recmove		PRM( (Id, int); )


/*-------------------------------- d_keyread -------------------------------*\
 *
 * Purpose	 : Copies the contents of the current key into the buffer <buf>.
 *
 * Parameters: buf		- Pointer to key buffer. This buffer must be large
 *						  enough to hold the entire key.
 *
 * Returns	 : S_NOCR	- No current record.
 *			   S_OKAY	- Key copied ok.
 *
 */

FNCLASS int d_keyread(buf)
void *buf;
{
	/* Make sure that we have a current record */
	if( db_status != S_OKAY )
		RETURN_RAP(S_NOCR);

	/* Make sure that we have performed a key operation ....

		?????????????????????????????????????

		RETURN_RAP(S_KEYSEQ);

	*/

	/*return db_keyread(&db->key[curr-key], buf);*/

	memcpy(buf, typhoon.curr_keybuf, DB->key[CURR_KEY].size);

	RETURN S_OKAY;
}



/*-------------------------------- d_keyfind -------------------------------*\
 *
 * Purpose	 : Find a key in an index.
 *
 * Parameters: id			- Either key id or field id that is also a key.
 *
 * Returns	 : S_OKAY		- The key was found. The record can be read by
 * 							  d_recread().
 *			   S_NOTFOUND	- The key could not be found.
 *			   S_NOCD		- No current database.
 *			   S_NOTKEY		- The id is not a key id.
 *
 */

FNCLASS int d_keyfind(id, keyptr)
Id id;
void *keyptr;
{
	Key *key;
	int rc;

	/* Make sure that a database is open */
	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	/* Determine whether this id is a key id or a compound key id */
	if( id < REC_FACTOR )
	{
		if( id >= DB->header.keys )
			RETURN_RAP(S_NOTKEY);

		key = &DB->key[id];

		CURR_RECID = DB->field[ DB->keyfield[ key->first_keyfield ].field ].recid;
	}
	else
	{
		Field *fld;

		if( (rc = set_recfld(id, NULL, &fld)) != S_OKAY )
			return rc;

		if( !(fld->type & FT_KEY) )
			RETURN_RAP(S_NOTKEY);

		key = &DB->key[ fld->keyid ];
	}

	ty_lock();

    CURR_KEY = key - DB->key;

	rc = ty_keyfind(key, keyptr, &CURR_REC);

	ty_unlock();

	RETURN rc;
}



/*-------------------------------- d_keymove -------------------------------*\
 *
 * Purpose	 : Perform a d_keyfrst(), d_keylast(), d_keyprev() or d_keylast()
 *			   on an index. This function is called the macros defined in
 *			   typhoon.h.
 *
 * Parameters: id			- Field id.
 *			   direction	- 0 = next, 1 = prev, 2 = first, 3 = last
 *
 * Returns	 : S_OKAY		- Operation performed successfully.
 *			   S_NOTFOUND	- Not found.
 *			   S_NOTKEY		- The id is not a key.
 *			   S_NOCD		- No current database.
 *
 */

static int d_keymove(id, direction)
Id id;
int direction;
{
	static int (*movefunc[]) PRM((Key *, ulong *)) =
		{ ty_keynext, ty_keyprev, ty_keyfrst, ty_keylast };
	Key *key;
	int rc;

	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	/* Determine whether this id is a key id or a compound key id */
	if( id < REC_FACTOR )
	{
		if( id >= DB->header.keys )
			RETURN_RAP(S_NOTKEY);

		key = &DB->key[id];

		CURR_RECID = DB->field[ DB->keyfield[ key->first_keyfield ].field ].recid;
	}
	else
	{
		Field *fld;

		if( (rc = set_recfld(id, NULL, &fld)) != S_OKAY )
			return rc;

		if( !(fld->type & FT_KEY) )
			RETURN_RAP(S_NOTKEY);

		key = &DB->key[ fld->keyid ];
	}

	ty_lock();
    CURR_KEY = key - DB->key;
	rc = (*movefunc[direction])(key, &CURR_REC);
	ty_unlock();

	RETURN rc;
}


FNCLASS int d_keyfrst(field)
ulong field;
{
	RETURN d_keymove(field, 2);
}

FNCLASS int d_keylast(field)
ulong field;
{
	RETURN d_keymove(field, 3);
}

FNCLASS int d_keynext(field)
ulong field;
{
	RETURN d_keymove(field, 0);
}

FNCLASS int d_keyprev(field)
ulong field;
{
	RETURN d_keymove(field, 1);
}


/*-------------------------------- d_recmove -------------------------------*\
 *
 * Purpose	 : Perform a d_recnext(), d_recprev(), d_recfrst() or d_reclast()
 *			   on a data file.
 *
 * Parameters: record		- Record id.
 *			   direction	- 0 = next, 1 = prev, 2 = first, 3 = last
 *
 * Returns	 : S_OKAY		- Operation performed successfully.
 *			   S_NOTFOUND	- Not found.
 *			   S_NOCD		- No current database.
 *
 */

int d_recmove(record, direction)
Id record;
int direction;
{
	static int (*movefunc[]) PRM((Record *, void *)) =
		{ ty_recnext, ty_recprev, ty_recfrst, ty_reclast };
	Record *rec;
	int rc;

	if( (rc = set_recfld(record, &rec, NULL)) != S_OKAY )
    	return rc;

	ty_lock();
	if( (rc = (*movefunc[direction])(rec, DB->recbuf)) == S_OKAY )
	{
		ty_reccurr(rec, &CURR_REC);
		CURR_BUFREC = CURR_REC;				/* Mark buffer as updated		*/
	}
	else
		CURR_REC = 0;
	ty_unlock();

	RETURN rc;
}


FNCLASS int d_recfrst(record)
ulong record;
{
	RETURN d_recmove(record, 2);
}

FNCLASS int d_reclast(record)
ulong record;
{
	RETURN d_recmove(record, 3);
}

FNCLASS int d_recnext(record)
ulong record;
{
	RETURN d_recmove(record, 0);
}

FNCLASS int d_recprev(record)
ulong record;
{
	RETURN d_recmove(record, 1);
}



/*-------------------------------- d_crread --------------------------------*\
 *
 * Purpose	 : Read the contents of a field of the current record.
 *
 * Parameters: field		- Field id.
 *			   buf			- Pointer to buffer where field will be stored.
 *
 * Returns	 : S_OKAY		- Operation performed successfully.
 *			   S_NOCD		- No current database.
 *			   S_NOCR		- No current record.
 *			   S_DELETED	- The record has been deleted.
 *
 */

FNCLASS int d_crread(field, buf)
Id field;
void *buf;
{
	Record *rec;
	Field *fld;
	int size, rc;

	if( (rc = set_recfld(field, &rec, &fld)) != S_OKAY )
    	return rc;

	if( !CURR_REC )
		RETURN_RAP(S_NOCR);

	if( (rc = update_recbuf()) != S_OKAY )
		return rc;

	if( fld->type & FT_VARIABLE )
		/* Get the value of the size field (stored in keyid) */
		size = *(ushort *)((char *)buf + DB->field[ fld->keyid ].offset) * fld->elemsize;
	else
		size = fld->size;

	memcpy(buf, (char *)DB->recbuf + fld->offset, (size_t) size);

	RETURN S_OKAY;
}



/*--------------------------------- d_crget --------------------------------*\
 *
 * Purpose	 : Get the database of the current record.
 *
 * Parameters: addr		- Pointer to location where address will be stored.
 *
 * Returns	 : S_OKAY	- Successful.
 *
 */

FNCLASS int d_crget(addr)
DB_ADDR *addr;
{
	addr->recno = CURR_REC;
    addr->recid = INTERN_TO_RECID(CURR_RECID);

	RETURN S_OKAY;
}



/*--------------------------------- d_crset --------------------------------*\
 *
 * Purpose	 : Set the database of the current record.
 *
 * Parameters: addr		- Pointer to address of new record.
 *
 * Returns	 : S_OKAY	- Successful.
 *
 */

FNCLASS int d_crset(addr)
DB_ADDR *addr;
{
	ulong recid = addr->recid;

	recid = RECID_TO_INTERN(addr->recid);

	/* Validate record id */
	if( recid >= DB->header.records )
		RETURN S_INVREC;

	CURR_REC	= addr->recno;
    CURR_RECID	= recid;

    RETURN S_OKAY;
}



/*-------------------------------- d_records -------------------------------*\
 *
 * Purpose	 : Return the number of records in a file.
 *
 * Parameters: record	- Record id.
 *			   number	- Pointer to location where the value will be stored.
 *
 * Returns	 : S_OKAY	- Number of records stored in <number>.
 *			   S_NOCD	- No current database.
 *			   S_INVREC	- Invalid record id.
 *
 */

FNCLASS int d_records(record, number)
Id record;
ulong *number;
{
	Record *rec;
	int rc;

	if( (rc = set_recfld(record, &rec, NULL)) != S_OKAY )
		return rc;

	RETURN ty_reccount(rec, number);
}


FNCLASS int d_seterrfn(fn)
void (*fn) PRM( (int, long); )
{
	typhoon.ty_errfn = fn;
	return S_OKAY;
}


/* end-of-file */
