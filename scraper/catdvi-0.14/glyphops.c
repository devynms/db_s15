/* catdvi - get text from DVI files
   Copyright (C) 2000, 2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>
 
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
 
#include "glyphops.h"
#include "sparse.h"
#include "glyphenm.h"

#define EOT 0x04    /* end-of-table mark */


/* known formatting hints */ 
struct glyph_hintentry_t {
    glyph_t glyph;
    enum glyph_hint_t hint;
};

static struct glyph_hintentry_t hints[] = {
    {GLYPH_asciicircum,    	    GH_DIACRITIC},
    {GLYPH_grave,	    	    GH_DIACRITIC},
    {GLYPH_asciitilde, 	    	    GH_DIACRITIC},

    {GLYPH_dieresis,     	    GH_DIACRITIC},
    {GLYPH_UNI_macron,	    	    GH_DIACRITIC},
    {GLYPH_acute, 	    	    GH_DIACRITIC},
    {GLYPH_cedilla,	    	    GH_DIACRITIC},

    {GLYPH_circumflex,	    	    GH_DIACRITIC},
    {GLYPH_caron, 	    	    GH_DIACRITIC},
    {GLYPH_UNI_macronmodifier,	    GH_DIACRITIC},
    {GLYPH_UNI_acutemodifier,	    GH_DIACRITIC},
    {GLYPH_UNI_gravemodifier,	    GH_DIACRITIC},
    {GLYPH_breve,   	    	    GH_DIACRITIC},
    {GLYPH_dotaccent,	    	    GH_DIACRITIC},
    {GLYPH_ring,    	    	    GH_DIACRITIC},
    {GLYPH_ogonek,	    	    GH_DIACRITIC},
    {GLYPH_tilde,     	    	    GH_DIACRITIC},
    {GLYPH_CATDVI_polishstroke,     GH_DIACRITIC},

    {GLYPH_CATDVI_negationslash,    GH_DIACRITIC},
    {GLYPH_UNI_circlelarge, 	    GH_DIACRITIC},
    {GLYPH_CATDVI_vector,   	    GH_DIACRITIC},
    
    {GLYPH_gravecomb,	    	    GH_COMBINING_DIACRITIC},
    {GLYPH_acutecomb,	    	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_circumflexcomb,	    GH_COMBINING_DIACRITIC},
    {GLYPH_tildecomb, 	    	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_macroncomb,          GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_overlinecomb,        GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_brevecomb, 	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_dotaccentcomb,       GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_dieresiscomb,   	    GH_COMBINING_DIACRITIC},
    {GLYPH_hookabovecomb,  	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_ringcomb,    	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_caroncomb,   	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_cedillacomb,  	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_ogonekcomb,  	    GH_COMBINING_DIACRITIC},
    {GLYPH_CATDVI_polishstrokecomb, GH_COMBINING_DIACRITIC},

    {GLYPH_UNI_slashlongcomb,	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_circlecomb,  	    GH_COMBINING_DIACRITIC},
    {GLYPH_UNI_vectorcomb,  	    GH_COMBINING_DIACRITIC},

    {GLYPH_ADOBE_periodsuperior,    GH_DIACRITIC},

    {GLYPH_CATDVI_Eurodblstroke,    GH_DIACRITIC},
    {GLYPH_CATDVI_Eurodblstrokecomb,    GH_COMBINING_DIACRITIC},

    {GLYPH_radical,	GH_RADICAL},
    	/* Is in the math symbols font but used just like the big
	 * variants in the math extension font.
	 */

    /* The TeX math extension stuff */
    {GLYPH_CATDVI_parenleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenrightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_bracketleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracketrightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_floorleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_floorrightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_ceilingleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_ceilingrightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_braceleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracerightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_angbracketleftbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_angbracketrightbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_vextendsingle,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_vextenddouble,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_slashbig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_backslashbig,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenrightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_parenleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenrightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_bracketleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracketrightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_floorleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_floorrightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_ceilingleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_ceilingrightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_braceleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracerightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_angbracketleftbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_angbracketrightbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_slashbigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_backslashbigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_parenrightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_bracketleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracketrightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_floorleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_floorrightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_ceilingleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_ceilingrightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_braceleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracerightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_angbracketleftBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_angbracketrightBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_slashBigg,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_backslashBigg,	GH_ON_AXIS},
    {GLYPH_CATDVI_slashBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_backslashBig,	GH_ON_AXIS},
    {GLYPH_ADOBE_parenlefttp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_parenrighttp,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_bracketlefttp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracketrighttp,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_bracketleftbt,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracketrightbt,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_bracketleftex,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracketrightex,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_bracelefttp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracerighttp,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_braceleftbt,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracerightbt,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_braceleftmid,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_bracerightmid,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_braceex,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_arrowvertex,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_parenleftbt,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_parenrightbt,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_ADOBE_parenleftex,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_ADOBE_parenrightex,	GH_EXTENSIBLE_RECIPE | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_angbracketleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_angbracketrightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_unionsqtext,	GH_ON_AXIS},
    {GLYPH_CATDVI_unionsqdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_contintegraltext,	GH_ON_AXIS},
    {GLYPH_CATDVI_contintegraldisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_circledottext,	GH_ON_AXIS},
    {GLYPH_CATDVI_circledotdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_circleplustext,	GH_ON_AXIS},
    {GLYPH_CATDVI_circleplusdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_circlemultiplytext,	GH_ON_AXIS},
    {GLYPH_CATDVI_circlemultiplydisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_summationtext,	GH_ON_AXIS},
    {GLYPH_CATDVI_producttext,	GH_ON_AXIS},
    {GLYPH_CATDVI_integraltext,	GH_ON_AXIS},
    {GLYPH_CATDVI_uniontext,	GH_ON_AXIS},
    {GLYPH_CATDVI_intersectiontext,	GH_ON_AXIS},
    {GLYPH_CATDVI_unionmultitext,	GH_ON_AXIS},
    {GLYPH_CATDVI_logicalandtext,	GH_ON_AXIS},
    {GLYPH_CATDVI_logicalortext,	GH_ON_AXIS},
    {GLYPH_CATDVI_summationdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_productdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_integraldisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_uniondisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_intersectiondisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_unionmultidisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_logicalanddisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_logicalordisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_coproducttext,	GH_ON_AXIS},
    {GLYPH_CATDVI_coproductdisplay,	GH_ON_AXIS},
    {GLYPH_CATDVI_hatwide,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_hatwider,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_hatwidest,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_tildewide,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_tildewider,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_tildewidest,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_bracketleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracketrightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_floorleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_floorrightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_ceilingleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_ceilingrightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_braceleftBig,	GH_ON_AXIS},
    {GLYPH_CATDVI_bracerightBig,	GH_ON_AXIS | GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_radicalbig,	GH_RADICAL},
    {GLYPH_CATDVI_radicalBig,	GH_RADICAL},
    {GLYPH_CATDVI_radicalbigg,	GH_RADICAL},
    {GLYPH_CATDVI_radicalBigg,	GH_RADICAL},
    {GLYPH_CATDVI_radicalbt,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_radicalvertex,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_radicaltp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_arrowvertexdbl,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_arrowtp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_arrowbt,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_bracehtipdownright,	GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_bracehtipupright,	GH_MOREMATH_LEFT},
    {GLYPH_CATDVI_arrowdbltp,	GH_EXTENSIBLE_RECIPE},
    {GLYPH_CATDVI_arrowdblbt,	GH_EXTENSIBLE_RECIPE},
    /* end TeX math extension */

    {GLYPH_CATDVI_tildexwide,	GH_WIDE_DIACRITIC},
    {GLYPH_CATDVI_hatxwide,	GH_WIDE_DIACRITIC},

    /* end of table */
    {0, 0}
};

static struct spars32_t hint_spars32;


/* known mappings between combining and non-combining diacritics.
 * Incomplete. Use 0 for unavailable or unknown variants.
 *
 * variant3 and variant4 are for diacritics that are doubled in the
 * unicode character set (doubled at least for our purposes and IMHO -
 * if there is some underlying philosophy, I just can't see it) and
 * should be mapped to something canonical.
 *
 * we try to keep the spacing variant in the ISO 8859-1 range if possible.
 */
struct diacritic_variantentry_t {
    glyph_t spacing;
    glyph_t combining;
    glyph_t variant3;
    glyph_t variant4;
};

static struct diacritic_variantentry_t diavars[] =
{
    {GLYPH_acute, GLYPH_acutecomb, GLYPH_UNI_acutemodifier, 0},
    {GLYPH_grave, GLYPH_gravecomb, GLYPH_UNI_gravemodifier, 0},
    {GLYPH_asciicircum, GLYPH_UNI_circumflexcomb, GLYPH_circumflex, 0},
    {GLYPH_dieresis, GLYPH_UNI_dieresiscomb, 0, 0},
    {GLYPH_tilde, GLYPH_tildecomb, GLYPH_asciitilde, 0},
    {GLYPH_ring, GLYPH_UNI_ringcomb, 0, 0},

    {GLYPH_cedilla, GLYPH_UNI_cedillacomb, 0, 0},

    {GLYPH_caron, GLYPH_UNI_caroncomb, 0, 0},
    {GLYPH_UNI_macron, GLYPH_UNI_macroncomb, GLYPH_UNI_macronmodifier, 0},
    {GLYPH_breve, GLYPH_UNI_brevecomb, 0, 0},
    {GLYPH_dotaccent, GLYPH_UNI_dotaccentcomb, GLYPH_ADOBE_periodsuperior, 0},

    {GLYPH_ogonek, GLYPH_UNI_ogonekcomb, 0, 0},
    {GLYPH_CATDVI_polishstroke, GLYPH_CATDVI_polishstrokecomb, 0, 0},

    {GLYPH_CATDVI_negationslash, GLYPH_UNI_slashlongcomb, 0, 0},
    {GLYPH_UNI_circlelarge, GLYPH_UNI_circlecomb, 0, 0},
    {GLYPH_CATDVI_vector, GLYPH_UNI_vectorcomb, 0, 0},

    {GLYPH_CATDVI_Eurodblstroke, GLYPH_CATDVI_Eurodblstrokecomb, 0, 0},

    /* end of table */
    {EOT, 0, 0, 0}
};

static struct sparp_t diavar_sparp;

void glyphops_init()
{
    struct glyph_hintentry_t * ph;
    struct diacritic_variantentry_t * pd;
    
    /* The glyph hint sparp is indexed by the glyph and points directly to
     * the hint.
     */
    spars32_init(&hint_spars32, 0);
    for(ph = hints; ph->glyph != 0; ++ph) {
    	spars32_write(&hint_spars32, ph->glyph, ph->hint);
    }
    
    /* The diacritics variant sparp is indexed by any variant of the diacritic.
     * The values point to the corresponding dicaritic_variantentry_t .
     */
    sparp_init(&diavar_sparp);
    for(pd = diavars; pd->spacing != EOT; ++pd) {
    	if(pd->spacing != 0) sparp_write(&diavar_sparp, pd->spacing, pd);
    	if(pd->combining != 0) sparp_write(&diavar_sparp, pd->combining, pd);
    	if(pd->variant3 != 0) sparp_write(&diavar_sparp, pd->variant3, pd);
    	if(pd->variant4 != 0) sparp_write(&diavar_sparp, pd->variant4, pd);
    }
    
}

enum glyph_hint_t glyph_get_hint(glyph_t glyph)
{
    return (enum glyph_hint_t) spars32_read(&hint_spars32, glyph);
}

glyph_t diacritic_combining_variant(glyph_t diacritic)
{
    struct diacritic_variantentry_t * p;
    
    p = sparp_read(&diavar_sparp, diacritic);
    return (p != NULL) ? p->combining : 0;
}

glyph_t diacritic_spacing_variant(glyph_t diacritic)
{
    struct diacritic_variantentry_t * p;
    
    p = sparp_read(&diavar_sparp, diacritic);
    return (p != NULL) ? p->spacing : 0;
}
