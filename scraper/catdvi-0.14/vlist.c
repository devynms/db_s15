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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "vlist.h"
#include "util.h"

#ifndef NDEBUG

/* try to catch accesses to freed list nodes */
static const vlist_node_t dead_node = {NULL, NULL, NULL};
#define free_node(p) {*(p) = dead_node; free(p);}

#else

#define free_node(p) free(p)

#endif


void vlist_init(vlist_t * this)
{
    this->rend_node.prev = NULL;
    this->rend_node.next = vlist_end(this);
    this->rend_node.data = NULL;

    this->end_node.prev = vlist_rend(this);
    this->end_node.next = NULL;
    this->end_node.data = NULL;
    
    this->size = 0;
}

void vlist_done(vlist_t * this)
{
    vlist_clear(this);
    this->rend_node.next = NULL;
    this->end_node.prev = NULL;
    this->size = 0;
}


void vlist_push_front(vlist_t * this, void const * newdata)
{
    vlist_insert_before(this, vlist_begin(this), newdata);
}

void vlist_pop_front(vlist_t * this)
{
    assert(this->size > 0);
    vlist_erase(this, vlist_begin(this));
}

void vlist_push_back(vlist_t * this, void const * newdata)
{
    vlist_insert_after(this, vlist_rbegin(this), newdata);
}

void vlist_pop_back(vlist_t * this)
{
    assert(this->size > 0);
    vlist_erase(this, vlist_rbegin(this));
}


vitor_t vlist_insert_before(
    vlist_t * this,
    vitor_t where,
    void const * newdata
)
{
    vitor_t prev, newnode;

    assert(where != vlist_rend(this));
    prev = where->prev;

    newnode = malloc(sizeof(vlist_node_t));
    if(newnode == NULL) enomem();

    newnode->prev = prev;
    newnode->next = where;
    newnode->data = (void *) newdata; /* de-const is intentional */

    prev->next = newnode;
    where->prev = newnode;

    this->size += 1;
    return newnode;
}

vitor_t vlist_insert_after(
    vlist_t * this,
    vitor_t where,
    void const * newdata
)
{
    vitor_t next, newnode;

    assert(where != vlist_end(this));
    next = where->next;

    newnode = malloc(sizeof(vlist_node_t));
    if(newnode == NULL) enomem();

    newnode->prev = where;
    newnode->next = next;
    newnode->data = (void *) newdata; /* de-const is intentional */

    where->next = newnode;
    next->prev = newnode;

    this->size += 1;
    return newnode;
}

void vlist_erase(vlist_t * this, vitor_t what)
{
    vitor_t prev, next;

    assert(what != vlist_rend(this));
    assert(what != vlist_end(this));
    prev = what->prev;
    next = what->next;

    prev->next = next;
    next->prev = prev;

    free_node(what);
    this->size -= 1;
}

void vlist_clear(vlist_t * this)
{
    vitor_t p, q;
    
    for(p = vlist_begin(this); p != vlist_end(this); p = q) {
    	q = p->next;
	free_node(p);
    }
    
    this->rend_node.next = vlist_end(this);
    this->end_node.prev = vlist_rend(this);

    this->size = 0;
}


void vlist_swap_consecutive(vlist_t * this, vitor_t first, vitor_t second)
{
    vitor_t prev, next;

    assert(first != vlist_rend(this));
    assert(second != vlist_end(this));
    assert(first->next == second);

    prev = first->prev;
    next = second->next;

    prev->next = second;
    second->next = first;
    first->next = next;

    next->prev = first;
    first->prev = second;
    second->prev = prev;
}

void vlist_move_before(vlist_t * this, vitor_t where, vitor_t what)
{
    vitor_t oldprev, oldnext, newprev;

    assert(what != vlist_rend(this));
    assert(what != vlist_end(this));
    assert(where != vlist_rend(this));
    assert(what != where);

    /* unlink what */
    oldprev = what->prev;
    oldnext = what->next;
    oldprev->next = oldnext;
    oldnext->prev = oldprev;

    /* and link in before where */
    newprev = where->prev;
    	/* don't do this earlier or it will break if where->prev == what */
    newprev->next = what;
    what->next = where;
    where->prev = what;
    what->prev = newprev;
}

void vlist_move_after(vlist_t * this, vitor_t where, vitor_t what)
{
    vitor_t oldprev, oldnext, newnext;

    assert(what != vlist_rend(this));
    assert(what != vlist_end(this));
    assert(where != vlist_end(this));
    assert(what != where);

    /* unlink what */
    oldprev = what->prev;
    oldnext = what->next;
    oldprev->next = oldnext;
    oldnext->prev = oldprev;

    /* and link in after where */
    newnext = where->next;
    	/* don't do this earlier or it will break if where->next == what */
    where->next = what;
    what->next = newnext;
    newnext->prev = what;
    what->prev = where;
}
