/*----------------------------------------------------------------------------
 * File    : ddlpsym.h
 * Program : ddlp
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
 *   Symbol table definitions.
 *
 * $Id: ddlpsym.h,v 1.3 1999/10/04 05:29:45 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

/* The following constants define translation limits of the parser according to
 * ISO/IEC DIS 9899 section 2.2.41.
 */

#define NEST_MAX		15					/* Max nesting level			*/
#define NAME_LEN		31					/* Number of significant initial*/
											/* characters in a name			*/

typedef struct sym_member {
	char				type;				/* See FT_.. constants			*/
	char				name[NAME_LEN+1];	/* Member name					*/
    unsigned            elemsize;           /* Size of each element         */
	unsigned			size;				/* sizeof (total size incl. dim)*/
	unsigned			offset;				/* Offset from start of struct	*/
	struct sym_member	*next;				/* Next member					*/
	struct sym_struct	*struc;				/* If type is FT_STRUCT this	*/
											/* points to the structdef that	*/
											/* defines the struct type		*/
	/*---------- ddlp extensions -------------------------------------------*/
	Id					id;					/* Field id						*/
	/*----------------------------------------------------------------------*/
	unsigned			dims;				/* Number of dimensions			*/
	unsigned			dim[1];				/* Dimensions if <dims> > 0		*/
} sym_member;

typedef struct sym_struct {
	char				name[NAME_LEN+1];	/* Struct name					*/
	char				printed;			/* This field ensures that the	*/
											/* full structure definition is */
											/* printed once only.			*/
	unsigned char		is_union;			/* True if this is a union		*/
	unsigned			size;				/* sizeof						*/
	unsigned			members;			/* Number of members			*/
	sym_member		 	*first_member;		/* First member					*/
	sym_member		 	*last_member;		/* Last member					*/
	struct sym_struct	*next;				/* Next structdef				*/
	/*---------- ddlp extensions -------------------------------------------*/
	Id					fieldid;   			/* Field id						*/
	/*----------------------------------------------------------------------*/
} sym_struct;



/*
	+--------+    +--------+    +--------+    +--------+
	| struct |--->| member |--->| member |--->| member |--->NULL
	+--------+    +--------+    +--------+    +--------+
		 |
		 |
		 v
	+--------+    +--------+    +--------+    +--------+
	| struct |--->| member |--->| member |--->| member |--->NULL
	+--------+    +--------+    +--------+    +--------+
		 |
		 |
		 v
	+--------+    +--------+
	| struct |--->| member |
	+--------+    +--------+
		 |
		 |
		 v
	   NULL
*/

extern sym_struct *structnest[], *cur_str, *last_str;
extern int curnest;

void		 sym_addmember		PRM( (char *, int, sym_struct *); )
void		 sym_addstruct		PRM( (char *, int); )
void		 sym_endstruct		PRM( (void); )
sym_struct	*sym_findstruct		PRM( (char *, int); )
sym_member	*sym_findmember		PRM( (sym_struct *, char *); )
void		 print_structures	PRM( (void); )


/* end-of-file */
