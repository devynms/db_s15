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

#include <string.h>
#include <stdlib.h>
#include "fntenc.h"
#include "util.h"

/* Font encodings */
#include "enc/blackboard.h"
#include "enc/cork.h"
#include "enc/ot1.h"
#include "enc/ot1wofl.h"
#include "enc/texmext.h"
#include "enc/texmital.h"
#include "enc/texmsym.h"
#include "enc/textt.h"
#include "enc/eurosym.h"
#include "enc/textcomp.h"
#include "enc/marvo98.h"
#include "enc/marvo00.h"
#include "enc/latexsym.h"
#include "enc/eufrak.h"
#include "enc/dummy.h"
#include "enc/amssymba.h"
#include "enc/amssymbb.h"
#include "enc/euex.h"

#define TBLLEN(t) (sizeof((t))/sizeof((t)[0]))

struct fontenc_t {
        char const * encname;
	char const * familypat;
        sint32 * tbl;
        size_t len_tbl;
};

#define DEF_FONTENC(encname, familypat, tbl) \
    { (encname), (familypat), (tbl), TBLLEN((tbl)) }

static struct fontenc_t fontenctbl[] = {
        /* Encoding and family names all uppercase!
	 * If more than one entry matches: first come, first serve.
	 */

    	/* The first entry will be taken as fallback if necessary. */
        DEF_FONTENC("TEX TEXT", "*", OT1Encoding),

    	/* blacklist of fonts lying about their encoding */
        DEF_FONTENC("TEX TEXT WITHOUT F-LIGATURES", "WNCY? V*.*", DummyEncoding),
        DEF_FONTENC("TEX TEXT WITHOUT F-LIGATURES", "WNCY?? V*.*", DummyEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS", "MSAM V2.2", AMSSymbolsAEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS", "MSAM V*.*", DummyEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS", "MSBM V2.2", AMSSymbolsBEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS", "MSBM V*.*", DummyEncoding),

    	/* the other real encodings */
        DEF_FONTENC("TEX TEXT WITHOUT F-LIGATURES", "*", OT1woflEncoding),
        DEF_FONTENC("EXTENDED TEX FONT ENCODING - LATIN", "*", CorkEncoding),
        DEF_FONTENC("TEX MATH ITALIC", "*", TeXMathItalicEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS", "*", TeXMathSymbolEncoding),
        DEF_FONTENC("TEX MATH EXTENSION", "*", TeXMathExtensionEncoding),
        DEF_FONTENC("TEX TYPEWRITER TEXT", "*", TeXTypewriterTextEncoding),
	DEF_FONTENC("U", "EUROSYM", EurosymEncoding),
        DEF_FONTENC("TEX TEXT COMPANION SYMBOLS 1---TS1", "*", TextCompanionEncoding),
        DEF_FONTENC("UNSPECIFIED", "MARTIN_VOGELS_SYMBO", Marvosym1998Encoding),
        DEF_FONTENC("FONTSPECIFIC", "MARVOSYM", Marvosym2000Encoding),
        DEF_FONTENC("LATEX SYMBOLS", "*", LaTeXSymbolsEncoding),
        DEF_FONTENC("BLACKBOARD", "*", BlackboardEncoding),
        DEF_FONTENC("TEX TEXT SUBSET", "EUF? V2.2", EulerFrakturEncoding),
        DEF_FONTENC("TEX MATH SYMBOLS SUBSET", "*", TeXMathSymbolEncoding),
        DEF_FONTENC("TEX MATH ITALIC SUBSET", "*", TeXMathItalicEncoding),
        DEF_FONTENC("EULER SUBSTITUTIONS ONLY", "*", EulerExtensionEncoding)
};

int find_fntenc(char const * encname, char const * family)
{
        int rv = 0; /* default is TeX text */
        int i;
        char * upenc, * upfamily;

        if (encname == 0 || family == 0) return rv;

        upenc = strupcasedup(encname);
	if(upenc == NULL) enomem();
        upfamily = strupcasedup(family);
	if(upfamily == NULL) enomem();

        for (i = 0; i < TBLLEN(fontenctbl); i++) {
                pmesg(85, "[matching fontenc %s to %s... ",
                      fontenctbl[i].encname, upenc);
                if (strcmp(fontenctbl[i].encname, upenc) != 0) {
                        pmesg(85, "failed]\n");
			continue;
		}
    	    	pmesg(85, "OK]\n");

                pmesg(85, "[matching font family %s to %s... ",
                      fontenctbl[i].familypat, upfamily);
		if(!patmatch(fontenctbl[i].familypat, upfamily)) {
                        pmesg(85, "failed]\n");
			continue;
		}
    	    	pmesg(85, "OK]\n");

                rv = i;
		goto exit;
        }
        
        warning(
	    	"unknown font encoding `%s' for family `%s',"
		" reverting to `%s'\n",
                encname,
		family,
		fontenctbl[rv].encname
	);

 exit:
        free(upenc);
        free(upfamily);
        pmesg(70, "[returning fontenc %i]\n", rv);
        return rv;
}

sint32 fnt_convert(int fontenc, sint32 sglyph)
{
        const int def = 0;
        uint32 glyph = sglyph;

        if (fontenc < 0 || (size_t) fontenc > TBLLEN(fontenctbl)) {
                warning("invalid fontenc `%i', reverting to `%i'\n",
                        fontenc, def);
                fontenc = def;
        }
        if (glyph >= fontenctbl[fontenc].len_tbl) {
                warning("glyph out of range\n");
                glyph = 0;
        }
        pmesg(99, "[Converting glyph %li to fontenc %i]\n", glyph, fontenc);
        return fontenctbl[fontenc].tbl[glyph];
}
