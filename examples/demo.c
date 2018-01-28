/*----------------------------------------------------------------------------
 * File    : demo.c
 * OS      : UNIX
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
 *   A small program that demonstrates some of the features of API.
 *
 *--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "environ.h"
#ifdef CONFIG_OS2
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "demo.h"
#include "typhoon.h"

static CONFIG_CONST char rcsid[] = "$Id: demo.c,v 1.4 1999/10/04 04:11:29 kaz Exp $";

typedef enum {
	TYPE_INT,
	TYPE_LONG,
	TYPE_STRING
} field_type;

static	int	menu		PRM ( (char *);				)
static	void	getfield	PRM ( (char *, field_type, void *);	)
static	void	report		PRM ( (char *);				)
static	void	print_company	PRM ( (struct company *);		)
static	void	print_product	PRM ( (struct product *);		)
static	void	find_menu	PRM ( (void);				)
static	void	create_menu	PRM ( (void);				)
	int	main		PRM ( (void); 				)

static int menu(options)
char *options;
{
	char s[10];

	printf("%s: ", options);
	fflush(stdout);
	fgets(s, sizeof s, stdin);
	
	return tolower(s[0]);
}


static void getfield(name, type, var)
char *name;
field_type type;
void *var;
{
	char s[2048];
	char *nl;

	printf("%s: ", name);
	fflush(stdout);
	fgets(s, sizeof s, stdin);

	if ((nl = strchr(s, '\n')) != 0) 
	    *nl = 0;
	
	switch( type )
	{
		case TYPE_INT:
			*(int *)var = atoi(s);
			break;
		case TYPE_LONG:
			*(long *)var = atol(s);
			break;
		case TYPE_STRING:
			strcpy((char *)var, s);
			break;
	}
}


static void report(s)
char *s;
{
	printf("%s - db_status %d\n", s, db_status);
}


static void print_company(company)
struct company *company;
{
	printf("[id=%lu, name='%s']\n", company->id, company->name);
}


static void print_product(product)
struct product *product;
{
	printf("[company_id=%lu, name='%s', description='%s']\n",
		product->company_id,
		product->name,
		product->description);
}


static void find_menu()
{
	struct product product;
	struct company company;
	int stop = 0;
	int key = ID;
	int listall = 0;

	while( !stop )
	{
		switch( menu("First  Last  Prev  Next  Delete  "
					 "Search  list All  Back") )	
		{
			case 'a':
				d_keyfrst(key);
				
				listall = 1;
				break;
			case 'f':
				d_keyfrst(key);
				break;
			case 'l':
				d_keylast(key);
				break;
			case 'p':
				d_keyprev(key);
				break;
			case 'n':
				d_keynext(key);
				break;
			case 's':
				switch( menu("Company  Product") )	
				{
					case 'c':
						getfield("Id", TYPE_LONG,  &company.id);
						d_keyfind(ID, &company.id);
						key = ID;
						break;
					case 'p':
						key = PRODUCT_NAME;
						getfield("Name", TYPE_STRING,  product.name);
						d_keyfind(PRODUCT_NAME, product.name);
						break;
					default:
						continue;
				}
				break;
			case 'd':
				if( db_status == S_OKAY ) {
					if( d_delete() == S_OKAY )
						puts("Deleted.");
					else
						report("Could not delete");
				}
				continue;
			case 'b':
				stop++;
			default:
				continue;
		}

		if( db_status == S_OKAY )
		{
			do
			{
				switch( key )
				{
					case ID:
						d_recread(&company);
						print_company(&company);
						break;
					case PRODUCT_NAME:
						d_recread(&product);
						print_product(&product);
						break;
				}

				if( listall )
					d_keynext(key);
			}
			while( listall && db_status == S_OKAY );
		}			
		else
			puts("Not found");

		listall = 0;
	}		
}



static void create_menu()
{
	struct product product;
	struct company company ;
	int stop = 0;
	
	while( !stop )
	{
		switch( menu("Company  Product  Back") )	
		{
			case 'c':
				getfield("Id  ", TYPE_LONG,  &company.id);
				getfield("Name", TYPE_STRING, company.name);
				if( d_fillnew(COMPANY, &company) != S_OKAY )
					report("Could not create company");
				break;
			case 'p':
				getfield("Company id  ", TYPE_LONG,  &product.company_id);
				getfield("Product name", TYPE_STRING, product.name);
				getfield("Description ", TYPE_STRING, product.description);
				
				/* Set the length determinator of description */
				product.descr_len = strlen(product.description) + 1;
				
				if( d_fillnew(PRODUCT, &product) != S_OKAY )
					report("Could not create product");
				break;
			case 'b':
				stop++;
		}
	}			
}

int main()
{
	int stop = 0;
	
#ifdef CONFIG_UNIX
	mkdir("data", 0777);
#else
	mkdir("data");
#endif

	d_dbfpath("data");
	if( d_open("demo", "s") != S_OKAY )
	{
		fprintf(stderr, "Cannot open database (db_status %d)\n", db_status);
		exit(1);
	}

	while( !stop )
	{
		switch( menu("Create  Find  Delete  Quit") )
		{
			case 'c':	create_menu();	break;
			case 'f':	find_menu();	break;
			case 'q':	stop++;
		}
	}

	d_close();
	return 0;
}

/* end-of-file */

