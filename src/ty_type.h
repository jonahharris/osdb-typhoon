/*----------------------------------------------------------------------------
 * File    : ty_type.h
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
 *   Contains file descriptor definition for the B-tree, record and vlr 
 *   modules.
 *
 * $Id: ty_type.h,v 1.4 1999/10/04 03:45:08 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_TY_TYPE_H
#define TYPHOON_TY_TYPE_H

#ifndef TYPHOON_INCLUDED
#include "typhoon.h"
#endif

#ifndef TYPHOON_TY_DBD_H
#include "ty_dbd.h"
#endif

/*---------- Internal constants --------------------------------------------*/
#define DB_MAX			10		/* Maximum number of concurrent databases	*/
#define BTREE_DEPTH_MAX	10		/* Maximum B-tree depth						*/
#define BIT_DELETED		0x01

/*---------- Macros --------------------------------------------------------*/
#define FREE(p)			if( p ) free(p)
#define RETURN          return db_status =
#define RETURN_RAP(v)	return report_err(v);

/*---------- Structures ----------------------------------------------------*/
typedef ulong ix_addr;
typedef int (*CMPFUNC)PRM((void *, void *));
typedef struct {
	char	type;  					/* = 'k'								*/
	ulong	seqno;					/* Sequence number						*/
    int     fh;                     /* File handle                      	*/
    char    fname[80];              /* File name                        	*/
	struct {						/* Index file header					*/
	    char    id[16];         	/* Version id                           */
	    ushort  version;        	/* Version number                       */
	    ix_addr first_deleted;  	/* Pointer to first node in delete list */
	    ushort  nodesize;       	/* Node size                            */
	    ushort  keysize;       		/* Size of key in bytes                 */
	    ushort  order;         	 	/* Node order                           */
	    ushort  dups;           	/* Duplicate keys allowed?              */
	    ulong	keys;				/* Number of keys in index				*/
	    ulong	timestamp;			/* Timestamp. Changed by d_keyadd/del()	*/
	    char    spare[2];	    	/* Not used								*/
	} H;
    CMPFUNC cmpfunc;                /* Comparison function              	*/
    struct {						/* Path to current node and key			*/
        ix_addr a;                  /* Node address                     	*/
        ushort  i;                  /* Node index                       	*/
    } path[BTREE_DEPTH_MAX+1];
    int		level;                  /* Path level                       	*/
    int		shared;					/* Opened in shared mode?				*/
    int		tsize;                  /* Tuple size                       	*/
	int		aligned_keysize;		/* Aligned keysize						*/
    int		curr;                   /* Do we have a current key?        	*/
	int		hold;					/* Used by d_keynext and d_keyprev		*/
	char   *curkey;					/* 'current key' buffer					*/
    char    node[1];				/* This array is size nodesize      	*/
} INDEX;

typedef struct {					/* Record head (found in every record)	*/
	ulong		prev;				/* Pointer to previous record           */
	ulong		next;				/* Pointer to next record               */
	char		flags;				/* Delete bit							*/
	char		data[1];			/* Record data. this field MUST be the  */
} RECORDHEAD;						/* Last field in the RECORD structure   */

typedef struct {
	char			type;			/* = 'd'								*/
	ulong			seqno;			/* Sequence number						*/
    int             fh;				/* File handle							*/
	char			fname[80];		/* File name							*/
    struct {
        char        id[16];         /* Version id                           */
        ushort      version;        /* Record file version number           */
        ulong       first_deleted;  /* Pointer to first deleted record      */
        ulong       first;          /* Pointer to first record in chain     */
        ulong       last;           /* Pointer to last record in chain      */
        ulong       numrecords;     /* Number of records in file            */
        ushort      datasize;       /* Size of data block                   */
        ushort      recsize;        /* Size of record (and chain)           */
    } H;
	int				first_possible_rec;	/* Record number of the first		*/
									/* record in the file					*/
	int				share;			/* Opened in shared mode?				*/
    ulong           recno;          /* Current record number. 0 = no current*/
	RECORDHEAD		rec;
} RECORD;

/*--------------------------------------------------------------------------*/
/*                 Variable Length Record file structures                   */
/*--------------------------------------------------------------------------*/
typedef struct {
	ulong			nextblock;		/* Pointer to next block				*/
	unsigned		recsize;		/* Size of record						*/
	char			data[1];		/* Data 								*/
} VLRBLOCK;

typedef struct {
	char			type;			/* = 'v'								*/
	ulong			seqno;	 		/* Sequence number						*/
	int 			fh;				/* File handle							*/
	char			fname[80];		/* File name							*/
    int				shared;			/* Opened in shared mode?				*/
	unsigned		datasize;		/* Number of bytes in each block		*/
	VLRBLOCK		*block;			/* Pointer to buffer					*/
	struct {
		char		version[32];	/* VLR version number					*/
		char		id[32]; 		/* User provided ID 					*/
		unsigned	blocksize;		/* Block size							*/
		ulong		firstfree;		/* First free data block				*/
		ulong		numrecords;		/* Number of records in file			*/
	} header;
} VLR;

typedef union {
	struct {
		char 	type;				/* 'd' = data,, 'k' = key, 'v'=vlr file	*/
		ulong	seqno;				/* Sequence number						*/
		int		fh;
	} *any;
	INDEX		*key;				/* Index File Descriptor				*/
	RECORD		*rec;				/* Record File Descriptor				*/
    VLR         *vlr;               /* Variable Length Record Descriptor    */
} Fh;

typedef struct {
	int			use_count;			/* First remove shared memory when 0	*/
	int			backup_active;
	int			restore_active;
	ulong		curr_recid;
	ulong		curr_recno;
	ulong		num_trans_active;
	char		spare[96];
} TyphoonSharedMemory;

typedef struct {					/* Database table entry					*/
	char		name[15];			/* Database name						*/
	char		mode;				/* [s]hared, [o]ne user, e[x]clusive	*/
	char		clients;			/* Number of clients using this database*/
	char		dbfpath[256];		/* Database file path					*/
	char		logging;			/* Is replication logging on?			*/
	uchar		prog_id;			/* Program ID (used with logging)		*/
	ulong		curr_rec;			/* These 4 fields hold curr_ variables	*/
	ulong		curr_recid;			/* when the database is not the current	*/
	ulong		curr_bufrec;
	ulong		curr_bufrecid;
	int			db_status;
	Header		header;
	void		*dbd;
	Fh			*fh;				/* Array [dbentry.files] of handles		*/
	File		*file;
	Record		*record;
	Field		*field;
	Key 		*key;
	KeyField	*keyfield;
	Structdef	*structdef;
	Sequence	*sequence;
	TyphoonSharedMemory *shm;
	int			seq_fh;
	int			shm_id;
	char		*recbuf;			/* This points to where the actual data	*/
									/* starts (bypassing foreign key refs)	*/
	char  		*real_recbuf;		/* This points to the real start of the	*/
									/* buffer								*/
} Dbentry;

typedef struct {
	ulong		parent;				/* Address of parent record				*/
	DB_ADDR		dependent;			/* Address of dependent record			*/
} REF_ENTRY;



typedef struct {
	Dbentry	 dbtab[DB_MAX];					/* Database table				*/
	Dbentry	*db;							/* Current database				*/

	int		 do_rebuild;					/* Rebuild indexes on d_open()?	*/
	int		 dbs_open;

	int		 cur_open;						/* Current number of open files	*/
	int		 max_open;						/* Maximum number of open files	*/

	ulong	 curr_keybuf[KEYSIZE_MAX/sizeof(long)];

	Id		 curr_key;						/* Current key. It is			*/
											/* used to tell compoundkeycmp	*/
											/* which key is being compared	*/
	int		 curr_db;						/* Current database 			*/
	void	(*ty_errfn)		PRM( (int,long); )

	char 	 dbfpath[256];
	char 	 dbdpath[256];
} TyphoonGlobals;

#endif

/* end-of-file */
