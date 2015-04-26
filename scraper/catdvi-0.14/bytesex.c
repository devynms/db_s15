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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "bytesex.h"
#include "util.h"

#define SKIPBYTES_BUFLEN 128

void efread(void *ptr, size_t size, size_t nmemb,
                   FILE *stream, char *errmsg)
{
        if (fread(ptr, size, nmemb, stream) != nmemb) {
                if (feof(stream)) {
                        panic("Unexpected end of file\n");
                } else {
                        panic("%s\n", strerror(errno));
                }
                panic(errmsg);
        }
}

byte readbyte(FILE * stream)
{
        byte b;

        efread(&b, 1, 1, stream, "Failed to read byte\n");

        return b;
}

/* A portable reader for unsigned muli-octet big-endian numbers. */
uint32 u_readbigendiannumber(byte count, FILE *stream) 
{
        uint32 ures = 0;
        int i;
    	unsigned char buf[4];

        assert(count <= 4);

    	efread(buf, count, 1, stream, "Failed to read big endian number.\n");
	    /* We want to read one chunk of count bytes, not count chunks
	     * of one byte.
	     */
        for (i = 0; i < count; i++) {
                ures = (ures << 8) + buf[i];
        }

        return ures;
}

/* An extremely portable reader for signed (two's complement)
   muli-octet big-endian numbers. */
sint32 s_readbigendiannumber(byte count, FILE *stream) 
{
        uint32 ures;
        const uint32 msb_bitmask = ((uint32) 1) << (8 * count - 1);
        /* Beware of overflow! 2 * msb_bitmask may be too much for
           uint32. */
        const uint32 allset_bitmask = msb_bitmask + (msb_bitmask - 1);

        assert(count <= 4);

        /* First read the octets, taking care of the bytesex.  */
        ures = u_readbigendiannumber(count, stream);
        
        assert(ures <= allset_bitmask);

        /* We need to handle the sign conversion ourselves, as a
           simple cast is implementation-defined.  The value in ures
           is in two's complement format.  */

        if (ures >= msb_bitmask) {
                /* It is negative. */
                return - (sint32) (ures ^ allset_bitmask) - 1;
        }
    	else {
            /* Positive (or 0). */
            return ures;
	}
}

void readbcblstring(byte * buffer, uint32 slen, FILE * fp)
{
        uint32 len;
        
        len = u_readbigendiannumber(1, fp);

        if (len > slen - 1) {
                warning("invalid BCBL string length");
                len = 0;
        }

        if (len > 0) {
                efread(buffer, 1, len, fp, "unable to read BCBL string");
        }
        buffer[len] = 0;

        skipbytes(slen - len - 1, fp);
}

void skipbytes(int count, FILE *stream)
{
        byte buf[SKIPBYTES_BUFLEN];

        while (count > 0) {
                int numbytes = count > SKIPBYTES_BUFLEN
                        ? SKIPBYTES_BUFLEN
                        : count;
                count = count - SKIPBYTES_BUFLEN;
                efread(buf, 1, numbytes, stream,
                       "Error while skipping bytes\n");
        }
}

