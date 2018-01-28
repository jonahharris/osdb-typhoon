/*----------------------------------------------------------------------------
 * File    : bt_open.c
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
 *   Contains functions for opening and closing B-tree index files.
 *
 * Functions:
 *   db_keygetheader	- Read B-tree inde xfile header.
 *   db_keyputheader	- Write B-tree index file header.
 *   d_keyopen			- Open a B-tree index file.
 *   d_keyclose			- Close a B-tree index file.
 *   nodesearch			- Perform binary search in the node.
 *   d_search			- Find a key.
 *
 *--------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "environ.h"
#ifdef CONFIG_UNIX
# 	include <unistd.h>
#	ifdef __STDC__
#		include <stdlib.h>
#	endif
#else
#	include <sys/stat.h>
#	include <stdlib.h>
#	include <io.h>
#endif
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"
#include "btree.h"

static CONFIG_CONST char rcsid[] = "$Id: bt_open.c,v 1.8 1999/10/04 03:45:07 kaz Exp $";

int find_firstoccurrence  PRM( (INDEX *, void *, ix_addr *, int *); )

/*----------------------------- btree_getheader ----------------------------*\
 *
 * Purpose	 : Reads the header of a B-tree index file.
 *
 * Parameters: I		- Pointer to index file descriptor.
 *
 * Returns	 : Nothing.
 *
 */

void btree_getheader(I)
INDEX *I;
{
    lseek(I->fh, 0L, SEEK_SET);
    read(I->fh, &I->H, sizeof I->H);
}


/*----------------------------- btree_getheader ----------------------------*\
 *
 * Purpose	 : Writes the header of a B-tree index file.
 *
 * Parameters: I		- Pointer to index file descriptor.
 *
 * Returns	 : Nothing.
 *
 */

void btree_putheader(I)
INDEX *I;
{
    lseek(I->fh, 0L, SEEK_SET);
    write(I->fh, &I->H, sizeof I->H);
}


/*-------------------------------- btree_open -------------------------------*\
 *
 * Purpose	 : Opens a B-tree index file with the name <fname>. If the file
 *			   does not already exist, the file is created. If the version of
 *			   the existing file does not match the version of the B-tree
 *			   library db_status is set to S_VERSION, and NULL is returned.
 *
 * Parameters: fname		- File name.
 *			   keysize		- Key size.
 *			   nodesize		- Node size. Multiples of 512 are recommended.
 *			   cmpfunc		- Comparison function. This function must take two
 *							  parameters, i.e. like strcmp().
 *			   dups			- True if duplicates are allowed.
 *			   shared		- Open the index file in shared mode?.
 *
 * Returns	 : If the file was successfully opened, a pointer to a B-tree
 *			   index file descriptor is returned, otherwise NULL is returned
 *			   and db_status is set to one of following:
 *
 * 			   S_NOMEM		- Out of memory.
 * 			   S_IOFATAL	- File could not be opened.
 * 			   S_VERSION	- B-tree file on disk has wrong version.
 *			   S_UNAVAIL	- The file is already opened in non-shared mode.
 *
 */

INDEX *btree_open(fname, keysize, nodesize, cmpfunc, dups, shared)
char   *fname;
int     keysize, dups, nodesize, shared;
CMPFUNC cmpfunc;
{
    INDEX *I;
    int tuplesize, isnew, fh;
    int aligned_keysize;

	/* See if file exists and then open it */
	isnew = access(fname, 0);
	if( (fh=os_open(fname, CONFIG_O_BINARY|O_CREAT|O_RDWR,CONFIG_CREATMASK)) == -1 )
	{
		db_status = S_IOFATAL;
		return NULL;
	}

	/* Lock the file if it is not opened in shared mode */
	if( !shared )
		if( os_lock(fh, 0L, 1, 't') == -1 )
        {
        	db_status = S_NOTAVAIL;
            return NULL;
        }

	/* Ensure that the size of a tuple is multiple of four */
	aligned_keysize = keysize;
#ifdef CONFIG_RISC
	if( aligned_keysize & (sizeof(long)-1) )
		aligned_keysize += sizeof(long) - (aligned_keysize & (sizeof(long)-1));
#endif

    /* calculate memory requirements */
    tuplesize = sizeof(A_type) + sizeof(R_type) + aligned_keysize;

	/* allocate memory for INDEX structure */
	if( (I = (INDEX *)calloc(sizeof(*I) + nodesize + tuplesize,1)) == NULL )
	{
    	os_close(fh);
		db_status = S_NOMEM;
		return NULL;
	}

	/* allocate memory for current key */
	if( (I->curkey = (char *)malloc((size_t) keysize)) == NULL )
	{
        os_close(fh);
		free(I);
		db_status = S_NOMEM;
		return NULL;
	}

	I->fh = fh;

    if( isnew )
    {
        I->H.version        = KEYVERSION_NUM;
		I->H.first_deleted  = 0;
		I->H.order 			= ((nodesize-sizeof(N_type)-sizeof(A_type)) / tuplesize) & 0xfffe;
        I->H.keysize        = keysize;
        I->H.dups           = dups;
        I->H.nodesize       = nodesize;
        I->H.keys			= 0;
        strcpy(I->H.id, KEYVERSION_ID);
        memset(I->H.spare, 0, sizeof I->H.spare);
        btree_putheader(I);
    }
	else
	{
		btree_getheader(I);

		if( I->H.version != KEYVERSION_NUM )
		{
			db_status = S_VERSION;
			os_close(fh);
			free(I->curkey);
			free(I);
			return NULL;
		}
	}

    I->cmpfunc  	    = cmpfunc;
    I->tsize    	    = tuplesize;
    I->hold				= 0;
    I->shared			= shared;
    I->aligned_keysize	= aligned_keysize;
    strcpy(I->fname, fname);

	db_status = S_OKAY;

    return I;
}


/*-------------------------------- btree_close ------------------------------*\
 *
 * Purpose	 : Closes a B-tree index file previously opened with d_keyopen().
 *			   All nodes that might be in the cache are written to disk.
 *
 * Parameters: I	- Index file descriptor.
 *
 * Returns	 : Nothing.
 *
 */
void btree_close(I)
INDEX *I;
{
	if( I->fh != -1 )
	   	os_close(I->fh);

	free(I->curkey);
    free(I);
}


int btree_dynclose(I)
INDEX *I;
{
	if( I->fh != -1 )
	{
		close(I->fh);
		I->fh = -1;
	}

	RETURN S_OKAY;
}


int btree_dynopen(I)
INDEX *I;
{
	if( I->fh == -1 )
		if( (I->fh=os_open(I->fname, CONFIG_O_BINARY|O_CREAT|O_RDWR, CONFIG_CREATMASK)) == -1 )
			RETURN S_IOFATAL;

	RETURN S_OKAY;
}


/*------------------------------- nodesearch -------------------------------*\
 *
 * Purpose	 : Performs a binary search for the key value pointed to by <key>
 *			   in the node in I. When the function returned <i> contains the
 *			   entry in the node where the searched stopped. If the key was
 *			   found REF(i) contains the reference to be returned by the
 *			   calling, otherwise CHILD(i) contains the node address of the
 *			   child to be processed next.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   key		- Key value being searched for.
 *			   i		- Contains node index when function returns.
 *
 * Returns	 : 0		- The key was found.
 *			   not 0	- The key was not found.
 *
 */
int nodesearch(I, key, i)
INDEX *I;
void  *key;
int   *i;
{
    int cmp, mid, upr, lwr;

    upr = NSIZE(I->node) - 1;
    lwr = 0;

    /* Perform binary search in node */
    while( lwr <= upr )
    {
        mid = (lwr + upr) >> 1;
        cmp = (*I->cmpfunc)(key, KEY(I->node, mid));

        if( cmp > 0 )
            lwr = mid + 1;
        else if( cmp < 0 )
            upr = mid - 1;
        else
		{
			if( I->H.dups )
			{
				/* Find the leftmost occurrence */
				while( mid > 0 )
				{
					mid--;
					if( (cmp = (*I->cmpfunc)(key, KEY(I->node, mid))) )
						break;
				}
				if( cmp )
 					mid++;

				*i = mid;
				return 0;
			}
            break;
		}
    }

	/* If the comparison yielded greater than, move a step to the right */
	if( cmp > 0 )
    	mid++;

    *i = mid;
    return cmp;
}


/*-------------------------------- d_search --------------------------------*\
 *
 * Purpose	 : Searches a B-tree for the key value pointed to by <key>. If
 *			   the key is found the reference is in REF(i). The path from the
 *			   root to the last access node is stored in I->path[], which
 *			   enables key traversal from the current point in the tree.
 *
 *			   The search starts at the root, which is always at address 1. For
 *			   each node nodesearch is called to perform a binary search in the
 *			   node. If the key is not found, the search ends in a leaf node.
 *
 *			   If duplicates are allowed, the first occurrence of the key
 *			   value must be found.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   key		- Key value being searched for.
 *			   addr		- Contains node address when function returns.
 *			   i		- Contains node index when function returns.
 *
 * Returns	 : 0		- The key was found.
 *			   1		- The key was not found.
 *
 */
int d_search(I, key, addr, i)
INDEX     *I;
void      *key;
ix_addr   *addr;
int       *i;
{
    int cmp;

	/* Start the search at the root */
    *addr = 1;
    *i = 0;
    I->level = 0;


    for( ;; )
    {
    	/* Save the addresses and indexes of the traversed nodes */
        I->path[++I->level].a = *addr;

        if( noderead(I, I->node, *addr) == (ix_addr)-1 )
        {
        	/* The node could not be read - zero the number of keys */
			memset(I->node, 0, I->H.nodesize);
            return 0;
        }

        cmp = nodesearch(I,key,i);

        I->path[I->level].i = *i;

        if( !cmp )
		{
			if( I->H.dups )
				return find_firstoccurrence(I, key, addr, i);
            return 1;
		}

		if( CHILD(I->node, *i) )
			*addr = CHILD(I->node, *i);
        else
            return 0;
    }
}


/*-------------------------- find_firstoccurrence --------------------------*\
 *
 * Purpose	 : This function is called in order to find the first occurrence
 *			   of a duplicate key in a non-unique index.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   key		- Key value being searched for.
 *			   addr		- Contains node address when function returns.
 *			   i		- Contains node index when function returns.
 *
 * Returns	 : 0		- The key was found.
 *			   1		- The key was not found.
 *
 */
int find_firstoccurrence(I, key, addr, i)
INDEX     *I;
void      *key;
ix_addr   *addr;
int       *i;
{
	int first_occurlevel = I->level;
	int cmp = 0;

	while( CHILD(I->node, 0) )
	{
		I->level++;
		I->path[I->level].a = CHILD(I->node, *i);
		I->path[I->level].i = *i;

		noderead(I, I->node, I->path[I->level].a);

        cmp = nodesearch(I,key,i);

		I->path[I->level].i = *i;

		/* If the key was found in the current node we search the left
		 * subtree of the key. Otherwise, we search the right subtree of
		 * the rightmost key.
		 */

		if( !cmp )
			first_occurlevel = I->level;
		else
			*i = NSIZE(I->node);
	}

	if( cmp )
	{
		I->level = first_occurlevel;
		*i       = I->path[I->level].i;
		*addr	 = I->path[I->level].a;

		noderead(I, I->node, I->path[I->level].a);
	}
	else
	{
		*i       = I->path[I->level].i;
		*addr	 = I->path[I->level].a;
	}

	return 1;
}

/* end-of-file */

