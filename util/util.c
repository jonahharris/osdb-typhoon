/*----------------------------------------------------------------------------
 * File    : util.c
 * Program : tybackup, tyrestore
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
 *   Contains miscellaneous function used by tybackup and tyrestore.
 *
 * Functions:
 *   clock_on			- Start timer.
 *   clock_off			- Stop timer.
 *   clock_secs			- Report number of seconds passed.
 *   printlong			- Print a long with 1000 separators.
 *
 *--------------------------------------------------------------------------*/

static char rcsid[] = "$Id: util.c,v 1.2 1999/10/03 23:36:49 kaz Exp $";

#include <sys/types.h>
#include <time.h>
#include "environ.h"

#ifndef	NULL
#define	NULL	0
#endif


/*---------------------------- Global variables ----------------------------*/
static ulong		secs_used = 0;
static ulong		clock_start = 0;



void clock_on()
{
	clock_start = time(NULL);
}


void clock_off()
{
	if( clock_start )
		secs_used += time(NULL) - clock_start;
}


ulong clock_secs()
{
	return secs_used;
}


char *printlong(number)
ulong number;
{
	static char s[20];
	char *p = s + sizeof(s)-1;
	int n = 3;
	
	s[19] = 0;
	if( !number )
	{
		p = s + sizeof(s)-2;
		*p = '0';
	}
	
	while( number )
	{
		*--p = '0' + (number % 10);
		number /= 10;
		
		if( !--n && number )
		{
			*--p = ',';
			n = 3;
		}
	}

	return p;
}

/* end-of-file */
