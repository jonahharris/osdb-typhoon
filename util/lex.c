/*----------------------------------------------------------------------------
 * File    : @(#)lex.c       93/11/08         Copyright (c) 1993-94 CT Danmark
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

/*-------------------------- Function prototypes ---------------------------*/

#include <stdlib.h>

static int 	keywordcmp		PRM( (char *, char *); )



void lex_skip_comment()
{
	int c, start;

	start = lex_lineno;
	for( ;; )
	{
		switch( getc(lex_file) )
		{
			case '*':
				if( (c=getc(lex_file)) == '/' )
					return;
				else
					ungetc(c, lex_file);
				break;
			case '\n':
				lex_lineno++;
				break;
			case '/':
				if( (c=getc(lex_file)) == '*' )		/* nested comment		*/
					lex_skip_comment();
				else
					ungetc(c, lex_file);
				break;
			case EOF:
				fprintf(stderr, "unterminated comment starting in line %d\n", start);
				exit(1);
		}
	}
}


static int keywordcmp(ck,ce)
char *ck;
char *ce;
{
	return strcmp(ck, ((LEX_KEYWORD *)ce)->s);
}



int lex_parse_keyword(c)
int c;
{
	char *p = yylval.s;
	LEX_KEYWORD *kword;

	*p = c;

	do
		*++p = getc(lex_file);
	while( isalnum(*p) || *p=='_' );

	ungetc(*p, lex_file);
	*p = '\0';

	if( p - yylval.s > sizeof(yylval.s)-1 )
	{
		fprintf(stderr, "line %d: identifier too long, truncated\n", lex_lineno);
		yylval.s[sizeof(yylval.s)-1] = '\0';
	}

	kword = (LEX_KEYWORD *)bsearch(yylval.s,
					 		lex_keywordtab,
							lex_keywords,
							sizeof(LEX_KEYWORD),
							(LEX_CMPFUNC)keywordcmp);
	if( kword == NULL )
	{
#ifdef T_NUMBER
		int i;
		/* See if identifier is in define table */
		for( i=0; i<defines; i++ )
			if( !strcmp(yylval.s, define[i].name) )
			{
				yylval.val = define[i].value;
				return T_NUMBER;
			}
#endif	
		return T_IDENT;
	}

	return kword->token;
}

#ifdef T_NUMBER
int lex_parse_number(c)
int c;
{
  	yylval.val = 0;
	do
	{
		yylval.val = yylval.val * 10 + c - '0';
		c = getc(lex_file);
	}
	while( isdigit(c) );
	ungetc(c, lex_file);

	return T_NUMBER;
}
#endif


#ifdef T_STRING
int lex_parse_string()
{
	char *p = yylval.s;

	while( (*p = getc(lex_file)) != '"' )
	{
		if( *p == '\n' )
			yyerror("newline in string");
		else				
	        p++;
	}

	*p = '\0';

	return T_STRING;
}
#endif

#ifdef T_CHARCONST
int lex_parse_charconst()
{
	/* ' has already been read */
	yylval.val = getc(lex_file);
	
	if( getc(lex_file) != '\'' )
		yyerror("unterminated character constant");
		
	return T_CHARCONST;
}
#endif

void lex_skip_line()
{
	int c;

	while( (c = getc(lex_file)) != '\n' && c != EOF )
		;
	if( c == EOF )
		ungetc(EOF, lex_file);

	lex_lineno++;
}

/* end-of-file */
 
