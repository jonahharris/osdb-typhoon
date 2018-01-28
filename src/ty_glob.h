/*----------------------------------------------------------------------------
 * File    : ty_glob.h
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
 *   Contains the global variables for the library.
 *
 * $Id: ty_glob.h,v 1.6 1999/10/04 03:45:07 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

/*---------------------------------- OS/2 ----------------------------------*\
 *
 * If Typhoon is compiled as an OS/2 DLL, the functions must be a special
 * class in order to be known to other programs.
 *
 */

#ifndef TYPHOON_TY_GLOB_H
#define TYPHOON_TY_GLOB_H

#ifndef TYPHOON_TY_TYPE_H
#include "ty_type.h"
#endif

#ifdef CONFIG_OS2
#	define INCL_NOPMAPI
#	include <os2.h>
#	define FNCLASS APIRET EXPENTRY
#else
#	define FNCLASS
#	define VAR
#endif

#ifdef DEFINE_GLOBALS

TyphoonGlobals typhoon = {
	{ { { 0 } } },							/* dbtab 						*/
	NULL,									/* db							*/
	0,										/* do_rebuild					*/
	0,										/* dbs_open						*/
	0,										/* cur_open						*/
	20,										/* max_open						*/
	{ 0 },									/* curr_keybuf					*/
	0,										/* curr_key						*/
	-1,										/* curr_db						*/
	NULL,									/* ty_errfn						*/
	{ '.', CONFIG_DIR_SWITCH, 0 },			/* dbfpath						*/
	{ '.', CONFIG_DIR_SWITCH, 0 }			/* dbdpath						*/
};

int			 db_status = 0;					/* Status code					*/
long		 db_subcode = 0;				/* Sub error code				*/

#else


extern TyphoonGlobals typhoon;
extern int db_status;
extern long db_subcode;


#endif

extern		 CMPFUNC keycmp[];				/* Comparison function table	*/


#define DB				typhoon.db
#define CURR_DB			typhoon.curr_db
#define CURR_KEY		typhoon.curr_key
#define CURR_KEYBUF		typhoon.curr_keybuf
#define CURR_REC		typhoon.db->curr_rec
#define CURR_RECID		typhoon.db->curr_recid
#define CURR_BUFREC		typhoon.db->curr_bufrec
#define CURR_BUFRECID	typhoon.db->curr_bufrecid

#endif

/* end-of-file */

