/* catdvi - get text from DVI files
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
   Copyright (C) 2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>

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

/* The TFM format is described in the DVI Driver Standard, Level 0,
   available from CTAN. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "bytesex.h"
#include "findtfm.h"
#include "fixword.h"
#include "fontinfo.h"
#include "readdvi.h"
#include "util.h"
#include "fntenc.h"

/* Don't make this smaller than 256, you'd break the spec that way. */
#define LEN_TFM_TBL 256

/* These values are from the spec... don't touch them! */
#define ENCODING_LEN 40
#define FAMILY_LEN 20

struct char_info_t {
        unsigned width_index : 8;
        unsigned height_index : 4;
        unsigned depth_index : 4;
        unsigned italic_index : 6;
        unsigned tag : 2;
        unsigned remainder : 8;
};

struct lig_kern_command_t {
        byte skip_byte;
        byte next_char;
        byte op_byte;
        byte remainder;
};

struct extensible_recipe_t {
        byte top;
        byte mid;
        byte bot;
        byte rep;
};

struct tfm_t {
        /* Info from the font definition in DVI */
        fix_word_t scale;
        fix_word_t design;
        /* Header */
        uint32 lf;
        uint32 lh;
        uint32 bc;
        uint32 ec;
        uint32 nw;
        uint32 nh;
        uint32 nd;
        uint32 ni;
        uint32 nl;
        uint32 nk;
        uint32 ne;
        uint32 np;

        /* Data header */
        uint32 checksum;
        fix_word_t design_size;
        char encoding[ENCODING_LEN];
        char family[FAMILY_LEN];

        /* Param array */
        fix_word_t slant;
        fix_word_t space;
        fix_word_t space_stretch;
        fix_word_t space_shrink;
        fix_word_t x_height;
        fix_word_t quad;
        fix_word_t extra_space;
	fix_word_t * other_params;
	
        /* Data arrays */
        struct char_info_t * char_info;
        fix_word_t * width;
        fix_word_t * height;
        fix_word_t * depth;
        fix_word_t * italic;
        struct lig_kern_command_t * lig_kern;
        fix_word_t * kern;
        struct extensible_recipe_t * exten;

    	/* misc precomputed stuff */
    	fix_word_t axis_height; /* 0 if the font doesn't have it */
	sint32 scaled_thinspace; /* space - space_shrink, guarded against
	    	    	    	  * missing parameters and already scaled
				  */
	sint32 scaled_usequad;   /* ditto for quad */
};

static struct tfm_t tfm_tbl[LEN_TFM_TBL];

static const double spaces_per_quad = 4.0;
    /* font_w_to_space() [see below] uses the quantity
     * (tfm_tbl[font].space - tfm_tbl[font].space_shrink) 
     * for the conversion (DVI x distances) -> (number of spaces in text).
     * Some fonts don't supply spacing information (i.e. .space is zero)
     * and we have to estimate above quantity by tfm_tbl[font].quad .
     * 
     * The correct factor is not clear, but here are some data points:
     *
     * - q&d statistics over calls to font_w_to_space with .space != 0
     *   on several large files gave (rounded to two decimals)
     *   .quad / .space :                   arithmetic mean 2.93, median 3.00
     *   .quad / (.space - .space_shrink) : arithmetic mean 4.31, median 4.50
     *
     * - The main examples of fonts with .space == 0 are the math fonts.
     *   In math mode we have multiple spacing instructions: \, \: \; yield
     *   {3/18 = 1/6, 4/18 = 1/4.5, 5/18 = 1/3.6} .quad respectively.
     *   I'm not sure whether \, and \: should result in printed-out spaces,
     *   but I'd say \; should.
     */

static struct char_info_t * read_char_info_array(uint32 bc, uint32 ec,
                                                 FILE * fp)
{
        struct char_info_t * rv;
        uint32 i, count;

        count = ec - bc + 1;

        rv = xmalloc(count * sizeof(struct char_info_t));

        for (i = 0; i < count; i++) {                
                byte b;

                rv[i].width_index = readbyte(fp);

                b = readbyte(fp);
                rv[i].height_index = b / 16;
                rv[i].depth_index = b % 16;
                
                b = readbyte(fp);
                rv[i].italic_index = b / 4;
                rv[i].tag = b % 4;

                rv[i].remainder = readbyte(fp);
        }

        return rv;
}

static fix_word_t * read_fixword_array(uint32 count, FILE * fp)
{
        uint32 i;
        fix_word_t * rv;

        rv = xmalloc(count * sizeof(fix_word_t));

        for (i = 0; i < count; i++) rv[i] = read_fw(fp);

        return rv;
}

static struct tfm_t read_tfm(char const * fname)
{
        FILE * fp;
        size_t dh_already_read = 0; /* of data header */

	struct tfm_t rv = {
        	0, /* scale */
        	0, /* design */

        	0, /* lf */
        	0, /* lh */
        	0, /* bc */
        	0, /* ec */
        	0, /* nw */
        	0, /* nh */
        	0, /* nd */
        	0, /* ni */
        	0, /* nl */
        	0, /* nk */
        	0, /* ne */
        	0, /* np */

        	0, /* checksum */
        	0, /* design_size */
        	{0}, /* encoding[ENCODING_LEN] */
        	{0}, /* family[FAMILY_LEN] */

        	0, /* slant */
        	0, /* space */
        	0, /* space_stretch */
        	0, /* space_shrink */
        	0, /* x_height */
        	0, /* quad */
        	0, /* extra_space */
		0, /* other_params */

        	0, /* char_info */
        	0, /* width */
        	0, /* height */
        	0, /* depth */
        	0, /* italic */
        	0, /* lig_kern */
        	0, /* kern */
        	0, /* exten */
		
		0, /* axis_height */
    	    	0, /* scaled_thinspace */
    	    	0  /* scaled_usequad */
	};

        fp = fopen(fname, "rb");
        if (fp == 0) {
	    panic("Could not open %s: %s\n", fname, strerror(errno));
        }

        /* Header */
        rv.lf = u_readbigendiannumber(2, fp);
        rv.lh = u_readbigendiannumber(2, fp);
        rv.bc = u_readbigendiannumber(2, fp);
        rv.ec = u_readbigendiannumber(2, fp);
        rv.nw = u_readbigendiannumber(2, fp);
        rv.nh = u_readbigendiannumber(2, fp);
        rv.nd = u_readbigendiannumber(2, fp);
        rv.ni = u_readbigendiannumber(2, fp);
        rv.nl = u_readbigendiannumber(2, fp);
        rv.nk = u_readbigendiannumber(2, fp);
        rv.ne = u_readbigendiannumber(2, fp);
        rv.np = u_readbigendiannumber(2, fp);

        /* Header data */
        rv.checksum = u_readbigendiannumber(4, fp);
        rv.design_size = s_readbigendiannumber(4, fp);

        dh_already_read = 4 + 4;

        if (4 * rv.lh > dh_already_read) {
                readbcblstring((byte *) rv.encoding, ENCODING_LEN, fp);
                dh_already_read += ENCODING_LEN;
        }
                
        if (4 * rv.lh > dh_already_read) {
                readbcblstring((byte *) rv.family, FAMILY_LEN, fp);
                dh_already_read += FAMILY_LEN;
        }

        pmesg(95, "TFM file %s, encoding %s, family %s\n",
              fname, rv.encoding, rv.family);

        /* Ignore the rest of the data header */
        skipbytes(4 * rv.lh - dh_already_read, fp);

        if (rv.bc <= rv.ec) {
                rv.char_info = read_char_info_array(rv.bc, rv.ec, fp);
        }

        rv.width = read_fixword_array(rv.nw, fp);

        rv.height = read_fixword_array(rv.nh, fp);

        rv.depth = read_fixword_array(rv.nd, fp);

        rv.italic = read_fixword_array(rv.ni, fp);

        /* FIXME: Read lig_kern array for real. */
        skipbytes(4 * rv.nl, fp);

        rv.kern = read_fixword_array(rv.nk, fp);

        /* FIXME: Read exten array for real. */
        skipbytes(4 * rv.ne, fp);

        /* Read the param array. */
        /**/                            if (rv.np == 0) goto skip;
        rv.slant         = read_fw(fp); if (rv.np == 1) goto skip;
        rv.space         = read_fw(fp); if (rv.np == 2) goto skip;
        rv.space_stretch = read_fw(fp); if (rv.np == 3) goto skip;
        rv.space_shrink  = read_fw(fp); if (rv.np == 4) goto skip;
        rv.x_height      = read_fw(fp); if (rv.np == 5) goto skip;
        rv.quad          = read_fw(fp); if (rv.np == 6) goto skip;
        rv.extra_space   = read_fw(fp); if (rv.np == 7) goto skip;
	rv.other_params  = read_fixword_array(rv.np - 7, fp);
 skip:
        /* We should have reached the end of the tfm file now. */

        fclose(fp);

        return rv;
}

void font_def(sint32 k, uint32 c, uint32 s, uint32 d,
              byte a, byte l, char const * n)
{
    	static const char fallback_font[] = "cmr10";
        char const * fname;
	fix_word_t thinspace, usequad;

        pmesg(90, "FONT DEF: k = %li, c = %lu, s = %lu, d = %lu,\n"
              "          a = %u, l = %u, n = %s\n",
              k, c, s, d, a, l, n);

        if (k < 0 || k >= LEN_TFM_TBL) {
                warning("Font index out of range, ignoring font definition\n");
                return;
        }

        fname = find_tfm(n);
	if(fname == NULL) {
	    fname = find_tfm(fallback_font);
    	    if(fname != NULL) {
		warning(
		    "Could not find font metrics file %s.tfm --" \
		    " using %s.tfm instead. Expect incorrect output.\n",
		    n,
		    fallback_font
	    	);
		c = 0;  /* Don't compare checksums, they will not match. */
	    }
	    else {
	    	panic(
		    "Cannot find font metrics file %s.tfm, fallback" \
		    " %s.tfm is also missing.\nAborting.\n",
		    n,
		    fallback_font
	    	);
	    }
    	}

        tfm_tbl[k] = read_tfm(fname);
        if (tfm_tbl[k].checksum != c && c && tfm_tbl[k].checksum) {
	    	/* don't compare checksums if either is zero */ 
                warning("Checksum mismatch reading font `%s'\n", n);
        }
        tfm_tbl[k].scale = s;
        tfm_tbl[k].design = d;

    	/* do we have axis_height ? */
	if(tfm_tbl[k].np == 22) {
	    /* We are a little bit lax here and just assume that a font
	     * carries the same set of parameters as OMS encoded fonts
	     * iff it has the same _number_ of parameters (which is 22,
	     * and axis_height happens to be the last one).
	     */
	    tfm_tbl[k].axis_height = font_param(k, 22);
	}
	else tfm_tbl[k].axis_height = 0;

    	/* Compute two values required by font_w_to_space(). That function
	 * is called VERY often, so it makes sense to precompute the values.
	 */
    	usequad = tfm_tbl[k].quad;
	if(usequad <= 0) {
		/* Some fonts (e.g. cmman) don't even have quad set. Last
		 * resort: assume quad == design size (usually about right).
		 */
	    	usequad = double2fw(1.0);
	}
    	tfm_tbl[k].scaled_usequad = font_scale_fw(k, usequad);

	thinspace = tfm_tbl[k].space - tfm_tbl[k].space_shrink;
        if (thinspace > 0) {
	    	tfm_tbl[k].scaled_thinspace = font_scale_fw(k, thinspace);
        }
    	else {
	    	/* Some fonts (e.g. math) don't have space set - we have
		 * to estimate a useful value.
		 */
	    	tfm_tbl[k].scaled_thinspace = font_scale_fw(k, usequad) / 
		    	spaces_per_quad;
	}
}


char const * font_enc(sint32 k)
{
        if (k < 0 || k >= LEN_TFM_TBL) {
                warning("Font index out of range, reverting to TeX text\n");
                return "TEX TEXT";
        }
        pmesg(80, "[font %li, encoding: %s]\n", k, tfm_tbl[k].encoding);
        return tfm_tbl[k].encoding;
}

char const * font_family(sint32 k)
{
    	static const char fallback[] = "CMR";
        if (k < 0 || k >= LEN_TFM_TBL) {
                warning("Font index out of range, reverting to CMR\n");
                return fallback;
        }
        return tfm_tbl[k].family;
}

fix_word_t font_scale_fw(sint32 font, fix_word_t fw)
{
        return fw_prod(fw, tfm_tbl[font].scale);
}

uint32 font_char_width(sint32 font, sint32 glyph)
{
        uint32 wi, ugly, rv;

        if (glyph < 0) {
                warning("ignoring negative glyph index");
                return 0;
        }
        ugly = glyph;

        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range, pretending like glyph were empty\n");
                return 0;
        }

        if (ugly < tfm_tbl[font].bc || ugly > tfm_tbl[font].ec) {
                warning("Glyph does not exist in font\n");
                return 0;
        }

        wi = tfm_tbl[font].char_info[ugly - tfm_tbl[font].bc].width_index;

        if (wi >= tfm_tbl[font].nw) {
                warning("TFM file corrupt - width index out of bounds\n");
                return 0;
        }

        rv = font_scale_fw(font, tfm_tbl[font].width[wi]);
        pmesg(105, "FONT_CHAR_WIDTH: font %li, glyph %li, width %li\n",
              font, glyph, rv);
        return rv;

}

uint32 font_char_height(sint32 font, sint32 glyph)
{
        uint32 he, ugly, rv;

        if (glyph < 0) {
                warning("ignoring negative glyph index");
                return 0;
        }
        ugly = glyph;

        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range, pretending like glyph were empty\n");
                return 0;
        }

        if (ugly < tfm_tbl[font].bc || ugly > tfm_tbl[font].ec) {
                warning("Glyph does not exist in font\n");
                return 0;
        }

         he = tfm_tbl[font].char_info[ugly - tfm_tbl[font].bc].height_index;

        if (he >= tfm_tbl[font].nh) {
                warning("TFM file corrupt - height index out of bounds\n");
                return 0;
        }

        rv = font_scale_fw(font, tfm_tbl[font].height[he]);
        pmesg(105, "FONT_CHAR_HEIGH: font %li, glyph %li, height %li\n",
              font, glyph, rv);
        return rv;

}

uint32 font_char_depth(sint32 font, sint32 glyph)
{
        uint32 de, ugly, rv;

        if (glyph < 0) {
                warning("ignoring negative glyph index");
                return 0;
        }
        ugly = glyph;

        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range, pretending like glyph were empty\n");
                return 0;
        }

        if (ugly < tfm_tbl[font].bc || ugly > tfm_tbl[font].ec) {
                warning("Glyph does not exist in font\n");
                return 0;
        }

        de = tfm_tbl[font].char_info[ugly - tfm_tbl[font].bc].depth_index;

        if (de >= tfm_tbl[font].nd) {
                warning("TFM file corrupt - depth index out of bounds\n");
                return 0;
        }

        rv = font_scale_fw(font, tfm_tbl[font].depth[de]);
        pmesg(105, "FONT_CHAR_DEPTH: font %li, glyph %li, depth %li\n",
              font, glyph, rv);
        return rv;

}

unsigned int font_nparams(sint32 font)
{
        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range, pretending 0 parameters\n");
                return 0;
        }
    	return tfm_tbl[font].np;
}

fix_word_t font_param(sint32 font, unsigned int num)
{
    	struct tfm_t * pf;
	
        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range, assuming parameter = 0\n");
                return 0;
        }
    	pf = tfm_tbl + font;
	
        if (num == 0 || num > pf->np) {
                warning("Parameter not defined in font, assuming zero value\n");
                return 0;
        }

	switch(num) {
	    case 1: return pf->slant;
	    case 2: return pf->space;
	    case 3: return pf->space_stretch;
	    case 4: return pf->space_shrink;
	    case 5: return pf->x_height;
	    case 6: return pf->quad;
	    case 7: return pf->extra_space;
	    default: return pf->other_params[num - 8];
	}
}


uint32 font_axis_height(sint32 font)
{
        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range\n");
                return 0;
        }
    	return tfm_tbl[font].axis_height;
}

int font_w_to_space(sint32 font, sint32 width)
{
        sint32 rv;
	
        if (font < 0 || font >= LEN_TFM_TBL) {
                warning("Font index out of range\n");
                return 0;
        }


    	rv = width / tfm_tbl[font].scaled_thinspace;

        if (rv >= 2) {
                rv = width / tfm_tbl[font].scaled_usequad;
                if (rv < 1) rv = 1;
        }

        pmesg(
	    80,
	    "W_TO_SPACE: thinspace = %li, usequad = %li"
	    "  width = %li, return value = %li\n",
             tfm_tbl[font].scaled_thinspace,
	     tfm_tbl[font].scaled_usequad,
	     width,
	     rv
	);

        return rv;
}

