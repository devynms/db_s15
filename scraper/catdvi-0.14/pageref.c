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

#include "pageref.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

pageref_flavour_t pageref_parse(pageref_t * this, const char * pagespec)
{
    char * tail, * colon;
    pageref_t r = {0, 0, 0, PRF_INVALID};
    sint32 * field;
    
    colon = strchr(pagespec, ':');
    
    if(pagespec[0] == '=' || pagespec[0] == '@') {
    	/* physical page */
	++pagespec;
	r.flavour = PRF_PHYSICAL;
	field = &r.physical;
    }
    else if(colon != NULL) {
    	/* chapter:count0 */
	if(pagespec[0] == ':') return PRF_INVALID;
	r.chapter = strtol(pagespec, &tail, 10);
	if(tail != colon) return PRF_INVALID;
	pagespec = colon + 1;
	r.flavour = PRF_COUNT0_CHAPTER;
	field = &r.count0;
    }
    else {
    	/* count0 without chapter */
    	r.flavour = PRF_COUNT0;
	field = &r.count0;
    }
    
    if(*pagespec == 0) return PRF_INVALID;
    *field = strtol(pagespec, &tail, 10);
    if(*tail != 0) return PRF_INVALID;
    *this = r;
    return r.flavour;
}


/* Return value like strcmp */
static int sint32_cmp(sint32 a, sint32 b)
{
    if (a > b) return 1;
    else if (a < b) return -1;
    else return 0;
}

int pageref_count0_cmp(sint32 a, sint32 b)
{
    if ((a < 0) && (b < 0)) return sint32_cmp(-a, -b);
    else return sint32_cmp(a, b);
    	/* if only one of them is negative we get it right the usual way */
}

int pageref_cmp(const struct pageref_t * a, const struct pageref_t * b)
{
    enum pageref_flavour_t fa, fb;
    int r;
    
    fa = a->flavour;
    fb = b->flavour;

    /* flavour PRF_COMPLETE is compatible with everything */    
    if (fa == PRF_COMPLETE) {
    	if (fb == PRF_COMPLETE) {
	    /* OK, we choose physical pages then */
	    fa = PRF_PHYSICAL;
	    fb = PRF_PHYSICAL;
	}
	else fa = fb;
    }
    else if (fb == PRF_COMPLETE) fb = fa;
    
    if (fa != fb)
    	panic("pageref_cmp: incompatible page reference flavours\n");
    
    switch(fa) {
	case PRF_INVALID:
    	    panic("pageref_cmp: invalid page reference\n");
	    break;
    	case PRF_PHYSICAL:
	    return sint32_cmp(a->physical, b->physical);
	    break;
	case PRF_COUNT0:
	    return pageref_count0_cmp(a->count0, b->count0);
	    break;
    	case PRF_COUNT0_CHAPTER:
	    r = sint32_cmp(a->chapter, b->chapter);
	    if (r != 0) return r;
	    else return pageref_count0_cmp(a->count0, b->count0);
	    break;
	default:
    	    panic("pageref_cmp: unknown page reference flavour\n");
    }
    NOTREACHED;
    return 0; /* get rid of pointless compiler warning */
}

void pageref_print(pageref_t * this, FILE * f)
{
    switch(this->flavour) {
    	case PRF_INVALID:
	    fputs("INVALID\n", f);
	    break;
    	case PRF_PHYSICAL:
	    fprintf(
	    	f,
		"physical=%5ld\n",
		this->physical
	    );
	    break;
	case PRF_COUNT0:
	    fprintf(
	    	f,
		"count0=%5ld\n",
		this->count0
	    );
	    break;
    	case PRF_COUNT0_CHAPTER:
	    fprintf(
	    	f,
		"count0=%5ld, chapter=%5ld\n",
		this->count0,
		this->chapter
	    );
	    break;
    	case PRF_COMPLETE:
	    fprintf(
	    	f,
		"physical=%5ld, count0=%5ld, chapter=%5ld\n",
		this->physical,
		this->count0,
		this->chapter
	    );
	    break;
	default:
	    fputs("UNKNOWN\n", f);
    }
}
