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

#ifndef FIXWORD_H
#define FIXWORD_H

#include "bytesex.h"

/* fix_word is used in TFM files; read the TFM file spec for more
   info. */

#define FW_FIX_WORD_BIT  32
#define FW_WHOLEPART_BIT 12
#define FW_FRACTION_BIT  (FW_FIX_WORD_BIT - FW_WHOLEPART_BIT)

typedef sint32 fix_word_t;

/* fix_word_t read_fw(FILE *); */
#define read_fw(fp) (s_readbigendiannumber(4,(fp)))

double fw2double(fix_word_t);

/* fix_word_t double2fw(double); */
#define double2fw(dbl) ((fix_word_t) ((dbl) * (1 << FW_FRACTION_BIT)))

#define fw2int(fw) ((fw) / (1 << FW_FRACTION_BIT))

/* fix_word_t fw_negate(fix_word_t); */
#define fw_negate(fw) (-(fw))

/* fix_word_t fw_sum(fix_word_t, fix_word_t); */
#define fw_sum(fw1,fw2) ((fw1) + (fw2))

/* Calculates x * y. */
fix_word_t fw_prod(fix_word_t x, fix_word_t y);



#endif /* FIXWORD_H */
