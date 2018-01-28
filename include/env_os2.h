/*----------------------------------------------------------------------------
 * File    : environ.h
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
 *   Multi Operating System compatibility include file.
 *
 * $Id: env_os2.h,v 1.3 1999/10/03 23:28:28 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

#ifndef TYPHOON_ENVIRON_INCLUDED
#define TYPHOON_ENVIRON_INCLUDED

/*--------------------------------------------------------------------------*/
/*                               OS/2 v2.0                                  */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_CONFIG_OS2
#ifdef __BORLANDC__
#   define __STDC__ 1
#endif
#define CONFIG_DIR_SWITCH		'\\'
#define CONFIG_CREATMASK		(S_IREAD|S_IWRITE)
#define CONFIG_PROTOTYPES		1
#define CONFIG_LITTLE_ENDIAN	1

typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;

#endif


/*--------------------------------------------------------------------------*/
/*                                    DOS                                   */
/*--------------------------------------------------------------------------*/

#ifdef CONFIG_DOS

#define CONFIG_DIR_SWITCH		'\\'
#define CONFIG_CREATMASK		(S_IREAD|S_IWRITE)
#define CONFIG_PROTOTYPES		1
#define CONFIG_LITTLE_ENDIAN	1

typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;

#endif


#ifdef CONFIG_PROTOTYPES
#	define PRM(x)		x
#	define CONFIG_ELLIPSIS		,...
#else
#	define PRM(x)		();
#	define CONFIG_ELLIPSIS		/**/
#endif

#ifndef offsetof
#	define offsetof(s,m)		((int)&(((s *)0))->m)
#endif

#endif

/* end-of-file */
