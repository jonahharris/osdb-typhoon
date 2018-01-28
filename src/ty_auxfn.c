/*----------------------------------------------------------------------------
 * File    : ty_auxfn.c
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
 *   Auxiliary functions for the API functions.
 *
 * Functions:
 *   set_keyptr		- Return a pointer to a key or compound key.
 *   reckeycmp		- Compare two (compound) keys in separate records.
 *   set_subcode	- Set <db_subcode>.
 *   set_recfld		- 
 *   keyfind
 *   keyadd
 *   keydel
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

static CONFIG_CONST char rcsid[] = "$Id: ty_auxfn.c,v 1.5 1999/10/03 23:28:29 kaz Exp $";

int aux_getkey(id, key)
Id id;
Key **key;
{
	Field *fld;
	int rc;

	/* Determine whether this id is a key id or a compound key id */
	if( id < REC_FACTOR )
	{
		if( id >= DB->header.keys )
			RETURN_RAP(S_NOTKEY);

		*key = &DB->key[id];
	}
	else
	{
		if( (rc = set_recfld(id, NULL, &fld)) != S_OKAY )
			return rc;

		if( !(fld->type & FT_KEY) )
			RETURN_RAP(S_NOTKEY);

		*key = &DB->key[fld->keyid];
	}

	return S_OKAY;
}



/*------------------------------- set_keyptr -------------------------------*\
 *
 * Purpose	 : Returns a pointer to the key value of <key>. This function
 *			   is necessary because the fields of a compound can be spread
 *			   all over the record.
 *
 * Parameters: key	- Pointer to key table entry.
 *			   buf	- Buffer to store key value in.
 *			   
 *
 * Returns	 : A pointer to the key value.
 *
 */

void *set_keyptr(key, buf)
Key *key;
void *buf;
{
	static char keybuf[KEYSIZE_MAX];
	int n;

    CURR_KEY = key - DB->key;

	if( (n = key->fields) > 1 )
	{
		KeyField *keyfld = DB->keyfield + key->first_keyfield;

		/* Build compound key from record */
		while( n-- )
		{
			memcpy(keybuf + keyfld->offset,
				   (char *)buf + DB->field[ keyfld->field ].offset,
							     DB->field[ keyfld->field ].size);
			keyfld++;
		}

		return (void *)keybuf;
	}

	return (void *)((char *)buf + DB->field[ DB->keyfield[ key->first_keyfield ].field ].offset);
}





/*-------------------------------- reckeycmp -------------------------------*\
 *
 * Purpose	 : The function compares two fields in separate records. This
 *			   function is necessary because the fields of a compound can be
 *			   spread all over the record (i.e. compound key).
 *
 * Parameters: key		- Pointer to key struct.
 *			   a		- Pointer to first record buffer.
 *			   b		- Pointer to seconds record buffer.
 *
 * Returns	 : 0		- The keys match.
 *			   !=0		- The keys do not match.
 *
 */

int reckeycmp(key, a, b)
Key *key;
void *a, *b;
{
	KeyField *keyfld = DB->keyfield + key->first_keyfield;
	Field *field;
	int fields = key->fields;
	int type, diff;

    CURR_KEY = key - DB->key;

	/* An optional key is regarded as changed if 
	 *   a) One and only one of the null determinatator is not set
	 *   b) If both null determinators are set and the key values different
	 */
	if( key->type & KT_OPTIONAL )
	{
		int a_null = null_indicator(key, a);
		int b_null = null_indicator(key, b);

		if( a_null && b_null )
			return 0;

		if( a_null || b_null )
			return 1;

		/* Otherwise both keys are not null - compare */
	}

	do
	{
		field	= DB->field + keyfld->field;
		type	= field->type & (FT_BASIC|FT_UNSIGNED);

		if( (diff = (*keycmp[type])((char *)a + field->offset, (char *)b + field->offset)) )
			break;

		keyfld++;
	}
	while( --fields );

	/* Ascending or descending is not significant here */
	return diff;
}



/*------------------------------- set_subcode ------------------------------*\
 *
 * Purpose	 : This function sets <db_subcode> to the id of <key>. The id
 *			   can either be a compound key id or a field id.
 *
 * Parameters: key		- Pointer to key table entry.
 *
 * Returns	 : Nothing.
 *
 */
void set_subcode(key)
Key *key;
{
	Field *fld;

	if( key->fields > 1 )
		db_subcode = key - DB->key /* + 1 */;
	else
	{
		fld = DB->field + DB->keyfield[ key->first_keyfield ].field;

		db_subcode = (fld->recid + 1) * REC_FACTOR +
			 (fld - DB->field) - DB->record[ fld->recid ].first_field + 1; 
	}
}



/*------------------------------- set_recfld -------------------------------*\
 *
 * Purpose	 : This function sets the pointers <recptr> and <fldptr> to the
 *			   record and field denoted by <id>. The following calls are
 *			   legal:
 *
 *			   If <id> is -1, <rec> will be set to the current record (<CURR_RECID>) a
 *			   <fld> to the first field of that record.
 *
 *			   Both <rec> and <fld> may be NULL.
 *
 * Parameters: id		- Record or field id.
 *			   recptr	- Pointer to record pointer.
 *			   fldptr	- Pointer to field pointer.
 *
 * Returns	 : S_OKAY	- recid and fldid were okay, recptr and fldptr set.
 *			   S_NOCD	- No current database.
 *			   S_INVREC - Invalid record id.
 *			   S_INVFLD - Invalid field id.
 *
 */

int set_recfld(id, recptr, fldptr)
Id id;
Record **recptr;
Field **fldptr;
{
	Id recid, fldid;

	if( CURR_DB == -1 )
		RETURN_RAP(S_NOCD);

	/* If is -1 the return pointers to the current record */
	if( id == -1 )
    {
    	recid = CURR_RECID;
        fldid = DB->record[recid].first_field;
    }
    else
    {
		/* Remove record factor */
		recid = id / REC_FACTOR - 1;

		if( (fldid = id % REC_FACTOR) )
			fldid--;

		/* Validate record id */
		if( recid >= DB->header.records )
			RETURN_RAP(S_INVREC);

		/* Validate field id */
        if( fldid >= DB->record[recid].fields )
        	RETURN_RAP(S_INVFLD);

		fldid += DB->record[recid].first_field;
		CURR_RECID = recid;
    }

	if( recptr )
		*recptr = DB->record + recid;

	if( fldptr )
		*fldptr = DB->field + fldid;

	RETURN S_OKAY;
}


int keyfind(key, buf, ref)
Key *key;
void *buf;
ulong *ref;
{
    CURR_KEY = key - DB->key;

	return ty_keyfind(key, set_keyptr(key, buf), ref);
}


int keyadd(key, buf, ref)
Key *key;
void *buf;
ulong ref;
{
    CURR_KEY = key - DB->key;

	return ty_keyadd(key, set_keyptr(key, buf), ref);
}


int keydel(key, buf, ref)
Key *key;
void *buf;
ulong ref;
{
    CURR_KEY = key - DB->key;

	return ty_keydel(key, set_keyptr(key, buf), ref);
}


/*------------------------------ compress_vlr ------------------------------*\
 *
 * Purpose	 : Compresses or uncompresses a the variable length record
 *			   defined by <rec>.
 *
 * Parameters: action	- COMPRESS/UNCOMPRESS.
 *			   rec		- Pointer to record table entry.
 *			   src		- Pointer to source buffer.
 *			   dest		- Pointer to destination buffer.
 *			   size		- Will contain compressed size when the function returns.
 *
 * Returns	 : -1		- Error. db_status set to cause.
 *			   > 0		- Size of compress record in <dest>.
 *
 */
int compress_vlr(action, rec, dest, src, size)
int action;
Record *rec;
void *src, *dest;
unsigned *size;
{
	Field *fld;
	unsigned offset;
	ushort count;
	int fields = rec->fields;

	/* Find the first variable length field (this cannot be 0)*/
	fld = &DB->field[rec->first_field];

	while( fields  )
		if( fld->type & FT_VARIABLE )
			break;
		else
			fld++, fields--;
	
	/* Copy the static part of the record */
	memcpy(dest, src, offset = fld->offset);

	while( fields )
	{
		/* Get the value of the size field (stored in keyid) */
		count = *(ushort *)((char *)src + DB->field[ fld->keyid ].offset) * fld->elemsize;

		/* If the size value is too high report an error to the user */
		if( count > fld->size )
		{
			db_subcode = (fld->recid + 1) * REC_FACTOR + fld->keyid + 1;
			RETURN S_RECSIZE;
		}

		if( action == COMPRESS )
			memcpy((char *)dest + offset, (char *)src + fld->offset, count);
		else
			memcpy((char *)dest + fld->offset, (char *)src + offset, count);

		offset += count;

		/* Find next variable length field */
		if( !--fields )
			break;

		fld++;
		while( fld->nesting )
			fld++, fields--;
	}

	if( action == COMPRESS )
		*size = offset;

	return S_OKAY;
}
										 

/*----------------------------- null_indicator -----------------------------*\
 *
 * Purpose	 : This function checks whether the null indicator of an optional
 *			   key is set.
 *
 * Parameters: key		- Pointer to key.
 *			   buf		- Pointer to record buffer.
 *
 * Returns	 : 0		- The null indicator is set.
 *			   1		- The null indicator is not set.
 *
 */
int null_indicator(key, buf)
Key *key;
void *buf;
{
	return !(((char *)buf)[ DB->field[ key->null_indicator ].offset ]);
}



/*------------------------------ update_recbuf -----------------------------*\
 *
 * Purpose	 : Makes sure that the contents of the current record is the same
 *			   in DB->recbuf as on the disk.
 *
 * Parameters: None.
 *
 * Returns	 : None.
 *
 */

int update_recbuf()
{
	Record *rec = DB->record + CURR_RECID;
	int rc;
	unsigned recsize;

/*	if( curr_bufrec == CURR_REC && curr_bufrecid == CURR_RECID )
		return S_OKAY;*/

	DB->recbuf = DB->real_recbuf + rec->preamble;

	if( rec->is_vlr )
		rc = ty_vlrread(rec, DB->real_recbuf, CURR_REC, &recsize);
	else
		rc = ty_recread(rec, DB->real_recbuf, CURR_REC);

	CURR_BUFREC		= CURR_REC;
	CURR_BUFRECID	= CURR_RECID;
	
	return rc;
}

/* end-of-file */
