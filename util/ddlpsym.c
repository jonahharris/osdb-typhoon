/*----------------------------------------------------------------------------
 * File    : ddlpsym.c
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
 *   Symbol table functions for ddlp.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "environ.h"
#include "ty_dbd.h"

#include "ddlp.h"
#include "ddlpsym.h"
#include "ddlpglob.h"

static CONFIG_CONST char rcsid[] = "$Id: ddlpsym.c,v 1.5 1999/10/03 23:36:49 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/
static void print_struct		PRM( (sym_struct *, int); )

/*---------------------------- Global variables ----------------------------*/
sym_struct	*first_str = NULL;			/* Pointer to start of struct table	*/
sym_struct	*last_str = NULL;			/* Pointer to last struct 			*/
sym_struct	*cur_str = NULL;			/* Pointer to current struct 		*/
sym_struct	*structnest[NEST_MAX];		/* Structure nesting table			*/
int			curnest = -1;				/* Current structure nesting		*/


/*------------------------------ sym_addmember -----------------------------*\
 *
 * Purpose:		This function adds a member to the structure currently
 *				being defined. The member is aligned according to <align>.
 * 
 * Params :		name	- struct member name.
 *				type	- member type, see FT_.. constants.
 *				...		- If <type> == FT_STRUCT the 3rd parameter is a
 *						  pointer to a structdef record, otherwise it is
 *						  the size of the field.
 * 
 * Returns : 	Nothing
 *
 */

void sym_addmember(name, type, struc)
char *name;
int type;
sym_struct *struc;
{
	sym_struct *str = structnest[curnest];
	sym_member *mem;
	int size;

	/* Make sure that this member name is not already used in this struct */
	for( mem = str->first_member; mem; mem = mem->next )
		if( !strcmp(name, mem->name) )
		{
			yyerror("duplicate member name '%s'", name);
			return;
		}

	size = sizeof(*mem) + (dims-1) * sizeof(*mem->dim);

	if( !(mem = (sym_member *)calloc((size_t) 1, (size_t) size)) )
		err_quit("out of memory");

	if( type == FT_STRUCT )
	{
		mem->struc	= struc;
		mem->size	= struc->size;
	}
	else
		mem->size	= typeinfo[(type & FT_BASIC)-1].size;

	strcpy(mem->name, name);
	mem->type		= type;
	mem->dims		= dims;
    mem->elemsize	= mem->size;

	/* If this member has arrays they must be stored and the size adjusted */
	if( dims )
	{
		memcpy(mem->dim, dim, sizeof(*dim) * dims);

		while( --dims )
			dim[0] *= dim[dims];
		mem->size *= dim[0];
	}
		
	/* Add the new member to the member list */
	if( str->last_member )
		str->last_member->next = mem;
	else
		str->first_member = mem;
	str->last_member = mem;
	str->members++;

	/*
	 * If the structure is a union all members must be at offset 0, and 
	 * the union must have the size of the biggest member.
	 */
	if( str->is_union )
	{
		if( mem->size > str->size )
			str->size = mem->size;
	}
	else
	{
		align_offset(&str->size, type);
		mem->offset  = str->size;
		str->size	+= mem->size;
	}
}


void sym_addstruct(name, is_union)
char *name;
int is_union;
{
	sym_struct *str;

	/* Allocate new structdef and clear all fields */
 	if( !(str = (sym_struct *)calloc(1, sizeof *str)) )
		err_quit("out of memory");

	strcpy(str->name, name);
	str->is_union		= is_union;

	/* Insert structure in list */
	if( !first_str )
		first_str = str;
	else
		last_str->next = str;
	cur_str = last_str = str;

	structnest[++curnest] = str;
}


void sym_endstruct()
{
	/* The size of a structure depends depends on the type of the last member
	 */
/*	align_offset(&structnest[curnest]->size, FT_LONG);*/
	align_offset(&structnest[curnest]->size, cur_str->last_member->type);

	cur_str = structnest[--curnest];
}


sym_struct *sym_findstruct(name, is_union)
char *name;
int is_union;
{
	sym_struct *str = first_str;
	
	while( str )
	{
		if( !strcmp(name, str->name) && str->is_union == is_union )
			return str;
	
		str = str->next;
	}
	
	yyerror("unknown struct '%s'", name);
	return NULL;
}


sym_member *sym_findmember(str, name)
sym_struct *str;
char *name;
{
	sym_member *mem = str->first_member;
	
	while( mem )
	{
		if( !strcmp(name, mem->name) )
			return mem;

		mem = mem->next;
	}
	
	return NULL;		
}


static void print_struct(str, nesting)
sym_struct *str;
int nesting;
{
	static char *type[] = { "struct", "union" };
	sym_member *mem = str->first_member;
    extern FILE *hfile;
	int i;

	fprintf(hfile, "%*s%s %s {  /* size %d */\n", nesting * 4, "",
		type[str->is_union],
		str->name,
		str->size);

	while( mem )
	{
		if( mem->type == FT_STRUCT )
		{
			if( mem->struc->printed )
			{
				fprintf(hfile, "%*s%s %s ",
					(nesting+1)*4, "",
					type[mem->struc->is_union],
					mem->struc->name);
			}
			else
			{
				mem->struc->printed = 1;			
				print_struct(mem->struc, nesting+1);
				fprintf(hfile, "%*s} ", (nesting+1)*4, "");
			}
		} 
		else
		{
			fprintf(hfile, "%*s", (nesting+1) * 4, "");
			if( mem->type & FT_UNSIGNED )
				fprintf(hfile, "unsigned ");
			fprintf(hfile, "%-8s", typeinfo[(mem->type & FT_BASIC) - 1].name);
		}

		fprintf(hfile, "%s", mem->name);

		for( i=0; i<mem->dims; i++ )
			fprintf(hfile, "[%d]", mem->dim[i]);

		fprintf(hfile, ";\n");
		mem = mem->next;
	}		

	if( nesting == 0 )
		fprintf(hfile, "};\n\n");
}


void print_structures()
{
    sym_struct *str = first_str;

    while( str )
    {
        if( !str->printed )
            print_struct(str, 0);
        str = str->next;
    }
}


/* end-of-file */
