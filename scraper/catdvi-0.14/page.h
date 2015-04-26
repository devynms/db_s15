/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
   Copyright (C) 2001 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

#ifndef PAGE_H
#define PAGE_H

#include "bytesex.h"
#include "pageref.h"

typedef sint32 glyph_t, font_t;

extern int page_sequential;
extern int page_list_numbers;
extern struct pageref_t page_start_output, page_last_output;

void page_begin(sint32 count0);

/* glyph is Unicode value */
void page_set_glyph(
    font_t font, glyph_t glyph,
    sint32 width, sint32 height, sint32 depth, sint32 axis_height,
    sint32 x, sint32 y
);

/* page_end will output the resulting page */
void page_end(void);

#endif /* PAGE_H */
