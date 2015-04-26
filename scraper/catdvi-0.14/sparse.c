/* catdvi - get text from DVI files
   Copyright (C) 2001, 2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include "sparse.h"
#include "util.h"


/******************************* spar_t *******************************/
#define SPAR_NODES_PER_BLOCK (1 << SPAR_BITS_PER_STAGE)
#define SPAR_STAGE_MASK ((1 << SPAR_BITS_PER_STAGE) - 1)


/* Readability notes:
 *  - stage(node) := this->height - distance(root, node)
 *                 = distance(node, nearest leaf)
 * 
 *  - invariant: this->max_shift = (this->height - 1) * SPAR_BITS_PER_STAGE
 */
 

/* Allocate a block of SPAR_NODES_PER_STAGE nodes and initialize them
 * with fill_with.
 */
static spar_node_t * spar_new_block(spar_node_t fill_with);

/* Free block block of nodes at stage stage and recursively all of its
 * children.
 */
static void spar_free_block(spar_node_t * block, int stage);

/* Create a new tree root and make the old one the 0-th child of the new,
 * thus increasing the height of the tree by 1.
 */
static void spar_grow(spar_t * this);

/* the default non-leaf node */
static const spar_node_t default_branch = {NULL};


void spar_init(spar_t * this, spar_node_t default_entry)
{
    this->height = 0;
    this->max_shift = - SPAR_BITS_PER_STAGE;
    this->max_index = 0;
    this->default_leaf = default_entry;

    /* we start with a single entry (index 0) with default value */
    this->root = default_entry;
}

void spar_done(spar_t * this)
{
    if(this->height > 0) {
    	spar_free_block(this->root.children, this->height - 1);
    }

    this->root.children = NULL;
    	/* block erroneous access to now freed memory */
}


const spar_node_t * spar_const_entry(const spar_t * this,  spar_index_t i)
{
    int shift;
    const spar_node_t * p;
    
    if(i > this->max_index) return &(this->default_leaf);

    p = &(this->root);
    for(shift = this->max_shift; shift >= 0; shift -= SPAR_BITS_PER_STAGE) {
    	if(p->children == NULL) return &(this->default_leaf);
	p = p->children + ((i >> shift) & SPAR_STAGE_MASK);
    }

    return p;
}


spar_node_t * spar_assignable_entry(spar_t * this, spar_index_t i)
{
    int shift;
    spar_node_t * p;

    while(i > this->max_index) spar_grow(this);
    
    p = &(this->root);
    for(shift = this->max_shift; shift >= 0; shift -= SPAR_BITS_PER_STAGE) {
    	if(p->children == NULL) {
	    /* The path from root to requested leaf exists only until here.
	     * Create the rest and return address of new leaf.
	     */
    	    for( ; shift > 0; shift -= SPAR_BITS_PER_STAGE) {
	    	p->children = spar_new_block(default_branch);
	    	p = p->children + ((i >> shift) & SPAR_STAGE_MASK);
	    }
    	    /* shift == 0 is different because the new children are leaves */
	    p->children = spar_new_block(this->default_leaf);
	    p = p->children + (i & SPAR_STAGE_MASK);

	    return p;
	}
	p = p->children + ((i >> shift) & SPAR_STAGE_MASK);
    }

    return p;
}


static spar_node_t * spar_new_block(spar_node_t fill_with)
{
    spar_node_t * p, * q;
    
    p = xmalloc(SPAR_NODES_PER_BLOCK * sizeof(spar_node_t));
    for(q = p; q < p + SPAR_NODES_PER_BLOCK; ++q) *q = fill_with;
    return p;
}


static void spar_free_block(spar_node_t * block, int stage)
{
    int i;
    
    if(stage > 0) {
    	for(i = 0; i < SPAR_NODES_PER_BLOCK; ++i) {
	    if(block[i].children != NULL) {
	    	spar_free_block(block[i].children, stage - 1);
	    }
	}
    }
    free(block);
}


static void spar_grow(spar_t * this)
{
    spar_node_t * p;
    
    /* We grow at the root, i.e. we have a new root node, and the old one
     * becomes the new roots zeroth child.
     */
    if(this->height ==  0) {
    	p = spar_new_block(this->default_leaf);
    }
    else {
    	p = spar_new_block(default_branch);
    }
    p[0] = this->root;
    this->root.children = p;

    /* tell the world */
    this->height += 1;
    this->max_shift += SPAR_BITS_PER_STAGE;
    this->max_index = (this->max_index << SPAR_BITS_PER_STAGE)
    	| SPAR_STAGE_MASK;

}


/******************************* sparp_t *******************************/

void sparp_init(sparp_t * this) {
    spar_node_t default_leaf;

    default_leaf.p = NULL;
    spar_init(&this->p_spar, default_leaf);
}


void sparp_done(sparp_t * this) {
    spar_done(&this->p_spar);
}


/******************************* spars32_t *******************************/

void spars32_init(spars32_t * this, sint32 default_value) {
    spar_node_t default_leaf;

    default_leaf.s32 = default_value;
    spar_init(&this->s32_spar, default_leaf);
}


void spars32_done(spars32_t * this) {
    spar_done(&this->s32_spar);
}
