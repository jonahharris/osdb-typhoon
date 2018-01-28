/*----------------------------------------------------------------------------
 * File    : ty_dbd.h
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
 *   Contains the definitions for the dbd-file.
 *
 * $Id: ty_dbd.h,v 1.3 1999/10/04 03:45:07 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_TY_DBD_H
#define TYPHOON_TY_DBD_H

#ifndef TY_ENVIRON_H
#include "environ.h"
#endif

/*-------------------------------- Constants -------------------------------*/
#define DBD_VERSION	"Typhoon 2.02"
#define DBDVERSION_LEN	20		/* Version name of dbd-file					*/
#define DBNAME_LEN		8		/* Maximum database name length				*/
#define IDENT_LEN		32		/* Identifier length						*/
#define FILENAME_LEN	12		/* OS dependent file name length			*/
#define KEYSIZE_MAX 	255 	/* Maximum size of a key					*/
#define RECSIZE_MAX     64000   /* Maximum size of a record                 */
#define RECKEYS_MAX		32		/* Maximum number of keys per record		*/
#define REC_FACTOR		1000L	/* rec 1 is 1000, rec 2 is 2000 etc.		*/
#define SORTTABLE_SIZE	256		/* Number of entries in sort table			*/

/*------------------------------- Field types ------------------------------*/
#define FT_CHAR			0x01	/* The first three bits are for type		*/
#define FT_CHARSTR		0x02	/* 		char string							*/
#define FT_SHORT		0x03	/* 		short								*/
#define FT_INT			0x04	/* 		int					 				*/
#define FT_LONG			0x05	/* 		long								*/
#define FT_FLOAT		0x06	/*		float								*/
#define FT_DOUBLE		0x07	/*		double								*/
#define FT_LDOUBLE		0x08	/*		long double (not used)				*/
#define FT_STRUCT		0x09	/* The type is a structure					*/
#define FT_UNSIGNED		0x10	/* Bit 4 is unsigned bit					*/
#define FT_KEY			0x20	/* Field is a key field						*/
#define FT_UNIQUE		0x40	/* Field is a unique key field				*/
#define FT_VARIABLE		0x80	/* Field has variable length (only 1-arrays)*/

#define FT_BASIC		0x0f	/* Bits occupied by integral types			*/
#define FT_GETBASIC(f)	((f)& FT_BASIC)	/* Extracts the integral type 		*/
#define FT_GETSIGNEDBASIC(f)  ((f) & (FT_BASIC|FT_UNSIGNED))

/*-------------------------------- Key types -------------------------------*/
#define KT_PRIMARY		0x01	/* Primary key (preceedes alternate in key[]*/
#define KT_ALTERNATE	0x02	/* Alternate key (preceedes foreign in key[]*/
#define KT_FOREIGN		0x03	/* Foreign key								*/
#define KT_CASCADE		0x08	/* Used with KT_FOREIGN						*/
#define KT_RESTRICT		0x10	/* Used with KT_FOREIGN						*/
#define KT_OPTIONAL		0x20	/* Used with KT_FOREIGN and KT_ALTERNATE	*/
#define KT_UNIQUE  	FT_UNIQUE	/* Must be the same bit as FT_UNIQUE		*/

#define KT_BASIC		0x03	/* The bits occupied by basic types			*/
#define KT_GETBASIC(k)	((k)&KT_BASIC)	/* Extracts the type of the key	 	*/


#define KEY_ISFOREIGN(key)		(KT_GETBASIC(key->type) == KT_FOREIGN)
#define KEY_ISALTERNATE(key)	(KT_GETBASIC(key->type) == KT_ALTERNATE)
#define KEY_ISOPTIONAL(key)		(key->type & KT_OPTIONAL)


#define RECID_TO_INTERN(id)		((id)/REC_FACTOR-1)
#define INTERN_TO_RECID(id)		(((id)+1)*REC_FACTOR)


typedef unsigned long	Id;		/* Identifies an element in the dbd file	*/

typedef struct {
	char	version[DBDVERSION_LEN];	/* Version of dbd-file				*/
	ushort	files;				/* Number of files in database				*/
	ushort	keys;				/* Number of key definitions				*/
	ushort	keyfields;			/* Number of key fields 					*/
	ushort	records;			/* Number of records in database			*/
	ushort	fields;				/* Number of fields in database				*/
	ushort	structdefs;			/* Number of structdefs in database			*/
	uchar	sorttable[SORTTABLE_SIZE];
	char	alignment;			/* Structure alignment						*/
	char	spare0;
	ushort	sequences;			/* Number of sequences in database			*/
	char	spare[16];			/* Not used									*/
} Header;

typedef struct {
	Id		fileid;				/* File id									*/
	ushort	entry;				/* Entry in field[] or record[]				*/
	ushort	line;				/* The line where it was defined			*/
	char	type;				/* 'd'=data, 'k'=key, 'r'=ref				*/
	char	record[IDENT_LEN+1];/* Record name								*/
	char	key[IDENT_LEN+1];	/* Key name. Only applicable if type is 'k' */
} Contains;


typedef struct {
	Id		id;					/* Record/Key id							*/
	ushort	pagesize;			/* Page size								*/
	char	type;				/* 'd'=data, 'v'=vlr, 'k'=key, 'r'=ref file	*/
	char	name[FILENAME_LEN+1];/* Name of file							*/
	char	spare[16];
} File;

typedef struct {
	Id		recid;				/* The record it is a member of				*/
	Id		keyid;				/* a) Key ID if field is a key				*/
								/* b) If the field is of variable length	*/
								/*	  keyid denotes the size field (since a	*/
								/*	  variable length field cannot be a key)*/
								/*    field.type & FT_VARIABLE				*/
	Id		structid; 			/* ID of struct if type is FT_STRUCT		*/
	ushort	offset;				/* Byte offset in record					*/
	ushort	size;				/* Size of field							*/
    ushort  elemsize;           /* Size of each element                     */
	ushort	type;				/* Field type. See FT_.. constants			*/
	uchar	nesting;			/* Structure nesting. 0=first level			*/
	uchar	spare2[4];
	char	name[IDENT_LEN+1];	/* Field name								*/
} Field;

typedef struct {
	Id		fileid;
	Id		first_keyfield;		/* Index of KeyField						*/
	Id		parent;				/* Only applicable if type = KT_FOREIGN		*/
	ushort	fields;				/* Number of fields in compound key			*/
	ushort	size;				/* Total key size							*/
	ushort	null_indicator;		/* ID of null indicator field of FT_OPTIONAL*/
	uchar	spare[14];
	uchar	type;				/* See KT_... flags							*/
    char	name[IDENT_LEN+1];	/* Name of key								*/
} Key;

typedef struct {
	Id		field;				/* Field id									*/
	ushort	offset; 			/* Offset in compound key					*/
	short	asc;				/* 1=ascending, '0'=descending				*/
	uchar	spare[4];
} KeyField;

typedef struct {
	Id		fileid;				/* Id of file record is contained in		*/
	Id		first_field;		/* Id of first field						*/
	Id		first_key;			/* Id of first key							*/
	Id		first_foreign;		/* Id of first foreign key (-1 = no foreign)*/
	Id		ref_file;			/* Id of reference file (-1 = no ref file)	*/
	Id		structid;			/* Id of structure defining record			*/
	ushort	dependents;			/* Number of dependent tables				*/
	ushort	fields;				/* Number of fields							*/
    ushort  keys;               /* Number of keys                           */
	ushort	size;				/* Size of record							*/
	ushort	preamble;			/* Number of bytes used preamble information*/
	uchar	aux;				/* Not used by Typhoon						*/
	uchar	spare[15];
	char    is_vlr;             /* Is record of variable length?            */
	char	name[IDENT_LEN+1];
} Record;

typedef struct {
	Id		first_member;		/* First struct member (Field[])			*/
	Id		members;			/* First struct member (Field[])			*/
	Id		control_field;		/* If is_union this is the control field	*/
	ushort	size;				/* Struct size								*/
	char	is_union;			/* Is the structure a union?				*/
	char	name[IDENT_LEN+1];	/* Structure name							*/
} Structdef;

typedef struct {
	ulong	start;				/* Starting number							*/
	ulong	step;				/* Number the sequence is in/decreased by	*/
	char	asc;				/* 1=ascending, '0'=descending				*/
	char	name[IDENT_LEN+1];	/* Sequence name							*/
	char	spare[2];
} Sequence;

#endif

/* end-of-file */
