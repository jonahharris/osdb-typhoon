/*----------------------------------------------------------------------------
 * File    : import.c
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
 *   Typhoon import utility.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include "environ.h"
#ifndef CONFIG_UNIX
#  include <sys\stat.h>
#  include <io.h>
#else
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#define DEFINE_GLOBALS
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"

#include "lex.h"
#include "import.h"

static CONFIG_CONST char rcsid[] = "$Id: import.c,v 1.7 1999/10/04 04:11:32 kaz Exp $";

/*-------------------------------- prototypes ------------------------------*/
static int  ReadValue		PRM( (int); )
static int  ReadString		PRM( (void); )
static int  ReadChar		PRM( (void); )
static int 	ReadField		PRM( (Field *, unsigned); )
static int	GetControlField PRM( (Structdef *, unsigned); )
static int	ReadFields		PRM( (Structdef *, int, unsigned, int); )
static void	Import			PRM( (char *); )
static void ImportTable		PRM( (ulong); )
	   int	yyparse			PRM( (void); )
static void import_error	PRM( (char * CONFIG_ELLIPSIS); )
	   int	main			PRM( (int, char **); )

/*------------------------------ public variables --------------------------*/
       FILE 	*lex_file;
static FILE		*infile;
static char		*recbuf;
static char		*fldptr;
static int		fldtype;
static int		lineno;
static char		import_fname[256];

/*------------------------------ local variables ---------------------------*/
static char paramhelp[] = "\
Syntax: tyimport [option]... database[.dbd]\n\
Options:\n\
    -f<path>    Specify data files path\n\
    -g          Generate import specification\n";

#ifdef CONFIG_PROTOTYPES
void err_quit(char *s, ...)
#else
void err_quit(s)
char *s;
#endif
{
	va_list ap;
	
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	puts("");
	va_end(ap);
	exit(1);
}


#ifdef CONFIG_PROTOTYPES
void import_error(char *fmt, ...)
#else
void import_error(fmt)
char *fmt;
#endif
{
	va_list ap;

	printf("%s %d: ", import_fname, lineno);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	puts("");
	va_end(ap);
	errors++;
}



/* Floats not supported!!! */

static int ReadValue(c)
int c;
{
	/* Integer formats:  002 (octal) 0x29 (hex) 231 (decimal) */
	ulong value;
	int negate = 0;

	if( c == '-' )
	{
		negate = 1;
		c = getc(infile);
	}

	if( c == '0' )
	{
		value = 0;

		c = getc(infile);

		if( c == 'x' )
		{
			while( (c = getc(infile)) && isxdigit(c) )
			{
				if( isdigit(c) )
					value = value * 16 + c - '0';
				else
					value = value * 16 + 6 + tolower(c) - 'a';
			}
			ungetc(c, infile);
		}
		else if( isdigit(c) )
		{
			do
			{
				value = value * 8 + c - '0';
				c = getc(infile);
			}
			while( isdigit(c) );
			ungetc(c, infile);
		}
		else
			ungetc(c, infile);
	}
	else
	{
		value = c - '0';

		c = getc(infile);

		while( isdigit(c) )
		{
			value = value * 10 + c - '0';
			c = getc(infile);
		}

/*
		do 
		{
			value = value * 10 + c - '0';
			c = getc(infile);
		}
		while( isdigit(c) );*/
		ungetc(c, infile);
	}

	if( fldtype & FT_UNSIGNED )
	{
		switch( fldtype )
		{
			case FT_UNSIGNED|FT_CHAR:	*(uchar *)fldptr	= value;	break;
			case FT_UNSIGNED|FT_SHORT:	*(ushort *)fldptr	= value;	break;
			case FT_UNSIGNED|FT_INT:	*(unsigned *)fldptr	= value;	break;
			case FT_UNSIGNED|FT_LONG:	*(ulong *)fldptr	= value;	break;
		}
	}
	else
	{
		long svalue = negate ? -value : value;

		switch( fldtype )
		{
			case FT_CHAR:  	*(char *)fldptr   	= svalue;	break;
			case FT_SHORT: 	*(short *)fldptr	= svalue;	break;
			case FT_INT:   	*(int *)fldptr		= svalue;	break;
			case FT_LONG:  	*(long *)fldptr   	= svalue;	break;
			case FT_FLOAT:
			case FT_DOUBLE:	puts("floats not supported"); exit(1);
		}
	}

	return 0;
}



/* check for max length */

static int ReadString()
{
	char *p = fldptr;
	int c;

	while( (*p = getc(infile)) != '"' )
	{
		if( *p == '\\' )
		{
			switch( c = getc(infile) )
			{	
				case 'n':							/* Newline				*/
					*p = '\n';
					break;
				case '\\':							/* Backslash			*/
					*p = '\\';
					break;
				case '\"':							/* Double-quote			*/
					*p = '"';
					break;
				case 'x':							/* Hexadecimal number	*/
				case 'X':
					c = getc(infile);
					if( isxdigit(c) )
					{
						*p = isalpha(c) ? tolower(c) - 'a' + 10 : c - '0';
						c = getc(infile);
						if( isxdigit(c) )
						{
							*p <<= 4;
							*p += isalpha(c) ? tolower(c) - 'a' + 10 : c - '0';
						}
						else
							ungetc(c, infile);
					}
/*					*p = 0;
					c = getc(infile);

					while( isxdigit(c) )
					{
		 				if( isdigit(c) )
		 					*p = *p * 16 + c - '0';
		 				else
		 					*p = *p * 16 + 10 + tolower(c) - 'a';
		 				c = getc(infile);
					}

					ungetc(c, infile);*/
					break;
				default:
					import_error("illegal character '%c' following '\\'", c);
					break;
			}		  
		}

		p++;		
	}
	*p = 0;

	return 0;
}


static int ReadChar()
{
	int c = getc(infile);
	int value, tmp;

	if( c == '\\' )
	{
		switch( tmp = getc(infile) )
		{
			case 'n':	value = '\n';	break;
			case 'r':	value = '\r';	break;
			case 't':	value = '\t';	break;
			case '\"':
			case '\\':
			case '\'':	value = tmp;	break;
			case 'x':
				c = getc(infile);
				value = 0;

				while( isxdigit(c) )
				{
				 	if( isdigit(c) )
				 		value = value * 16 + c - '0';
				 	else
				 		value = value * 16 + 10 + tolower(c) - 'a';
				 	c = getc(infile);
				}

				ungetc(c, infile);
				break;
			default:
				import_error("invalid character constant");
				return -1;
		}
	}
	else
		value = c;

	if( getc(infile) != '\'' )
		import_error("unterminated character constant");

	switch( FT_GETBASIC(fldtype) )
	{
		case FT_CHAR:	*(char *)fldptr = value;	break;
		case FT_INT:	*(int *)fldptr = value;		break;
		case FT_SHORT:	*(short *)fldptr = value;	break;
		case FT_LONG:	*(long *)fldptr = value;	break;
	}

	return 0;
}


static int ReadField(fld, offset)
Field *fld;
unsigned offset;
{
	int c;

	fldptr 	= recbuf + offset;
	fldtype = FT_GETBASIC(fld->type);

	for( ;; )
	{
		c = getc(infile);

		if( c == ' ' || c == '\t' || c == '{' || c == '}' || c == ',' )
			;
		else if( isdigit(c) || c == '-' )
			return ReadValue(c);
		else if( c == '"' )
			return ReadString();
		else if( c == '\'' )
			return ReadChar();
		else if( c == '\n' )
			lineno++;
		else if( c == '/' )
		{
			if( (c = getc(infile)) == '*' )	/* C comment   					*/
				lex_skip_comment();
			else if( c == '/' )			   	/* C++ comment 					*/
			{
				while( getc(infile) != '\n' && !feof(infile) )
					;
				lineno++;
			}
			else
				import_error("unexpected '/'\n");
		}
		else if( c == EOF )
			return -1;
		else
			import_error("unexpected '%c'\n", c);
	}
}



static int GetControlField(str, offset)
Structdef *str;
unsigned offset;
{
	return recbuf[offset + dbd.field[str->control_field].offset];
}



static int ReadFields(str, nest, offset, control_value)
Structdef *str;
int nest, control_value;
unsigned offset;
{
    Field	*fld		= dbd.field + str->first_member;
	int 	fields		= str->members;
    int 	old_fields	= fields;
    int 	i, n, rc;

	if( str->is_union )
		fld += control_value;

    while( fields-- )
    {
		if( fld->size != fld->elemsize && FT_GETBASIC(fld->type) != FT_CHARSTR )
		{
			if( fld->type & FT_VARIABLE )
				n = *(ushort *)(recbuf + dbd.field[ fld->keyid ].offset);
			else
				n = fld->size / fld->elemsize;
		}
		else
			n = 1;

		for( i=0; i<n; i++ )
		{
	        if( FT_GETBASIC(fld->type) == FT_STRUCT )
    	    {
	            Structdef *struc = dbd.structdef + fld->structid;

	            rc  = ReadFields(struc, nest+1, 
						offset + fld->offset + i * fld->elemsize,
				     	struc->is_union ? GetControlField(struc, offset) : 0);
			}
        	else if( fld->nesting == nest && fld->type & FT_INCLUDE )
        	{
				if( ReadField(fld, offset + fld->offset + fld->elemsize * i) == -1 )
					return -1;
			}
        }

		/* If n was 0 this array was a variable length array of size 0.
		 * Move fld to the next field at the same nesting.
 		 */
		if( n == 0 )
		{
			rc = 0;

			if( !fields )
				break;

			do
				rc++;
			while( fld[rc].nesting != fld->nesting );
			rc--;
		}

		if( FT_GETBASIC(fld->type) == FT_STRUCT )
		{
   	        old_fields += rc;
            fld += rc;
		}

        fld++;

		if( str->is_union )
			break;
    }

    return old_fields;
}


static void ImportTable(recid)
ulong recid;
{
	Record *rec = &dbd.record[recid];

	recid = INTERN_TO_RECID(recid);

	memset(recbuf, 0, rec->size);

	while( ReadFields(&dbd.structdef[rec->structid], 0, 0, 0) != -1 )
	{
		if( d_fillnew(recid, recbuf) != S_OKAY )
			printf("d_fillnew: db_status %d, db_subcode %ld\n", 
				db_status, db_subcode);
		memset(recbuf, 0, rec->size);
	}
}


static void Import(dbname)
char *dbname;
{
	int i;

	if( d_open(dbname, "s") != S_OKAY )
		err_quit("Cannot open database '%s'", dbname);

	for( i=0; i<dbd.header.records; i++ )
	{
		if( dbd.record[i].aux )
		{
			lineno = 1;
			sprintf(import_fname, "%s.kom", dbd.record[i].name);

			if( !(infile = fopen(import_fname, "r")) )
				err_quit("Cannot open '%s'", import_fname);

			printf("importing from '%s'\n", import_fname);
			ImportTable(i);
			fclose(infile);
		}
	}

	d_close();
}



int main(argc, argv)
int argc;
char *argv[];
{
	char *p, *realname;
	int i;
	unsigned biggest_rec=0;

	puts("Typhoon Import Utility version 1.06");

	if( argc == 1 )
	{
		printf(paramhelp);
		exit(1);
	}

	/* Imtract the real name of the file */
	if( (realname = strrchr(argv[argc-1], CONFIG_DIR_SWITCH)) != NULL )
		realname++;
	else
		realname = argv[argc-1];

	/* remove extension if present */
	if( ( p = strchr(realname, '.') ) != 0 )
		*p = 0;

	/* generate file names for .ddl-file, .dbd-file and header file */
	sprintf(dbd_fname,	 "%s.dbd", realname);
	sprintf(spec_fname,  "%s.imp", realname);

	if( read_dbdfile(&dbd, dbd_fname) != S_OKAY )
		err_quit("Cannot open '%s'\n", dbd_fname);

	/* Find the size of the biggest record */
	for( i=0; i<dbd.header.records; i++ )
		if( biggest_rec < dbd.record[i].size )
			biggest_rec = dbd.record[i].size;

	/* Allocate record buffer */
	if( !(recbuf = (char *)malloc(biggest_rec)) )
		err_quit("Out of memory");

	/* process command line options */
	for( i=1; i<argc-1; i++ )
	{
		if( argv[i][0] == '-' || argv[i][0] == '/' )
		{
			switch( argv[i][1] )
			{
				case 'f':
					if( d_dbfpath(argv[i]+2) != S_OKAY )
						err_quit("Invalid data files path");
					break;
				case 'g':
					GenerateImportSpec(realname);
					exit(1);
				default:
					err_quit("unknown command line option");
			}
		}
		else
			err_quit("unknown command line option");
	}

	/* Read the import specification */
	ReadImportSpec(realname);

	if( !errors )
		Import(realname);

	free(dbd.dbd);
	free(recbuf);
	return 0;
}


/* end-of-file */
