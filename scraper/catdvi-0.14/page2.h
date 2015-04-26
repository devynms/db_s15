/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
   Copyright (C) 2000-02 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

/* Files that need some of the internals of page.c exposed should include
 * this header rather than page.h
 */

#ifndef PAGE2_H
#define PAGE2_H

#include "page.h"

/* A page is modeled as an ordered doubly-linked list of boxes (x, y,
   glyph), where the ordering is left-to-right top-to-bottom. */

enum box_flag_t {
    BF_SWIMMING =   1 << 0,
    	/* (x, y) != (left border, baseline). We need to
    	 * move this one to the "logically right" place.
	 */
    BF_HAS_AXIS =   1 << 1,
    	/* box_t.axis is valid, i.e. we know where the math
    	 * axis passes through this box.
	 */
    BF_ON_AXIS =    1 << 2,
    	/* This box is centered on the math axis (e.g. the
    	 * big operators). Implies BF_SWIMMING.
    	 */
    BF_RADICAL =    1 << 3,
    	/* This is a radical sign (and hence "hanging down"
    	 * from the y coordinate). Implies BF_SWIMMING.
	 */
    BF_DIACRITIC =  1 << 4 
    	/* This is a diacritical mark. Often its baseline is vertically
	 * displaced against the baseline of the glyph it should accent
    	 * because the combination looks better this way).
	 * Implies BF_SWIMMING.
	 */
};
   
struct box_t {
        sint32 x;
        sint32 y;
	sint32 axis;
	
        sint32 width;
        sint32 height;
	sint32 depth;
	sint32 axis_height; /* The TeX font parameter. <= 0 if unknown. */
	
        glyph_t glyph;
        font_t font;
	enum box_flag_t flags;
};

typedef struct list_node_t list_node_t;
struct list_node_t {
        struct box_t b;
        struct list_node_t * prev;
        struct list_node_t * next;
};

/* List support variables.  Note that most insertions happen near the
   previous insertion, so it's beneficial to keep a pointer to the
   last inserted node. */
extern struct list_node_t * list_head;
extern struct list_node_t * list_tail;
extern struct list_node_t * list_latest; /* Node that was inserted last. */


/* normalize placement of combining diacritics */
void page_adjust_diacritics(void);

/* normalize placement of big operators etc. */
void page_adjust_texmext(void);

/* normalize placement of radicals */
void page_adjust_radicals(void);

#endif /* PAGE2_H */
