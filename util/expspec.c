/*----------------------------------------------------------------------------
 * File    : expspec.c
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
 *   Functions for reading and generating the specification.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <stdio.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"

#include "export.h"

static char CONFIG_CONST rcsid[] = "$Id: expspec.c,v 1.5 1999/10/04 00:03:23 kaz Exp $";


/*-------------------------- Function prototypes ---------------------------*/
static void Indent          PRM( (int); )
static int  PrintFields     PRM( (Structdef *, int); )


/*---------------------------- Global variables ----------------------------*/
static FILE *outfile;
extern FILE *lex_file;



/*----------------------------- ReadExportSpec -----------------------------*\
 *
 * Purpose   : This function reads an export specification.
 *
 * Parameters: exportspec_fname     - Output file name.
 *
 * Returns   : Nothing.
 *
 */

extern int yyparse PRM ( (void); )

void ReadExportSpec(dbname)
char *dbname;
{
	char exportspec_fname[256];

	sprintf(exportspec_fname, "%s.exp", dbname);

    if( !(lex_file = fopen(exportspec_fname, "r")) )
        err_quit("Cannot open '%s'", exportspec_fname);

	yyparse();

    fclose(lex_file);
}


static void Indent(level)
int level;
{
	fprintf(outfile, "%*s", (level+2) * 4, "");
}


static int PrintFields(str, nest)
Structdef *str;
int nest;
{
    Field *fld = dbd.field + str->first_member;
	int fields = str->members;
	int old_fields = fields;
    int rc;

    while( fields-- )
    {
        if( FT_GETBASIC(fld->type) == FT_STRUCT )
        {
            Structdef *struc = dbd.structdef + fld->structid;

            Indent(nest);
            fprintf(outfile, "%s %s {\n", struc->is_union ? "union" : "struct", fld->name);
            rc = PrintFields(struc, nest+1);
            Indent(nest);
            fprintf(outfile, "};\n");
            old_fields += rc;
            fld += rc;
        }
        else if( fld->nesting == nest )
        {
            Indent(nest);
            fprintf(outfile, "%s;\n", fld->name);
        }

        fld++;
    }

    return old_fields;
}




/*--------------------------- GenerateExportSpec ---------------------------*\
 *
 * Purpose   : This function automatically generates a full export 
 *          specification.
 *
 * Parameters: exprotspec_fname     - Output file name.
 *
 * Returns   : Nothing.
 *
 */
void GenerateExportSpec(dbname)
char *dbname;
{
	char exportspec_fname[256];			
    int i;
    Record *rec = dbd.record;

	printf("Generating export specification...");
	fflush(stdout);

	sprintf(exportspec_fname, "%s.exp", dbname);

    if( !(outfile = fopen(exportspec_fname, "w")) )
        err_quit("Cannot write to '%s'", exportspec_fname);

    fprintf(outfile, "export %s {\n\n", dbname); 

    for( i=0; i<dbd.header.records; i++, rec++ )
    {
        fprintf(outfile, "    record %s in \"%.8s.kom\" {\n", rec->name, rec->name);

        PrintFields(&dbd.structdef[rec->structid], 0);

        fprintf(outfile, "    }\n\n");
    }

    fprintf(outfile, "}\n");
    fclose(outfile);

	puts("done");
}

/* end-of-file */
