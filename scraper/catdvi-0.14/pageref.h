/* catdvi - get text from DVI files
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

#ifndef PAGEREF_H
#define PAGEREF_H

#include "bytesex.h"

enum pageref_flavour_t {
    PRF_INVALID = 0,
    PRF_PHYSICAL,
    PRF_COUNT0,
    PRF_COUNT0_CHAPTER,
    PRF_COMPLETE
};
typedef enum pageref_flavour_t pageref_flavour_t;

typedef struct pageref_t {
    sint32 physical;
    sint32 count0;
    sint32 chapter;
    enum pageref_flavour_t flavour;
} pageref_t;

/* Make up a pageref_t from a string (syntax documented in catdvi manpage).
 * Return flavour (!= 0 if no error).
 * If an error occurs, we return PRF_INVALID (== 0) and do NOT modify *this.
 */
pageref_flavour_t pageref_parse(pageref_t * this, const char * pagespec);

/* \count0 usually holds the TeX page number. Plain TeX uses negative count0
 * values for roman-numbered frontmatter (preface, toc, etc.) so we have
 * the integers ordered like -1 < -2 < -3 < ... < 0 < 1 < 2 < 3 < ...
 * (0 shouldn't appear as page number though).
 *
 * Compare two count0 values wrt this ordering. Return value like strcmp.
 */
int pageref_count0_cmp(sint32 a, sint32 b);

/* Compare two page references, given their flavours are compatible.
 * Return value like strcmp.
 */
int pageref_cmp(const struct pageref_t * a, const struct pageref_t * b);

void pageref_print(pageref_t * this, FILE * f);

#endif /* PAGEREF_H */
