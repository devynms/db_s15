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

#include "bytesex.h"
#include "fixword.h"
#include "util.h"

fix_word_t fw_remainder = 0;

double fw2double(fix_word_t fw)
{
        return ((double) fw) / (((uint32) 1) << FW_FRACTION_BIT);
}


fix_word_t fw_prod(fix_word_t a, fix_word_t b)
{
        fix_word_t al, bl;

        if (a < 0) return - fw_prod(-a, b);
        if (b < 0) return - fw_prod(a, -b);
	
	/* We have to compute a*b >> 20 without overflowing 32 bits */
        al = a & 32767;
        bl = b & 32767;
        a >>= 15;
        b >>= 15;
	
	return (((al*bl >> 15) + a*bl + al*b) >> 5) + (a*b << 10);
}
