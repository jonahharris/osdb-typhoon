/*----------------------------------------------------------------------------
 * File    : ddlpglob.h
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
 *   Contains global variables for ddlp.
 *
 * $Id: ddlpglob.h,v 1.2 1999/10/03 01:34:19 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifdef DEFINE_GLOBALS
#define CLASS
#define I(x)	= x
#else
#define CLASS	extern
#define I(x)
#endif

CLASS int       varlen_field_occurred			/* True if a varlen field   */
							I( 0		);		/* has occurred in the rec  */
CLASS sym_member*size_field	I( NULL		);      /* This var contains the    */
												/* name of the size field   */
                                                /* of a varlen field        */
CLASS int		only_keys	I( 0		);		/* Only extract key consts?	*/
CLASS int		warnholes	I( 0		);		/* Warn about align. holes	*/
CLASS int		lex_lineno 	I( 1		);		/* Current line number		*/
CLASS int		errors		I( 0		);		/* Number of errors			*/
CLASS int		dbpg_size	I( 512   	);		/* Database page size		*/
CLASS int		align		I( sizeof(long) );	/* Structure alignment		*/
CLASS int		conts		I( 0		);		/* Number of contains		*/
CLASS int		defines		I( 0		);		/* Number of defines		*/
CLASS unsigned	dims		I( 0		);		/* Number of dimensions		*/
CLASS unsigned	dim[20];						/* Array dimension			*/
CLASS char		dbname[DBNAME_LEN+1];			/* Database name			*/
CLASS char		dbdname[256];					/* .dbd file name			*/
CLASS char		ddlname[256];					/* .ddl file name			*/
CLASS char		hname[256];						/* Header file name			*/
CLASS Define	define[DEFINES_MAX];
CLASS Contains	*contains;

/*------------------- These tables go into the .dbd-file -------------------*/
CLASS Header	header;							/* .dbd file header			*/
CLASS Record	*record		I( NULL		);		/* Record table				*/
CLASS Field		*field		I( NULL		);		/* Field table				*/
CLASS File		*file		I( NULL		);		/* File table				*/
CLASS Key		*key		I( NULL		);		/* Key table				*/
CLASS KeyField	*keyfield	I( NULL		);		/* Key field table			*/
CLASS Structdef *structdef	I( NULL		);		/* Struct definition table	*/
CLASS Sequence	*sequence	I( NULL		);		/* Sequence definition table*/

CLASS int		records		I( 0        );		/* Number of records in db	*/
CLASS int		fields		I( 0		);		/* Number of fields in db	*/
CLASS int		files		I( 0	    );		/* Number of files in dbase	*/
CLASS int		keys	  	I( 0		); 		/* # of entries in key      */
CLASS int		keyfields 	I( 0		); 		/* # of entries in keyfield */
CLASS int		structdefs	I( 0		);		/* # of entries in structdef*/
CLASS int		sequences	I( 0		);		/* # of entries in sequence	*/

#ifdef DEFINE_GLOBALS

struct {
	char type;
	short size;
	char *name;
} typeinfo[] = {
	{ FT_CHAR,	sizeof(char),		"char",	 },
	{ FT_CHARSTR,	sizeof(char),		"char",	 },
	{ FT_SHORT,	sizeof(short),		"short", },
	{ FT_INT,		sizeof(int),		"int",	 },
	{ FT_LONG,	sizeof(long),		"long",		 },
	{ FT_FLOAT,	sizeof(float),		"float",	 },
	{ FT_DOUBLE,	sizeof(double),		"double",	 },
#ifdef __STDC__
	{ FT_LDOUBLE,	sizeof(long double),"long double",	 },
#else
	{ FT_LDOUBLE,	sizeof(double),		"double"	 },
#endif
};

#else

extern struct {
	char type;
	short size;
	char *name;
} typeinfo[];



#endif

/* end-of-file */
