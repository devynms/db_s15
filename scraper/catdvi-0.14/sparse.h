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

#ifndef SPARSE_H
#define SPARSE_H

/* Implements "sparse arrays" of void * and sint32.
 *
 * The assigned entries are stored as the leaves of a dynamically growing
 * (2^SPAR_BITS_PER_STAGE)-ary tree. Branches not leading to an assigned
 * entry are not created, and looking them up returns a default value.
 *
 * Two structures with accompanying methods are defined,
 * sparp_t (for void *) and spars32_t (for sint32). Almost all of the code
 * is shared and factored out into a common "base class" spar_t.
 *
 * Note sparp_t is not at all interested in what the void *'s it holds
 * may point to.
 *
 * The default value is NULL in the case of void * and settable in the
 * constructor call in the case of sint32.
 */

#include "bytesex.h" 	/* for sint32, uint32 */

/******************************* spar_t *******************************/
typedef uint32 spar_index_t;
    /* Could be changed to any unsigned integral type */

#define SPAR_BITS_PER_STAGE 8
    /* Configurable. Need not divide (bits per spar_index_t) */

typedef union spar_node_t {
    union spar_node_t * children;
    /* payload */
    void * p;
    sint32 s32;
} spar_node_t;

typedef struct spar_t {
    int height;
    int max_shift;
    spar_index_t max_index;
    spar_node_t root;
    spar_node_t default_leaf;
} spar_t;

/* constructor */
void spar_init(spar_t * this, spar_node_t default_entry);

/* destructor */
void spar_done(spar_t * this);

/* Get pointer to const leaf node (array entry) with index i.
 * Returns pointer to some node with default value if unassigned.
 */
const spar_node_t * spar_const_entry(const spar_t * this, spar_index_t i);

/* Get pointer to leaf node (array entry) with index i, creating it
 * if necessary. Newly created nodes have the default value. The pointer
 * target is assignable.
 * 
 * Consecutive calls to this function (and to spar_const_entry()) with
 * the same arguments will always return the same address.
 */
spar_node_t * spar_assignable_entry(spar_t * this, spar_index_t i);

/******************************* sparp_t *******************************/
typedef spar_index_t sparp_index_t;

typedef struct sparp_t {
    spar_t p_spar;
} sparp_t;

/* constructor */
void sparp_init(sparp_t * this);

/* destructor */
void sparp_done(sparp_t * this);

/* Read array entry with index i.
 * This behaves like a function with prototype
 *   void * sparp_read(sparp_t * this, sparp_index_t i);
 */
#define sparp_read(this, i) (spar_const_entry(&((this)->p_spar), i)->p)

/* Write array entry with index i.
 * This behaves like a function with prototype
 *   void sparp_write(sparp_t * this, sparp_index_t i, void * v);
 */
#define sparp_write(this, i, v) \
    ((void) (spar_assignable_entry(&((this)->p_spar), i)->p = v))

/* Get pointer to array entry with index i, creating it
 * if necessary. Newly created entries have value NULL. The pointer
 * target is assignable.
 * 
 * Consecutive calls to this function with
 * the same arguments will always return the same address.
 *
 * This behaves like a function with prototype
 *  void ** sparp_assignable_entry(sparp_t * this, sparp_index_t i);
 */
#define sparp_assignable_entry(this, i) \
     (&(spar_assignable_entry(&((this)->p_spar), i)->p))

/******************************* spars32_t *******************************/
typedef spar_index_t spars32_index_t;

typedef struct spars32_t {
    spar_t s32_spar;
} spars32_t;

/* constructor */
void spars32_init(spars32_t * this, sint32 default_value);

/* destructor */
void spars32_done(spars32_t * this);

/* Read array entry with index i.
 * This behaves like a function with prototype
 *   sint32 spars32_read(spars32_t * this, spars32_index_t i);
 */
#define spars32_read(this, i) (spar_const_entry(&((this)->s32_spar), i)->s32)

/* Write array entry with index i.
 * This behaves like a function with prototype
 *   void spars32_write(spars32_t * this, spars32_index_t i, sint32 v);
 */
#define spars32_write(this, i, v) \
    ((void) (spar_assignable_entry(&((this)->s32_spar), i)->s32 = v))

/* Get pointer to array entry with index i, creating it
 * if necessary. Newly created entries have default value. The pointer
 * target is assignable.
 * 
 * Consecutive calls to this function with
 * the same arguments will always return the same address.
 *
 * This behaves like a function with prototype
 *  sint32* spars32_assignable_entry(spars32_t * this, spars32_index_t i);
 */
#define spars32_assignable_entry(this, i) \
     (&(spar_assignable_entry(&((this)->s32_spar), i)->s32))

#endif
