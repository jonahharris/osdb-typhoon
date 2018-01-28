/*----------------------------------------------------------------------------
 * File    : bt_del
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
 *   Contains function for removing a tuple from a B-tree index file.
 *   The algorithm is the one is described in "Data structures in Pascal", 
 *   Horowitz & Sahni, Computer Science Press.
 *
 * Functions:
 *   delchain_insert			- Add a node to the delete chain.
 *   merge_siblings				- Merge two sibling nodes to a single node.
 *   move_parentkey				- Move a parent key to another tuple.
 *   find_ref					- Find tuple with correct reference.
 *   replace_with_leftmost_tuple- Copy the leftmost tuple in a subtree.
 *   btree_del					- 
 *
 *--------------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "environ.h"
#ifndef CONFIG_UNIX
#	include <io.h>
#	include <stdlib.h>
#else
#	include <unistd.h>
#	ifdef __STDC__
#		include <stdlib.h>
#	endif
#endif
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_prot.h"
#include "ty_glob.h"
#include "btree.h"

static CONFIG_CONST char rcsid[] = "$Id: bt_del.c,v 1.8 1999/10/04 03:45:07 kaz Exp $";

/*--------------------------- Function prototypes ---------------------------*/
static void	delchain_insert		PRM( (INDEX *,	ix_addr); )
static void merge_siblings		PRM( (INDEX		*I,
									  ix_addr	lsib,
									  ix_addr	rsib, 
									  ix_addr	z,
									  int		zi,
									  char 		*znode,
									  ix_addr	*y,
									  char		*ynode,
									  ix_addr	*p,
									  int		*i); )

static void move_parentkey		PRM( (INDEX		*I,
									  ix_addr	rsib,
									  int		zi,
									  ix_addr	z,
									  char		*znode,
									  ix_addr	y, 
									  char		*ynode); )

static int find_ref					PRM( (INDEX		*I,
									  ulong		ref,
									  ix_addr	*addr,
									  int		*idx,
									  void		*key); )

static void replace_with_leftmost_tuple
								PRM( (INDEX		*I,
									  ix_addr	*y, 
									  char		*ynode,
									  ix_addr	*p,
									  int		*i); )


/*----------------------------- delchain_insert ----------------------------*\
 *
 * Purpose	 : Inserts a deleted B-tree node in the delete chain.
 *
 * Parameters: I		- B-tree index file descriptor.
 *			   addr		- Address of node to insert in delete chain.
 *
 * Returns	 : Nothing.
 *
 */

static void delchain_insert(I, addr)
INDEX *I;
ix_addr addr;
{
	lseek(I->fh, (off_t) ((ulong)I->H.nodesize * (ulong)addr), SEEK_SET);
	write(I->fh, &I->H.first_deleted, sizeof I->H.first_deleted);
	I->H.first_deleted = addr;
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : merge_siblings
 *
 * Purpose   : Merge two siblings nodes.
 *
 * Parameters: I		- Index handle.
 *			   lsib		- Address of left sibling node.
 *			   rsib		- Address of right sibling node.
 *			   z		-
 *			   zi		-
 *			   znode	-
 *			   y		-
 *			   ynode	-
 *			   p		-
 *			   i		-
 *
 * Returns   : Nothing.
 *
 */
static void merge_siblings(I, lsib, rsib, z, zi, znode, y, ynode, p, i)
INDEX	*I;
ix_addr	lsib, rsib, *y, z, *p;
int		zi, *i;
char	*znode;
char	*ynode;
{
	if( rsib )
	{
		/* copy parent key */
		keycopy(I->node, NSIZE(I->node), znode, zi);

		/* copy sibling keys */
		tuplecopy(I->node, NSIZE(I->node)+1, ynode, 0, (size_t) NSIZE(ynode));

		CHILD(I->node,NSIZE(I->node)+1+NSIZE(ynode)) = CHILD(ynode,NSIZE(ynode));

		delchain_insert(I, *p);
	}
	else
	{
		/* make room for left sibling */
		tupleins(I->node, 0, NSIZE(ynode)+1);

		/* copy parent key */
		keycopy(I->node, NSIZE(ynode), znode, zi);

		/* copy sibling keys */
		tuplecopy(I->node, 0, ynode, 0, (size_t) NSIZE(ynode));

		CHILD(I->node,NSIZE(ynode)) = CHILD(ynode,NSIZE(ynode));
		*y = *p;

		delchain_insert(I, lsib);
	}

	tupledel(znode, zi);			/* remove parent key			*/
	NSIZE(znode)--;
	NSIZE(I->node) += 1 + NSIZE(ynode);

	nodewrite(I, I->node, *y);

	/* create new root? */
	if( z == 1 && !NSIZE(znode) )
	{
		*p = 1;
		delchain_insert(I, *y);
	}
	else
	{
		/* process parent */
		nodecopy(I->node, znode);
		*p = z;
		*i = zi;
		I->level--;
	}
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : move_parentkey
 *
 * Purpose   : 
 *
 * Parameters: I		-
 *			   rsib		-
 *			   zi		-
 *			   z		-
 *			   znode	-
 *			   y		-
 *			   ynode	-
 *
 * Returns   : Nothing.
 *
 */
static void move_parentkey(I, rsib, zi, z, znode, y, ynode)
INDEX	*I;
ix_addr	rsib, y, z;
int		zi;
char	*znode, *ynode;
{
	if( rsib )
	{
		/* copy key from parent to p */
		keycopy(I->node, NSIZE(I->node), znode, zi);

		/* copy reference of sibling moved to parent */
		CHILD(I->node, NSIZE(I->node)+1) = CHILD(ynode,0);

		/* copy sibling key to parent */
		keycopy(znode, zi, ynode, 0);
		tupledel(ynode, 0);
	}
	else
	{
		tupleins(I->node, 0, 1);

		/* copy key from parent */
		keycopy(I->node, 0, znode, zi);

		/* copy reference of sibling moved to parent */
		CHILD(I->node,0) = CHILD(ynode, NSIZE(ynode));

		/* copy sibling key to parent */
		keycopy(znode, zi, ynode, NSIZE(ynode)-1);
	}

	NSIZE(ynode)--;
	NSIZE(I->node)++;

	nodewrite(I, ynode, y);			/* update nodes					*/
	nodewrite(I, znode, z);
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : find_ref
 *
 * Purpose   : Find a tuple with a specified reference.
 *
 * Parameters: I		- INDEX handle.
 *			   ref		- Reference number.
 *			   addr		- Address of node to start search in. If the tuple
 *						  is found, the address of the tuple is returned herein
 *
 * Returns   : S_OKAY	- Reference found.
 *
 */

#define Keys		NSIZE(I->node)
#define Child(i)	CHILD(I->node, (i))
#define Key(i)		KEY(I->node, (i))
#define Ref(i)		REF(I->node, (i))
#define Pos			(I->path[I->level].i)
#define Addr		(I->path[I->level].a)
#define Level		(I->level)



 
static int find_ref(I, ref, addr, idx, key)
INDEX	*I;
ulong	ref;
ix_addr	*addr;
int		*idx;
void	*key;
{
	for( ;; )
	{
		*idx  = Pos;
		*addr = Addr;

        if( (*I->cmpfunc)(key, Key(*idx)) )
        {
        	puts("key mismatch");
			break;
		}

		if( Ref(*idx) == ref )
			return S_OKAY;

		if( Child(Pos) > 0 )					/* Non-leaf node			*/
		{
			/* Get the leftmost child in the left subtree */
			Pos++;
			get_leftmostchild(I, Child(Pos));
		}
		else if( Pos >= Keys-1 )				/* Leaf node at first pos	*/
		{
			if( Pos >= Keys-1 && Addr == 1 )
			{
				I->curr = 0;
				RETURN S_NOTFOUND;
			}

			/* Move upward until a node with Pos<Keys-1 or root is reached	*/
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
		else									/* Leaf node				*/
			Pos++;
	}
	
	RETURN S_NOTFOUND;
}

#undef Keys
#undef Child
#undef Key
#undef Ref
#undef Pos
#undef Addr
#undef Level




/*--------------------------------------------------------------------------*\
 *
 * Function  : replace_with_leftmost_tuple
 *
 * Purpose   : Replaces the current tuple [p, I->node, i] with the leftmost
 *			   key in the right subtree (of the current tuple).
 *
 * Parameters: I		- Index handle.
 *			   y		-
 *			   ynode	-
 *			   p		- Address of I->node.
 *			   i		- Index of I->node.
 *
 * Returns   : 
 *
 */
static void replace_with_leftmost_tuple(I, y, ynode, p, i)
INDEX	*I;
ix_addr	*y;
ix_addr	*p;
char	*ynode;
int		*i;
{
	I->path[I->level].i++;

	*y = noderead(I, ynode, CHILD(I->node, *i+1));

	I->path[++I->level].a = CHILD(I->node, *i+1);
	I->path[  I->level].i = 0;

	while( CHILD(ynode,0) )
	{
		*y = noderead(I, ynode, CHILD(ynode, 0));

		I->path[++I->level].a = *y;
		I->path[  I->level].i = 0;
	}

	keycopy(I->node, *i, ynode, 0);			/* copy leftmost key to p,i		*/

	nodewrite(I, I->node, *p);				/* update node p                */
	nodecopy(I->node, ynode);

	*p = *y;
	*i = 0;
}




/*-------------------------------- btree_del -------------------------------*\
 *
 * Purpose	 : Deletes a key in a B-tree. If the deletion causes underflow in
 *			   a node, two nodes are merged and the B-tree possibly shrunk.
 *
 * Parameters: I			- B-tree index file descriptor.
 *			   key			- Key value to delete.
 *			   ref			- Reference of key value. Only used if the B-tree
 *							  contains duplicates.
 *
 * Returns	 : S_OKAY		- Key value successfully deleted.
 *			   S_NOTFOUND	- The key was not in the B-tree.
 *
 */
int btree_del(I, key, ref)
INDEX *I;
void *key;
ulong ref;
{
    ix_addr	p, y, z, lsib, rsib;
    int		i, zi, rc;
    char	*ynode, *znode;

    I->curr = 0;
	I->hold = 0;

	btree_getheader(I);

	if( !d_search(I, key, &p, &i) )
        RETURN S_NOTFOUND;

    if( I->H.dups )
		if( (rc = find_ref(I, ref, &p, &i, key)) != S_OKAY )
			return rc;

	/* Allocate temporaty node buffers */
	if( !(ynode = (char *)malloc((size_t) (I->H.nodesize + I->tsize))) )
		RETURN S_NOMEM;
	if( !(znode = (char *)malloc((size_t) (I->H.nodesize + I->tsize))) )
	{
		free(ynode);
		RETURN S_NOMEM;
	}

     /* if node is a nonleaf, replace key with leftmost key in right subtree */
	if( CHILD(I->node, 0) )
		replace_with_leftmost_tuple(I, &y, ynode, &p, &i);
		
    tupledel(I->node, i);					/* remove key from leaf			*/
    NSIZE(I->node)--;						/* decrease node size by 1		*/

    /* run loop as long there is underflow in p and p is not root			*/
    while( NSIZE(I->node) < (ulong)I->H.order/2 && p != 1 )
    {
        z  = I->path[I->level-1].a;			/* set z = parent				*/
        zi = I->path[I->level-1].i;			/* set zi = parent i			*/

        noderead(I,znode,z);				/* read parent node from disk	*/

		lsib = zi ? CHILD(znode, zi-1) : 0;
		rsib = zi < NSIZE(znode) ? CHILD(znode, zi+1) : 0;

        y = rsib ? rsib : lsib;

        noderead(I, ynode, y);

        if( !rsib )
            zi--;

        if( NSIZE(ynode) > (ulong)I->H.order/2 )
        {
            /* move parent key to p, move nearest key in sibling to p */
        	move_parentkey(I, rsib, zi, z, znode, y, ynode);
        
            goto out;
        }
		else
		{
			/* there is underflow in leaf p - merge with a sibling	*/
			merge_siblings(I, lsib, rsib, z, zi, znode, &y, ynode, &p, &i);
		}
	}

	I->H.keys--;

out:

	if( !NSIZE(I->node) )                  /* if index is empty, truncate	*/
	{
		I->H.first_deleted = 0;
		I->H.keys = 0;
#if defined(CONFIG_DOS) || defined(CONFIG_OS2) 
		chsize(I->fh, 0);
#else
#if	defined(CONFIG_SCO) || defined(CONFIG_NEED_FTRUNCATE)
		os_close(os_open(I->fname, O_RDWR|O_TRUNC, CREATMASK));
#else
		ftruncate(I->fh, I->H.nodesize);
#endif
#endif
	}
	else
		nodewrite(I, I->node, p);  			/* else update node p			*/

	I->H.timestamp++;
	btree_putheader(I);

    free(znode);
    free(ynode);

    RETURN S_OKAY;
}

/* end-of-file */
