/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "outenc.h"
#include "util.h"
#include "sparse.h"
#include "vlist.h"
#include "glyphenm.h"
#include "glyphops.h"
#include "page.h"   /* for definition of glyph_t */


/* these are declared extern in the header */
enum outenc_num_t outenc_num = OE_ASCII;
int outenc_show_unicode_number = 0;
static char const * const outenc_name_tbl[OE_TOOBIG] = {
        "UTF-8",
        "US-ASCII",
        "ISO-8859-1",
	"ISO-8859-15"
};
char const * const * const outenc_names = outenc_name_tbl;


/***********************************************************************
 * private methods + data
 ***********************************************************************/

/* How many columns would that glyph use in printout. */
static int outenc_glyph_width(glyph_t g);

/* Same for compositions, i.e. a glyph followed by one or more combining
 * diacritics.
 */
static int (* outenc_composition_width)(const glyph_t c[], int length) = NULL;

/* In some cases, a glyph can't be directly mapped to the output encoding,
 * but we know another glyph (or sequence of glyphs) that can and gives a
 * good visual approximation.
 * If such a translation is known to be neccessary for the given glyph g,
 * perform it and append the result to the existing linebuf l. If not
 * (usual case), just append the glyph.
 */
static void outenc_xlat_glyph(glyph_t g, linebuf_t * l);

/* Ditto for compositions. */
static void (* outenc_xlat_composition)(
    const glyph_t c[],
    int length,
    linebuf_t * l
) = NULL;

/* take a glyph (translations already performed), convert
 * it into a sequence of bytes appropriate for the output encoding
 * (like utf-8, ucs-2, ebcdic, koi-8r, ...) and write that to f.
 */
static void (* outenc_write_raw_glyph)(FILE * f, glyph_t g) = NULL;

#define WIDTH_AUTO (-1)
typedef struct glyphtweak_t glyphtweak_t;
struct glyphtweak_t {
    glyph_t glyph;
    	/* the one to "tweak" */
    const glyph_t * xlat;
    	/* null-terminated array of glyph_t, the translation.
    	 * Use NULL if you want to override the output width
	 * but no translation should be done.
	 * (maybe we want to support e.g. double-width glyphs
	 * some time).
	 * Note the translation is still Unicode! Mapping from
	 * Unicode to the output encoding should be done in
	 * outenc_write_raw_glyph().
	 * This two-level system is required because sometimes
	 * output width != number of bytes for the output
	 * encoding (nearly always for utf-8).
	 */
    int width;
    	/* The output width. Use WIDTH_AUTO to get it filled
    	 * in automatically with the length of the translation
	 * string or specify an explicit override.
	 */
};
/* FIXME: at the moment we don't support iterated translations (e.g.
 * minus -> hyphen -> dash). We could as long as we care for the
 * right order of initialization (i.e the translation tables), but it adds
 * complexity and overhead. Check if there is any real need.
 */

/* Insert the table tweaks[] of glyph tweaks into the glyph tweak sparp. */
static void outenc_register_glyphtweaks(const glyphtweak_t tweaks[], int length);

/* Remove registered tweak for glyph g. */
static void outenc_unregister_glyphtweak(glyph_t g);

/* Write something ('?' or the Unicode number) for unencodable glyphs */
static void outenc_show_unknown(FILE * f, glyph_t g);


/***********************************************************************
 * "Generic" stuff (could - but need not - be used by any output encoding).
 * In OO speak, this would belong to the base class from which specific
 * output encoding classes are then derived.
 ***********************************************************************/

static const glyph_t genx_CATDVI_DoubleS[] = {GLYPH_S, GLYPH_S, 0};
static const glyph_t genx_CATDVI_Ng[] = {GLYPH_N, GLYPH_G, 0};
static const glyph_t genx_CATDVI_ng[] = {GLYPH_n, GLYPH_g, 0};
static const glyph_t genx_CATDVI_negationslash[] = {GLYPH_slash, 0};
static const glyph_t genx_CATDVI_vector[] = {GLYPH_arrowright, 0};
static const glyph_t genx_ADOBE_zerooldstyle[] = {GLYPH_zero, 0};
static const glyph_t genx_ADOBE_oneoldstyle[] = {GLYPH_one, 0};
static const glyph_t genx_ADOBE_twooldstyle[] = {GLYPH_two, 0};
static const glyph_t genx_ADOBE_threeoldstyle[] = {GLYPH_three, 0};
static const glyph_t genx_ADOBE_fouroldstyle[] = {GLYPH_four, 0};
static const glyph_t genx_ADOBE_fiveoldstyle[] = {GLYPH_five, 0};
static const glyph_t genx_ADOBE_sixoldstyle[] = {GLYPH_six, 0};
static const glyph_t genx_ADOBE_sevenoldstyle[] = {GLYPH_seven, 0};
static const glyph_t genx_ADOBE_eightoldstyle[] = {GLYPH_eight, 0};
static const glyph_t genx_ADOBE_nineoldstyle[] = {GLYPH_nine, 0};
static const glyph_t genx_ADOBE_dotlessj[] = {GLYPH_j, 0};

/* for the TEX math extension stuff */
static const glyph_t genx_2parenleft[] = {GLYPH_parenleft, 0};
static const glyph_t genx_2parenright[] = {GLYPH_parenright, 0};
static const glyph_t genx_2bracketleft[] = {GLYPH_bracketleft, 0};
static const glyph_t genx_2bracketright[] = {GLYPH_bracketright, 0};
static const glyph_t genx_2braceleft[] = {GLYPH_braceleft, 0};
static const glyph_t genx_2braceright[] = {GLYPH_braceright, 0};
static const glyph_t genx_2angleleftmath[] = {GLYPH_UNI_angleleftmath, 0};
static const glyph_t genx_2anglerightmath[] = {GLYPH_UNI_anglerightmath, 0};
static const glyph_t genx_2UNI_floorleft[] = {GLYPH_UNI_floorleft, 0};
static const glyph_t genx_2UNI_floorright[] = {GLYPH_UNI_floorright, 0};
static const glyph_t genx_2UNI_ceilingleft[] = {GLYPH_UNI_ceilingleft, 0};
static const glyph_t genx_2UNI_ceilingright[] = {GLYPH_UNI_ceilingright, 0};
static const glyph_t genx_2slash[] = {GLYPH_slash, 0};
static const glyph_t genx_2backslash[] = {GLYPH_backslash, 0};

static const glyph_t genx_2integral[] = {GLYPH_integral, 0};
static const glyph_t genx_2UNI_contintegral[] = {GLYPH_UNI_contintegral, 0};
static const glyph_t genx_2UNI_unionsq[] = {GLYPH_UNI_unionsq, 0};
static const glyph_t genx_2UNI_circledot[] = {GLYPH_UNI_circledot, 0};
static const glyph_t genx_2circleplus[] = {GLYPH_circleplus, 0};
static const glyph_t genx_2circlemultiply[] = {GLYPH_circlemultiply, 0};
static const glyph_t genx_2summation[] = {GLYPH_summation, 0};
static const glyph_t genx_2product[] = {GLYPH_product, 0};
static const glyph_t genx_2UNI_coproduct[] = {GLYPH_UNI_coproduct, 0};
static const glyph_t genx_2union[] = {GLYPH_union, 0};
static const glyph_t genx_2intersection[] = {GLYPH_intersection, 0};
static const glyph_t genx_2UNI_unionmulti[] = {GLYPH_UNI_unionmulti, 0};
static const glyph_t genx_2logicaland[] = {GLYPH_logicaland, 0};
static const glyph_t genx_2logicalor[] = {GLYPH_logicalor, 0};
static const glyph_t genx_2radical[] = {GLYPH_radical, 0};

static const glyph_t genx_2asciicircum[] = {GLYPH_asciicircum, 0};
static const glyph_t genx_2asciitilde[] = {GLYPH_asciitilde, 0};

static const glyphtweak_t generic_glyphtweaks[] = {
    {GLYPH_CATDVI_DoubleS,  	genx_CATDVI_DoubleS,   	    WIDTH_AUTO},
    {GLYPH_CATDVI_Ng,	    	genx_CATDVI_Ng,	    	    WIDTH_AUTO},
    {GLYPH_CATDVI_ng,	    	genx_CATDVI_ng,	    	    WIDTH_AUTO},
    {GLYPH_CATDVI_negationslash, genx_CATDVI_negationslash, WIDTH_AUTO},
    {GLYPH_CATDVI_vector,    	genx_CATDVI_vector, 	    WIDTH_AUTO},
    {GLYPH_ADOBE_zerooldstyle,	genx_ADOBE_zerooldstyle,    WIDTH_AUTO},
    {GLYPH_ADOBE_oneoldstyle,	genx_ADOBE_oneoldstyle,     WIDTH_AUTO},
    {GLYPH_ADOBE_twooldstyle,	genx_ADOBE_twooldstyle,     WIDTH_AUTO},
    {GLYPH_ADOBE_threeoldstyle, genx_ADOBE_threeoldstyle,   WIDTH_AUTO},
    {GLYPH_ADOBE_fouroldstyle,	genx_ADOBE_fouroldstyle,    WIDTH_AUTO},
    {GLYPH_ADOBE_fiveoldstyle,	genx_ADOBE_fiveoldstyle,    WIDTH_AUTO},
    {GLYPH_ADOBE_sixoldstyle,	genx_ADOBE_sixoldstyle,     WIDTH_AUTO},
    {GLYPH_ADOBE_sevenoldstyle, genx_ADOBE_sevenoldstyle,   WIDTH_AUTO},
    {GLYPH_ADOBE_eightoldstyle, genx_ADOBE_eightoldstyle,   WIDTH_AUTO},
    {GLYPH_ADOBE_nineoldstyle,	genx_ADOBE_nineoldstyle,    WIDTH_AUTO},
    {GLYPH_ADOBE_dotlessj,	genx_ADOBE_dotlessj,        WIDTH_AUTO},

    /* TeX Math Extension */
    {GLYPH_CATDVI_parenleftbig,	    genx_2parenleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenleftBig,	    genx_2parenleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenleftbigg,    genx_2parenleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenleftBigg,    genx_2parenleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenrightbig,    genx_2parenright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenrightBig,    genx_2parenright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenrightbigg,   genx_2parenright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_parenrightBigg,   genx_2parenright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketleftbig,   genx_2bracketleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketleftBig,   genx_2bracketleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketleftbigg,  genx_2bracketleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketleftBigg,  genx_2bracketleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketrightbig,  genx_2bracketright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketrightBig,  genx_2bracketright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketrightbigg, genx_2bracketright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracketrightBigg, genx_2bracketright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_braceleftbig,     genx_2braceleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_braceleftBig,     genx_2braceleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_braceleftbigg,    genx_2braceleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_braceleftBigg,    genx_2braceleft,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracerightbig,    genx_2braceright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracerightBig,    genx_2braceright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracerightbigg,   genx_2braceright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_bracerightBigg,   genx_2braceright,	    WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketleftbig, genx_2angleleftmath,   WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketleftBig, genx_2angleleftmath,   WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketleftbigg, genx_2angleleftmath,  WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketleftBigg, genx_2angleleftmath,  WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketrightbig, genx_2anglerightmath, WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketrightBig, genx_2anglerightmath, WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketrightbigg, genx_2anglerightmath,	WIDTH_AUTO},
    {GLYPH_CATDVI_angbracketrightBigg, genx_2anglerightmath,    WIDTH_AUTO},
    {GLYPH_CATDVI_floorleftbig,     genx_2UNI_floorleft,    WIDTH_AUTO},
    {GLYPH_CATDVI_floorleftBig,     genx_2UNI_floorleft,    WIDTH_AUTO},
    {GLYPH_CATDVI_floorleftbigg,    genx_2UNI_floorleft,    WIDTH_AUTO},
    {GLYPH_CATDVI_floorleftBigg,    genx_2UNI_floorleft,    WIDTH_AUTO},
    {GLYPH_CATDVI_floorrightbig,    genx_2UNI_floorright,   WIDTH_AUTO},
    {GLYPH_CATDVI_floorrightBig,    genx_2UNI_floorright,   WIDTH_AUTO},
    {GLYPH_CATDVI_floorrightbigg,   genx_2UNI_floorright,   WIDTH_AUTO},
    {GLYPH_CATDVI_floorrightBigg,   genx_2UNI_floorright,   WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingleftbig,   genx_2UNI_ceilingleft,  WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingleftBig,   genx_2UNI_ceilingleft,  WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingleftbigg,  genx_2UNI_ceilingleft,  WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingleftBigg,  genx_2UNI_ceilingleft,  WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingrightbig,  genx_2UNI_ceilingright, WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingrightBig,  genx_2UNI_ceilingright, WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingrightbigg, genx_2UNI_ceilingright, WIDTH_AUTO},
    {GLYPH_CATDVI_ceilingrightBigg, genx_2UNI_ceilingright, WIDTH_AUTO},
    {GLYPH_CATDVI_slashbig, 	    genx_2slash,    	    WIDTH_AUTO},
    {GLYPH_CATDVI_slashBig, 	    genx_2slash,    	    WIDTH_AUTO},
    {GLYPH_CATDVI_slashbigg,	    genx_2slash,    	    WIDTH_AUTO},
    {GLYPH_CATDVI_slashBigg,	    genx_2slash,    	    WIDTH_AUTO},
    {GLYPH_CATDVI_backslashbig,     genx_2backslash,	    WIDTH_AUTO},
    {GLYPH_CATDVI_backslashBig,     genx_2backslash,	    WIDTH_AUTO},
    {GLYPH_CATDVI_backslashbigg,    genx_2backslash,	    WIDTH_AUTO},
    {GLYPH_CATDVI_backslashBigg,    genx_2backslash,	    WIDTH_AUTO},
    {GLYPH_CATDVI_radicalbig,	    genx_2radical,	    WIDTH_AUTO},
    {GLYPH_CATDVI_radicalBig,	    genx_2radical,	    WIDTH_AUTO},
    {GLYPH_CATDVI_radicalbigg,	    genx_2radical,	    WIDTH_AUTO},
    {GLYPH_CATDVI_radicalBigg,	    genx_2radical,	    WIDTH_AUTO},

    {GLYPH_CATDVI_integraltext,	    	genx_2integral,     	WIDTH_AUTO},
    {GLYPH_CATDVI_integraldisplay,	genx_2integral,     	WIDTH_AUTO},
    {GLYPH_CATDVI_contintegraltext,	genx_2UNI_contintegral,	WIDTH_AUTO},
    {GLYPH_CATDVI_contintegraldisplay,	genx_2UNI_contintegral,	WIDTH_AUTO},
    {GLYPH_CATDVI_unionsqtext,	    	genx_2UNI_unionsq,  	WIDTH_AUTO},
    {GLYPH_CATDVI_unionsqdisplay,	genx_2UNI_unionsq,	WIDTH_AUTO},
    {GLYPH_CATDVI_circledottext,	genx_2UNI_circledot,	WIDTH_AUTO},
    {GLYPH_CATDVI_circledotdisplay,	genx_2UNI_circledot,	WIDTH_AUTO},
    {GLYPH_CATDVI_circleplustext,	genx_2circleplus,	WIDTH_AUTO},
    {GLYPH_CATDVI_circleplusdisplay,	genx_2circleplus,	WIDTH_AUTO},
    {GLYPH_CATDVI_circlemultiplytext,	genx_2circlemultiply,	WIDTH_AUTO},
    {GLYPH_CATDVI_circlemultiplydisplay, genx_2circlemultiply,	WIDTH_AUTO},
    {GLYPH_CATDVI_summationtext,	genx_2summation,	WIDTH_AUTO},
    {GLYPH_CATDVI_summationdisplay,	genx_2summation,	WIDTH_AUTO},
    {GLYPH_CATDVI_producttext,	    	genx_2product,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_productdisplay,	genx_2product,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_uniontext,	    	genx_2union,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_uniondisplay,	    	genx_2union,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_intersectiontext,	genx_2intersection,	WIDTH_AUTO},
    {GLYPH_CATDVI_intersectiondisplay,	genx_2intersection,	WIDTH_AUTO},
    {GLYPH_CATDVI_unionmultitext,	genx_2UNI_unionmulti,	WIDTH_AUTO},
    {GLYPH_CATDVI_unionmultidisplay,	genx_2UNI_unionmulti,	WIDTH_AUTO},
    {GLYPH_CATDVI_logicalandtext,	genx_2logicaland,	WIDTH_AUTO},
    {GLYPH_CATDVI_logicalanddisplay,	genx_2logicaland,	WIDTH_AUTO},
    {GLYPH_CATDVI_logicalortext,	genx_2logicalor,	WIDTH_AUTO},
    {GLYPH_CATDVI_logicalordisplay,	genx_2logicalor,	WIDTH_AUTO},
    {GLYPH_CATDVI_producttext,	    	genx_2product,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_productdisplay,	genx_2product,	    	WIDTH_AUTO},
    {GLYPH_CATDVI_coproducttext,	genx_2UNI_coproduct,	WIDTH_AUTO},
    {GLYPH_CATDVI_coproductdisplay,	genx_2UNI_coproduct,	WIDTH_AUTO},

    {GLYPH_CATDVI_hatwide,	genx_2asciicircum,	    WIDTH_AUTO},
    {GLYPH_CATDVI_hatwider,	genx_2asciicircum,	    WIDTH_AUTO},
    {GLYPH_CATDVI_hatwidest,	genx_2asciicircum,	    WIDTH_AUTO},
    {GLYPH_CATDVI_tildewide,	genx_2asciitilde,	    WIDTH_AUTO},
    {GLYPH_CATDVI_tildewider,	genx_2asciitilde,	    WIDTH_AUTO},
    {GLYPH_CATDVI_tildewidest,	genx_2asciitilde,	    WIDTH_AUTO},
    /* End TeX Math Extension */

    {GLYPH_CATDVI_hatxwide,	genx_2asciicircum,	    WIDTH_AUTO},
    {GLYPH_CATDVI_tildexwide,	genx_2asciitilde,	    WIDTH_AUTO}
};

/* How to combine a base glyph and a single diacritic */
struct generic_accententry_t {
    glyph_t base;
    glyph_t combining_diacritic;
    glyph_t accented;
};

/* If the combining diacritic lives in the private use area, we want
 * to do away with in any imaginable output encoding.
 * This happens when TeX builds glyphs by overlaying strange pieces
 * and we emulate this with strange combining diacritics.
 */
static struct generic_accententry_t generic_accentings[] = {
    {GLYPH_C, GLYPH_CATDVI_Eurodblstrokecomb, GLYPH_Euro},
    {GLYPH_L, GLYPH_CATDVI_polishstrokecomb, GLYPH_Lslash},
    {GLYPH_l, GLYPH_CATDVI_polishstrokecomb, GLYPH_lslash},
    {0, 0, 0}
};

static struct sparp_t accenting_sparp;
static struct vlist_t accenting_vlist;

/* Insert the table pa of accenting entries into the accenting sparp.
 * The table need not be sorted by base glyph. End of table = {0, 0, 0}.
 * In the case of conflicting entries, the one registered last wins.
 */
static void generic_register_accentings(const struct generic_accententry_t pa[])
{
    /* We keep a single list of all registered accentings. Accentings
     * with the same base glyph are consecutive, the latest registered
     * first.
     *
     * The accentings sparp is indexed by the base glyph we want to accent
     * and the value is a vitor to the first known accenting of the base glyph.
     */
    for( ; pa->base != 0; ++pa) {
    	vitor_t old_va, new_va;

        old_va = sparp_read(&accenting_sparp, pa->base);
	if(old_va == NULL) old_va = vlist_begin(&accenting_vlist);

	new_va = vlist_insert_before(&accenting_vlist, old_va, pa);
	sparp_write(&accenting_sparp, pa->base, new_va);
    }
    
}

static glyph_t generic_accent(glyph_t base, glyph_t diacritic)
{
    vitor_t va;

    va = sparp_read(&accenting_sparp, base);
    if(va == NULL) return 0;

    while(va != vlist_end(&accenting_vlist)) {
    	struct generic_accententry_t * p;

	p = vitor2ptr(va, struct generic_accententry_t);
	if(p->base != base) break;
    	if(p->combining_diacritic == diacritic) return(p->accented);

	va = va->next;
    }

    return 0;
}

static int generic_composition_width(const glyph_t c[], int length)
{
    glyph_t acc, sd;
    int i;
    int w;
    
    assert(length >= 2);

    if(length == 2) {
    	/* Only a single diacritic. Try to accent. */
    	acc = generic_accent(c[0], c[1]);
	if(acc != 0) return outenc_glyph_width(acc);
    }
    
    /* Accenting failed, or more than one diacritic. Strategy: the best
     * we can do is print the base glyph and the spacing variant of all
     * diacritics. */
    w = outenc_glyph_width(c[0]);
    for(i = 1; i < length; ++i) {
    	sd = diacritic_spacing_variant(c[i]);
	if(sd == 0) sd = c[i];
	    /* Not even a spacing variant known. */
	w += outenc_glyph_width(sd);
    };

    return w;
}

static void generic_xlat_composition(
    const glyph_t c[],
    int length,
    linebuf_t * l
)
{
    glyph_t acc, sd;
    int i;
    
    assert(length >= 2);

    if(length == 2) {
    	/* Only a single diacritic. Try to accent. */
    	acc = generic_accent(c[0], c[1]);
	if(acc != 0) {
	    outenc_xlat_glyph(acc, l);
	    return;
	}
    }
    
    /* Accenting failed, or more than one diacritic. Strategy: the best
     * we can do is print the base glyph and the spacing variant of all
     * diacritics (but in customary TeX order, i.e. reverse to the
     * Unicode convention).
     */
    for(i = length - 1; i > 0; --i) {
    	sd = diacritic_spacing_variant(c[i]);
	if(sd == 0) sd = c[i];
	    /* Not even a spacing variant known. */
	outenc_xlat_glyph(sd, l);
    };
    outenc_xlat_glyph(c[0], l);

}


/***********************************************************************
 * Routines for "simple" output encodings, i.e. those where each
 * character fits in one char and the character stream is stateless.
 ***********************************************************************/
struct simple_charmap_entry_t {
    glyph_t 	    unicode;
    unsigned char   output;
};

static spars32_t simple_charmap_spars32;
static unsigned char simple_identity_range;

/* implements outenc_write_raw_glyph for simple output encodings */
static void simple_write_raw_glyph(FILE * f, glyph_t g)
{
    if(g <= simple_identity_range) putc((int) g, f);
    else {
    	int c;
	c = spars32_read(&simple_charmap_spars32, g);

	if (c > 0) putc(c, f);
	else outenc_show_unknown(f, g);
    }
}

/* Has to be called exactly once before use. Defines the output encoding:
 *
 * glyph <= identity_range -> write as is
 * glyph has charmap entry -> write entry.output
 * otherwise               -> outenc_show_unknown()
 *
 * charmap_tbl == NULL is OK if also tbl_length == 0
 */
static void simple_init_charmap(
    unsigned char identity_range,
    struct simple_charmap_entry_t charmap_tbl[],
    size_t tbl_length
)
{
    size_t i;

    simple_identity_range = identity_range;
    spars32_init(&simple_charmap_spars32, 0);
    for(i = 0; i < tbl_length; ++i) {
    	spars32_write(
	    &simple_charmap_spars32,
	    charmap_tbl[i].unicode,
	    charmap_tbl[i].output
	);
    }

}


/***********************************************************************
 * utf-8
 ***********************************************************************/

/* A few characters in AMSSymbolsB are not in Unicode, but can be represented
 * in Unicode by use of combining characters.
 * FIXME: these translations should really be generic, but our substitution
 * mechanism is not up to it.
 */
static const glyph_t utf8x_2not_lessequalslanted[] = {
    GLYPH_UNI_lessequalslanted, GLYPH_UNI_slashlongcomb, 0
};
static const glyph_t utf8x_2not_greaterequalslanted[] = {
    GLYPH_UNI_greaterequalslanted, GLYPH_UNI_slashlongcomb, 0
};
static const glyph_t utf8x_2not_lessequal2[] = {
    GLYPH_UNI_lessequal2, GLYPH_UNI_slashlongcomb, 0
};
static const glyph_t utf8x_2not_greaterequal2[] = {
    GLYPH_UNI_greaterequal2, GLYPH_UNI_slashlongcomb, 0
};
static const glyph_t utf8x_2not_subsetequal2[] = {
    GLYPH_UNI_subsetequal2, GLYPH_UNI_slashlongcomb, 0
};
static const glyph_t utf8x_2not_supersetequal2[] = {
    GLYPH_UNI_supersetequal2, GLYPH_UNI_slashlongcomb, 0
};

static const glyphtweak_t utf8_glyphtweaks[] = {
    {GLYPH_CATDVI_notlessequalslanted, utf8x_2not_lessequalslanted, 1},
    {GLYPH_CATDVI_notgreaterequalslanted, utf8x_2not_greaterequalslanted, 1},
    {GLYPH_CATDVI_notlessequal2, utf8x_2not_lessequal2, 1},
    {GLYPH_CATDVI_notgreaterequal2, utf8x_2not_greaterequal2, 1},
    {GLYPH_CATDVI_notsubsetequal2, utf8x_2not_subsetequal2, 1},
    {GLYPH_CATDVI_notsupersetequal2, utf8x_2not_supersetequal2, 1}
};


/* UCS-4 to UTF-8 translation control tables. */

#define UTFVEC_MAXLEN (6+1) /* UTF-8 takes max 6 bytes + null terminator */

struct ucs2utf_elem {
        int numbits; /* number of variable bits, 1-8 */
        unsigned char octet; /* template for this octet */
};

struct ucs2utf_ctbl {
        uint32 maxval;  /* maximum UC-4 value encodable */
        int len;        /* length of the UTF-8 bit stream in octets */
        struct ucs2utf_elem template[UTFVEC_MAXLEN]; /* the vector containing
                                                        info on each octet */
};

static struct ucs2utf_ctbl utfcnv_ctbl[] = {
        /* maximum, num entries, template vector */
        /*                  num bits, initial value */
        { 0x7f,       1, { { 7, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 } } },
        { 0x7ff,      2, { { 5, 0xc0 },
                           { 6, 0x80 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 }, } },
        { 0xffff,     3, { { 4, 0xe0 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 } } },
        { 0x1fffff,   4, { { 3, 0xf0 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 0, 0 },
                           { 0, 0 },
                           { 0, 0 } } },
        { 0x3ffffff,  5, { { 2, 0xf4 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 0, 0 },
                           { 0, 0 } } },
        { 0x7fffffff, 6, { { 1, 0xfc },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 6, 0x80 },
                           { 0, 0 } } }
};

static void utf8_write_raw_glyph(FILE * f, glyph_t g)
{
    size_t tblinx;  /* index to conversion control table */
    unsigned char outv[UTFVEC_MAXLEN];
    int i;

    /* Unicode -> UTF-8, see RFC 2279 */

    /* 1. Determine the row in the translation table. */
    for (tblinx = 0; tblinx < lengthof(utfcnv_ctbl); tblinx++) {
    	if (g <= utfcnv_ctbl[tblinx].maxval) {
            break;
        }
    }
    if (tblinx >= lengthof(utfcnv_ctbl)) {
            pmesg(60, "character code too big");
            return;
    }

    /* 2. Do the conversion from right to left. */
    for (i = UTFVEC_MAXLEN - 1; i >= 0; i--) {
        /* At each iteration we first apply the initial bit
           pattern and then patch the rest with the indicated
           number of lower-order bits in g.  When we are
           done, we shift these out.  Zero-bit codes are
           exceptional, we just leave the initial bit pattern
           and move on.*/

        int numbits = utfcnv_ctbl[tblinx].template[i].numbits;

        outv[i] = utfcnv_ctbl[tblinx].template[i].octet;

        if (numbits == 0) continue;

        outv[i] |= ((1 << numbits) - 1) & g;

        g = g >> numbits;
    }

    /* Output it. */
    fputs((char *) outv, f);
}

static int utf8_composition_width(const glyph_t c[], int length)
{
    glyph_t a;


    assert(length >= 2);

    if(length == 2 && (a = generic_accent(c[0], c[1])) != 0) {
    	return outenc_glyph_width(a);
    }
    else {
    	return outenc_glyph_width(c[0]);
    }
}

static void utf8_xlat_composition(
    const glyph_t c[],
    int length,
    linebuf_t * l
)
{
    int i;
    glyph_t a;
    
    if(length == 2 && (a = generic_accent(c[0], c[1])) != 0) {
    	linebuf_putg(l, a);
    }
    else {
    	for(i = 0; i < length; ++i) linebuf_putg(l, c[i]);
    }
}

static void utf8_init(void)
{
    outenc_register_glyphtweaks(
    	utf8_glyphtweaks,
	lengthof(utf8_glyphtweaks)
    );
    outenc_write_raw_glyph = utf8_write_raw_glyph;
    outenc_composition_width = utf8_composition_width;
    outenc_xlat_composition = utf8_xlat_composition;
}


/***********************************************************************
 * ascii
 ***********************************************************************/

static const glyph_t a2l1x_IJ[] = {GLYPH_I, GLYPH_J, 0};
static const glyph_t a2l1x_ij[] = {GLYPH_i, GLYPH_j, 0};
static const glyph_t a2l1x_OE[] = {GLYPH_O, GLYPH_E, 0};
static const glyph_t a2l1x_oe[] = {GLYPH_o, GLYPH_e, 0};
static const glyph_t a2l1x_Eng[] = {GLYPH_N, GLYPH_g, 0};
static const glyph_t a2l1x_eng[] = {GLYPH_n, GLYPH_g, 0};
static const glyph_t a2l1x_endash[] = {GLYPH_UNI_hyphen, GLYPH_UNI_hyphen, 0};
static const glyph_t a2l1x_emdash[] = {GLYPH_UNI_hyphen, GLYPH_UNI_hyphen, GLYPH_UNI_hyphen, 0};
static const glyph_t a2l1x_quoteleft[] = {GLYPH_quotesingle, 0};
static const glyph_t a2l1x_quoteright[] = {GLYPH_quotesingle, 0};
static const glyph_t a2l1x_quotereversed[] = {GLYPH_quotesingle, 0};
static const glyph_t a2l1x_quotedblleft[] = {GLYPH_quotedbl, 0};
static const glyph_t a2l1x_quotedblright[] = {GLYPH_quotedbl, 0};
static const glyph_t a2l1x_UNI_quotedblreversed[] = {GLYPH_quotedbl, 0};
static const glyph_t a2l1x_ff[] = {GLYPH_f, GLYPH_f, 0};
static const glyph_t a2l1x_fi[] = {GLYPH_f, GLYPH_i, 0};
static const glyph_t a2l1x_fl[] = {GLYPH_f, GLYPH_l, 0};
static const glyph_t a2l1x_ffi[] = {GLYPH_f, GLYPH_f, GLYPH_i, 0};
static const glyph_t a2l1x_ffl[] = {GLYPH_f, GLYPH_f, GLYPH_l, 0};
static const glyph_t a2l1x_UNI_st[] = {GLYPH_s, GLYPH_t, 0};
static const glyph_t a2l1x_UNI_longst[] = {GLYPH_s, GLYPH_t, 0};
static const glyph_t a2l1x_minus[] = {GLYPH_UNI_hyphen, 0};
static const glyph_t a2l1x_asteriskmath[] = {GLYPH_asterisk, 0};
static const glyph_t a2l1x_2emptystring[] = {0};
static const glyph_t a2l1x_2asciitilde[] = {GLYPH_asciitilde, 0};
static const glyph_t a2l1x_lessequal[] = {GLYPH_equal, GLYPH_less, 0};
static const glyph_t a2l1x_greaterequal[] = {GLYPH_greater, GLYPH_equal, 0};

static const glyph_t a2l1x_2A[] = {GLYPH_A, 0};
static const glyph_t a2l1x_2B[] = {GLYPH_B, 0};
static const glyph_t a2l1x_2C[] = {GLYPH_C, 0};
static const glyph_t a2l1x_2D[] = {GLYPH_D, 0};
static const glyph_t a2l1x_2E[] = {GLYPH_E, 0};
static const glyph_t a2l1x_2F[] = {GLYPH_F, 0};
static const glyph_t a2l1x_2G[] = {GLYPH_G, 0};
static const glyph_t a2l1x_2H[] = {GLYPH_H, 0};
static const glyph_t a2l1x_2I[] = {GLYPH_I, 0};
static const glyph_t a2l1x_2J[] = {GLYPH_J, 0};
static const glyph_t a2l1x_2K[] = {GLYPH_K, 0};
static const glyph_t a2l1x_2L[] = {GLYPH_L, 0};
static const glyph_t a2l1x_2M[] = {GLYPH_M, 0};
static const glyph_t a2l1x_2N[] = {GLYPH_N, 0};
static const glyph_t a2l1x_2O[] = {GLYPH_O, 0};
static const glyph_t a2l1x_2P[] = {GLYPH_P, 0};
static const glyph_t a2l1x_2Q[] = {GLYPH_Q, 0};
static const glyph_t a2l1x_2R[] = {GLYPH_R, 0};
static const glyph_t a2l1x_2S[] = {GLYPH_S, 0};
static const glyph_t a2l1x_2T[] = {GLYPH_T, 0};
static const glyph_t a2l1x_2U[] = {GLYPH_U, 0};
static const glyph_t a2l1x_2V[] = {GLYPH_V, 0};
static const glyph_t a2l1x_2W[] = {GLYPH_W, 0};
static const glyph_t a2l1x_2X[] = {GLYPH_X, 0};
static const glyph_t a2l1x_2Y[] = {GLYPH_Y, 0};
static const glyph_t a2l1x_2Z[] = {GLYPH_Z, 0};

static const glyph_t a2l1x_2a[] = {GLYPH_a, 0};
static const glyph_t a2l1x_2b[] = {GLYPH_b, 0};
static const glyph_t a2l1x_2c[] = {GLYPH_c, 0};
static const glyph_t a2l1x_2d[] = {GLYPH_d, 0};
static const glyph_t a2l1x_2e[] = {GLYPH_e, 0};
static const glyph_t a2l1x_2f[] = {GLYPH_f, 0};
static const glyph_t a2l1x_2g[] = {GLYPH_g, 0};
static const glyph_t a2l1x_2h[] = {GLYPH_h, 0};
static const glyph_t a2l1x_2i[] = {GLYPH_i, 0};
static const glyph_t a2l1x_2j[] = {GLYPH_j, 0};
static const glyph_t a2l1x_2k[] = {GLYPH_k, 0};
static const glyph_t a2l1x_2l[] = {GLYPH_l, 0};
static const glyph_t a2l1x_2m[] = {GLYPH_m, 0};
static const glyph_t a2l1x_2n[] = {GLYPH_n, 0};
static const glyph_t a2l1x_2o[] = {GLYPH_o, 0};
static const glyph_t a2l1x_2p[] = {GLYPH_p, 0};
static const glyph_t a2l1x_2q[] = {GLYPH_q, 0};
static const glyph_t a2l1x_2r[] = {GLYPH_r, 0};
static const glyph_t a2l1x_2s[] = {GLYPH_s, 0};
static const glyph_t a2l1x_2t[] = {GLYPH_t, 0};
static const glyph_t a2l1x_2u[] = {GLYPH_u, 0};
static const glyph_t a2l1x_2v[] = {GLYPH_v, 0};
static const glyph_t a2l1x_2w[] = {GLYPH_w, 0};
static const glyph_t a2l1x_2x[] = {GLYPH_x, 0};
static const glyph_t a2l1x_2y[] = {GLYPH_y, 0};
static const glyph_t a2l1x_2z[] = {GLYPH_z, 0};

static const glyph_t a2l1x_2zero[] = {GLYPH_zero, 0};
static const glyph_t a2l1x_2one[] = {GLYPH_one, 0};
static const glyph_t a2l1x_2two[] = {GLYPH_two, 0};
static const glyph_t a2l1x_2three[] = {GLYPH_three, 0};
static const glyph_t a2l1x_2four[] = {GLYPH_four, 0};
static const glyph_t a2l1x_2five[] = {GLYPH_five, 0};
static const glyph_t a2l1x_2six[] = {GLYPH_six, 0};
static const glyph_t a2l1x_2seven[] = {GLYPH_seven, 0};
static const glyph_t a2l1x_2eight[] = {GLYPH_eight, 0};
static const glyph_t a2l1x_2nine[] = {GLYPH_nine, 0};

/* These are shared between ascii and latin1. Latin9 uses them as well,
 * but discards the GLYPH_oe and GLYPH_OE tweaks. */
static const glyphtweak_t a2l1_glyphtweaks[] = {
    { GLYPH_IJ, a2l1x_IJ, WIDTH_AUTO },
    { GLYPH_ij, a2l1x_ij, WIDTH_AUTO },
    { GLYPH_OE, a2l1x_OE, WIDTH_AUTO },
    { GLYPH_oe, a2l1x_oe, WIDTH_AUTO },
    { GLYPH_Eng, a2l1x_Eng, WIDTH_AUTO},
    { GLYPH_eng, a2l1x_eng, WIDTH_AUTO},
    { GLYPH_endash, a2l1x_endash, WIDTH_AUTO },
    { GLYPH_emdash, a2l1x_emdash, WIDTH_AUTO },
    { GLYPH_quoteleft, a2l1x_quoteleft, WIDTH_AUTO },
    { GLYPH_quoteright, a2l1x_quoteright, WIDTH_AUTO },
    { GLYPH_quotereversed, a2l1x_quotereversed, WIDTH_AUTO },
    { GLYPH_quotedblleft, a2l1x_quotedblleft, WIDTH_AUTO },
    { GLYPH_quotedblright, a2l1x_quotedblright, WIDTH_AUTO },
    { GLYPH_UNI_quotedblreversed, a2l1x_UNI_quotedblreversed, WIDTH_AUTO },
    { GLYPH_ff, a2l1x_ff, WIDTH_AUTO },
    { GLYPH_fi, a2l1x_fi, WIDTH_AUTO },
    { GLYPH_fl, a2l1x_fl, WIDTH_AUTO },
    { GLYPH_ffi, a2l1x_ffi, WIDTH_AUTO },
    { GLYPH_ffl, a2l1x_ffl, WIDTH_AUTO },
    { GLYPH_UNI_st, a2l1x_UNI_st, WIDTH_AUTO },
    { GLYPH_UNI_longst, a2l1x_UNI_longst, WIDTH_AUTO },
    { GLYPH_minus, a2l1x_minus, WIDTH_AUTO },
    { GLYPH_asteriskmath, a2l1x_asteriskmath, WIDTH_AUTO},
    { GLYPH_UNI_ZWNJ, a2l1x_2emptystring, WIDTH_AUTO},
    { GLYPH_tilde, a2l1x_2asciitilde, WIDTH_AUTO},
    { GLYPH_similar, a2l1x_2asciitilde, WIDTH_AUTO},
    { GLYPH_lessequal, a2l1x_lessequal, WIDTH_AUTO},
    { GLYPH_greaterequal, a2l1x_greaterequal, WIDTH_AUTO},

    { GLYPH_UNI_Amathscript, a2l1x_2A, WIDTH_AUTO },
    { GLYPH_UNI_Bscript, a2l1x_2B, WIDTH_AUTO },
    { GLYPH_UNI_Cmathscript, a2l1x_2C, WIDTH_AUTO },
    { GLYPH_UNI_Dmathscript, a2l1x_2D, WIDTH_AUTO },
    { GLYPH_UNI_Escript, a2l1x_2E, WIDTH_AUTO },
    { GLYPH_UNI_Fscript, a2l1x_2F, WIDTH_AUTO },
    { GLYPH_UNI_Gmathscript, a2l1x_2G, WIDTH_AUTO },
    { GLYPH_UNI_Hscript, a2l1x_2H, WIDTH_AUTO },
    { GLYPH_UNI_Iscript, a2l1x_2I, WIDTH_AUTO },
    { GLYPH_UNI_Jmathscript, a2l1x_2J, WIDTH_AUTO },
    { GLYPH_UNI_Kmathscript, a2l1x_2K, WIDTH_AUTO },
    { GLYPH_UNI_Lscript, a2l1x_2L, WIDTH_AUTO },
    { GLYPH_UNI_Mscript, a2l1x_2M, WIDTH_AUTO },
    { GLYPH_UNI_Nmathscript, a2l1x_2N, WIDTH_AUTO },
    { GLYPH_UNI_Omathscript, a2l1x_2O, WIDTH_AUTO },
    { GLYPH_UNI_Pmathscript, a2l1x_2P, WIDTH_AUTO },
    { GLYPH_UNI_Qmathscript, a2l1x_2Q, WIDTH_AUTO },
    { GLYPH_UNI_Rscript, a2l1x_2R, WIDTH_AUTO },
    { GLYPH_UNI_Smathscript, a2l1x_2S, WIDTH_AUTO },
    { GLYPH_UNI_Tmathscript, a2l1x_2T, WIDTH_AUTO },
    { GLYPH_UNI_Umathscript, a2l1x_2U, WIDTH_AUTO },
    { GLYPH_UNI_Vmathscript, a2l1x_2V, WIDTH_AUTO },
    { GLYPH_UNI_Wmathscript, a2l1x_2W, WIDTH_AUTO },
    { GLYPH_UNI_Xmathscript, a2l1x_2X, WIDTH_AUTO },
    { GLYPH_UNI_Ymathscript, a2l1x_2Y, WIDTH_AUTO },
    { GLYPH_UNI_Zmathscript, a2l1x_2Z, WIDTH_AUTO },

    { GLYPH_UNI_lscript, a2l1x_2l, WIDTH_AUTO },

    { GLYPH_UNI_Amathbb, a2l1x_2A, WIDTH_AUTO },
    { GLYPH_UNI_Bmathbb, a2l1x_2B, WIDTH_AUTO },
    { GLYPH_UNI_Cdblstruck, a2l1x_2C, WIDTH_AUTO },
    { GLYPH_UNI_Dmathbb, a2l1x_2D, WIDTH_AUTO },
    { GLYPH_UNI_Emathbb, a2l1x_2E, WIDTH_AUTO },
    { GLYPH_UNI_Fmathbb, a2l1x_2F, WIDTH_AUTO },
    { GLYPH_UNI_Gmathbb, a2l1x_2G, WIDTH_AUTO },
    { GLYPH_UNI_Hdblstruck, a2l1x_2H, WIDTH_AUTO },
    { GLYPH_UNI_Imathbb, a2l1x_2I, WIDTH_AUTO },
    { GLYPH_UNI_Jmathbb, a2l1x_2J, WIDTH_AUTO },
    { GLYPH_UNI_Kmathbb, a2l1x_2K, WIDTH_AUTO },
    { GLYPH_UNI_Lmathbb, a2l1x_2L, WIDTH_AUTO },
    { GLYPH_UNI_Mmathbb, a2l1x_2M, WIDTH_AUTO },
    { GLYPH_UNI_Ndblstruck, a2l1x_2N, WIDTH_AUTO },
    { GLYPH_UNI_Omathbb, a2l1x_2O, WIDTH_AUTO },
    { GLYPH_UNI_Pdblstruck, a2l1x_2P, WIDTH_AUTO },
    { GLYPH_UNI_Qdblstruck, a2l1x_2Q, WIDTH_AUTO },
    { GLYPH_UNI_Rdblstruck, a2l1x_2R, WIDTH_AUTO },
    { GLYPH_UNI_Smathbb, a2l1x_2S, WIDTH_AUTO },
    { GLYPH_UNI_Tmathbb, a2l1x_2T, WIDTH_AUTO },
    { GLYPH_UNI_Umathbb, a2l1x_2U, WIDTH_AUTO },
    { GLYPH_UNI_Vmathbb, a2l1x_2V, WIDTH_AUTO },
    { GLYPH_UNI_Wmathbb, a2l1x_2W, WIDTH_AUTO },
    { GLYPH_UNI_Xmathbb, a2l1x_2X, WIDTH_AUTO },
    { GLYPH_UNI_Ymathbb, a2l1x_2Y, WIDTH_AUTO },
    { GLYPH_UNI_Zdblstruck, a2l1x_2Z, WIDTH_AUTO },

    { GLYPH_UNI_amathbb, a2l1x_2a, WIDTH_AUTO },
    { GLYPH_UNI_bmathbb, a2l1x_2b, WIDTH_AUTO },
    { GLYPH_UNI_cmathbb, a2l1x_2c, WIDTH_AUTO },
    { GLYPH_UNI_dmathbb, a2l1x_2d, WIDTH_AUTO },
    { GLYPH_UNI_emathbb, a2l1x_2e, WIDTH_AUTO },
    { GLYPH_UNI_fmathbb, a2l1x_2f, WIDTH_AUTO },
    { GLYPH_UNI_gmathbb, a2l1x_2g, WIDTH_AUTO },
    { GLYPH_UNI_hmathbb, a2l1x_2h, WIDTH_AUTO },
    { GLYPH_UNI_imathbb, a2l1x_2i, WIDTH_AUTO },
    { GLYPH_UNI_jmathbb, a2l1x_2j, WIDTH_AUTO },
    { GLYPH_UNI_kmathbb, a2l1x_2k, WIDTH_AUTO },
    { GLYPH_UNI_lmathbb, a2l1x_2l, WIDTH_AUTO },
    { GLYPH_UNI_mmathbb, a2l1x_2m, WIDTH_AUTO },
    { GLYPH_UNI_nmathbb, a2l1x_2n, WIDTH_AUTO },
    { GLYPH_UNI_omathbb, a2l1x_2o, WIDTH_AUTO },
    { GLYPH_UNI_pmathbb, a2l1x_2p, WIDTH_AUTO },
    { GLYPH_UNI_qmathbb, a2l1x_2q, WIDTH_AUTO },
    { GLYPH_UNI_rmathbb, a2l1x_2r, WIDTH_AUTO },
    { GLYPH_UNI_smathbb, a2l1x_2s, WIDTH_AUTO },
    { GLYPH_UNI_tmathbb, a2l1x_2t, WIDTH_AUTO },
    { GLYPH_UNI_umathbb, a2l1x_2u, WIDTH_AUTO },
    { GLYPH_UNI_vmathbb, a2l1x_2v, WIDTH_AUTO },
    { GLYPH_UNI_wmathbb, a2l1x_2w, WIDTH_AUTO },
    { GLYPH_UNI_xmathbb, a2l1x_2x, WIDTH_AUTO },
    { GLYPH_UNI_ymathbb, a2l1x_2y, WIDTH_AUTO },
    { GLYPH_UNI_zmathbb, a2l1x_2z, WIDTH_AUTO },

    { GLYPH_UNI_zeromathbb, a2l1x_2zero, WIDTH_AUTO },
    { GLYPH_UNI_onemathbb, a2l1x_2one, WIDTH_AUTO },
    { GLYPH_UNI_twomathbb, a2l1x_2two, WIDTH_AUTO },
    { GLYPH_UNI_threemathbb, a2l1x_2three, WIDTH_AUTO },
    { GLYPH_UNI_fourmathbb, a2l1x_2four, WIDTH_AUTO },
    { GLYPH_UNI_fivemathbb, a2l1x_2five, WIDTH_AUTO },
    { GLYPH_UNI_sixmathbb, a2l1x_2six, WIDTH_AUTO },
    { GLYPH_UNI_sevenmathbb, a2l1x_2seven, WIDTH_AUTO },
    { GLYPH_UNI_eightmathbb, a2l1x_2eight, WIDTH_AUTO },
    { GLYPH_UNI_ninemathbb, a2l1x_2nine, WIDTH_AUTO },

    { GLYPH_UNI_Amathfrak, a2l1x_2A, WIDTH_AUTO },
    { GLYPH_UNI_Bmathfrak, a2l1x_2B, WIDTH_AUTO },
    { GLYPH_UNI_Cfraktur, a2l1x_2C, WIDTH_AUTO },
    { GLYPH_UNI_Dmathfrak, a2l1x_2D, WIDTH_AUTO },
    { GLYPH_UNI_Emathfrak, a2l1x_2E, WIDTH_AUTO },
    { GLYPH_UNI_Fmathfrak, a2l1x_2F, WIDTH_AUTO },
    { GLYPH_UNI_Gmathfrak, a2l1x_2G, WIDTH_AUTO },
    { GLYPH_UNI_Hfraktur, a2l1x_2H, WIDTH_AUTO },
    { GLYPH_Ifraktur, a2l1x_2I, WIDTH_AUTO },
    { GLYPH_UNI_Jmathfrak, a2l1x_2J, WIDTH_AUTO },
    { GLYPH_UNI_Kmathfrak, a2l1x_2K, WIDTH_AUTO },
    { GLYPH_UNI_Lmathfrak, a2l1x_2L, WIDTH_AUTO },
    { GLYPH_UNI_Mmathfrak, a2l1x_2M, WIDTH_AUTO },
    { GLYPH_UNI_Nmathfrak, a2l1x_2N, WIDTH_AUTO },
    { GLYPH_UNI_Omathfrak, a2l1x_2O, WIDTH_AUTO },
    { GLYPH_UNI_Pmathfrak, a2l1x_2P, WIDTH_AUTO },
    { GLYPH_UNI_Qmathfrak, a2l1x_2Q, WIDTH_AUTO },
    { GLYPH_Rfraktur, a2l1x_2R, WIDTH_AUTO },
    { GLYPH_UNI_Smathfrak, a2l1x_2S, WIDTH_AUTO },
    { GLYPH_UNI_Tmathfrak, a2l1x_2T, WIDTH_AUTO },
    { GLYPH_UNI_Umathfrak, a2l1x_2U, WIDTH_AUTO },
    { GLYPH_UNI_Vmathfrak, a2l1x_2V, WIDTH_AUTO },
    { GLYPH_UNI_Wmathfrak, a2l1x_2W, WIDTH_AUTO },
    { GLYPH_UNI_Xmathfrak, a2l1x_2X, WIDTH_AUTO },
    { GLYPH_UNI_Ymathfrak, a2l1x_2Y, WIDTH_AUTO },
    { GLYPH_UNI_Zfraktur, a2l1x_2Z, WIDTH_AUTO },

    { GLYPH_UNI_amathfrak, a2l1x_2a, WIDTH_AUTO },
    { GLYPH_UNI_bmathfrak, a2l1x_2b, WIDTH_AUTO },
    { GLYPH_UNI_cmathfrak, a2l1x_2c, WIDTH_AUTO },
    { GLYPH_UNI_dmathfrak, a2l1x_2d, WIDTH_AUTO },
    { GLYPH_UNI_emathfrak, a2l1x_2e, WIDTH_AUTO },
    { GLYPH_UNI_fmathfrak, a2l1x_2f, WIDTH_AUTO },
    { GLYPH_UNI_gmathfrak, a2l1x_2g, WIDTH_AUTO },
    { GLYPH_UNI_hmathfrak, a2l1x_2h, WIDTH_AUTO },
    { GLYPH_UNI_imathfrak, a2l1x_2i, WIDTH_AUTO },
    { GLYPH_UNI_jmathfrak, a2l1x_2j, WIDTH_AUTO },
    { GLYPH_UNI_kmathfrak, a2l1x_2k, WIDTH_AUTO },
    { GLYPH_UNI_lmathfrak, a2l1x_2l, WIDTH_AUTO },
    { GLYPH_UNI_mmathfrak, a2l1x_2m, WIDTH_AUTO },
    { GLYPH_UNI_nmathfrak, a2l1x_2n, WIDTH_AUTO },
    { GLYPH_UNI_omathfrak, a2l1x_2o, WIDTH_AUTO },
    { GLYPH_UNI_pmathfrak, a2l1x_2p, WIDTH_AUTO },
    { GLYPH_UNI_qmathfrak, a2l1x_2q, WIDTH_AUTO },
    { GLYPH_UNI_rmathfrak, a2l1x_2r, WIDTH_AUTO },
    { GLYPH_UNI_smathfrak, a2l1x_2s, WIDTH_AUTO },
    { GLYPH_UNI_tmathfrak, a2l1x_2t, WIDTH_AUTO },
    { GLYPH_UNI_umathfrak, a2l1x_2u, WIDTH_AUTO },
    { GLYPH_UNI_vmathfrak, a2l1x_2v, WIDTH_AUTO },
    { GLYPH_UNI_wmathfrak, a2l1x_2w, WIDTH_AUTO },
    { GLYPH_UNI_xmathfrak, a2l1x_2x, WIDTH_AUTO },
    { GLYPH_UNI_ymathfrak, a2l1x_2y, WIDTH_AUTO },
    { GLYPH_UNI_zmathfrak, a2l1x_2z, WIDTH_AUTO }
};

static void ascii_init(void)
{
    outenc_register_glyphtweaks(
    	a2l1_glyphtweaks,
	lengthof(a2l1_glyphtweaks)
    );
    simple_init_charmap(127, NULL, 0);

    outenc_write_raw_glyph = simple_write_raw_glyph;
    outenc_composition_width = generic_composition_width;
    outenc_xlat_composition = generic_xlat_composition;

    /* FIXME: we don't register any accentings, but we should register some
     * translations for the most common non-spacing diacritics instead.
     */
}


/***********************************************************************
 * latin1
 ***********************************************************************/

/* How to combine a base glyph and a diacritic - ISO 8859-1 cases */
static struct generic_accententry_t latin1_accentings[] = {
    {GLYPH_A, GLYPH_gravecomb, GLYPH_Agrave},
    {GLYPH_A, GLYPH_acutecomb, GLYPH_Aacute},
    {GLYPH_A, GLYPH_UNI_circumflexcomb, GLYPH_Acircumflex},
    {GLYPH_A, GLYPH_tildecomb, GLYPH_Atilde},
    {GLYPH_A, GLYPH_UNI_dieresiscomb, GLYPH_Adieresis},
    {GLYPH_A, GLYPH_UNI_ringcomb, GLYPH_Aring},
    {GLYPH_C, GLYPH_UNI_cedillacomb, GLYPH_Ccedilla},
    {GLYPH_C, GLYPH_UNI_circlecomb, GLYPH_copyright},
    {GLYPH_E, GLYPH_gravecomb, GLYPH_Egrave},
    {GLYPH_E, GLYPH_acutecomb, GLYPH_Eacute},
    {GLYPH_E, GLYPH_UNI_circumflexcomb, GLYPH_Ecircumflex},
    {GLYPH_E, GLYPH_UNI_dieresiscomb, GLYPH_Edieresis},
    {GLYPH_I, GLYPH_gravecomb, GLYPH_Igrave},
    {GLYPH_I, GLYPH_acutecomb, GLYPH_Iacute},
    {GLYPH_I, GLYPH_UNI_circumflexcomb, GLYPH_Icircumflex},
    {GLYPH_I, GLYPH_UNI_dieresiscomb, GLYPH_Idieresis},
    {GLYPH_N, GLYPH_tildecomb, GLYPH_Ntilde},
    {GLYPH_O, GLYPH_gravecomb, GLYPH_Ograve},
    {GLYPH_O, GLYPH_acutecomb, GLYPH_Oacute},
    {GLYPH_O, GLYPH_UNI_circumflexcomb, GLYPH_Ocircumflex},
    {GLYPH_O, GLYPH_tildecomb, GLYPH_Otilde},
    {GLYPH_O, GLYPH_UNI_dieresiscomb, GLYPH_Odieresis},
    {GLYPH_R, GLYPH_UNI_circlecomb, GLYPH_registered},
    {GLYPH_U, GLYPH_gravecomb, GLYPH_Ugrave},
    {GLYPH_U, GLYPH_acutecomb, GLYPH_Uacute},
    {GLYPH_U, GLYPH_UNI_circumflexcomb, GLYPH_Ucircumflex},
    {GLYPH_U, GLYPH_UNI_dieresiscomb, GLYPH_Udieresis},
    {GLYPH_Y, GLYPH_acutecomb, GLYPH_Yacute},
    {GLYPH_a, GLYPH_gravecomb, GLYPH_agrave},
    {GLYPH_a, GLYPH_acutecomb, GLYPH_aacute},
    {GLYPH_a, GLYPH_UNI_circumflexcomb, GLYPH_acircumflex},
    {GLYPH_a, GLYPH_tildecomb, GLYPH_atilde},
    {GLYPH_a, GLYPH_UNI_dieresiscomb, GLYPH_adieresis},
    {GLYPH_a, GLYPH_UNI_ringcomb, GLYPH_aring},
    {GLYPH_c, GLYPH_UNI_cedillacomb, GLYPH_ccedilla},
    {GLYPH_c, GLYPH_UNI_circlecomb, GLYPH_copyright},
    {GLYPH_e, GLYPH_gravecomb, GLYPH_egrave},
    {GLYPH_e, GLYPH_acutecomb, GLYPH_eacute},
    {GLYPH_e, GLYPH_UNI_circumflexcomb, GLYPH_ecircumflex},
    {GLYPH_e, GLYPH_UNI_dieresiscomb, GLYPH_edieresis},
    {GLYPH_i, GLYPH_gravecomb, GLYPH_igrave},
    {GLYPH_i, GLYPH_acutecomb, GLYPH_iacute},
    {GLYPH_i, GLYPH_UNI_circumflexcomb, GLYPH_icircumflex},
    {GLYPH_i, GLYPH_UNI_dieresiscomb, GLYPH_idieresis},
    {GLYPH_dotlessi, GLYPH_gravecomb, GLYPH_igrave},
    {GLYPH_dotlessi, GLYPH_acutecomb, GLYPH_iacute},
    {GLYPH_dotlessi, GLYPH_UNI_circumflexcomb, GLYPH_icircumflex},
    {GLYPH_dotlessi, GLYPH_UNI_dieresiscomb, GLYPH_idieresis},
    {GLYPH_n, GLYPH_tildecomb, GLYPH_ntilde},
    {GLYPH_o, GLYPH_gravecomb, GLYPH_ograve},
    {GLYPH_o, GLYPH_acutecomb, GLYPH_oacute},
    {GLYPH_o, GLYPH_UNI_circumflexcomb, GLYPH_ocircumflex},
    {GLYPH_o, GLYPH_tildecomb, GLYPH_otilde},
    {GLYPH_o, GLYPH_UNI_dieresiscomb, GLYPH_odieresis},
    {GLYPH_u, GLYPH_gravecomb, GLYPH_ugrave},
    {GLYPH_u, GLYPH_acutecomb, GLYPH_uacute},
    {GLYPH_u, GLYPH_UNI_circumflexcomb, GLYPH_ucircumflex},
    {GLYPH_u, GLYPH_UNI_dieresiscomb, GLYPH_udieresis},
    {GLYPH_y, GLYPH_acutecomb, GLYPH_yacute},
    {GLYPH_y, GLYPH_UNI_dieresiscomb, GLYPH_ydieresis},
    /* end of table */
    {0, 0, 0}
};

static void latin1_init(void)
{
    outenc_register_glyphtweaks(
    	a2l1_glyphtweaks,
	lengthof(a2l1_glyphtweaks)
    );

    simple_init_charmap(255, NULL, 0);
    outenc_write_raw_glyph = simple_write_raw_glyph;

    outenc_composition_width = generic_composition_width;
    outenc_xlat_composition = generic_xlat_composition;

    generic_register_accentings(latin1_accentings);
}


/***********************************************************************
 * latin9
 ***********************************************************************/
/* How to combine a base glyph and a diacritic -- ISO 8859-15 cases not
 * already covered by ISO 8859-1
 */
static struct generic_accententry_t latin9_accentings[] = {
    {GLYPH_S, GLYPH_UNI_caroncomb, GLYPH_Scaron},
    {GLYPH_s, GLYPH_UNI_caroncomb, GLYPH_scaron},
    {GLYPH_Z, GLYPH_UNI_caroncomb, GLYPH_Zcaron},
    {GLYPH_z, GLYPH_UNI_caroncomb, GLYPH_zcaron},
    {GLYPH_Y, GLYPH_UNI_dieresiscomb, GLYPH_Ydieresis},
    /* end of table */
    {0, 0, 0}
};

static struct simple_charmap_entry_t latin9_charmap[] = {
    {GLYPH_Euro, 0xa4},
    {0x00a5, 0xa5},
    {GLYPH_Scaron, 0xa6},
    {0x00a7, 0xa7},
    {GLYPH_scaron, 0xa8},
    {0x00a9, 0xa9},
    {0x00aa, 0xaa},
    {0x00ab, 0xab},
    {0x00ac, 0xac},
    {0x00ad, 0xad},
    {0x00ae, 0xae},
    {0x00af, 0xaf},
    {0x00b0, 0xb0},
    {0x00b1, 0xb1},
    {0x00b2, 0xb2},
    {0x00b3, 0xb3},
    {GLYPH_Zcaron, 0xb4},
    {0x00b5, 0xb5},
    {0x00b6, 0xb6},
    {0x00b7, 0xb7},
    {GLYPH_zcaron, 0xb8},
    {0x00b9, 0xb9},
    {0x00ba, 0xba},
    {0x00bb, 0xbb},
    {GLYPH_OE, 0xbc},
    {GLYPH_oe, 0xbd},
    {GLYPH_Ydieresis, 0xbe},
    {0x00bf, 0xbf},
    {0x00c0, 0xc0},
    {0x00c1, 0xc1},
    {0x00c2, 0xc2},
    {0x00c3, 0xc3},
    {0x00c4, 0xc4},
    {0x00c5, 0xc5},
    {0x00c6, 0xc6},
    {0x00c7, 0xc7},
    {0x00c8, 0xc8},
    {0x00c9, 0xc9},
    {0x00ca, 0xca},
    {0x00cb, 0xcb},
    {0x00cc, 0xcc},
    {0x00cd, 0xcd},
    {0x00ce, 0xce},
    {0x00cf, 0xcf},
    {0x00d0, 0xd0},
    {0x00d1, 0xd1},
    {0x00d2, 0xd2},
    {0x00d3, 0xd3},
    {0x00d4, 0xd4},
    {0x00d5, 0xd5},
    {0x00d6, 0xd6},
    {0x00d7, 0xd7},
    {0x00d8, 0xd8},
    {0x00d9, 0xd9},
    {0x00da, 0xda},
    {0x00db, 0xdb},
    {0x00dc, 0xdc},
    {0x00dd, 0xdd},
    {0x00de, 0xde},
    {0x00df, 0xdf},
    {0x00e0, 0xe0},
    {0x00e1, 0xe1},
    {0x00e2, 0xe2},
    {0x00e3, 0xe3},
    {0x00e4, 0xe4},
    {0x00e5, 0xe5},
    {0x00e6, 0xe6},
    {0x00e7, 0xe7},
    {0x00e8, 0xe8},
    {0x00e9, 0xe9},
    {0x00ea, 0xea},
    {0x00eb, 0xeb},
    {0x00ec, 0xec},
    {0x00ed, 0xed},
    {0x00ee, 0xee},
    {0x00ef, 0xef},
    {0x00f0, 0xf0},
    {0x00f1, 0xf1},
    {0x00f2, 0xf2},
    {0x00f3, 0xf3},
    {0x00f4, 0xf4},
    {0x00f5, 0xf5},
    {0x00f6, 0xf6},
    {0x00f7, 0xf7},
    {0x00f8, 0xf8},
    {0x00f9, 0xf9},
    {0x00fa, 0xfa},
    {0x00fb, 0xfb},
    {0x00fc, 0xfc},
    {0x00fd, 0xfd},
    {0x00fe, 0xfe},
    {0x00ff, 0xff}
};

static void latin9_init(void)
{
    outenc_register_glyphtweaks(
    	a2l1_glyphtweaks,
	lengthof(a2l1_glyphtweaks)
    );
    outenc_unregister_glyphtweak(GLYPH_OE);
    outenc_unregister_glyphtweak(GLYPH_oe);
    	/* We are parasitic and simply tune the latin1 tweaks to our needs. */

    simple_init_charmap(163, latin9_charmap, lengthof(latin9_charmap));
    outenc_write_raw_glyph = simple_write_raw_glyph;

    outenc_composition_width = generic_composition_width;
    outenc_xlat_composition = generic_xlat_composition;

    generic_register_accentings(latin1_accentings);
    generic_register_accentings(latin9_accentings);
}


/***********************************************************************
 * implementations of encoding independent stuff
 ***********************************************************************/

static spars32_t glyphwidth_spars32;
static sparp_t glyphxlat_sparp;

void outenc_init(void)
{
    assert(outenc_num < OE_TOOBIG);

    pmesg(50, "BEGIN outenc_init\n");
    
    sparp_init(&accenting_sparp);
    vlist_init(&accenting_vlist);
    spars32_init(&glyphwidth_spars32, 1);
    sparp_init(&glyphxlat_sparp);

    outenc_register_glyphtweaks(
    	generic_glyphtweaks,
	lengthof(generic_glyphtweaks)
    );
    	/* The encoding-specific inits can override these if required */

    generic_register_accentings(generic_accentings);
    	/* Ditto. But any encoding wants them anyway. */

    switch(outenc_num) {
    	case OE_UTF8:
	    utf8_init();
	    break;
    	case OE_ASCII:
	    ascii_init();
	    break;
    	case OE_LATIN1:
	    latin1_init();
	    break;
    	case OE_LATIN9:
	    latin9_init();
	    break;
	default:
	    NOTREACHED;
    }
    /* check we didn't forget to set the virtual methods */
    assert(outenc_composition_width != NULL);
    assert(outenc_xlat_composition != NULL);
    assert(outenc_write_raw_glyph != NULL);

    pmesg(50, "END outenc_init\n");
}


int outenc_get_width(const linebuf_t * l)
{
    glyph_t * p;
    int w = 0;

    pmesg(50, "BEGIN outenc_get_width\n");

    p = l->gstring;
    while(*p != 0) {
    	if(glyph_get_hint(*(p + 1)) & GH_COMBINING) {
	    /* grok the sequence of combining diacritics after *p */
    	    glyph_t * q = p + 2;
	    while(glyph_get_hint(*q) & GH_COMBINING) ++q;
	    	/* will stop by itself on 0 termination */
	    w += outenc_composition_width(p, q - p);
	    p = q;
	}
	else {
	    w += outenc_glyph_width(*p);
	    ++p;
	}
    }

    pmesg(50, "END outenc_get_width\n");
    return w;
}


void outenc_write(FILE * f, const linebuf_t * l)
{
    linebuf_t xl;
    glyph_t * p;

    pmesg(50, "BEGIN outenc_write\n");

    linebuf_init(&xl, 2*l->size_curr);

    p = l->gstring;
    while(*p != 0) {
    	if(glyph_get_hint(*(p + 1)) & GH_COMBINING) {
	    /* grok the sequence of combining diacritics after *p */
    	    glyph_t * q = p + 2;
	    while(glyph_get_hint(*q) & GH_COMBINING) ++q;
	    	/* will stop by itself on 0 termination */
	    outenc_xlat_composition(p, q - p, &xl);
	    p = q;
	}
	else {
	    outenc_xlat_glyph(*p, &xl);
	    ++p;
	}
    }

    p = xl.gstring;
    while(*p != 0) outenc_write_raw_glyph(f, *p++);

    linebuf_done(&xl);

    pmesg(50, "END outenc_write\n");
}

static int outenc_glyph_width(glyph_t g)
{
    return (int) spars32_read(&glyphwidth_spars32, g);
}

static void outenc_xlat_glyph(glyph_t g, linebuf_t * l)
{
    linebuf_t * px;
    
    px = sparp_read(&glyphxlat_sparp, g);
    if(px == NULL) linebuf_putg(l, g);
    else linebuf_append(l, px);

}

static void outenc_register_glyphtweaks(const glyphtweak_t tweaks[], int length)
{
    int i;
    linebuf_t * px;
    glyphtweak_t tweak;
        
    px = malloc(length * sizeof(linebuf_t));
    if(px == NULL) enomem();
    /* Note we "leak memory" here since we leave px allocated but
     * forget them on returning from this function. This is OK since
     * the allocated memory is used throughout the whole lifetime of
     * the process.
     *
     * FIXME: this has to stop if we want to become a library.
     */
     
    for(i=0; i < length; ++i) {
    	tweak = tweaks[i];
    	assert(tweak.width != WIDTH_AUTO || tweak.xlat != NULL);
	    /* We can't calculate width without translation string */ 
    	if(tweak.xlat != NULL) {
	    linebuf_garray0_init(px+i, tweak.xlat);
	    sparp_write(&glyphxlat_sparp, tweak.glyph, px+i);
	    if(tweak.width == WIDTH_AUTO) tweak.width = px[i].size_curr;
	}
	spars32_write(&glyphwidth_spars32, tweak.glyph, tweak.width);
    }

}

static void outenc_unregister_glyphtweak(glyph_t g)
{
    if(sparp_read(&glyphxlat_sparp, g) != NULL) {
    	sparp_write(&glyphxlat_sparp, g, NULL);
    }
    if(spars32_read(&glyphwidth_spars32, g) != 1) {
    	spars32_write(&glyphwidth_spars32, g, 1);
    }
}

static void outenc_show_unknown(FILE * f, glyph_t g)
/* FIXME: prints always in the execution character set, even if that
 * is not a superset of ASCII. Is this good or bad?
 */
{
    if (outenc_show_unicode_number) {
    	if(g <= 0xffff)	fprintf(f,  "[U+%04lX]", g);
	else fprintf(f,  "[U+%08lX]", g);
    } else {
    	putc('?', f);
    }
}

