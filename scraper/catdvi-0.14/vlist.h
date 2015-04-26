/* catdvi - get text from DVI files
   Copyright (C) 2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

#ifndef VLIST_H
#define VLIST_H

/* Non-intrusive generic backlinked list, realised as list of void*.
 * For variety, I've tried a C++ STL style list instead of traditional
 * C style.
 * It turns out this can be done with some elegance even in C.
 */

typedef struct vlist_node_t vlist_node_t;
struct vlist_node_t {
    vlist_node_t * prev;
    vlist_node_t * next;
    void * data;
};

/* The iterator type, in C++ nomenclature */
typedef vlist_node_t * vitor_t;

typedef struct vlist_t vlist_t;
struct vlist_t {
    /* private */
    vlist_node_t rend_node; /* "inaccessible" node before list head */
    vlist_node_t end_node;  /* "inaccessible" node after list tail */
    /* public const */
    size_t size;
};

/* constructor */
void vlist_init(vlist_t * this);

/* destructor */
void vlist_done(vlist_t * this);

/* rend is the iterator before the list head, used as stop value (instead
 * of NULL) in loops traversing the list backwards.
 */
#define vlist_rend(this) (&(this)->rend_node)

/* end is the iterator after the list tail, used as stop value (instead
 * of NULL) in loops traversing the list forward.
 * This implementation as macros allows the compiler to recognise end and
 * rend as run-time constant inside the loop, so that the calculation can
 * be moved out of the loop and done only once.
 */
#define vlist_end(this) (&(this)->end_node)

/* list head */
#define vlist_begin(this) ((this)->rend_node.next)

/* list tail */
#define vlist_rbegin(this) ((this)->end_node.prev)

/* insert before head */
void vlist_push_front(vlist_t * this, void const * newdata);

/* remove head */
void vlist_pop_front(vlist_t * this);

/* insert after tail */
void vlist_push_back(vlist_t * this, void const * newdata);

/* remove tail */
void vlist_pop_back(vlist_t * this);

/* insert before node */
vitor_t vlist_insert_before(
    vlist_t * this,
    vitor_t where,
    void const * newdata
);

/* insert after node */
vitor_t vlist_insert_after(
    vlist_t * this,
    vitor_t where,
    void const * newdata
);

/* remove node */
void vlist_erase(vlist_t * this, vitor_t what);

/* remove all nodes */
void vlist_clear(vlist_t * this);

/* swap consecutive nodes */
void vlist_swap_consecutive(vlist_t * this, vitor_t first, vitor_t second);

/* move node what before node where */
void vlist_move_before(vlist_t * this, vitor_t where, vitor_t what);

/* move node what after node where */
void vlist_move_after(vlist_t * this, vitor_t where, vitor_t what);

/* get *data as lvalue of appropriate type */
#define vitor_deref(it, type) (*(type *) (it)->data)

/* get data as pointer to appropriate type */
#define vitor2ptr(it, type) ((type *) (it)->data)

#endif /* VLIST_H */
