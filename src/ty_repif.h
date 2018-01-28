/*----------------------------------------------------------------------------
 * File    : ty_repif.h
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
 *   Contains structures used in the interface between the database and the
 *   Replication Server.
 *
 * $Id: ty_repif.h,v 1.4 1999/10/04 05:29:45 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_TY_REPIF_INCLUDED
#define TYPHOON_TY_REPIF_INCLUDED

#ifndef TYPHOON_INCLUDED
#include "typhoon.h"
#endif

#define REPLLOG_NAME	"replserv.log"

#define ACTION_UPDATE	'u'			/* Update or create a record			*/
#define ACTION_DELETE	'd'			/* Delete a record						*/
#define ACTION_NEWSITE	'n'			/* Scan catalog for new site			*/
#define ACTION_DELSITE	'e'			/* Remove a site from memory			*/
#define ACTION_DELTABLE	't'			/* Remove a table fro memmory			*/

typedef struct {
	char		action;				/* See ACTION_...						*/
	char		prog_id;			/* Program ID							*/
	ulong		recid;				/* Record ID							*/
	union {
		DB_ADDR	addr;				/* action = UPDATE						*/
		char	key[KEYSIZE_MAX];	/* action = DELETE						*/
		ulong	site_id;			/* action = NEWSITE, DELSITE or DELTABLE*/
	} u;
} LOGENTRY;


/*--------------------------- Protocol block IDs ---------------------------*/
#define REPL_ACKNOWLEDGE    0       /* ID of acknowledge block              */
#define REPL_UPDATE         100     /* ID of update block                   */
#define REPL_DELETE         101     /* ID of delete block                   */
#define REPL_CLEARTABLE		102
#define REPL_ERROR			103
#define REPL_PROTBUF_SIZE   25000   /* Max buffer size passed               */
#define REPL_HEADER_SIZE	8

struct repl_header {
	ulong		seqno;				/* Sequence number						*/
	ulong		len;				/* Length of rest of block				*/
};

struct repl_acknowledge {
	ulong		seqno;				/* Sequence number						*/
	ulong		len;				/* Length of rest of block				*/
	uchar		id; 				/* Must be 0							*/
	uchar		spare[3];			/* Used to maintain dword alignment     */
	ulong		sequence;           /* Sequence number acknowledged         */
};

struct repl_update {
	ulong		seqno;				/* Sequence number						*/
	ulong		len;				/* Length of rest of block				*/
	uchar		id; 				/* Must be 100							*/
	uchar		prog_id;			/* Program ID							*/
	ushort		rec_len;			/* Number of bytes in rec[]				*/
	ulong		recid;				/* Record ID							*/
	ulong		sequence;			/* Update sequence number				*/
	uchar		rec[1];				/* Record buffer						*/
};

struct repl_delete {
	ulong		seqno;				/* Sequence number						*/
	ulong		len;				/* Length of rest of block				*/
	uchar		id; 				/* Must be 101							*/
	uchar		prog_id;			/* Program ID							*/
	ushort		key_len;			/* Number of bytes in buf[]				*/
	ulong		recid;				/* Record ID							*/
	ulong		sequence;			/* Update sequence number				*/
	uchar		key[1];				/* Key buffer							*/
};	


struct repl_cleartable {
    ulong       seqno;              /* Sequence number                      */
    ulong       len;                /* Length of rest of block              */
    uchar       id;                 /* Must be 102                          */
	uchar		spare[3];
	ulong		recid;				/* ID of table to clear					*/
};



/*--------------------------------------------------------------------------*\
 *
 * Block    : repl_error
 *
 * Purpose  : This protocol block is used to report an error to the
 *			  Replication Server. 
 *
 * Direction: Site -> Replication Server.
 *
 */
struct repl_error {
    ulong       seqno;              /* Sequence number                      */
    ulong       len;                /* Length of rest of block              */
    uchar       id;                 /* Must be 103                          */
    uchar		error;				/* 0=Record contains unknown reference	*/
    								/* 1=Record is referenced by other rec	*/
    uchar       spare[2];           /* Used to maintain dword alignment     */
    ulong		arg;				/* error=0: ID of referenced table		*/
    ulong       sequence;           /* Sequence number of erroneous block	*/
};

#endif

/* end-of-file */

