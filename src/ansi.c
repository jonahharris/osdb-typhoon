/*----------------------------------------------------------------------------
 * File    : ansi.c
 * Library : typhoon
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
 *   Contiains functions specific to UNIX.
 *
 * Functions:
 *   strstr			- ANSI C strstr
 *   memmove		- ANSI C memmove
 *
 *--------------------------------------------------------------------------*/

#include "environ.h"
#include "ansi.h"

static CONFIG_CONST char rcsid[] = "$Id: ansi.c,v 1.3 1999/10/03 23:28:28 kaz Exp $";

#ifdef STRSTR_MISSING
#define NOT_EMPTY 

char *strstr(s1, s2)
char *s1, *s2;
{
	while( *s1 )
	{
		if( *s2 == *s1 )
		{
			char *ss1 = s1;
			char *ss2 = s2;
		
			while( *ss2 && *ss1++ == *ss2++ )
				;
			
			if( !*ss2 )
				return s1;
		}

		s1++;
	}

	return (char *)0;
}

#endif



#ifdef MEMMOVE_MISSING
#define NOT_EMPTY 

void memmove(dest, src, len)
char *dest, *src;
int len;
{
	if( dest < src )
	{
		while( len-- )
			*dest++ = *src++;
	}
	else
	{
		dest += len;
		src  += len;
		
		while( len-- )
			*--dest = *--src;
	}
}

#endif

#ifndef NOT_EMPTY
struct dummy;	/* ANSI C forbids empty translation units */
#endif

/* end-of-file */
