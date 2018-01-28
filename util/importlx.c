/*----------------------------------------------------------------------------
 * File    : importlx.c
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
 *   Lexical analyser for import utility.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "environ.h"
#include "ty_dbd.h"

#include "ddlp.h"
#include "imp.h"
#include "ddlpsym.h"
#include "ddlpglob.h"
#include "lex.h"
#include "lex.c"

#if YYDEBUG
#define D(x) x
#else
#define D(x)
#endif


static CONFIG_CONST char rcsid[] = "$Id: importlx.c,v 1.6 1999/10/04 04:11:32 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/

int yylex PRM ( (void); )

/*---------------------------- Global variables ----------------------------*/
FILE	*lex_file;					/* input file							*/


LEX_KEYWORD lex_keywordtab[] = {
	{ T_IMPORT,		"import", },
	{ T_IN,			"in", },
	{ T_RECORD,		"record", },
	{ T_STRUCT,		"struct", },
	{ T_UNION,		"union" }
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
#if 0
		else if( isdigit(c) )				/* number						*/
			return lex_parse_number(c);
#endif
		else if( c == '"' )                 /* string       				*/
			return lex_parse_string();
		else if( c == '\n' )                /* increase line count			*/
			lex_lineno++;
		else if( c == EOF )
			return EOF;
 		else if( strchr("[]{};,+*-().", c) )
			return c;
		else if( c == '/' )
		{
			if( (c = getc(lex_file)) == '*' )/* C comment     				*/
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
		else
	        yyerror("syntax error");
	}
}

/* end-of-file */

