/* catdvi - get text from DVI files
   Copyright (C) 2000 Bjoern Brill <brill@fs.math.uni-frankfurt.de>
 
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


#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "linebuf.h"
#include "util.h"

void linebuf_init(linebuf_t * this, size_t initial_size_alloc)
{
    if (initial_size_alloc == 0) initial_size_alloc = 81;
    this->size_alloc = initial_size_alloc;
    
    this->gstring = malloc(initial_size_alloc * sizeof(glyph_t));
    if(!this->gstring) enomem();
    
    linebuf_clear(this);
}

void linebuf_garray_init(linebuf_t * this, const glyph_t garray[], size_t len)
{
    this->size_curr = len;
    this->size_alloc = len + 1;
    
    this->gstring = malloc(this->size_alloc * sizeof(glyph_t));
    if(!this->gstring) enomem();
    
    memcpy(this->gstring, garray, sizeof(glyph_t) * this->size_alloc);
    this->gstring[this->size_curr] = 0;
}

void linebuf_garray0_init(linebuf_t * this, const glyph_t garray0[])
{
    int len = 0;
    while(garray0[len] != 0) ++len;
    linebuf_garray_init(this, garray0, len);
}

void linebuf_done(linebuf_t * this)
{
    assert(this->gstring);
    
    free(this->gstring);
    this->gstring = NULL;
}


void linebuf_clear(linebuf_t * this)
{
    assert(this->gstring);
    
    this->size_curr = 0;
    this->gstring[0] = 0;
}

static void linebuf_need_size(linebuf_t * this, size_t size)
{
    assert(this->gstring);
    
    if (size < this->size_alloc) return;

    while(size >= this->size_alloc) this->size_alloc *= 2;
    this->gstring = realloc(
	    this->gstring,
	    this->size_alloc * sizeof(glyph_t)
    );
    if (!this->gstring) enomem();
}
    
void linebuf_putg(linebuf_t * this, glyph_t g)
{
    assert(this->gstring);
    
    this->gstring[this->size_curr++] = g;
    if(this->size_curr >= this->size_alloc) {
    	/* This check is not required before calling linebuf_need_size(),
	 * but saves a function call in most cases.
	 */
    	linebuf_need_size(this, this->size_curr);
    }
    this->gstring[this->size_curr] = 0;
}


glyph_t linebuf_peekg(linebuf_t * this)
{
    assert(this->gstring);
    
    if(this->size_curr == 0) return(0);
    return(this->gstring[this->size_curr - 1]);
}


glyph_t linebuf_unputg(linebuf_t * this)
{
    glyph_t last;
    
    assert(this->gstring);
    
    if(this->size_curr == 0) return(0);
    
    last = this->gstring[--(this->size_curr)];
    this->gstring[this->size_curr] = 0;
    return(last);
}

void linebuf_append(linebuf_t * this, const linebuf_t * appendix)
{
    assert(this->gstring);
    
    linebuf_need_size(this, this->size_curr + appendix->size_curr);
    memcpy(
    	this->gstring + this->size_curr,
	appendix->gstring,
	sizeof(glyph_t) * appendix->size_curr
    );
    this->size_curr += appendix->size_curr;
    this->gstring[this->size_curr] = 0;
}

