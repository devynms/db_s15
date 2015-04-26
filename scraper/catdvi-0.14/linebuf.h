/* catdvi - get text from DVI files
   Copyright (C) 2000, 2001 Bjoern Brill <brill@fs.math.uni-frankfurt.de>
 
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


/* Implements a simplistic "string of glyph_t" class. Can grow dynamically.
 * Null termination is provided, but not used by the class methods.
 */

#ifndef LINEBUF_H
#define LINEBUF_H

#include <stddef.h>

/* for definition of glyph_t */
#include "page.h"

typedef struct linebuf_t linebuf_t;
struct linebuf_t {
    size_t size_alloc;
    size_t size_curr;
    glyph_t * gstring;
};

/* Construct as empty string. Specify initially allocated space (to avoid
 * continuous realloc()s). Use 0 for default size (currently 80 glyphs).
 */
void linebuf_init(struct linebuf_t * this, size_t initial_size_alloc);

/* Construct from array garray of length len. */
void linebuf_garray_init(linebuf_t * this, const glyph_t garray[], size_t len);

/* Construct from 0-terminated array garray0. */
void linebuf_garray0_init(linebuf_t * this, const glyph_t garray0[]);

/* destructor */
void linebuf_done(struct linebuf_t * this);

void linebuf_clear(struct linebuf_t * this);

/* Append another linebuf */
void linebuf_append(linebuf_t * this, const linebuf_t * appendix);

/* Append one glpyh */
void linebuf_putg(struct linebuf_t * this, glyph_t g);

/* Look at last glyph */
glyph_t linebuf_peekg(struct linebuf_t * this);

/* Remove last glyph */
glyph_t linebuf_unputg(struct linebuf_t * this);

#endif
