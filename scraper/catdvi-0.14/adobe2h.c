/* catdvi - get text from DVI files
   Copyright (C) 2001-2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

/* Standalone program - generates a header which enumerates
 * (in fact #defines, since ISO C restricts enums to int and we don't know
 * if int is 32 bit wide) constant names for all the glyphs in adobetbl.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

typedef unsigned long uint32;

/* FIXME: remove this struct in pse2unic.c and here, move it to adobetbl.h? */
struct adobe2unicode_t {
        char const * adobe;
        uint32 unicode;
};
#define A_MAX_LEN 32
    /* including 0 terminator */


#define PROGNAME "adobe2h"

#define ORIGFILE "adobetbl.h"
#include ORIGFILE

/* #define OUTFILE "glyphenm.h" */
#define GUARD "GLYPHENM_H"



int main(void) {
    FILE * f;
    int i;
    int tbl_len;
    time_t now;

    time(&now);

#ifdef OUTFILE
    f = fopen(OUTFILE, "w");
    if(f == NULL) {
	perror(PROGNAME ": could not open " OUTFILE " for writing");
	exit(1);
    }
#else
    f = stdout;
#endif

    fputs(
    	"/* AUTOMATICALLY GENERATED -- DO NOT EDIT ! */\n"    	    \
	"\n"	    	    	    	    	    	    	    \
	"#ifndef " GUARD "\n"	    	    	    	    	    \
	"#define " GUARD "\n"	    	    	    	    	    \
	"\n",
	f
    );
    fprintf(
    	f,
	"/* created by " PROGNAME " on %s", /* newline from ctime */
    	ctime(&now)
    );
    fputs(
	" * from included glyph name table \"" ORIGFILE "\"\n" 	    \
	" */\n"     	    	    	    	    	    	    \
    	"\n",
	f
    );

    /* loop over all entries and output a #define for each */
    tbl_len = sizeof(adobe2unicode) / sizeof(adobe2unicode[0]);
    for(i = 0; i < tbl_len; ++i) {
    	uint32 u;
	const char * a, * prefix;
	char *p;
    	char s[A_MAX_LEN];
	int duplicate, is_private, is_catdvi;
	
	u = adobe2unicode[i].unicode;
	a = adobe2unicode[i].adobe;

    	/* Attention: adobetbl.h used to have duplicate definitions of some
	 * glyphs (removed by brute force).
	 */
	duplicate = (i > 0) && (strcmp(adobe2unicode[i-1].adobe, a) == 0);

	is_private = (u >= 0xe000 && u <= 0xf8ff) ||
	    (u >= 0xf0000 && u <= 0x10ffff);
   	
    	/* our additions in the glyph name table begin with `$' */
	is_catdvi = (a[0] == '$');

    	/* need to rename .notdef and .notavail */
	if(strcmp(a, ".notdef") == 0) a = "NOTDEF";
	if(strcmp(a, ".notavail") == 0) a = "NOTAVAIL";

	 /* $ is not valid in C identifiers */
	if(is_catdvi) ++a;

    	/* As adobetbl.h is based on the adobe glyph list, we choose a
	 * naming convention that tries to avoid clashes with Adobe's as
	 * defined in
    	 * http://partners.adobe.com/asn/developer/type/unicodegn.html
	 * (to retain source level compatibility with future glyph list
	 * extensions and other software using Adobe glyph names, e.g. dvips).
	 * Some things have to be changed, though, since we want to
	 * generate a sane C header file :)
	 */
	if(is_private) {
	    /* mark Adobes + our private use area */
	    if(is_catdvi) prefix = "GLYPH_CATDVI_";
	    else prefix = "GLYPH_ADOBE_";
	}
	else {
	    if(is_catdvi) prefix = "GLYPH_UNI_";
		/* if our additions are not in the private use area, assume
		 * we have added something that is defined in Unicode but
		 * missing in Adobes glyph list.
		 */
	    else prefix = "GLYPH_";
    	    	/* Is Adobe and Unicode. No special markers required. */
	}

	/* Check for non-alphanumeric characters in glyph names. Dot and
	 * underscore may appear. We change dot to underscore as well.
	 */
	strncpy(s, a, A_MAX_LEN);
	if(s[A_MAX_LEN - 1] != 0) {
	    fprintf(
		stderr,
		PROGNAME ": Warning: glyph name `%s' too long; "    \
		"glyph ignored.\n",
		a
	    );
	    goto some_error;
	}

	for(p = s; *p != 0; ++p) {
	    if(!isalnum(*p)) switch(*p) {
	    	case '_':
		    break;
		case '.':
		    *p = '_';
		    break;
		default:
		    fprintf(
			stderr,
			PROGNAME ": Warning: illegal character "   \
			"`%c\' in glyph name `%s'; glyph ignored.\n",
			*p,
			a
		    );
		    goto some_error;
	    }
	}

	if(duplicate) {
    	    /* We can't have duplicate definitions in the header; assume the
	     * first one is the best and document the others as comment.
	     */
	    fprintf(f, "/* #define %s%s %#06lxL ;Duplicate */\n", prefix, s, u);
	    fprintf(
		stderr,
		PROGNAME ": Warning: duplicate definition for "	\
		"glyph name `%s\'; commented out.\n",
		s
	    );
	}
	else fprintf(f, "#define %s%s  %#06lxL\n", prefix, s, u);

    	some_error: ;
		
    }	/* for i */

    fputs(
	"\n"	    	\
	"#endif /* " GUARD " */\n",
	f
    );
    fclose(f);
    exit(0);
}
