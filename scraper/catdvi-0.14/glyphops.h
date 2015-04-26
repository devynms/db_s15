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
 
 
#ifndef GLYPHOPS_H
#define GLYPHOPS_H

/* for definition of glyph_t */
#include "page.h"

/* assorted magic with unicode characters ("glyphs")
 */

/* bit flags for certain possible properties of glyphs. useful as
 * hints for correct rendering. Most are still unused.
 */
enum glyph_hint_t {
    GH_DIACRITIC =  	    1 <<  0,
    GH_COMBINING =  	    1 <<  1,
    GH_NON_SPACING =  	    1 <<  2,
    GH_GRAPHIC =    	    1 <<  3,
    GH_MATH = 	    	    1 <<  4,
    GH_LIGATURE =   	    1 <<  5,
    GH_ON_AXIS =    	    1 <<  6,
    GH_EXTENSIBLE_RECIPE =  1 <<  7, /* TeX specific */
    GH_WIDE_DIACRITIC =     1 <<  8,
    GH_MOREMATH_LEFT =	    1 <<  9, /* e.g. for right math delimiters */
    GH_RADICAL =     	    1 << 10
};

#define GH_COMBINING_DIACRITIC (GH_DIACRITIC | GH_COMBINING | GH_NON_SPACING)

/* set up internal tables used by the other functions */
void glyphops_init(void);

/* returns our ORed wisdom abut the given glyph */
enum glyph_hint_t glyph_get_hint(glyph_t glyph);

/* unicode knows two kinds of diacritics: combining diacritics are by
 * themselves unprintable characters that change the meaning of the
 * preceding glyph. spacing diacritics are printable (roughly
 * equivalent to space followed by the corresponding combining diacritic).
 *
 * the following functions translate one into the other. idempotent.
 * return zero on failure.
 */
glyph_t diacritic_combining_variant(glyph_t diacritic);
glyph_t diacritic_spacing_variant(glyph_t diacritic);

#endif
