/*----------------------------------------------------------------------------
 * File    : cmpfuncs.c
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
 *   Contains comparison functions for all the key types supported by
 *   Typhoon as well as compound keys.
 *
 * Functions:
 *   ucharcmp(a,b)		- Compare two unsigned chars.
 *   charcmp(a, b)		- Compare two chars.
 *   ustrcmp(s1, s2)	- Compare two unsigned char strings.
 *   shortcmp(a,b)		- Compare two shorts.
 *   intcmp(a,b)		- Compare two ints.
 *   longcmp(a,b)		- Compare two longs.
 *   floatcmp(a,b)		- Compare two floats.
 *   doublecmp(a,b)		- Compare two doubles.
 *   ushortcmp(a,b)		- Compare two unsigned shorts.
 *   uintcmp(a,b)		- Compare two unsigned ints.
 *   ulongcmp(a,b)		- Compare two unsigned longs.
 *   compoundkeycmp(a,b)- Compare two compound keys.
 *   refentrycmp(a,b)	- Compare two REF_ENTRY items.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "typhoon.h"
#include <ctype.h>
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"

#define CMP			return *a > *b ? 1 : *a < *b ? -1 : 0

static CONFIG_CONST char rcsid[] = "$Id: cmpfuncs.c,v 1.5 1999/10/03 23:28:29 kaz Exp $";

/*--------------------------- Function prototypes --------------------------*/
static int charcmp		PRM( (char *, char *); )
static int shortcmp		PRM( (short *, short *); )
static int intcmp		PRM( (int *, int *); )
static int longcmp		PRM( (long *, long *); )
static int ucharcmp	PRM( (uchar *, uchar *); )
static int ushortcmp	PRM( (ushort *, ushort *); )
static int uintcmp		PRM( (unsigned *, unsigned *); )
static int ulongcmp		PRM( (ulong *, ulong *); )
static int ustrcmp		PRM( (uchar *, uchar *); )
static int floatcmp		PRM( (float *, float *); )
static int doublecmp	PRM( (double *, double *); )
	   int refentrycmp	PRM( (REF_ENTRY *, REF_ENTRY *); )


CMPFUNC keycmp[] = {
	NULL,
	(CMPFUNC)charcmp,
	(CMPFUNC)ustrcmp,			/* This should be strcmp, I think...		*/
	(CMPFUNC)shortcmp,
	(CMPFUNC)intcmp,
	(CMPFUNC)longcmp,
	(CMPFUNC)floatcmp,
	(CMPFUNC)doublecmp,
	NULL,
	(CMPFUNC)refentrycmp,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	(CMPFUNC)ucharcmp,
	(CMPFUNC)ustrcmp,
	(CMPFUNC)ushortcmp,
	(CMPFUNC)uintcmp,
	(CMPFUNC)ulongcmp
};


static int charcmp(a, b)
char *a, *b;
{
	CMP;
}


static int ucharcmp(a, b)
uchar *a, *b;
{
	CMP;
}


FNCLASS int ty_ustrcmp(s1, s2)
uchar *s1, *s2;
{
	return ustrcmp(s1, s2);
}

static int ustrcmp(s1, s2)
uchar *s1, *s2;
{
	uchar *sorttable = typhoon.db->header.sorttable;

	while( *s1 )
    {
        if( sorttable[*s1] - sorttable[*s2] )
			break;

		s1++;
		s2++;
    }

    return sorttable[*s1] - sorttable[*s2];
}


static int shortcmp(a,b)
short *a, *b;
{
	CMP;
}

static int intcmp(a,b)
int *a, *b;
{
	CMP;
}

static int longcmp(a,b)
long *a, *b;
{
	CMP;
}

static int floatcmp(a,b)
float *a, *b;
{
	CMP;
}

static int doublecmp(a,b)
double *a, *b;
{
	CMP;
}

static int ushortcmp(a,b)
ushort *a, *b;
{
	CMP;
}

static int uintcmp(a,b)
unsigned *a, *b;
{
	CMP;
}

static int ulongcmp(a,b)
ulong *a, *b;
{
	CMP;
}


/*----------------------------- compoundkeycmp -----------------------------*\
 *
 * Purpose : This function compares two compound keys. The global variable
 *			 <curr_key> is set to the ID of the keys being compared. This
 *			 is because the call to the key comparison function in the B-tree
 *			 functions only passes two pointers and not the type.
 *
 * Params  : a		- The first key
 *			 b		- The second key
 *
 * Returns : < 0	- a < b
 *			 0		- a = b
 *			 > 0	- a > 0
 *
 */

int compoundkeycmp(a, b)
void *a, *b;
{
	Key *key		= typhoon.db->key + typhoon.curr_key;
	KeyField *keyfld= typhoon.db->keyfield + key->first_keyfield;
	int fields		= key->fields;
	int type, diff;

    while( fields-- )
	{
		type = typhoon.db->field[ keyfld->field ].type & (FT_BASIC|FT_UNSIGNED);

		if( (diff = (*keycmp[type])((char *)a + keyfld->offset, (char *)b + keyfld->offset)) )
            break;

        keyfld++;
	}

    /* If the field is sorted in descending order, invert the sign */
    if( !keyfld->asc )
        diff = -diff;

	return diff;
}


int refentrycmp(a, b)
REF_ENTRY *a, *b;
{
	if( a->parent          > b->parent )  			return  1;
	if( a->parent          < b->parent )  			return -1;
	if( a->dependent.recid > b->dependent.recid )	return  1;
	if( a->dependent.recid < b->dependent.recid )	return -1;
	if( a->dependent.recno > b->dependent.recno )	return  1;
	if( a->dependent.recno < b->dependent.recno )	return -1;

	return 0;
}

/* end-of-file */
