/*----------------------------------------------------------------------------
 * File    : ddlplex.c
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
 *   Lexical analyser for ddlp.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "environ.h"
#ifndef CONFIG_UNIX
#	include <stdlib.h>
#endif

#include "typhoon.h"
#include "ty_dbd.h"

#include "ddlp.h"
#include "ddl.h"
#include "ddlpsym.h"
#include "ddlpglob.h"
#include "lex.h"
#include "lex.c"

static CONFIG_CONST char rcsid[] = "$Id: ddlplex.c,v 1.6 1999/10/04 04:11:31 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/

int		yylex				PRM ( (void); )

/*---------------------------- Global variables ----------------------------*/
extern FILE *lex_file;		   		/* input file							*/
extern int 	 lex_lineno;	   		/* current line number					*/

LEX_KEYWORD lex_keywordtab[] = {
	{ T_ALTERNATE,	"alternate", },
	{ T_ASC,		"asc", },
	{ T_BY,       	"by", },
	{ T_CASCADE,	"cascade", },
	{ T_CHAR,		"char", },
	{ T_CONTAINS,	"contains", },
	{ T_CONTROLLED,	"controlled", },
	{ T_DATA,		"data", },
	{ T_DATABASE,	"database", },
	{ T_DEFINE,		"define", },
	{ T_DELETE,		"delete", },
	{ T_DESC,		"desc", },
	{ T_DOUBLE,		"double", },
	{ T_FILE,		"file", },
	{ T_FLOAT,		"float", },
	{ T_FOREIGN,	"foreign", },
	{ T_INT,		"int", },
	{ T_KEY,		"key", },
	{ T_LONG,		"long", },
	{ T_MAP,		"map", },
	{ T_NULL,		"null", },
	{ T_ON,			"on", },
	{ T_PRIMARY,	"primary", },
	{ T_RECORD,		"record", },
	{ T_REFERENCES,	"references", },
	{ T_RESTRICT,	"restrict", },
	{ T_SEQUENCE,	"sequence", },
	{ T_SHORT,		"short", },
	{ T_SIGNED,		"signed", },
	{ T_STRUCT,		"struct", },
	{ T_UCHAR,		"uchar", },
	{ T_ULONG,		"ulong", },
	{ T_UNION,		"union", },
	{ T_UNIQUE,		"unique", },
	{ T_UNSIGNED,	"unsigned", },
	{ T_UPDATE,		"update", },
	{ T_USHORT,		"ushort", },
    { T_VARIABLE, 	"variable" }
};

size_t lex_keywords = sizeof(lex_keywordtab) / sizeof(lex_keywordtab[0]);

int yylex()
{
	int c;

	for( ;; )
	{
		c = getc(lex_file);

		if( c == ' ' || c == '\t' )         /* skip whitespace 				*/
			;
		else if( isalpha(c) )				/* keyword      				*/
			return lex_parse_keyword(c);
		else if( isdigit(c) )				/* number						*/
			return lex_parse_number(c);
		else if( c == '"' )                 /* string       				*/
			return lex_parse_string();
		else if ( c== '\'' )
			return lex_parse_charconst();	/* character constant			*/
		else if( c == '\n' )                /* increase line count			*/
			lex_lineno++;
		else if( c == EOF )
			return EOF;
		else if( c == '/' )
		{
			if( (c = getc(lex_file)) == '*' )	/* C comment   				*/
				lex_skip_comment();
			else if( c == '/' )			   	/* C++ comment 					*/
			{
				while( getc(lex_file) != '\n' && !feof(lex_file) )
					;
				lex_lineno++;
			}
			else
			{
				ungetc(c, lex_file);
				return '/';
			}
		}
		else if( c == '-' )
		{
			if( (c=getc(lex_file)) == '>' )
				return T_ARROW;
			ungetc(c, lex_file);
			return '-';
		}
		else if( strchr("[]{};,+*().", c) )
			return c;
		else if( c == '#' )
		{
			while( getc(lex_file) != '\n' && !feof(lex_file) )
				;
			lex_lineno++;
		}
		else
	        yyerror("syntax error");
	}
}

/* e
nd-of-file */
