/*----------------------------------------------------------------------------
 * File    : catalog.ddl
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
 *	 Typhoon Database Definition Language file for System Catalog.
 *
 * $Id: catalog.ddl,v 1.1.1.1 1999/09/30 04:45:51 kaz Exp $
 *
 *--------------------------------------------------------------------------*/

database catalog {

    data file "dissite.dat"  contains sys_dissite;
    key  file "dissite.ix1"  contains sys_dissite.id;
    data file "distable.dat" contains sys_distab;
    key  file "distable.ix1" contains sys_distab.sys_distab_key;
    data file "dischg.dat"   contains sys_dischange;
	key  file "dischg.ix1"   contains sys_dischange.sys_dischange_key;
    data file "disprkey.dat" contains sys_disprkey;

	data file "disfultb.dat" contains sys_disfulltab;
	key  file "disfultb.ix1" contains sys_disfulltab.fulltab_key;

	define SITES_MAX		296
	define SITEBITMAP_SIZE	((SITES_MAX+7)/8)
    define SITENAME_LEN		20
	define NUA_LEN			16
	define UDATA_LEN		16
	define KEYSIZE_MAX		255

    record sys_dissite {
		ulong	id;						// Site ID
        long	last_call;				// Time of last call.
		ushort	tables;					// Number of distributed tables on
										// this site
        char	name[SITENAME_LEN+1];	// Mnemonic
        char    nua[NUA_LEN+1];			// X.25 address
		char	udata_len;
        char    udata[UDATA_LEN];		// X.25 Call User Data
        char    up_to_date;				// 1=Site is completely up-to-date
		char	in_error;				// 1=Site is malfunctioning
		char	spare[99];
	
		primary key id;
    }

    record sys_distab {
        long	id;						// Table ID
        long	site_id;				// Site ID
        long	last_seqno;				// Last seqno sent
		char	tag;					// Record identification tag
        char	up_to_date;				// 1=Table is up-to-date on site
		char	spare[10];

		primary key sys_distab_key { site_id, id };
    }

	record sys_dischange {
		ulong		seqno;				// Sequence number
        long		table_id;			// Table ID
		long		recno;				// action='u': address of record
										// action='d': address of primary key
										// in sys_disprkey
		ushort		sites;				// Number of 1-bits in site[]
		uchar		prog_id;			// Program ID
		char		action;				// 'u'=update, 'd'=delete
										// Bitmap of unupdated sites, indexes
										// by Site ID. 0=up-to-date
		uchar		site[SITEBITMAP_SIZE];

		primary key sys_dischange_key { table_id, recno };
	}

    record sys_disprkey {
    	ushort		size;
    	uchar		buf[KEYSIZE_MAX];
    }

	/*
	 * When a new site is inserted in the catalog, a record of type 
	 * <sys_disfulltab> is created for each record inserted in <sys_distab>.
	 * 
	 * The <sys_disfulltab> is used to control that the site receives all tables
	 * from scratch, before being updated from <sys_dischg>. When entries
	 * are present in the <sys_disfulltab>, the <updatable> flags in
	 * <sys_dissite> must be 0. 
	*/

	record sys_disfulltab {
		long		site_id;				// Site ID
		ulong		table_id;				// Table ID
		ulong		curr_rec;				// Last record read in seq. scan

		primary key fulltab_key { site_id, table_id };
	}

}

/* end-of-file */
