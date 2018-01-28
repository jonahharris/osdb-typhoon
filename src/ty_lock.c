/*----------------------------------------------------------------------------
 * File    : @(#)ty_lock.c   93/11/04         Copyright (c) 1991-93 CT Danmark
 * Library : typhoon
 * Compiler: UNIX C, Turbo C, Microsoft C
 * OS      : UNIX, OS/2, DOS
 * Author  : Thomas B. Pedersen
 *
 * Description:
 *   Locking functions.
 *
 * Functions:
 *   d_reclock
 *   d_recunlock
 *   d_tablock
 *   d_tabunlock
 *
 * History
 * ------------------------------
 * 23-Dec-1993 tbp  Initial version.
 *
 *--------------------------------------------------------------------------*/

static char rcsid[] = "$Id: ty_lock.c,v 1.3 1999/10/03 23:28:29 kaz Exp $";

#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"


FNCLASS d_reclock()
{
}


FNCLASS d_recunlock()
{
}


/* end-of-file */
