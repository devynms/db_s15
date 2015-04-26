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

/*
 * For a thorough description of the DVI file format, see the dvitype literate
 * program as distributed e.g. with teTeX and
 * "The DVI Driver Standard, Level 0" available from your neighbourhood CTAN
 * site.
 */

#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "fntenc.h"
#include "fontinfo.h"
#include "outenc.h"
#include "page.h"
#include "readdvi.h"
#include "regsta.h"
#include "util.h"

uint32 magnification = 1000;

#define COMMENT_MAXLEN 16384


#define DVI_set_char_0		  0
#define DVI_set_char_127	127
#define DVI_set1		128
#define DVI_set2		129
#define DVI_set3		130
#define DVI_set4		131
#define DVI_set_rule		132
#define DVI_put1		133
#define DVI_put2		134
#define DVI_put3		135
#define DVI_put4		136
#define DVI_put_rule		137
#define DVI_nop			138
#define DVI_bop			139
#define DVI_eop			140
#define DVI_push		141
#define DVI_pop			142
#define DVI_right1		143
#define DVI_right2		144
#define DVI_right3		145
#define DVI_right4		146
#define DVI_w0			147
#define DVI_w1			148
#define DVI_w2			149
#define DVI_w3			150
#define DVI_w4			151
#define DVI_x0			152
#define DVI_x1			153
#define DVI_x2			154
#define DVI_x3			155
#define DVI_x4			156
#define DVI_down1		157
#define DVI_down2		158
#define DVI_down3		159
#define DVI_down4		160
#define DVI_y0			161
#define DVI_y1			162
#define DVI_y2			163
#define DVI_y3			164
#define DVI_y4			165
#define DVI_z0			166
#define DVI_z1			167
#define DVI_z2			168
#define DVI_z3			169
#define DVI_z4			170
#define DVI_fnt_num_0		171
#define DVI_fnt_num_1		172
#define DVI_fnt_num_63		234
#define DVI_fnt1		235
#define DVI_fnt2		236
#define DVI_fnt3		237
#define DVI_fnt4		238
#define DVI_xxx1		239
#define DVI_xxx2		240
#define DVI_xxx3		241
#define DVI_xxx4		242
#define DVI_fnt_def1		243
#define DVI_fnt_def2		244
#define DVI_fnt_def3		245
#define DVI_fnt_def4		246
#define DVI_pre			247
#define DVI_post		248
#define DVI_post_post		249


static double conversion = 1; /* conversion factor DVI units --> 10^-7 m */

static void output_glyph(sint32 font, sint32 glyph)
{
        static sint32 currfont = -1;
        static int currfenc = -1;
	static fix_word_t last_axis_height = double2fw(0.25);
	static sint32 curr_math_axis;

        if (font != currfont) {
	    fix_word_t new_axis_height;
	    
            currfont = font;
            currfenc = find_fntenc(font_enc(font), font_family(font));
	    new_axis_height = font_axis_height(font);

	    if(new_axis_height > 0) {
		/* This font really has axis_height defined */
		last_axis_height = new_axis_height;
		curr_math_axis = font_scale_fw(font, new_axis_height);
	    }
	    else {
		/* Since the axis height is usually about constant in a
		 * whole family of fonts, we have a good chance that
		 * scaling up the last known one to the current font
		 * size works. But we make the value negative to mark
		 * it as bogus.
		 */
		curr_math_axis = - font_scale_fw(font, last_axis_height);
    	    }
        }

        dump_regs(86);
        pmesg(85, "OUTPUT_GLYPH: font %li, glyph %li, H %li, V %li\n",
              font, glyph, get_reg(REGISTER_H), get_reg(REGISTER_V));

        page_set_glyph(font, fnt_convert(currfenc, glyph),
                       font_char_width(currfont, glyph),
                       font_char_height(currfont, glyph),
                       font_char_depth(currfont, glyph),
		       curr_math_axis,
                       get_reg(REGISTER_H), get_reg(REGISTER_V));
}

void process_file(FILE *f) {
        byte b;
        sint32 sc;
        uint32 num, den, c;
        sint32 currfont = 0;
        char comment[COMMENT_MAXLEN+1];

        /* Process the preamble */
        efread(&b, 1, 1, f, "Could not read initial byte\n");
        if (b != DVI_pre)
                panic("Does not start with preamble\n");
        pmesg(100, "Read preamble byte\n");

        efread(&b, 1, 1, f, "Could not read second byte\n");
        if (b != 2)
                panic("Identification byte wrong\n");
        pmesg(100, "Read identification byte\n");

        num = u_readbigendiannumber(4, f);
        pmesg(80, "Read numerator: %li\n", num);

        den = u_readbigendiannumber(4, f);
        pmesg(80, "Read denominator: %li\n", den);

        conversion = ((double) num) / ((double) den);

        magnification = s_readbigendiannumber(4, f);
        pmesg(80, "Read magnification: %li\n", magnification);

        efread(&b, 1, 1, f, "Could not read comment length\n");
        pmesg(100, "Read comment length: %i\n", b);
        efread(comment, 1, b, f, "Could not read comment\n");
        comment[b+1] = '\0';
        pmesg(80, "Read comment: `%s'\n", comment);

        /* Core processing loop: read an opcode byte, and process its
         * arguments (if any) */
        while(1) {
                efread(&b, 1, 1, f, "Failed to read opcode byte\n");

                if (b == DVI_post)
                       break;

                if ((b >= DVI_set_char_0) && (b <= DVI_set_char_127)) {
                        c = b;
                        pmesg(80, "[set char %i: %c]\n", b-DVI_set_char_0, (unsigned char) (b-DVI_set_char_0));
                        output_glyph(currfont, b-DVI_set_char_0);
                        add_reg(REGISTER_H, font_char_width(currfont, b-DVI_set_char_0));
                        continue;
                }

                if ((b >= DVI_set1) && (b <= DVI_set3)) {
                        sc = u_readbigendiannumber(b-DVI_set1+1, f);
                        output_glyph(currfont, sc);
                        add_reg(REGISTER_H, font_char_width(currfont, sc));
                        pmesg(80, "[set char%i: %lX]\n", b-DVI_set1+1, c);
                        continue;
                }

                if (b == DVI_set4) {
                        sc = u_readbigendiannumber(b-DVI_set1+1, f);
                        output_glyph(currfont, sc);
                        add_reg(REGISTER_H, font_char_width(currfont, sc));
                        pmesg(80, "[set char%i: %lX]\n", b-DVI_set1+1, sc);
                        continue;
                }

                if (b == DVI_set_rule) {
	    	    	sint32 rule_width, rule_height;
		    	rule_height = s_readbigendiannumber(4, f);
		    	rule_width = s_readbigendiannumber(4, f);
			add_reg(REGISTER_H, rule_width);
                        pmesg(
			    	80,
			    	"[setrule: height=%li, width=%li]\n",
			    	rule_height,
			    	rule_width
			);
                        continue;
                }

                if ((b >= DVI_put1) && (b <= DVI_put4)) {
                        c = u_readbigendiannumber(b-DVI_put1+1, f);
                        pmesg(80, "[put char%i: %lX]\n", b-DVI_put1+1, c);
                        continue;
                }

                if (b == DVI_put_rule) {
	    	    	sint32 rule_width, rule_height;
		    	rule_height = s_readbigendiannumber(4, f);
		    	rule_width = s_readbigendiannumber(4, f);
                        pmesg(
			    	80,
			    	"[putrule: height=%li, width=%li]\n",
			    	rule_height,
			    	rule_width
			);
                        continue;
                }

                if (b == DVI_nop) {
                        pmesg(80, "[nop]\n");
                        continue;
                }

                if (b == DVI_bop) {
		    	sint32 count0;

			count0 = s_readbigendiannumber(4, f);
                        skipbytes(9*4+4, f);
                        pmesg(80, "[bop]\n");
                        init_regs_stack(100);
                        page_begin(count0);
                        continue;
                }

                if (b == DVI_eop) {
                        pmesg(80, "[eop]\n");
                        page_end();
                        continue;
                }

                if (b == DVI_push) {
                        pmesg(80, "[push]\n");
                        push_regs();
                        continue;
                }

                if (b == DVI_pop) {
                        pmesg(80, "[pop]\n");
                        pop_regs();
                        continue;
                }

                if ((b >= DVI_right1) && (b <= DVI_right4)) {
                        sint32 qwerty;
                        qwerty = s_readbigendiannumber(b-DVI_right1+1, f);
                        pmesg(80, "[right%i: %li]\n", b-DVI_right1+1, qwerty);
                        add_reg(REGISTER_H, qwerty);
                        continue;
                }

                if (b == DVI_w0) {
                        add_reg(REGISTER_H, get_reg(REGISTER_W));
                        pmesg(80, "[w0]\n");
                        continue;
                }

                if ((b >= DVI_w1) && (b <= DVI_w4)) {
                        set_reg(REGISTER_W,
                                s_readbigendiannumber(b-DVI_w1+1, f));
                        pmesg(80, "[w%i: w=%li]\n",
                              b-DVI_w1+1, get_reg(REGISTER_W));
                        add_reg(REGISTER_H, get_reg(REGISTER_W));
                        continue;
                }

                if (b == DVI_x0) {
                        pmesg(80, "[x0]\n");
                        add_reg(REGISTER_H, get_reg(REGISTER_X));
                        continue;
                }

                if ((b >= DVI_x1) && (b <= DVI_x4)) {
                        set_reg(REGISTER_X,
                                s_readbigendiannumber(b-DVI_x1+1, f));
                        pmesg(80, "[x%i: x=%li]\n",
                              b-DVI_x1+1, get_reg(REGISTER_X));
                        add_reg(REGISTER_H, get_reg(REGISTER_X));
                        continue;
                }

                if ((b >= DVI_down1) && (b <= DVI_down4)) {
                        sint32 qwerty;
                        qwerty = s_readbigendiannumber(b-DVI_down1+1, f);
                        pmesg(80, "[down%i, %li]\n", b-DVI_down1+1, qwerty);
                        add_reg(REGISTER_V, qwerty);
                        continue;
                }

                if (b == DVI_y0) {
                        pmesg(80, "[y0]\n");
                        add_reg(REGISTER_V, get_reg(REGISTER_Y));
                        continue;
                }

                if ((b >= DVI_y1) && (b <= DVI_y4)) {
                        set_reg(REGISTER_Y,
                                s_readbigendiannumber(b-DVI_y1+1, f));
                        pmesg(80, "[y%i: y=%li]\n",
                              b-DVI_y1+1, get_reg(REGISTER_Y));
                        add_reg(REGISTER_V, get_reg(REGISTER_Y));
                        continue;
                }

                if (b == DVI_z0) {
                        add_reg(REGISTER_V, get_reg(REGISTER_Z));                        
                        pmesg(80, "[z0]\n");
                        continue;
                }

                if ((b >= DVI_z1) && (b <= DVI_z4)) {
                        set_reg(REGISTER_Z,
                                s_readbigendiannumber(b-DVI_z1+1, f));
                        pmesg(80, "[z%i: z=%li]\n",
                              b-DVI_z1+1, get_reg(REGISTER_Z));
                        add_reg(REGISTER_V, get_reg(REGISTER_Z));
                        continue;
                }

                if ((b >= DVI_fnt_num_0) && (b <= DVI_fnt_num_63)) {
                        pmesg(80, "[fnt_num_%i]\n", b-DVI_fnt_num_0);
                        currfont = b-DVI_fnt_num_0;
                        continue;
                }

                if ((b >= DVI_fnt1) && (b <= DVI_fnt3)) {
                        pmesg(80, "[fnt%i]\n", b-DVI_fnt1+1);
                        currfont = u_readbigendiannumber(b-DVI_fnt1+1, f);
                        continue;
                }

                if (b == DVI_fnt4) {
                        pmesg(80, "[fnt4]\n");
                        currfont = s_readbigendiannumber(4, f);
                        continue;
                }

                if ((b >= DVI_xxx1) && (b <= DVI_xxx4)) {
                        pmesg(80, "[xxx%i]\n", b-DVI_xxx1+1);
                        c = u_readbigendiannumber(b-DVI_xxx1+1, f);
                        if (c > COMMENT_MAXLEN) {
                                panic("too long comment\n");
                        }
                        efread(comment, 1, c, f, "Could not read comment\n");
                        comment[c+1] = '\0';
                        pmesg(50, "[Comment: `%s']\n", comment);
#if 0
                        skipbytes(c, f);
#endif
                        continue;
                }

                if ((b >= DVI_fnt_def1) && (b <= DVI_fnt_def4)) {
                        sint32 k;
                        uint32 cs, s, d;
                        byte a, l;
                        char * n;

                        if (b == DVI_fnt_def4) {
                                k = s_readbigendiannumber(4, f);
                        } else {
                                k = u_readbigendiannumber(b-DVI_fnt_def1+1, f);
                        }
                        cs = u_readbigendiannumber(4, f);
                        s = u_readbigendiannumber(4, f);
                        d = u_readbigendiannumber(4, f);
                        a = u_readbigendiannumber(1, f);
                        l = u_readbigendiannumber(1, f);

                        n = malloc(a + l + 1);
                        if (n == 0) enomem();

                        efread(n, a + l, 1, f, "Failed to read font name");
                        n[a+l] = 0;

                        pmesg(80, "[fnt_def%i: %s]\n", b-DVI_fnt_def1+1, n);
                        font_def(k, cs, s, d, a, l, n);
                        free(n);
                        continue;
                }

                if (b == DVI_pre)
                        panic("Unexpected preamble start marker\n");

                if (b == DVI_post)
                        break;

                if (b == DVI_post_post)
                        panic("Unexpected postamble end marker\n");

                if (b > DVI_post_post)
                        panic("Unimplemented opcode %i\n", b);
        }

        /* FIXME: process postamble */
}

