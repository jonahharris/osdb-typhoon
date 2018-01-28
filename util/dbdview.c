/*----------------------------------------------------------------------------
 * File    : dbdview.c
 * Program : dbdview
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
 *   Small utility that dumps a dbd-file.
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#ifndef CONFIG_UNIX
#  include <io.h>
#else
#  include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include "ty_dbd.h"
#include "ddlp.h"
#define FREE(p)		if( p ) free(p)

static char CONFIG_CONST rcsid[] = "$Id: dbdview.c,v 1.6 1999/10/04 04:11:31 kaz Exp $";

/*-------------------------- Function prototypes ---------------------------*/
static void			viewdbd     PRM( (int); )
int					main		PRM( (int, char **); )

/*---------------------------- Global variables ----------------------------*/
static Header		 header;
static Field		*fieldtab;
static Record		*recordtab;
static File			*filetab;
static Key 			*keytab;
static KeyField		*keyfieldtab;
static Structdef	*structtab;
static Sequence		*seqtab;


/*--------------------------------------------------------------------------*\
 *
 * Function  : viewdbd
 *
 * Purpose   : View the tables in the dbd-file.
 *
 * Parameters: dbdfile	- File handle.
 *
 * Returns   : Nothing.
 *
 */
static void viewdbd(dbdfile)
int dbdfile;
{
	static char *key_type[] = {
		"primary",
		"altern.",
		"foreign",
		"referen"
	};
	char type[50];
	int i;
	Key *key;
	Field *field;
	Record *record;
	Structdef *struc;
	Sequence *seq;

	read(dbdfile, &header, sizeof header);

	if( strcmp(header.version, DBD_VERSION) )
	{
		puts("Illegal version ID");
		return;
	}


	printf("%d files\n", header.files);
	printf("%d keys\n", header.keys);
	printf("%d keyfields\n", header.keyfields);
	printf("%d records\n", header.records);
	printf("%d fields\n", header.fields);
	printf("%d structdefs\n", header.structdefs);
	printf("%d sequences\n", header.sequences);

	/*---------- allocate memory for tables ----------*/
	filetab 	= (void *)malloc(sizeof(File) * header.files);
	keytab		= (void *)malloc(sizeof(Key) * header.keys);
	keyfieldtab = (void *)malloc(sizeof(KeyField) * header.keyfields);
	fieldtab	= (void *)malloc(sizeof(Field) * header.fields);
	recordtab	= (void *)malloc(sizeof(Record) * header.records);
	structtab	= (void *)malloc(sizeof(Structdef) * header.structdefs);
	seqtab		= (void *)malloc(sizeof(Structdef) * header.structdefs);

	if( (header.files		&& !filetab)		||
		(header.keys		&& !keytab)			||
		(header.keyfields	&& !keyfieldtab)	||
		(header.fields		&& !fieldtab)		||
		(header.records		&& !recordtab)		||
		(header.structdefs	&& !structtab)		||
		(header.sequences	&& !seqtab) )
	{
		puts("out of memory");
		FREE(filetab);
		FREE(fieldtab);
		FREE(recordtab);
		FREE(keytab);
		FREE(keyfieldtab);
		FREE(structtab);
		FREE(seqtab);
		return;
	}	

	/*---------- read tables ----------*/
	read(dbdfile, filetab,		sizeof(File) 	  * header.files);
	read(dbdfile, keytab,		sizeof(Key)		  * header.keys);
	read(dbdfile, keyfieldtab,	sizeof(KeyField)  * header.keyfields);
	read(dbdfile, recordtab,	sizeof(Record)	  * header.records);
	read(dbdfile, fieldtab,		sizeof(Field)	  * header.fields);
	read(dbdfile, structtab,	sizeof(Structdef) * header.structdefs);
	read(dbdfile, seqtab,		sizeof(Sequence)  * header.sequences);

	puts("----------------------------------- FILES -------------------------------------");
	printf(" ID NAME                 PGSIZE REC/KEY ID TYPE\n");
	for( i=0; i<header.files; i++ )
		printf("%3d %-20s %6u %10ld %4c\n",
			i,
			filetab[i].name,
			filetab[i].pagesize,
			filetab[i].id,
			filetab[i].type);
	printf("\n");

	puts("----------------------------------- KEYS --------------------------------------");
	printf(" ID NAME                 TYPE               FILE SIZE 1ST_KEYFLD FIELDS PARENT\n");
	for( i=0, key=keytab; i<header.keys; i++, key++ )
	{
		strcpy(type, key_type[KT_GETBASIC(key->type)-1]);

		if( key->type & KT_OPTIONAL )
			strcat(type, " opt");

		if( KT_GETBASIC(key->type) == KT_ALTERNATE && (key->type & KT_UNIQUE) )
			strcat(type, " unique");

		printf("%3d %-20s %-18s %4ld %4u %10ld %6d",
			i,
			key->name,
			type,
			key->fileid,
			key->size,
			key->first_keyfield,
			key->fields);
		if( KT_GETBASIC(key->type) == KT_FOREIGN )
			printf(" %ld", key->parent);
		puts("");
	}

	puts("");
	puts("--------------------------------- KEY FIELDS ----------------------------------");
	printf(" ID FIELD      OFFSET ASCENDING\n");
	for( i=0; i<header.keyfields; i++ )
	{
		printf("%3d %-5ld      %6u %s\n",
			i,
			keyfieldtab[i].field,
			keyfieldtab[i].offset,
			keyfieldtab[i].asc ? "Yes" : "No");
	}

    puts("\n----------------------------------- RECORDS -----------------------------------");
	puts(" ID NAME                  SIZE VLR FILE 1ST_FLD FIELDS KEYS 1ST_KEY STR 1ST_FOR");
    for( i=0, record=recordtab; i<header.records; i++, record++ )
    {
		printf("%3d %-20s %5u %-3s %4ld %7ld %6u %4u %7ld %3ld",
			i,
            record->name,
            record->size,
            record->is_vlr ? "Yes" : "No",
            record->fileid,
            record->first_field,
            record->fields,
			record->keys,
            record->first_key,
			record->structid);
		if( record->first_foreign != -1 )
			printf(" %10ld", record->first_foreign);
		puts("");
    }

    puts("\n----------------------------------- STRUCTS -----------------------------------");
	puts(" ID NAME                 SIZE 1ST_MEMBER MEMBERS UNION CONTROL");
	for( i=0, struc=structtab; i<header.structdefs; i++, struc++ )
	{
		printf("%3d %-20s %4u %10lu %7lu %5s",
			i,
			struc->name,
			struc->size,
			struc->first_member,
			struc->members,
			struc->is_union ? "yes" : "no");

		if( struc->is_union )
			printf(" %7lu", struc->control_field);

		puts("");
    }


    puts("\n----------------------------------- FIELDS ------------------------------------");
	puts(" ID NAME                 REC NEST OFFSET ELEMSZ  SIZE TYPE STRID KEYID SIZE_FLD");
	for( i=0, field=fieldtab; i<header.fields; i++, field++ )
	{
		printf("%3d %-20s %3ld %4u %6d %6d %5u %4X",
			i,
			field->name,
			field->recid,
			field->nesting,
			field->offset,
            field->elemsize,
			field->size,
			field->type);

		if( field->type == FT_STRUCT )
			printf(" %5ld", field->structid);
		else
			printf("      ");

        if( field->type & FT_KEY )
			printf(" %5ld", field->keyid);
        else
        if( fieldtab[i].type & FT_VARIABLE )
            printf("      %8lu", field->keyid);
        puts("");
    }

    puts("\n---------------------------------- SEQUENCES ----------------------------------");
	puts(" ID NAME                    START     STEP ORDER");
	for( i=0, seq=seqtab; i<header.sequences; i++, seq++ )
	{
		printf("%3d %-20s %8lu %8lu %s\n",
			i,
			seq->name,
			seq->start,
			seq->step,
			seq->asc ? "asc" : "desc");
    }

	free(filetab);
	free(fieldtab);
	free(recordtab);
	free(keytab);
	free(keyfieldtab);
	free(structtab);
	free(seqtab);
}


int main(argc, argv)
int argc;
char *argv[];
{
	int dbdfile;
	char fname[80];

	if( argc < 2 )
	{
		puts("Syntax: dbdview dbd-file");
		exit(1);
	}

	/* check extension */
	strcpy(fname, argv[argc-1]);
	if( strstr(fname, ".dbd") == NULL )
		strcat(fname, ".dbd");

	if( (dbdfile=open(fname, O_RDONLY|CONFIG_O_BINARY)) == -1 )
	{
		printf("cannot open %s\n", fname);
		exit(1);
	}

	viewdbd(dbdfile);

	close(dbdfile);
	return 0;
}

/* end-of-file */

