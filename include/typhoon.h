/*----------------------------------------------------------------------------
 * File    : typhoon.h
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
 *   Header file for Typhoon library.
 *
 * $Id: typhoon.h,v 1.4 1999/10/04 04:38:32 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_INCLUDED
#define TYPHOON_INCLUDED

#ifndef TYPHOON_ENVIRON_H
#include "environ.h"
#endif

/*---------- Status codes --------------------------------------------------*/
#define S_NOCR				-2		/* No current record            	   	*/
#define S_NOCD				-1		/* No current database					*/

#define S_OKAY				0		/* Operation successful         		*/
#define S_NOTFOUND			1		/* Key not found                		*/
#define S_DUPLICATE			2		/* Duplicate key found 					*/
#define S_DELETED			3		/* Record is deleted					*/
#define S_RESTRICT			4		/* Restrict rule encountered(db_subcode)*/
#define S_FOREIGN			5		/* No foreign key (db_subcode)			*/
#define S_LOCKED			8		/* Record is locked						*/
#define S_UNLOCKED			9		/* Record is unlocked					*/
#define S_VERSION			10		/* B-tree or data file has wrong version*/
#define S_INVPARM			11		/* Invalid parameter					*/

#define S_NOMEM				200		/* Out of memory						*/
#define S_NOTAVAIL			201		/* Database not available				*/
#define S_IOFATAL			202		/* Fatal I/O operation					*/
#define S_FATAL				203		/* Fatal error - recover				*/
#define S_MAXCLIENTS		500		/* Too many clients						*/
#define S_NOSERVER			501		/* No server is installed				*/

/*---------- User errors ---------------------------------------------------*/
#define S_INVDB				1000	/* Invalid database						*/
#define S_INVREC			1001	/* Invalid record						*/
#define S_INVFLD			1002	/* Invalid field name					*/
#define S_NOTKEY			1003	/* Field is not a key					*/
#define S_RECSIZE           1004    /* Variable length record has invalid sz*/
#define S_BADTYPE			1005	/* Bad parameter type					*/
#define S_INVKEY			1006	/* Invalid key							*/
#define S_INVADDR			1007	/* Invalid database address (rec number)*/
#define S_INVSEQ			1008	/* Invalid sequence id					*/

/*---------- Lock types ----------------------------------------------------*/
#define LOCK_TEST			1		/* Test if a record is locked			*/
#define LOCK_UPDATE			2		/* Lock a record for update				*/

typedef struct {
	unsigned long	recid;
	unsigned long	recno;
} DB_ADDR;

extern unsigned long curr_rec;
extern int db_status;					/* See S_... constants				*/
extern long db_subcode;

#ifdef CONFIG_OS2
#	ifdef __BORLANDC__
#		define INCL_NOPMAPI
#	endif
#	ifdef __IBMC__
#		pragma map(db_status,  "_db_status")
#		pragma map(db_subcode, "_db_subcode")
#	endif
#	include <os2def.h>
#	define CL	APIRET EXPENTRY
#else
#	define CL	int
#endif

/*---------- Function prototypes -------------------------------------------*/
CL d_block			PRM( (void);									)
CL d_unblock		PRM( (void);									)
CL d_setfiles		PRM( (int);										)
CL d_keybuild		PRM( (void (*)(char *, ulong, ulong));			)
CL d_open			PRM( (char *, char *);							)
CL d_close			PRM( (void);       						        )
CL d_destroy		PRM( (char *);                                  )
CL d_keyfind		PRM( (unsigned long, void *);					)
CL d_keyfrst		PRM( (unsigned long);							)
CL d_keylast		PRM( (unsigned long);							)
CL d_keynext		PRM( (unsigned long);							)
CL d_keyprev		PRM( (unsigned long);							)
CL d_keyread		PRM( (void *);									)
CL d_fillnew		PRM( (unsigned long, void *);					)
CL d_keystore		PRM( (unsigned long);							)
CL d_recwrite		PRM( (void *);									)
CL d_recread		PRM( (void *);									)
CL d_crread			PRM( (unsigned long, void *);                   )

CL d_delete			PRM( (void);          							)
CL d_recfrst       	PRM( (unsigned long);	          			    )
CL d_reclast       	PRM( (unsigned long);	          			    )
CL d_recnext       	PRM( (unsigned long);	          			    )
CL d_recprev       	PRM( (unsigned long);	          			    )

CL d_crget			PRM( (DB_ADDR *);								)
CL d_crset			PRM( (DB_ADDR *);								)

CL d_dbget			PRM( (int *); )
CL d_dbset			PRM( (int); )

CL d_records		PRM( (unsigned long, unsigned long *);			)
CL d_keys			PRM( (unsigned long);							)

CL d_dbdpath		PRM( (char *);									)
CL d_dbfpath		PRM( (char *);									)

CL d_reclock		PRM( (DB_ADDR *, int); 							)
CL d_recunlock		PRM( (DB_ADDR *);								)

CL d_keyfrst		PRM( (unsigned long);							)
CL d_keylast		PRM( (unsigned long);							)
CL d_keyprev		PRM( (unsigned long);							)
CL d_keynext		PRM( (unsigned long);							)

CL d_recfrst		PRM( (unsigned long);							)
CL d_reclast		PRM( (unsigned long);							)
CL d_recprev		PRM( (unsigned long);							)
CL d_recnext		PRM( (unsigned long);							)

CL d_getsequence	PRM( (unsigned long, unsigned long *);			)

CL d_replicationlog	PRM( (int);										)
CL d_addsite		PRM( (unsigned long);							)
CL d_delsite		PRM( (unsigned long);							)
CL d_deltable		PRM( (unsigned long, unsigned long);							)
   
CL d_getkeysize		PRM( (unsigned long, unsigned *);				)
CL d_getrecsize		PRM( (unsigned long, unsigned *);				)
CL d_getfieldtype	PRM( (unsigned long, unsigned *);				)
CL ty_ustrcmp		PRM( (unsigned char *, unsigned char *);		)
CL d_getkeyid		PRM( (unsigned long, unsigned long *);							)
CL d_getforeignkeyid PRM( (unsigned long, unsigned long, unsigned long *); )
CL d_makekey		PRM( (unsigned long, void *, void *);			)

CL d_seterrfn		PRM( (void (*)(int, long));						)


#undef CL

#endif

/* end-of-file */
