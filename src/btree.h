/*----------------------------------------------------------------------------
 * File    : btree.h
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
 *   Contains miscellaneous constants and macros used by B-tree functions.
 *
 * $Id: btree.h,v 1.5 1999/10/04 05:29:45 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_BTREE_H
#define TYPHOON_BTREE_H

#ifndef TYPHOON_TY_TYPE_H
#include "ty_type.h"
#endif

/*--------------------------------------------------------------------------*/
/*                      miscellaneous constants                             */
/*--------------------------------------------------------------------------*/
#define KEYVERSION_ID	"KeyMan121"		/* Version ID						*/
#define KEYVERSION_NUM	121				/* Version number					*/

#define NEWPOS          (ix_addr)-1		/* Indicates new pos for nodewrite	*/
#define ROOT			1				/* Root is always node 1			*/

typedef ix_addr A_type;         /* node address type                        */
typedef long R_type;            /* record reference type                    */
#ifdef CONFIG_RISC
typedef long N_type;
#else
typedef short N_type;
#endif


/*
 * The format of a node is illustrated below. <n> is the number of tuples in
 * the node. We call the set [A,K,R] a tuple, since the reference is considered
 * a part of the key.
 *
 *  +---+----+--------+----+----+--------+----+-- - -+------+---------+------+
 *  | n ! A0 |   K0   | R0 | A1 |   K1   | R1 |      | An-1 |  Kn-1   | Rn-1 |
 *  +---+----+--------+----+----+--------+----+-- - -+------+---------+------+
 *
 */

/*
 * The following macros are used to easily access the elements of a node. The
 * macros KEY, CHILD and REF assume that a variable <I->node> points to the
 * node operated on.
 */

#define NSIZE(N)	(*(N_type *)(N))
#define KEY(N,i)    (void   *)  (N+sizeof(N_type)+sizeof(A_type) + \
					I->tsize * (i))
#define CHILD(N,i)  (*(A_type *)(N+sizeof(N_type) + I->tsize * (i)))
#define REF(N,i)    (*(R_type *) (N+sizeof(N_type)+sizeof(A_type) + \
					I->aligned_keysize + I->tsize * (i)))



/*
 * The following macros are used to insert, delete and copy tuples in nodes.
 *
 * tupledel(N,i)			- Delete the i'th tuple of the node N.
 * tupleins(N,i,n)			- Insert the tuple n in the i'th position in the
 *							  node N.
 * tuplecopy(N1,i1,N2,i2,n) - Copy n tuples starting from the i2'th position
 *							  of node N2 to the i1'th position of the node N1.
 * nodecopy(N1,N2)			- Copy node N2 to node N1.
 * keycopy(N1,i1,N2,i2)		- Copy the i2'th key of node N2 to the i1'th key
 *							  of N1.
 */

#define tupledel(N,i)			memmove(&CHILD(N,i), &CHILD(N,(i)+1), \
								I->tsize * (NSIZE(N) - (i) - 1) + sizeof(A_type))
#define tupleins(N,i,n)         memmove(&CHILD(N,(i)+n), &CHILD(N,i), \
								I->tsize * (NSIZE(N) - (i)) + sizeof(A_type))
#define tuplecopy(N1,i1,N2,i2,n)memcpy(&CHILD(N1,i1), &CHILD(N2,i2), I->tsize*(n))
#define nodecopy(N1,N2)        	memcpy(N1, N2, sizeof(N_type) + sizeof(A_type) \
								+ (I->tsize * NSIZE(N2)))
#define keycopy(N1,i1,N2,i2)	memcpy(KEY(N1,i1),KEY(N2,i2),I->aligned_keysize + sizeof(R_type))


/*--------------------------------- bt_open --------------------------------*/
int		nodesearch		PRM( (INDEX *, void *, int *);					)
int		d_search		PRM( (INDEX *, void *, ix_addr *, int *);		)

/*--------------------------------- bt_io.c --------------------------------*/
ix_addr noderead        PRM( (INDEX *, char *, ix_addr);                )
ix_addr nodewrite       PRM( (INDEX *, char *, ix_addr);                )

#endif
/* end-of-file */
