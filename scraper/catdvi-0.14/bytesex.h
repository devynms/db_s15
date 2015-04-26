/* catdvi - get text from DVI files
   Copyright (C) 1999 J.H.M. Dassen (Ray) <jdassen@wi.LeidenUniv.nl>
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


#ifndef BYTESEX_H
#define BYTESEX_H

#include <limits.h>
#include <stdio.h>

#if !defined(CHAR_BIT) || CHAR_BIT < 8
#    error "the C environment is not standards compliant"
#elif CHAR_BIT > 8
#    warning "char is wider than octet; problems may occur"
#endif

typedef unsigned char byte;
/* FIXME ? */
typedef unsigned long uint32;
typedef signed long sint32;

/* The limit macros should not need machine-specific fixing even if there's no
 * type exactly 32 bit wide on the machine -- we will never have to deal
 * with values that do not fit into 32 bits.
 */
#ifndef UINT32_MAX
#define UINT32_MAX  0xffffffffUL
#endif
#ifndef SINT32_MAX
#define SINT32_MAX  0x7fffffffL
#endif
#ifndef SINT32_MIN
#define SINT32_MIN (-SINT32_MAX - 1L)
#endif

void efread(void *ptr, size_t size, size_t nmemb,
            FILE *stream, char *errmsg);

/* A portable reader for muli-octet big-endian numbers. */
uint32 u_readbigendiannumber(byte count, FILE *stream);
sint32 s_readbigendiannumber(byte count, FILE *stream);

byte readbyte(FILE * stream);

/* Read a string in BCBL format.  len is the expected length of the
   string. */
void readbcblstring(byte * buffer, uint32 slen, FILE *);

void skipbytes(int count, FILE *stream);

#endif /* BYTESEX_H */
