/*----------------------------------------------------------------------------
 * File    : ddlp.h
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
 *   ddlp header file.
 *
 * $Id: ddlp.h,v 1.2 1999/10/03 23:28:29 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

/*-------------------------------- Constants -------------------------------*/
#define RECORDS_MAX		256		/* Maximum number of records				*/
#define STRUCTDEFS_MAX	256		/* Maximum number of structure definitions	*/
#define FIELDS_MAX		512		/* Maximum number of fields					*/
#define FILES_MAX		512		/* Maximum number of files					*/
#define KEYS_MAX		256		/* Maximum number of keys					*/
#define KEYFIELDS_MAX	1024	/* Maximum number of key fields				*/
#define DEFINES_MAX		200		/* Maximum number of defines				*/
#define CONTAINS_MAX	512		/* Maximum number of contains statements	*/

/*
 * This structure contains integer constants defined in the database
 * definition. These definitions are exported to the header file.
 */

typedef struct {
	int		value;
	char	name[IDENT_LEN+1];
} Define;


/*------------------------------- ddlp.c -----------------------------------*/
void	err_quit   			PRM( (char * CONFIG_ELLIPSIS);       )
void	align_offset		PRM( (unsigned *, int); )
void	add_key				PRM( (char *); )
void	add_keyfield		PRM( (Id, int); )
void	add_define			PRM( (char *, int); )
void	add_file			PRM( (int, char *, unsigned); )
void	add_contains		PRM( (int, char *, char *); )
void	add_record			PRM( (char *); )
void	add_field			PRM( (char *, int); )
void	add_structdef		PRM( (char *, int, int); )
void	add_sequence		PRM( (char *, ulong, int, ulong); )
void	check_foreign_key	PRM( (char *, Key *); )

/*-------------------------------- ddl.y -----------------------------------*/
int		yyerror				PRM( (char * CONFIG_ELLIPSIS); )

/*------------------------------- ddlplex.c --------------------------------*/
void	init_lex			PRM( (void); )


/* end-of-file */
