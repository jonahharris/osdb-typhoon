/*----------------------------------------------------------------------------
 * File    : ddl.y
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
 * $Id: ddl.y,v 1.6 1999/10/03 23:36:49 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

%{

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"

#include "ddlp.h"
#include "ddlpsym.h"
#include "ddlpglob.h"

/*--------------------------- Function prototypes --------------------------*/
int yylex				PRM( (void); )
static void add_keymember               PRM( (sym_struct *, char *, int); )

/*---------------------------- Global variables ----------------------------*/
static unsigned type;               /* Holds FT_.. flags of current field	*/
static sym_member *control_field;	/* Field that controls current union	*/


%}

%union {
	int 	val;
	char	is_union;
	char	s[IDENT_LEN+1];
}

%start database

%token				T_DATABASE T_KEY T_DATA T_FILE T_CONTAINS T_RECORD
%token				T_UNIQUE T_DEFINE T_CONTROLLED T_MAP T_ARROW
%token				T_PRIMARY T_ALTERNATE T_FOREIGN T_ON T_DELETE T_RESTRICT
%token				T_REFERENCES T_UPDATE T_CASCADE T_NULL T_SEQUENCE
%token				T_CHAR T_SHORT T_INT T_LONG T_SIGNED T_UNSIGNED T_FLOAT 
%token				T_DOUBLE T_UCHAR T_USHORT T_ULONG T_STRUCT T_UNION
%token				T_COMPOUND T_ASC T_DESC T_VARIABLE T_BY
%token <s>			T_IDENT T_STRING
%token <val>		T_NUMBER T_CHARCONST
%token				'[' ']' '{' '}' ';' ',' '.' '>'
%token				'+' '-' '*' '/' '(' ')'

%type <val>			expr opt_sortorder opt_unique opt_null pagesize action
%type <val>			map_id
%type <is_union>	struct_or_union
%type <s>			opt_ident key_type

%left '+' '-'
%left '*' '/'

%%

database 	: T_DATABASE T_IDENT '{' decl_list '}'
		 		{ strcpy(dbname, $2); }
		 	;

pagesize	: /* use default page size */
				{ $$ = 512;       }
			| '[' expr ']'
				{
					if( $2 < 512 )
					{
						printf("Page size too small\n");
						$$ = 512;
					}
					else
						$$ = $2;
				}
			;

decl_list	: decl  
			| decl_list decl
			;

decl        : T_DATA T_FILE pagesize T_STRING T_CONTAINS T_IDENT ';'
				{
					add_contains('d', $6, "");
					add_file('d', $4, (unsigned) $3);
				}
			| T_KEY T_FILE pagesize T_STRING T_CONTAINS T_IDENT '.' key_type ';'
				{
					char type = strcmp($8, "<ref>") ? 'k' : 'r';

					add_contains(type, $6, $8);
					add_file(type, $4, (unsigned) $3);
				}
			| record_head '{' member_list opt_key_list '}'
				{
					sym_endstruct();
					record[records-1].size = structnest[curnest+1]->size;
					structdef[record[records-1].structid].size = record[records-1].size;
				}
			| T_DEFINE T_IDENT expr	{ add_define($2, $3);					}
			| sequence
			| T_MAP '{' map_list '}'
			;

record_head	: T_RECORD T_IDENT		{ add_record($2);
									  add_structdef($2, 0, 0);
									  sym_addstruct($2, 0);					}
			;

key_type	: T_IDENT     			{ strcpy($$, $1);     					}
			| T_REFERENCES			{ strcpy($$, "<ref>");					}
			;

member_list	: member
			| member_list member
			;

member		: membertype T_IDENT opt_dimen ';'
				{
					sym_addmember($2, (int) type, NULL);
					cur_str->last_member->id = fields;

					if( type != FT_STRUCT )
						add_field($2, (int) type);

					/* Increase the number of level-0 fields of the record */
					if( curnest == 0 )
						structdef[record[records-1].structid].members++;

					type = 0;
				}
			| struct_specifier ';'
			;

opt_dimen	: 
			| dimension opt_varlen_decl
			;

dimension	: array
			| dimension array
			;

array		: '[' expr ']'			{ dim[dims++] = $2;						}
			;


opt_varlen_decl
			:
			| T_VARIABLE T_BY T_IDENT
				{ 
/*					if( curnest != 1 )
						yyerror("variable size fields cannot be nested");*/

					if( !(size_field = sym_findmember(structnest[0], $3)) )
						yyerror("unknown struct member '%s'", $3);
				}
			;


/*--------------------------------------------------------------------------*/
/*                        Key declaration part                              */
/*--------------------------------------------------------------------------*/

opt_key_list: opt_primary_key_decl
			  opt_alternate_key_decl_list
			  opt_foreign_key_decl_list
			;

/*--------------------------------------------------------------------------*/
/*                            Primary key                                   */
/*--------------------------------------------------------------------------*/

opt_primary_key_decl
			: /* No primary key */
			| T_PRIMARY key_decl ';'
				{
					key[keys-1].type = KT_PRIMARY|KT_UNIQUE;
				}
			;


/*--------------------------------------------------------------------------*/
/*                            Alternate key                                 */
/*--------------------------------------------------------------------------*/

opt_alternate_key_decl_list
			:
			| alternate_key_decl_list
			;

alternate_key_decl_list
			: alternate_key_decl
			| alternate_key_decl_list alternate_key_decl
			;

alternate_key_decl
			: T_ALTERNATE opt_unique key_decl opt_null ';'
				{
					key[keys-1].type = KT_ALTERNATE | $2 | $4;
				}
			;


/*--------------------------------------------------------------------------*/
/*                             Foreign key                                  */
/*--------------------------------------------------------------------------*/

opt_foreign_key_decl_list
			:
			| foreign_key_decl_list
			;

foreign_key_decl_list
			: foreign_key_decl
			| foreign_key_decl_list foreign_key_decl
			;


foreign_key_decl
			: T_FOREIGN key_decl T_REFERENCES T_IDENT 
			  T_ON T_UPDATE action 
			  T_ON T_DELETE action opt_null ';'
				{
					/* Set the record's first_foreign field */
					if( record[records-1].first_foreign == -1 )
						record[records-1].first_foreign = keys-1;

					key[keys-1].type = KT_FOREIGN | $7 | $10 | $11;
					check_foreign_key($4, &key[keys-1]);
				}
			;

action		: T_RESTRICT			{ $$ = KT_RESTRICT;					}
			| T_CASCADE 			{ $$ = KT_CASCADE;
									  yyerror("'cascade' not supported");
				 					}
			;


/*--------------------------------------------------------------------------*/
/*    Key declaration rules used by primary, alternate and foreign keys     */
/*--------------------------------------------------------------------------*/

opt_null	: /* not optional */
				{
					$$ = 0;
				}
			| T_NULL T_BY T_IDENT	
				{
					sym_member *mem;
					int type;

					if( !(mem = sym_findmember(cur_str, $3)) )
						yyerror("'%s' is not a member the record", $3);
					else
					{
						key[keys-1].null_indicator = mem->id;
						type = FT_GETBASIC(field[mem->id].type);

						if( type != FT_CHAR && type != FT_CHARSTR )
							yyerror("field determiner must be char or string");
					}

					$$ = KT_OPTIONAL;
				}
			;


key_decl	: key_decl_head comkey_member_list '}'
				{
					sym_endstruct();

					/* Set the size of the compound key.
					* Foreign keys have special size.
					*/
					if( KT_GETBASIC(key[keys-1].type) == KT_FOREIGN )
						key[keys-1].size = sizeof(REF_ENTRY);
					else
						key[keys-1].size = last_str->size;
				}
			| T_KEY T_IDENT
				{
					sym_member *mem;

					if( !(mem = sym_findmember(cur_str, $2)) )
						yyerror("unknown field '%s'", $2);

					add_key($2);
					add_keyfield(mem->id, 1);

					key[keys-1].size = mem->size;
					field[mem->id].type |= FT_KEY;
					field[mem->id].keyid = keys-1;
				}
			;

key_decl_head
			: T_KEY T_IDENT '{'
				{
					/* Ensure that the key name is not also a field name */
					if( sym_findmember(cur_str, $2) ) 
						yyerror("the key '%s' is already a field name", $2);

					sym_addstruct($2, 0);
					add_key($2);
				}
			;


/*--------------------------------------------------------------------------*/
/*                       Compound key declaration                           */
/*--------------------------------------------------------------------------*/

comkey_member_list
			: comkey_member
			| comkey_member_list ',' comkey_member
			;

comkey_member
			: T_IDENT opt_sortorder 
				{
					add_keymember(structnest[curnest-1], $1, $2);   
				}
			;

opt_unique	: 						{ $$ = 0;		  	/* Not unique */}
			| T_UNIQUE				{ $$ = KT_UNIQUE; 	/* Unique */	}
			;

opt_sortorder
			: /* default */			{ $$ = 1;		/* Ascending */ 	}
			| T_ASC        			{ $$ = 1;		/* Ascending */ 	}
			| T_DESC       			{ $$ = 0;		/* Descending */	}
			;

/*--------------------------------------------------------------------------*/
/*                      Structure member definition                         */
/*--------------------------------------------------------------------------*/

membertype	: int_type
			| int_sign				{ if( type == FT_UNSIGNED )
									      type |= FT_INT;				}
			| int_sign int_type
			| float_type
			| u_type
			| T_LONG float_type		{ type |= FT_LONG;					}
			;

int_type	: T_CHAR 				{ type |= FT_CHAR; 					}
			| T_SHORT				{ type |= FT_SHORT;					}
			| T_INT  				{ type |= FT_INT;  					}
			| T_LONG 				{ type |= FT_LONG; 					}
			;

int_sign	: T_SIGNED
			| T_UNSIGNED			{ type |= FT_UNSIGNED;				}
			;

float_type	: T_FLOAT 				{ type |= FT_FLOAT; 				}
			| T_DOUBLE				{ type |= FT_DOUBLE;				}
			;

u_type		: T_UCHAR 				{ type |= FT_UNSIGNED|FT_CHAR; 		}
	  		| T_USHORT				{ type |= FT_UNSIGNED|FT_SHORT;		}
	  		| T_ULONG 				{ type |= FT_UNSIGNED|FT_LONG; 		}
	  		;

expr		: expr '+' expr			{ $$ = $1 + $3; }
			| expr '-' expr			{ $$ = $1 - $3; }
			| expr '*' expr			{ $$ = $1 * $3; }
			| expr '/' expr			{ $$ = $1 / $3; }
			| '(' expr ')' 			{ $$ = $2;      }
			| T_NUMBER
			;

/*--------------------------------------------------------------------------*/
/*                       struct or union declaration                        */
/*--------------------------------------------------------------------------*/

struct_specifier
			: struct_or_union_opt_ident '{' member_list '}' T_IDENT opt_dimen
				{
					sym_struct *str;
					Field *fld;
					Structdef *strdef;

					sym_endstruct();

					str     = structnest[curnest + 1];
					fld     = &field[str->fieldid];
					strdef  = &structdef[ fld->structid ];

					strcpy(fld->name, $5);
					strdef->size    = str->size;
					strdef->members = str->members;

					sym_addmember($5, FT_STRUCT, structnest[curnest+1]);

					fld->size		= cur_str->last_member->size;
					fld->elemsize	= cur_str->last_member->elemsize;
					fld->offset		= cur_str->last_member->offset;

					if( size_field )
					{
						fld->type |= FT_VARIABLE;
						fld->keyid = size_field->id;
						size_field = NULL;
					}
				}
			| struct_or_union T_IDENT T_IDENT opt_dimen
				{												  
					sym_struct *str = sym_findstruct($2, $1);

					if( str )
					{
						sym_addmember($3, FT_STRUCT, str);
/*						add_field($3, FT_STRUCT);*/
					}
					/* else error message */
				}
			;

struct_or_union_opt_ident
			: struct_or_union opt_ident     
				{
					add_field("<struct>", FT_STRUCT);
					sym_addstruct($2, $1);          
					cur_str->fieldid = fields-1;
					field[fields-1].structid = structdefs;

					add_structdef($2, $1, (int) ($1 ? control_field->id : 0));

					/* Increase the number of level-0 field of the record.
					 * Actually, at this point the grammar we have moved to
					 * level 1.
					 */
					if( curnest == 1 )
						structdef[record[records-1].structid].members++;
				}
			;

struct_or_union
			: T_STRUCT                              
				{
					$$ = 0;
				}
			| T_UNION T_CONTROLLED T_BY T_IDENT
				{
					$$ = 1;                                                               
					if( !(control_field = sym_findmember(cur_str, $4)) )
						yyerror("'%s' is not a member of the union/struct '%s'",
						$4, cur_str->name);
				}
			;

opt_ident	:        				{ *$$ = 0;       					}
			| T_IDENT				{ strcpy($$, $1);					}
			;


/*--------------------------------------------------------------------------*/
/*                                 sequence									*/
/*--------------------------------------------------------------------------*/

sequence	: T_SEQUENCE T_IDENT T_NUMBER opt_sortorder T_BY T_NUMBER ';'
				{ add_sequence($2, (unsigned long) $3,$4, (unsigned long) $6);							}
			;

/*--------------------------------------------------------------------------*/
/*                                 map_list									*/
/*--------------------------------------------------------------------------*/

map_list	: map
			| map_list map
			;

map			: map_id T_ARROW map_id ';'	{ header.sorttable[$1] = $3;	}
			;

map_id		: T_CHARCONST				{ $$ = $1;						}
			| T_NUMBER					{ $$ = $1;						}
			;


%%

#include <stdio.h>



static void add_keymember(str, name, sortorder)
sym_struct *str;
char *name;
int sortorder;
{
	sym_member *mem;

	if( ( mem = sym_findmember(str, name) ) )
	{
		dims = mem->dims;
		memcpy(dim, mem->dim, sizeof(*dim) * dims);
		sym_addmember(name, mem->type, mem->struc);
		add_keyfield(mem->id, sortorder);
	}
	else
		yyerror("unknown struct member '%s'", name);
}




extern int errors;

int yyerror(char *fmt CONFIG_ELLIPSIS)
{
	va_list ap;

	printf("%s %d: ", ddlname, lex_lineno);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	puts("");
	va_end(ap);
	errors++;
	return 0;
}

/* end-of-file */
