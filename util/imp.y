/*----------------------------------------------------------------------------
 * File    : imp.y
 * Program : tyimport
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
 *   Grammar for import specification.
 *
 * $Id: imp.y,v 1.6 1999/10/04 05:29:46 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

%{

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"

#include "import.h"

#ifndef	NULL
#define	NULL	0
#endif

#define NEST_MAX	15

/*--------------------------- Function prototypes --------------------------*/
Record		*GetRecord	PRM( (char *); )
Field		*GetField	PRM( (Structdef *, char *); )
Structdef	*GetStruct	PRM( (Structdef *, char *); )
int			 yylex		PRM( (void); )

/*---------------------------- Global variables ----------------------------*/
static Record		*cur_rec = NULL;		/* Current record				*/
static Field		*cur_fld = NULL;		/* Current field				*/
static Structdef	*cur_str = NULL;		/* Current structure			*/
static Structdef	*strnest[NEST_MAX];		/* Pointers to structures		*/
static int			cur_nest = -1;			/* Current nesting				*/

%}

%union {
	char  	s[IDENT_LEN+1];
}

%start import_spec

%token 			T_IMPORT T_RECORD T_STRUCT T_UNION T_IN
%token <s>		T_IDENT T_STRING
%token 			'{' '}' ';'

%%

import_spec		: T_IMPORT T_IDENT '{' record_list '}'
				;

record_list		: record
				| record_list record
				;

record			: record_head '{' field_list '}'
					{
						cur_nest--;
					}
				;

record_head		: T_RECORD T_IDENT T_IN T_STRING 
					{
						if( ( cur_rec = GetRecord($2) ) != 0 )
						{
							cur_rec->aux = 1;
							cur_str = &dbd.structdef[cur_rec->structid];
						}
						else
							cur_str = NULL;
						strnest[++cur_nest] = cur_str;
					}
				;

field_list		: field 
				| field_list field 
				;
	
field			: T_IDENT ';'
					{
						if( cur_str )
							cur_fld = GetField(cur_str, $1);
					}

				| struct_head '{' field_list '}' ';'
					{
						cur_str = strnest[--cur_nest];
					}
				;

struct_head		: struct_or_union T_IDENT
					{
						if( cur_str )
							cur_str = GetStruct(cur_str, $2);
						strnest[++cur_nest] = cur_str;
					}
				;

struct_or_union	: T_STRUCT
				| T_UNION
				;


%%


#include <stdio.h>

extern int errors;

#ifdef CONFIG_PROTOTYPES
int yyerror(char *fmt, ...)
#else
int yyerror(fmt)
char *fmt;
#endif
{
	va_list ap;

	printf("%s %d: ", spec_fname, lex_lineno);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	puts("");
	va_end(ap);
	errors++;
	return 0;
}




Record *GetRecord(name)
char *name;
{
	int i;

	for( i=0; i<dbd.header.records; i++ )
		if( !strcmp(dbd.record[i].name, name) )
			return &dbd.record[i];

	yyerror("unknown record '%s'", name);
	exit(1);
	return NULL;
}



Field *GetField(str, name)
Structdef *str;
char *name;
{
	Field *fld = &dbd.field[str->first_member];
	int n = str->members;

	while( n )
	{
		if( fld->nesting == cur_nest )
		{
			if( !strcmp(fld->name, name) )
			{
				fld->type |= FT_INCLUDE;
				return fld;
			}
			n--;
		}
		fld++;
	}

	yyerror("'%s' is not a member of '%s'", name, str->name);
	exit(1);
	return NULL;
}


Structdef *GetStruct(str, name)
Structdef *str;
char *name;
{
	Field *fld;
	Structdef *struc;

	if( !(fld = GetField(str, name)) || 
		FT_GETBASIC(fld->type) != FT_STRUCT )
		return NULL;

	struc = &dbd.structdef[fld->structid];

	/* If the structure is a union the control field must also have been 
	 * specified
	 */
	if( struc->is_union )
	{
		if( !(dbd.field[struc->control_field].type & FT_INCLUDE) )
		{
			yyerror("The control field of the union '%s' is not included",
				name);
			exit(1);
		}
	}

	return struc;
}

/* end-of-file */

