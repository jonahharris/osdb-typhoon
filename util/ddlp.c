/*----------------------------------------------------------------------------
 * File    : ddlp.c
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
 *   Grammar for ddl-files.
 *
 *--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include "environ.h"
#ifndef CONFIG_UNIX
#  include <sys\types.h>
#  include <sys\stat.h>
#  include <io.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#define DEFINE_GLOBALS
#include <sys/types.h>

#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"

#include "ddlp.h"
#include "ddlpsym.h"
#include "ddlpglob.h"
#include "ddl.h"

static CONFIG_CONST char rcsid[] = "$Id: ddlp.c,v 1.6 1999/10/04 04:11:31 kaz Exp $";


/*-------------------------- Function prototypes ---------------------------*/
int			yyparse				PRM( (void); )
void	   *extend_table		PRM( (void **, int *, int); )
void		align_offset		PRM( (unsigned *, int); )
void		check_consistency	PRM( (void); )
void		print_fieldname 	PRM( (FILE *, int, Id); )
void		fix_file			PRM( (void); )
void		fix_fields			PRM( (void); )

int			main				PRM( (int, char **); )

static void	init_vars			PRM( (void); )
static char *strupr				PRM( (char *); )
static int	istrcmp				PRM( (char *, char *); )

/*---------------------------- Global variables ----------------------------*/
FILE *lex_file, *hfile;
int dbdfile;

static char paramhelp[] = "\
Syntax: ddlp [option]... file[.ddl]\n\
Options:\n\
    -a[1|2|4|8]   Set structure alignment (default is %d)\n\
    -f            Only generate field constants for keys\n\
    -h<file>      Use <file> as header file\n";



static char *strupr(s)
char *s;
{
	char *olds = s;

	while( *s )
	{
    	*s = toupper(*s);
		s++;
	}

	return olds;
}


static int istrcmp(s1, s2)
char *s1, *s2;
{
   	while( tolower(*s1) == tolower(*s2) && *s1 && *s2 )
      	s1++, s2++;
   
   	return tolower(*s1) - tolower(*s2);
}


void *extend_table(table, elems, elemsize)
void **table;
int *elems, elemsize;
{
	/* Some implementations of realloc() choke if they get a NULL pointer,
	 * so we handle an empty table as a special case.
	 */
	if( !*table )
	{
		if( !(*table = (void *)calloc((size_t) 50, (size_t) elemsize)) )
		{
			yyerror("out of memory");
			exit(1);
		}
	}
	else if( !(*elems % 50) )
	{
		if( !(*table = (void *)realloc((void *) *table, (size_t) ((*elems + 50) * elemsize))) )
		{
			yyerror("out of memory");
			exit(1);
		}
		
		memset((char *)*table + elemsize * (*elems), 0, (size_t) (50 * elemsize));
	}

	return (void *)((char *)*table + elemsize * ((*elems)++));
}



/*------------------------------ align_offset ------------------------------*\
 *
 * Purpose   : Align an address to fit the next field type.
 *
 * Parameters: offset   - Offset to align.
 *			   type     - Field type (FT_...).
 *
 * Returns   : offset   - The aligned offset.
 *
 */
void align_offset(offset, type)
unsigned *offset;
int type;
{
	int rem = *offset % align;
	int size;

/*	if( type == FT_STRUCT )
		return;*/

	if( !rem || FT_GETBASIC(type) == FT_CHAR || !*offset )
    	return;

	if( type == FT_STRUCT )
    	size = sizeof(long);
	else
    	size = typeinfo[FT_GETBASIC(type) - 1].size;

	if( rem == size )
    	return;

	if( size < align - rem  )
    	*offset += rem;
	else
    	*offset += align - rem;
}


/*--------------------------------- add_key --------------------------------*\
 *
 * Purpose   : Adds a key definition to the record currently being defined.
 *
 * Parameters: name     - Key name
 *
 * Returns   : Nothing.
 *
 */
void add_key(name)
char *name;
{
	Key *k = extend_table((void **)&key, &keys, sizeof *key);

	/* Add an entry to the key table */
	k->first_keyfield	= keyfields;
	k->fields			= 0;
	k->size				= 0;
	k->fileid			= -1;
	strcpy(k->name, name);

	/* Increase the number of keys in the record */
	record[records-1].keys++;
}


/*------------------------------- add_keyfield -----------------------------*\
 *
 * Purpose   : Adds a field to the key currently being defined.
 *
 * Parameters: id       - Field id.
 *			   ascending   - Is key sorted in ascending order?
 *
 * Returns   : Nothing.
 *
 */

void add_keyfield(id, ascending)
Id id;
int ascending;
{
	KeyField *keyfld = extend_table((void **)&keyfield, &keyfields, 
							sizeof *keyfld);

	keyfld->field  = id;
	keyfld->asc    = ascending;

	/* The offset is only used in compound keys */
	if( key[keys-1].fields )
		keyfld->offset = cur_str->last_member->offset;
	else
		keyfld->offset = 0;

	/* Increase the number of fields of the key currently being defined */
	key[keys-1].fields++;
}


void add_define(name, value)
char *name;
int value;
{
	strcpy(define[defines].name, name);
	define[defines++].value = value;
}


void add_file(type, name, pagesize)
int type;
char *name;
unsigned pagesize;
{
	File *fil = extend_table((void **)&file, &files, sizeof *file);

	fil->id			= -1;   	          /* Unresolved           */
	fil->type		= type;
	fil->pagesize	= pagesize;
	strcpy(fil->name, name);
}


/*------------------------------ add_contains ------------------------------*\
 *
 * Purpose   : Add a new entry to the contains table. This table is used to
 *             determine which records and keys references and are referenced
 *             by the file table entries.
 *
 * Parameters: type     - 'k' = key, 'r' = record.
 *             record   - Record name.
 *             key      - Key name ("" if type is 'r').
 *
 * Returns   : Nothing.
 *
 */
void add_contains(type, record, key)
int type;
char *record, *key;
{
    Contains *con = extend_table((void **)&contains, &conts, sizeof *con);

	strcpy(con->record, record);
	strcpy(con->key, key);
	con->fileid = files;
	con->type   = type;
	con->line   = lex_lineno;
}


/*------------------------------- add_record -------------------------------*\
 *
 * Purpose   : This function adds a new entry to the record table. The fileid
 *             is set to -1 to indicate that the index into file[] is
 *             unresolved. This file be fixed by fix_file().
 *
 * Parameters: name  - Record name.
 *
 * Returns   : Nothing.
 *
 */
void add_record(name)
char *name;
{
    Record *rec = extend_table((void **)&record, &records, sizeof *record);
	int i;

	/* Add an entry to the record table */
	rec->first_field	= fields;
	rec->first_key		= keys;
	rec->first_foreign	= -1;
	rec->size			= 0;
	rec->is_vlr			= 0;
	rec->fileid			= -1;
	rec->structid		= structdefs;
	strcpy(rec->name, name);

	/* If this record has a reference file, <ref_file> must be set */
	for( i=0; i<conts; i++ )
    	if( !strcmp(contains[i].record, name) && !strcmp(contains[i].key, "<ref>") )
        	break;

	if( i < conts )
    	rec->ref_file = contains[i].fileid;
	else
    	rec->ref_file = -1;

	/* No variable length fields have occurred in this record so far */
	varlen_field_occurred = 0;
}


/*------------------------------- add_field --------------------------------*\
 *
 * Purpose   : Adds a new entry to the field table.
 *
 *             If the <size_field> variable is not empty it contains the
 *             name of the field that determines the size of the field being
 *             added. Thus, if <size_field> is non-empty, the field being
 *             added is of variable length.
 *
 * Parameters: name  - Field name.
 *             type  - Field type. Contains FT_... flags.
 *
 * Returns   : Nothing.
 *
 */
void add_field(name, type)
char *name;
int type;
{
	sym_member *mem = cur_str->last_member;
	Field *fld = extend_table((void **)&field, &fields, sizeof *field);

	/* If the field is a char string the FT_CHARSTR flag must be set */
	if( FT_GETBASIC(type) == FT_CHAR && mem->size > 1 )
	{
    	type &= ~FT_BASIC;			/* Clear the basic flags and set		*/
    	type |=  FT_CHARSTR;		/* the FT_CHARSTR flag instead     		*/
	}

	fld->recid     = records-1;		/* Link field to current record			*/
	fld->type      = type;
	fld->nesting   = curnest;
	strcpy(fld->name, name);

	if( type != FT_STRUCT )
	{
    	fld->size      = mem->size;
    	fld->offset    = mem->offset;
    	fld->elemsize  = mem->elemsize;
	}

	record[records-1].fields++;

	/* Check if the current field is of variable length */
	if( size_field )
	{
    	if( mem->dims != 1 )
        	yyerror("variable size fields must be one-dimension arrays");

    	if( (size_field->type & (FT_SHORT|FT_UNSIGNED)) != (FT_SHORT|FT_UNSIGNED) )
        	yyerror("size field '%s' must be 'unsigned short'", size_field->name);

    	/* Store id of size field in keyid (dirty, but saves space) */
    	fld->keyid = size_field->id;
    	fld->type |= FT_VARIABLE;
    	record[records-1].is_vlr = 1;

    	size_field = NULL;

    	/* Ensure that no fixed length fields follow */
    	varlen_field_occurred = 1;
	}
	else if( varlen_field_occurred && curnest == 0 )
    	yyerror("fixed length field '%s' follows variable length field", name);
}



/*------------------------------ add_structdef -----------------------------*\
 *
 * Purpose   : Add a structure definition.
 *
 * Parameters: name			- Structure name.
 *			   is_union		- Is the structure a union.
 *			   control_field- If <is_union> is true this is the id of the
 *                     		  control field.
 *
 * Returns   : Nothing.
 *
 */
void add_structdef(name, is_union, control_field)
char *name;
int is_union, control_field;
{
	Structdef *str = extend_table((void **)&structdef, &structdefs, 
						sizeof *structdef);

	strcpy(str->name, name);
	str->first_member = fields;
	str->is_union     = is_union;

	if( is_union )
		str->control_field = control_field;
}


/*------------------------------ add_sequence -----------------------------*\
 *
 * Purpose   : Add a sequence definition.
 *
 * Parameters: name		- Sequence name.
 *			   start	- Starting number.
 *			   asc		- 1 = ascending, 0 = descending.
 *			   step		- Step.
 *
 * Returns   : Nothing.
 *
 */
void add_sequence(name, start, asc, step)
char *name;
ulong start, step;
int asc;
{
	Sequence *seq = extend_table((void **)&sequence, &sequences, 
						sizeof *sequence);

	strcpy(seq->name, name);
	seq->start	= start;
	seq->asc	= asc;
	seq->step	= step;
}


/*---------------------------- check_foreign_key ---------------------------*\
 *
 * Purpose   : Check that a foreign key matches its target (primary key).
 *
 * Parameters: name           - Target record name.
 *             foreign_key    - Pointer to foreign key.
 *
 * Returns   : Nothing.
 *
 */
void check_foreign_key(name, foreign_key)
char *name;
Key *foreign_key;
{
	Record		*rec;          		/* Ptr to parent record             	*/
	Key         *primary_key;		/* Ptr to parent's primary key         	*/
	KeyField *primary_fld;			/* Ptr to first field of primary_key   	*/
	KeyField *foreign_fld;			/* Ptr to first field of foreign_key   	*/
	int i;

	/* Check if the record has a primary key */
	for( rec=record, i=0; i<records; rec++, i++ )
    	if( !strcmp(rec->name, name) )
        	break;

	if( i == records )
	{
    	yyerror("unknown target record '%s'", name);
    	return;
	}

	if( !rec->keys )
	{
    	yyerror("The target '%s' has no primary key", name);
    	return;
	}

	primary_key = &key[ rec->first_key ];

	if( KT_GETBASIC(primary_key->type) != KT_PRIMARY )
	{
    	yyerror("The target '%s' has no primary key", name);
    	return;
	}

	if( primary_key->fields != foreign_key->fields )
	{
    	yyerror("The foreign key '%s' does not match its target", name);
    	return;
	}

	primary_fld = &keyfield[ primary_key->first_keyfield ];
	foreign_fld = &keyfield[ foreign_key->first_keyfield ];

	/* Find the reference file of the parent */
	foreign_key->parent  = rec - record;
	rec->dependents++;


	if( rec->ref_file == -1 )
	{
    	yyerror("The parent table '%s' has no reference file", name);
    	return;
	}

	foreign_key->fileid = rec->ref_file;

	/* Compare the format of the primary key with that of the foreign key */
	for( i=0; i<primary_key->fields; i++, foreign_fld++, primary_fld++ )
	{
    	if( FT_GETSIGNEDBASIC(field[primary_fld->field].type)  !=
        	FT_GETSIGNEDBASIC(field[foreign_fld->field].type) )
        	break;

    	/* Set sort order */
    	foreign_fld->asc = primary_fld->asc;
	}

	if( i < primary_key->fields )
	{
    	yyerror("The foreign key '%s' does not match its target", name);
    	return;
	}
}


/*-------------------------------- fix_file --------------------------------*\
 *
 * Purpose   : This function sets the fileid of all the records and the keys.
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
void fix_file()
{
	Contains *con;
	Record *rec;
	int i, j, n;

	for( i=0, con=contains; i<conts; i++, con++ )
	{
		/* First find the record entry of this contains */
		for( j=0, rec=record; j<records; j++, rec++ )
			if( !strcmp(rec->name, con->record) )
				break;

		if( j == records )
		{
			printf("unknown record '%s' in contains statement\n", con->record);
			continue;
		}

		/* Link the record or the key to a file and vice versa */
		switch( con->type )
		{
			case 'r':
				break;

			case 'd':
				rec->fileid = con->fileid;

				if( rec->is_vlr )
					file[i].type = 'v';
				break;

			case 'k':
				for( n=rec->keys, j=rec->first_key; n--; j++ )
					if( !strcmp( key[j].name, con->key) )
						break;

				if( KT_GETBASIC(key[j].type) == KT_FOREIGN )
				{
					int tmp = lex_lineno;

					lex_lineno = con->line;
					yyerror("a foreign key cannot be contained in a file");
					lex_lineno = tmp;
					break;
				}

				if( n < 0 && strcmp(con->key, "<ref>") )
				{
					printf("unknown key '%s' contained in file '%s'\n",
						con->key, file[con->fileid].name);
					continue;
				}

				key[j].fileid = con->fileid;
				break;
		}

		con->entry = j;
		file[i].id = j;
	}

	/* Check that each record is contained is contained in a data file and
	 * each key is contained in a key file
	 */
	for( i=0, rec=record; i<records; i++, rec++ )
	{
		Key *keyptr;

		/* Calculate the size of the preamble */
		if( rec->first_foreign != -1 )
			rec->preamble = (rec->keys - (rec->first_foreign - rec->first_key)) 
				* sizeof(ulong);

		if( rec->fileid == -1 )
			printf("record '%s' is not contained in a file\n", rec->name);

		/* If the key is not a foreign key, it must be contained in a file */
		for( keyptr=&key[rec->first_key], j=0; j<rec->keys; j++, keyptr++ )
		{
			if( keyptr->fileid == -1 && KT_GETBASIC(keyptr->type) != KT_FOREIGN )
			{
				printf("key '%s.%s' is not contained in a file\n",
					rec->name, keyptr->name);
			}
 		}
	}
}



/*------------------------------- fix_fields -------------------------------*\
 *
 * Purpose   : The parser sets the FT_KEY flags on all fields that is a key
 *             or part of a key. This flag should be removed for foreign keys.
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
void fix_fields()
{
	Field *fld = field;
	int n = fields;
   
	while( n-- )
	{
		if( fld->type & FT_KEY )
		{
			if( KT_GETBASIC(key[fld->keyid].type) == KT_FOREIGN )
			fld->type &= ~FT_KEY;
		}
		fld++;
	}
}


/*---------------------------- print_fieldname -----------------------------*\
 *
 * Purpose   : Print a field name and its field id in the header file. If the
 *			   field name is defined in more than one record it is prefixed
 *			   with "<record name>_" to prevent name clashes.
 *
 * Parameters: file		- File descriptor.
 *			   fieldno	- Field number.
 *			   fieldid	- Field id.
 *
 * Returns   : Nothing.
 *
 */
void print_fieldname(file, fieldno, fieldid)
FILE *file;
int fieldno;
Id fieldid;
{
	int i;
	Field *fld = field + fieldno;
	char *name = fld->name;

	if( only_keys && !(field[fieldno].type & FT_KEY) )
    	return;

	fprintf(file, "#define ");

	for( i=0; i<fields; i++ )
	{
		if( field[i].nesting == 0 )
		{
	    	if( !strcmp(field[i].name, name) && i != fieldno )
    		{
        		/* Found a duplicate field name */
	        	fprintf(file, "%s_", record[field[fieldno].recid].name);
    	    	break;
    		}
		}
	}

	fprintf(file, "%s %ldL\n", name, fieldid);

	if( !strcmp(name, "USERCLASS") )
	{
    	puts("AAAAAAARRRRRRRRRGGGGGGGG!!!!!!");
    	printf("%d, %d\n", only_keys, field[fieldno].type & FT_KEY);
	}
}



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


static void init_vars()
{
	int i;

	memset(&header,0,sizeof header);
   	strcpy(header.version, DBD_VERSION);

	/* Initialize the sorttable */
	for( i=0; i<256; i++ )
		header.sorttable[i] = i;

	for( i=0; i<'z'-'a'+1; i++ )
		header.sorttable['A' + i] = 'a' + i;
}


int main(argc, argv)
int argc;
char *argv[];
{
	char *realname;
	char *p;
	int i;
	long n=0, prev;

	puts("Typhoon Data Definition Language Processsor version 2.25");

	if( argc == 1 )
	{
		printf(paramhelp, align);
    	exit(1);
	}

	/* Extract the real name of the file */
	if( (realname = strrchr(argv[argc-1], CONFIG_DIR_SWITCH)) != NULL )
    	realname++;
	else
    	realname = argv[argc-1];

	/* remove .ddl extension if present */
	if( ( p = strstr(realname, ".ddl") ) )
    	*p = 0;

	/* generate file names for .ddl-file, .dbd-file and header file */
	sprintf(ddlname, "%s.ddl", argv[argc-1]);
	sprintf(dbdname, "%s.dbd", realname);
	sprintf(hname, "%s.h", realname);

	/* process command line options */
	for( i=1; i<argc-1; i++ )
	{
    	if( argv[i][0] == '-' || argv[i][0] == '/' )
    	{
        	switch( argv[i][1] )
        	{
        		case 'h':
            		strcpy(hname, argv[i]+2);
            		break;
        		case 'a':
            		align = argv[i][2] - '0';
            		if( align != 1 && align != 2 && align != 4 )
                		err_quit("alignment must be 1, 2 or 4");
            		break;
        		case 'f':
            		only_keys = 1;
            		break;
        		default:
            		err_quit("unknown command line option");
        	}
    	}
    	else
        	err_quit("unknown command line option");
	}

	init_vars();

	/* open files */
	if( (lex_file=fopen(ddlname, "r")) == NULL )
    	err_quit("cannot open file '%s'", ddlname);

	if( !(dbdfile=open(dbdname, O_RDWR|CONFIG_O_BINARY|O_TRUNC|O_CREAT, CONFIG_CREATMASK)) )
    	err_quit("cannot create .dbd file '%s'", dbdname);

	if( (hfile=fopen(hname, "w")) == NULL )
    	err_quit("cannot create header file '%s'", hname);

	/*---------- initialize h-file ----------*/
	fprintf(hfile, "/*---------- headerfile for %s ----------*/\n", ddlname);
	fprintf(hfile, "/* alignment is %d */\n\n", align);

	if( !yyparse() )
	{
    	fix_file();
    	fix_fields();

    	header.files		= files;
    	header.keys			= keys;
    	header.keyfields	= keyfields;
    	header.records		= records;
    	header.fields		= fields;
    	header.structdefs	= structdefs;
    	header.sequences	= sequences;
		header.alignment	= align;

    	/*---------- write dbd-file ----------*/
    	write(dbdfile, &header,		sizeof header);
    	write(dbdfile, file,		sizeof(file[0])		* files);
    	write(dbdfile, key,			sizeof(key[0])		* keys);
    	write(dbdfile, keyfield,	sizeof(keyfield[0])	* keyfields);
    	write(dbdfile, record,		sizeof(record[0])	* records);
    	write(dbdfile, field,		sizeof(field[0])	* fields);
    	write(dbdfile, structdef,	sizeof(structdef[0])* structdefs);
    	write(dbdfile, sequence,	sizeof(sequence[0])	* sequences);

    	/*---------- write h-file ----------*/
		fprintf(hfile, "/*---------- structures ----------*/\n");
    	print_structures();

    	fprintf(hfile, "/*---------- record names ----------*/\n");
    	for( i=0; i<records; i++ )
        	fprintf(hfile, "#define %s %ldL\n", strupr(record[i].name), (i+1) * REC_FACTOR);

    	for( i=0; i<fields; i++ )
        	strupr(field[i].name);

    	fprintf(hfile, "\n/*---------- field names ----------*/\n");
    	for( i=0, prev=-2; i<fields; i++, n++ )
    	{
			if( field[i].nesting == 0 )
			{
        		if( field[i].recid != prev )
	        		n = (field[i].recid+1) * REC_FACTOR + 1;

        		print_fieldname(hfile, i, (Id) n);
        		prev = field[i].recid;
			}
    	}

    	/* Print all keys that are either a multi-field key or foreign key */
    	fprintf(hfile, "\n/*---------- key names ----------*/\n");
    	for( i=0; i<keys; i++ )
    	{
        	/* If the key only has one field, and the name of the key is
        	* the same as the name of the field, then don't print the key name.
        	*/
        	if( key[i].fields == 1 && 
				!istrcmp(key[i].name, field[ keyfield[key[i].first_keyfield].field].name) )
		        	continue;   

        	if( KT_GETBASIC(key[i].type) != KT_FOREIGN )
   	        	fprintf(hfile, "#define %s %d\n", strupr(key[i].name), i);
    	}

    	fprintf(hfile, "\n/*---------- sequence names ----------*/\n");
    	for( i=0; i<sequences; i++ )
    		fprintf(hfile, "#define %s %d\n", strupr(sequence[i].name), i);

    	fprintf(hfile, "\n/*---------- integer constants ----------*/\n");
    	for( i=0; i<defines; i++ )
        	fprintf(hfile, "#define %s %d\n", define[i].name, define[i].value);
	}

	fclose(hfile);
	fclose(lex_file);
	close(dbdfile);

	if( !errors )
    	printf("%d records, %d fields\n", records, fields);
	else
	{
    	unlink(dbdname);
    	unlink(hname);
	}

	free(record);
	free(field);
	free(file);
	free(key);
	free(keyfield);
	free(structdef);

	printf("%d lines, %d errors\n", lex_lineno, errors);

	return errors != 0;
}

/* end-of-file */
