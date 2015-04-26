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

#ifndef OUTENC_H
#define OUTENC_H

#include "linebuf.h"

/* The available encoding numbers. */
enum outenc_num_t {
    OE_UTF8,
    OE_ASCII,
    OE_LATIN1,
    OE_LATIN9,
    OE_TOOBIG /* do not use! */
};

/* The encoding we use. Initialized to OE_ASCII. Changes after calling
 * outenc_init() have no effect.
 */
extern enum outenc_num_t outenc_num;

/* Array of encoding names, indexed by encoding number */
extern char const * const * const outenc_names;

/* Flag: show unicode number instead of `?' for unencodeable glyphs.
 * Initialized to 0.
 */
extern int outenc_show_unicode_number;

/* constructor */
void outenc_init(void);

/* How many columns would that string of glyphs use in printout.
 * Mostly 1 per glyph of course, but more than one for e.g. $ng, hyphen
 * (in ascii and latin1 encodings) and less in the case of combining
 * diacritics.
 */
int outenc_get_width(const linebuf_t * l);

/* guess */
void outenc_write(FILE * f, const linebuf_t * l);

#endif /* OUTENC_H */
