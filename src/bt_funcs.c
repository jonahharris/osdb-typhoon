/*----------------------------------------------------------------------------
 * File    : bt_funcs
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
 *
 * Functions:
 *	 btree_add			- Insert a new key in a B-tree.
 *	 btree_find			- Find a key in a B-tree.
 *	 btree_exist			- See if a key exists in a B-tree.
 *	 btree_read			- Read the last key found.
 *	 btree_delall		- Delete all keys in a B-tree.
 *   get_rightmostchild	- Find the rightmost child in a subtree.
 *   get_leftmostchild	- Find the leftmost child in a subtree.
 *	 btree_frst			- Find the key with the lowest key value in a B-tree.
 *	 btree_last			- Find the key with the highest key value in a B-tree.
 *	 btree_prev			- Find the previous key in a B-tree.
 *	 btree_next			- Find the next key in a B-btree.
 *
 *--------------------------------------------------------------------------*/

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "environ.h"
#ifdef CONFIG_UNIX
#	include <unistd.h>
#else
#	include <io.h>
#endif
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"
#include "btree.h"

static CONFIG_CONST char rcsid[] = "$Id: bt_funcs.c,v 1.7 1999/10/04 03:45:07 kaz Exp $";

/*--------------------------- Function prototypes --------------------------*/
static void	synchronize			PRM( (INDEX *); )


/*------------------------------- btree_add --------------------------------*\
 *
 * Purpose	 : Inserts the key <key> in a B-tree index file.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   key		- Key value to insert.
 *			   ref		- Reference to insert together with key.
 *
 * Returns	 : S_OKAY		- Key value successfully inserted in B-tree.
 *			   S_DUPLICATE	- The key value is already in the B-tree and
 *							  duplicates are not allowed.
 *
 */

int btree_add(I, key, ref)
INDEX *I;
void  *key;
ulong ref;
{
    ulong Ref;
    ix_addr Addr, moved, p;
    int i, mid;

    I->curr = 0;
    I->hold = 0;

	btree_getheader(I);

	if( d_search(I, key, &p, &i) )
	{
		if( I->H.dups )
        {
			/* When a duplicate key is already in the tree, the current node
			 * at this point may not be a leaf node. However, insertions
			 * must always be performed in leaf nodes, so we must find the
			 * rightmost entry in the left subtree 
			 */

			if( CHILD(I->node, i) )
			{
				get_rightmostchild(I, CHILD(I->node, i));
				i = I->path[I->level].i;
				p = I->path[I->level].a;
			}
        }
        else
    	    RETURN S_DUPLICATE;
	}

    I->H.keys++;
    Addr = 0;
    Ref = ref;
    memcpy(I->curkey, key, I->H.keysize);

    do
    {
        tupleins(I->node, i, 1);
        memcpy(KEY(I->node,i), I->curkey, I->H.keysize);
		CHILD(I->node,i+1) = Addr;
        REF(I->node,i) = Ref;

        if( NSIZE(I->node) < (ulong)I->H.order )
        {
            NSIZE(I->node)++;
            nodewrite(I, I->node, p);
			btree_putheader(I);
            RETURN S_OKAY;
        }
 
        /* split node */
        mid = I->H.order / 2;
 
        /* write left part of node at its old position */
        NSIZE(I->node) = mid;
        nodewrite(I, I->node, p);
 
        /* Save mid K, A and R */
        memcpy(I->curkey, KEY(I->node,mid), I->H.keysize);
		Addr = CHILD(I->node, mid);
        Ref  = REF(I->node, mid);
 
        /* write right part of node at new position */
        NSIZE(I->node) = I->H.order - mid;
		memmove(&CHILD(I->node,0), &CHILD(I->node,mid+1), NSIZE(I->node) * I->tsize + sizeof(A_type));
        Addr = nodewrite(I, I->node, NEWPOS);
 
        if( (p = I->path[--I->level].a) != 0 )
        {
            noderead(I, I->node, p);                    /* p = I->path[p] */
/*            nodesearch(I,I->curkey,&i);*/
			i = I->path[I->level].i;
        }
    }
    while( p );
 
    /* Create a new root */
    noderead(I, I->node, 1);
    moved = nodewrite(I, I->node, NEWPOS);

    memcpy(KEY(I->node,0), I->curkey, I->H.keysize);
	CHILD(I->node,0) = moved;
	CHILD(I->node,1) = Addr;
    REF(I->node,0) = Ref;
    NSIZE(I->node) = 1;
    nodewrite(I, I->node, 1);
	I->H.timestamp++;
	btree_putheader(I);

    RETURN S_OKAY;
}


/*------------------------------- btree_find -------------------------------*\
 *
 * Purpose	 : Searches for the key value <key> in a B-tree index file.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   key			- Key value to find.
 *			   ref			- Contains reference when function returns.
 *
 * Returns	 : S_OKAY		- The key value was found. <ref> contains
 *							  reference.
 *			   S_NOTFOUND	- The key value was not found.
 *
 */
int btree_find(I, key, ref)
INDEX *I;
void *key;
ulong *ref;
{
	ix_addr dummy;
	int i;

	btree_getheader(I);		/* Inserted temporarily */

	if( !d_search(I, key, &dummy, &i) )
	{
		/* Do only set hold if there are actually keys in the index */
		I->hold = I->H.keys > 0 ? 1 : 0;
		I->curr = 0;
        RETURN S_NOTFOUND;
	}

    *ref = REF(I->node, i);
    memcpy(I->curkey, KEY(I->node, I->path[I->level].i), I->H.keysize);
	I->hold = 0;
    I->curr = 1;
    RETURN S_OKAY;
}


/*------------------------------- btree_keyread -------------------------------*\
 *
 * Purpose	 : Copies the contents of the current key value to <buf>.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   buf		- Buffer to copy current key value to.
 *
 * Returns	 : S_NOCR	- There is no current key.
 *			   S_OKAY	- Key value copied to <buf>.
 *
 */
int btree_keyread(I, buf)
INDEX *I;
void *buf;
{
    if( !I->curr )
        RETURN S_NOCR;

	memcpy(buf, I->curkey, I->H.keysize);
    RETURN S_OKAY;
}


/*------------------------------ btree_delall ------------------------------*\
 *
 * Purpose	 : Deletes all key values in a B-tree index file.
 *
 * Parameters: I		- B-tree index file descriptor.
 *
 * Returns	 : S_OKAY	- All keys deleted.
 *
 */
int btree_delall(I)
INDEX *I;
{
	btree_getheader(I);
	I->H.first_deleted = 0;
    I->H.keys = 0;
#ifdef CONFIG_UNIX
	os_close(open(I->fname, O_TRUNC));
#else
	chsize(I->fh, I->H.nodesize);
#endif
	I->curr = 0;
    I->hold = 0;
	btree_putheader(I);

	RETURN S_OKAY;
}


/*
 * The following macros are used to enhance the readability of the rest of the
 * functions in this file. B-tree traversal can be rather tricky, with lots
 * of pitfalls in it, but these macros should help a great deal. Here is a
 * short explanation of the macros.
 *
 *	Keys		The number of keys in the current node.
 *	Child(i)	The address of the i'th child in the current node.
 *	Key(i)		Pointer to the i'th key in the current node.
 *	Ref(i)		The reference of the i'th key in the current node.
 *	Pos			The current position within the current node.
 *	Addr		The current node address at the current level in the tree.
 *	Level		The current level in the tree.
 *
 */

#define Keys		NSIZE(I->node)
#define Child(i)	CHILD(I->node, i)
#define Key(i)		KEY(I->node, i)
#define Ref(i)		REF(I->node, i)
#define Pos			(I->path[I->level].i)
#define Addr		(I->path[I->level].a)
#define Level		(I->level)


/*--------------------------- get_rightmostchild ---------------------------*\
 *
 * Purpose	 : Reads the rightmost key in a subtree with root address <addr>.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   addr		- The root address of the subtree.
 *
 * Returns	 : Nothing.
 *
 */

void get_rightmostchild(I, addr)
INDEX *I;
ulong addr;
{
	/* No tree has root at address 0 */
	if( !addr )
		return;

	do
	{
		noderead(I, I->node, addr);

		Level++;
		Addr = addr;
		Pos  = Keys;
	}
	while( (addr = Child(Keys)) );
}


/*--------------------------- get_leftmostchild ----------------------------*\
 *
 * Purpose	 : Reads the leftmost key in a subtree with root address <addr>.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   addr		- The root address of the subtree.
 *
 * Returns	 : Nothing.
 *
 */

void get_leftmostchild(I, addr)
INDEX *I;
ulong addr;
{
	/* No tree has root at address 0 */
	if( !addr )
		return;

	do
	{
		noderead(I, I->node, addr);

		Level++;
		Addr = addr;
		Pos  = 0;
	}
	while( (addr = Child(0)) );
}


/*------------------------------- btree_frst -------------------------------*\
 *
 * Purpose	 : Read the smallest key value in a B-tree, i.e. the leftmost key.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   ref			- Contains reference when function returns.
 *
 * Returns	 : S_OKAY		- The key was found. <ref> contains reference.
 *			   S_NOTFOUND	- The B-tree is empty.
 *
 */
int btree_frst(I, ref)
INDEX *I;
ulong *ref;
{
	I->curr = 0;
	I->hold = 0;
	Level = 1;
	Addr = 1;
	Pos = 0;

	/* Get the nost recent sequence number */
   	btree_getheader(I);
	
	if( noderead(I, I->node, 1) == (ix_addr)-1 )
		RETURN S_NOTFOUND;

	get_leftmostchild(I, Child(0));

	I->curr = 1;
	*ref = Ref(Pos);
	memcpy(I->curkey, Key(Pos), I->H.keysize);

	RETURN S_OKAY;
}


/*------------------------------- btree_last -------------------------------*\
 *
 * Purpose	 : Read the greatest key value in a B-tree, i.e. the rightmost key.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   ref			- Contains reference when function returns.
 *
 * Returns	 : S_OKAY		- The key was found. <ref> contains reference.
 *			   S_NOTFOUND	- The B-tree is empty.
 *
 */
int btree_last(I, ref)
INDEX *I;
ulong *ref;
{
	I->curr = 0;
	I->hold = 0;
	Level = 1;
	Addr = 1;

	/* Get the nost recent sequence number */
   	btree_getheader(I);
	
	if( noderead(I, I->node, 1) == (ix_addr)-1 )
		RETURN S_NOTFOUND;

	Pos = Keys;
	get_rightmostchild(I, Child(Keys));

	/* Move pos one step leftwards so that Pos is the current key */
	Pos--;
	I->curr = 1;
	*ref = Ref(Pos);
	memcpy(I->curkey, Key(Pos), I->H.keysize);

	RETURN S_OKAY;
}


static void synchronize(I)
INDEX *I;
{
	ulong old_ts = I->H.timestamp;
	ulong ref;

 	btree_getheader(I);

	if( old_ts != I->H.timestamp )
		btree_find(I, I->curkey, &ref);
}




/*------------------------------- btree_prev -------------------------------*\
 *
 * Purpose	 : Find key value in a B-tree with less or equal (if duplicates)
 *			   key value that the current key. If there is no current key,
 *			   the function is equivalent to btree_last(). If the current
 *			   position before the call is the leftmost position in the tree,
 *			   S_NOTFOUND is returned.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   ref			- Contains reference if a key is found.
 *
 * Returns	 : S_OKAY		- Key value found. <ref> contains reference.
 *			   S_NOTFOUND	- The key with the smallest value has already
 *							  been reached.
 *
 */

int btree_prev(I,ref)
INDEX *I;
ulong *ref;
{
	if( I->shared )
		synchronize(I);

	if( I->hold )
		goto out;

	if( !I->curr )
		return btree_last(I, ref);

	if( Child(Pos) > 0 )						/* Non-leaf node			*/
	{
		/* Get the rightmost child in the left subtree */
		get_rightmostchild(I, Child(Pos));
	}
	else if( Pos == 0 )							/* Leaf node at first pos	*/
	{
		/* Move upwards until a node with Pos > 0 or root is reached */
		while( Pos == 0 && Addr != 1 )
		{
			Level--;
			noderead(I, I->node, Addr);
		}

		if( Pos == 0 && Addr == 1 )
		{
			I->curr = 0;
			RETURN S_NOTFOUND;
		}
	}
	Pos--;

out:
	I->curr = 1;
	I->hold = 0;

	*ref = Ref(Pos);
	memcpy(I->curkey, Key(Pos), I->H.keysize);

	RETURN S_OKAY;
}


/*------------------------------- btree_next -------------------------------*\
 *
 * Purpose	 : Find key value in a B-tree with greater or equal (if duplicates)
 *			   key value that the current key. If there is no current key,
 *			   the function is equivalent to btree_frst(). If the current
 *			   position before the call is the rightmost position in the tree,
 *			   S_NOTFOUND is returned.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   ref			- Contains reference if a key is found.
 *
 * Returns	 : S_OKAY		- Key value found. <ref> contains reference.
 *			   S_NOTFOUND	- The key with the greatest value has already
 *							  been reached.
 *
 */
int btree_next(I,ref)
INDEX *I;
ulong *ref;
{
	if( I->shared )
		synchronize(I);

	if( I->hold )
	{
		/* An unsuccessful keyfind must be handled as a special case:
		 * If we are at the rightmost position in a node, move upwards until
		 * we reach a node where the position is not the rightmost. This
		 * key should be the proper next key in the B-tree.
		 */

		while( Pos == Keys && Level > 1 )
		{
			Level--;
		    noderead(I, I->node, Addr);
 		}

		if( Level == 1 && Pos == Keys )
			RETURN S_NOTFOUND;

		goto out;
	}

	if( !I->curr )
		return btree_frst(I, ref);

	if( Child(Pos) > 0 )						/* Non-leaf node			*/
	{
		/* Get the leftmost child in the left subtree */
		Pos++;
		get_leftmostchild(I, Child(Pos));
	}
	else if( Pos >= Keys-1 )					/* Leaf node at first pos	*/
	{
#if 0
		if( Pos == Keys-1 && Addr == 1 )	94/03/17 TBP
#else
		if( Pos >= Keys-1 && Addr == 1 )
#endif
		{
			I->curr = 0;
			RETURN S_NOTFOUND;
		}

		/* Move upwards until a node with Pos < Keys-1 or root is reached */
		do
		{
			Level--;
			noderead(I, I->node, Addr);
		}
		while( Pos >= Keys && Addr != 1 );

		if( Pos == Keys && Addr == 1 )
		{
			I->curr = 0;
			RETURN S_NOTFOUND;
		}
	}
	else										/* Leaf node				*/
		Pos++;

out:
	I->curr = 1;
	I->hold = 0;

	*ref = Ref(Pos);
	memcpy(I->curkey, Key(Pos), I->H.keysize);

	RETURN S_OKAY;
}
 
/* end-of-file */

