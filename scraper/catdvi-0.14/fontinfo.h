/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>

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


#ifndef FONTINFO_H
#define FONTINFO_H

#include "bytesex.h"
#include "fixword.h"

/* Parameter names as with DVI fnt_defX command. */
void font_def(sint32 k, uint32 c, uint32 s, uint32 d,
              byte a, byte l, char const * n);


/* Return the encoding name of the referenced font. */
char const * font_enc(sint32 k);

/* Return the family name (aka identifier) of the referenced font. */
char const * font_family(sint32 k);

/* Return the width/height/depth of the refereced glyph in
 * the referenced font.
 */
uint32 font_char_width(sint32 font, sint32 glyph);
uint32 font_char_height(sint32 font, sint32 glyph);
uint32 font_char_depth(sint32 font, sint32 glyph);

/* Return the unscaled(!) axis height, or 0 for fonts without this parameter */
uint32 font_axis_height(sint32 font);

/* scale a fixword in font units to DVI units */
fix_word_t font_scale_fw(sint32 font, fix_word_t fw);

/* Parameter numbers as in the TFM format definition */
unsigned int font_nparams(sint32 font);
fix_word_t font_param(sint32 font, unsigned int num);

/* Return a count of space characters that approximately fit in the
   given width. */
int font_w_to_space(sint32 font, sint32 width);

#endif /* FONTINFO_H */
