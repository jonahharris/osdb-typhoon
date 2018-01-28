/*----------------------------------------------------------------------------
 * File    : export.c
 * Program : tyexport
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
 *   Typhoon export utility.
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

#include "export.h"

static CONFIG_CONST char rcsid[] = "$Id: export.c,v 1.7 1999/10/04 04:11:31 kaz Exp $";

/*-------------------------------- prototypes ------------------------------*/
static void PrintString		PRM( (uchar *, Field *fld); )
static void	PrintField		PRM( (Field *, unsigned); )
static int	GetControlField PRM( (Structdef *, unsigned); )
static int	PrintFields		PRM( (Structdef *, int, unsigned, int); )
static void	Export			PRM( (char *); )
static void ExportTable		PRM( (ulong); )
	   int	yyparse			PRM( (void); )
	   int	main			PRM( (int, char **); )

/*------------------------------ public variables --------------------------*/
int dbdfile;
int nocomma=0;
int nonull=0;
char *recbuf;
FILE *outfile;
FILE *lex_file;

/*------------------------------ local variables ---------------------------*/
static char paramhelp[] = "\
Syntax: tyexport [option]... database[.dbd]\n\
Options:\n\
    -f<path>    Specify data files path\n\
    -g          Generate export specification\n\
    -n          Strings are not null-terminated\n";

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

static void PrintString(s, fld)
uchar *s;
Field *fld;
{
	int len;

	if( fld->type & FT_VARIABLE )
		len = *(ushort *)(recbuf + dbd.field[ fld->keyid ].offset) * fld->elemsize;
	else
		len = fld->size;

	putc('"', outfile);

	while( len-- )
	{
		if( isprint(*s) )
		{
			if( *s == '"' )
				fputs("\\\"", outfile);
			else
			if( *s == '\\' )
				fputs("\\\\", outfile);
			else
				putc(*s, outfile);
		}
		else
		{
			if( !*s && !nonull )
				break;

			fprintf(outfile, "\\x%02X", *s);
		}
		s++;
	}
	putc('"', outfile);
}


static void PrintField(fld, offset)
Field *fld;
unsigned offset;
{
	void *ptr = (void *)(recbuf + offset);

	/*
	   Check for nonprintable characters in strings and character constants
	*/

	if( nocomma )
		nocomma = 0;
	else
		fprintf(outfile, ", ");

	if( fld->type & FT_UNSIGNED )
	{
		switch( FT_GETBASIC(fld->type) )
		{
			case FT_CHARSTR:
				PrintString(ptr, fld);
				break;
			case FT_CHAR:		
				fprintf(outfile, "%u", (unsigned)*(uchar *)ptr);	
				break;
			case FT_SHORT:
				fprintf(outfile, "%hu", *(ushort *)ptr);
				break;
			case FT_INT:
				fprintf(outfile, "%u", *(unsigned *)ptr);
				break;
			case FT_LONG:
				fprintf(outfile, "%lu", *(ulong *)ptr);
				break;
		}	
	}
	else
		switch( FT_GETBASIC(fld->type) )
		{
			case FT_CHARSTR:
				PrintString(ptr, fld);
				break;
			case FT_CHAR:
				fprintf(outfile, "%d", (int)*(char *)ptr);
				break;
			case FT_SHORT:
				fprintf(outfile, "%hd", *(short *)ptr);
				break;
			case FT_INT:
				fprintf(outfile, "%d", *(int *)ptr);
				break;
			case FT_UNSIGNED:
				fprintf(outfile, "%u", *(int *)ptr);
				break;
			case FT_LONG:
				fprintf(outfile, "%ld", *(long *)ptr);
				break;
			case FT_FLOAT:
				fprintf(outfile, "%f", *(float *)ptr);
				break;
			case FT_DOUBLE:
				fprintf(outfile, "%f", *(double *)ptr);
				break;
		}	
}


static int GetControlField(str, offset)
Structdef *str;
unsigned offset;
{
	unsigned id = recbuf[offset + dbd.field[str->control_field].offset];

	if( id >= str->members )
	{
		printf("Setting invalid control filed to 0\n");
		id = 0;
	}

	return id;
}


static int PrintFields(str, nest, offset, control_value)
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

				if( !nocomma )
					fprintf(outfile, ", ");
				if( fld->type & FT_INCLUDE )
					fprintf(outfile, "{ ");
				nocomma = 1;

	            rc  = PrintFields(struc, nest+1, 
						offset + fld->offset + i * fld->elemsize,
				     	struc->is_union ? GetControlField(struc, offset) : 0);

				if( fld->type & FT_INCLUDE )
					fprintf(outfile, " }");
			}
        	else if( fld->nesting == nest && fld->type & FT_INCLUDE )
				PrintField(fld, offset + fld->offset + fld->elemsize * i);
        }

		/* If n was 0 this array was a variable length array of size 0.
		 * Move fld to the next field at the same nesting.
 		 */
		if( n == 0 )
		{
			fprintf(outfile, ", { }");
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


static void ExportTable(recid)
ulong recid;
{
	Record *rec = &dbd.record[recid];
	Id keyid;

	recid = INTERN_TO_RECID(recid);
	keyid = rec->first_key;

	for( d_keyfrst(keyid); db_status == S_OKAY; d_keynext(keyid) )
	{
 		d_recread(recbuf);

		nocomma = 1;
		PrintFields(&dbd.structdef[rec->structid], 0, 0, 0);
		fprintf(outfile, "\n");
	}
}


static void Export(dbname)
char *dbname;
{
	char export_fname[256];
	int i;

	if( d_open(dbname, "s") != S_OKAY )
		err_quit("Cannot open database '%s'", dbname);

	for( i=0; i<dbd.header.records; i++ )
	{
		if( dbd.record[i].aux )
		{
			sprintf(export_fname, "%s.kom", dbd.record[i].name);

		 	if( !(outfile = fopen(export_fname, "w")) )
				err_quit("Cannot write to '%s'", export_fname);

			printf("exporting to '%s'\n", export_fname);
			ExportTable(i);
			fclose(outfile);
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

	puts("Typhoon Export Utility version 1.05");

	if( argc == 1 )
	{
		printf(paramhelp);
		exit(1);
	}

	/* Extract the real name of the file */
	if( (realname = strrchr(argv[argc-1], CONFIG_DIR_SWITCH)) != NULL )
		realname++;
	else
		realname = argv[argc-1];

	/* remove extension if present */
	if( ( p = strchr(realname, '.') ) != 0 )
		*p = 0;

	/* generate file names for .ddl-file, .dbd-file and header file */
	sprintf(dbd_fname,	"%s.dbd", realname);
	sprintf(spec_fname, "%s.exp", realname);

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
					d_dbfpath(argv[i]+2);
					break;
				case 'g':
					GenerateExportSpec(realname);
					exit(1);
				case 'n':
					nonull = 1;
					break;
				default:
					err_quit("unknown command line option");
			}
		}
		else
			err_quit("unknown command line option");
	}

	/* Read the export specification */
	ReadExportSpec(realname);

	if( !errors )
		Export(realname);

	free(dbd.dbd);
	free(recbuf);
	return 0;
}

/* end-of-file */
