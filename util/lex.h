/*----------------------------------------------------------------------------
 * File    : @(#)lex.h       93/11/08         Copyright (c) 1993-94 CT Danmark
 * Compiler: UNIX C, Turbo C, Microsoft C
 * OS      : UNIX, OS/2, DOS
 * Author  : Thomas B. Pedersen
 *
 * Description:
 *   General functions for lexical analysers.
 *
 * Revision History
 * ----------------------------------------
 * 11-Nov-1993 tbp  Initial version.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"

/*-------------------------- Function prototypes ---------------------------*/
void	lex_skip_comment	PRM( (void); )
int		lex_parse_keyword	PRM( (int); )
int		lex_parse_number	PRM( (int); )
int		lex_parse_string	PRM( (void); )
int		lex_parse_charconst	PRM( (void); )
void	lex_skip_line		PRM( (void); )

typedef int (*LEX_CMPFUNC) PRM( (const void *, const void *); )

typedef struct {
	short	token;
	char	*s;	
} LEX_KEYWORD;


/* These variables must be defined in the program that uses lex.c 
 */
extern LEX_KEYWORD	lex_keywordtab[];
extern size_t 			lex_keywords;
extern FILE		   *lex_file;
extern int		   	lex_lineno;


/* end-of-file */
																			  
