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

#ifndef CANVAS_H
#define CANVAS_H

#include "linebuf.h"
#include "bytesex.h"

typedef struct canv_cell_t canv_cell_t;

typedef struct canv_line_t canv_line_t;

typedef struct canv_t canv_t;
struct canv_t {
    sint32 ncols;
    sint32 nlines;
    
    canv_line_t * lines;    
    canv_cell_t * cells;
};

/* constructor */
void canv_init(canv_t * this, sint32 ncols, sint32 nlines);

/* destructor */
void canv_done(canv_t * this);

/* Put unicode string *lb starting at (x, y) -- this is the leftmost
 * glyph in the string. Coordinates are 0 <= x < ncols, 0 <= y < nlines
 * and increase left to right, top to bottom.
 * lb is assumed to be on the heap. canv takes ownership of lb
 * (e.g. canv_done will destruct and free it). If this is not desirable,
 * create a copy and pass that.
 */
void canv_put_linebuf(
    canv_t * this,
    linebuf_t * lb,
    sint32 xleft,
    sint32 y,
    sint32 width
);

/* Put a single unicode glyph. It is assumed to have output width 1. */
void canv_put_glyph(canv_t * this, glyph_t glyph, sint32 x, sint32 y);

/* Put a horizontal line (it is made up from line drawing characters) */
void canv_put_hline(canv_t * this, sint32 xleft, sint32 y, sint32 xright);

/* Put a vertical line (it is made up from line drawing characters) */
void canv_put_vline(canv_t * this, sint32 x, sint32 ybottom, sint32 ytop);

/* Put a rectangle of black ink */
void canv_put_black_box(
    canv_t * this,
    sint32 xleft,
    sint32 ybottom,
    sint32 xright,
    sint32 ytop
);

/* Write the whole character grid to f, using the current output encoding.
 * Trailing whitespace in each line is omitted.
 */
void canv_write(const canv_t * this, FILE * f);

#endif /* !defined CANVAS_H */
