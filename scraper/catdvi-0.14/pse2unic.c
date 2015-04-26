/* catdvi - get text from DVI files
   Copyright (C) 1999, 2000 Antti-Juhani Kaijanaho <gaia@iki.fi>
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

#define TOK_MAXLEN 256
#define TBL_LEN    256

static int lineno = 1;
static int lpp;     	/* table rows per page in the report */

typedef unsigned long uint32;

struct adobe2unicode_t {
        char const * adobe;
        uint32 unicode;
};
#include "adobetbl.h"

/* For each encoding, we want to present a sample font with the respective
 * encoding in the report. Therefore, we have to know:
 * - what we have to put into the preamble to use the sample font 
 * - a LaTeX command to activate the sample font 
 * - an appropriate number of lines per page for the report (some fonts tend
 *   to be large).
 * We have reasonable defaults for unlisted encodings.
 */
static struct enc_sample_t {
    char * enc_name;
    char * preamble_cmd;
    char * latex_cmd;
    int lpp;
} enc_samples[] = {    
    {"OT1Encoding", "", "\\usefont{OT1}{cmr}{m}{n}", 32},
    {"OT1woflEncoding", "", "\\usefont{OT1}{cmr}{m}{sc}", 32},
    {"CorkEncoding", "", "\\usefont{T1}{cmr}{m}{n}", 32},
    {"TeXMathItalicEncoding", "", "\\usefont{OML}{cmm}{m}{it}", 32},
    {"TeXMathSymbolEncoding", "", "\\usefont{OMS}{cmsy}{m}{n}", 32},
    {"TeXMathExtensionEncoding", "", "\\usefont{OMX}{cmex}{m}{n}", 16},
    {"TeXTypewriterTextEncoding", "", "\\usefont{OT1}{cmtt}{m}{n}", 32},
    {
    	"EurosymEncoding",
	"\\usepackage{eurosym}",
	"\\usefont{U}{eurosym}{m}{n}",
	32
    },
    {
    	"TextCompanionEncoding",
	"\\usepackage{textcomp}",
	"\\usefont{TS1}{cmr}{m}{n}",
	32
    },
    {
    	"Marvosym1998Encoding",
	"\\usepackage{marvosym}",
	"\\usefont{OT1}{mvs}{m}{n}",
	32
    },
    {
    	"Marvosym2000Encoding",
	"\\usepackage{marvosym}",
	"\\usefont{OT1}{mvs}{m}{n}",
	32
    },
    {
    	"LaTeXSymbolsEncoding",
	"\\usepackage{latexsym}",
	"\\usefont{U}{lasy}{m}{n}",
	32
    },
    {
    	"BlackboardEncoding",
	"\\usepackage{bbm}",
	"\\usefont{U}{bbm}{m}{n}",
	32
    },
    {
    	"EulerFrakturEncoding",
	"\\usepackage{amssymb}",
	"\\usefont{U}{euf}{m}{n}",
	32
    },
    {
    	"AMSSymbolsAEncoding",
	"\\usepackage{amssymb}",
	"\\usefont{U}{msa}{m}{n}",
	32
    },
    {
    	"AMSSymbolsBEncoding",
	"\\usepackage{amssymb}",
	"\\usefont{U}{msb}{m}{n}",
	32
    },
    {
    	"EulerExtensionEncoding",
	"\\usepackage{amssymb}",
	"\\usefont{U}{euex}{m}{n}",
	32
    },
    /* END OF TABLE */
    {NULL, NULL, NULL, 32}
};

/* Make extensive mixture of fputs and fprintf slightly more readable */
#define froputs(f, s) fputs(s, f)


static char enc_name[TOK_MAXLEN];  /* name of the current encoding */

static int my_getc(FILE * fp)
{
        int c;

        c = getc(fp);

        if (c == '\n') lineno++;

        return c;
}

static void my_ungetc(int c, FILE * fp)
{
        if (c == '\n') lineno--;

        ungetc(c, fp);
}

/* Read a token from a PS ENC file */
static unsigned char const * get_token(FILE * fp, char const * fname)
{
        int c;
        unsigned char ch;
        static unsigned char tok[TOK_MAXLEN];
        size_t i = 0;

        /* Ignore leading whitespace  & comments. */
        do {
                c = my_getc(fp); ch = c;
                if (c == EOF) return (unsigned char const *) "";

                /* Ignore comment. */
                if (ch == '%') {
                        do {
                                c = my_getc(fp); ch = c;
                                if (c == EOF) return (unsigned char const *) "";
                        } while (ch != '\n');
                }
                
        } while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

        my_ungetc(c, fp);

        while (1) {
                c = my_getc(fp); ch = c;

                if (ch == ' ' || ch == '\t' || ch == '\r'
                    || ch == '\n' || c == EOF) {
                        tok[i] = 0;
                        return tok;
                }

                if (i == TOK_MAXLEN) {
                        panic("token too long while reading %s\n", fname);
                }

                tok[i++] = c;
        }
        NOTREACHED;
}

static int a2u_cmp(void const * av, void const * bv)
{
        struct adobe2unicode_t const * a = av;
        struct adobe2unicode_t const * b = bv;

        return strcmp(a->adobe, b->adobe);
}

/* Warn about some easily made mistakes in adobetbl.h */
static void check_adobetbl(void)
{
    struct adobe2unicode_t * p, * last;
    int cmp;

    last = adobe2unicode + lengthof(adobe2unicode) - 1;
    for(p = adobe2unicode; p < last; ++p) {

    	cmp = a2u_cmp(p, p+1);
	if(cmp == 0) {
	    warning("duplicate entry `%s' in adobetbl.h\n", p->adobe);
	}
	else if(cmp > 0) {
	    warning(
	    	"adobetbl.h not sorted: `%s' before `%s'\n",
		p->adobe,
		(p+1)->adobe
	    );
	}

    }
}

		
/* Look a name up in the translation table. */
static uint32 lookup(unsigned char const * name, char const * fname)
{
        struct adobe2unicode_t key;
        struct adobe2unicode_t * datum;

        key.adobe = (char const *) name;

        datum = bsearch(&key, adobe2unicode,
                        sizeof(adobe2unicode) / sizeof(adobe2unicode[0]),
                        sizeof(adobe2unicode[0]),
                        a2u_cmp);

        if (datum == 0) {
                warning("%s:%i: Unknown Adobe glyph name `%s'\n",
                        fname, lineno, name);
                return 0;
        }

        return datum->unicode;
}

/* Write the conversion report header */
static void report_header(FILE * rp)
{
    time_t now;
    struct enc_sample_t * p;
    char * cmd;
    
    time(&now);

    /* find the LaTeX command for the right sample font */
    for(p = enc_samples; p->enc_name != NULL; ++p) {
    	if(strcmp(p->enc_name, enc_name) == 0) break;
    }
    cmd = p->latex_cmd;
    lpp = p->lpp;

    froputs(rp, "% AUTOMATICALLY GENERATED - DO NOT EDIT.\n");
    froputs(rp, "\n");
    froputs(rp, "\\documentclass{article}\n"); 
    froputs(rp, "\n");
    froputs(rp, "\\title{\n");
    froputs(rp, "    \\texttt{catdvi}'s Mapping of the\n");
    fprintf(rp, "    `%s' Font~Encoding to Unicode\n", enc_name);
    froputs(rp, "}\n");
    froputs(rp, "\\author{\\texttt{pse2unic}}\n");
    fprintf(rp, "\\date{%s}\n", ctime(&now));
    froputs(rp, "\n");
    if(p->preamble_cmd != NULL) fprintf(rp, "%s\n", p->preamble_cmd);
    froputs(rp, "\n");
    if(cmd != NULL) fprintf(
    	rp,
    	"\\newcommand{\\sample}[1]{{%s\\symbol{#1}}}\n",
	cmd
    );
    else froputs(rp, "\\newcommand{\\sample}[1]{N/A}\n");
    froputs(rp, "\n");
    froputs(rp, "\\begin{document}\n");
    froputs(rp, "\n");
    froputs(rp, "\\maketitle\n");
    froputs(rp, "\n");
}

/* Print one conversion table line in the report */
static void report_line(
    FILE * rp,
    int code,
    unsigned char const * name,
    uint32 unicode
)
{    
    if(code % lpp == 0) {
    	froputs(rp, "\\begin{tabular}{r|l|l|r}\n");
	froputs(rp, "code point & sample shape & internal name & Unicode\\\\\n");
    	froputs(rp, "\\hline\n");
    }

    fprintf(
    	rp,
	"\\texttt{%#04x} & \\sample{%d} & \\verb/%s/ & ",
	code,
	code,
	name
    );
    if(unicode != 0) {
	fprintf(rp, "\\texttt{U+%04lX}\\\\\n", unicode);
    }
    else {
	froputs(rp, "N/A\\\\\n");
    }
    froputs(rp, "\\hline\n");
    
    if((code % lpp == lpp - 1) || (code == TBL_LEN - 1)) {
    	froputs(rp, "\\end{tabular}\n");
    	froputs(rp, "\n\\newpage\n");
    }
}

/* Dump the conversion report footer */
static void report_footer(FILE * rp)
{
    froputs(rp, "\n");
    froputs(rp, "\\end{document}\n");
}

/* Parse a PS ENC file named fname. 
 * Return a statically allocated table of
 * TBL_LEN uint32's indexed by input octets.
 * As a byproduct, write a (LaTeX format) report about the PS ENC file
 * to a file named rname.
 */
static uint32 * parse_enc(char const * fname, char const * rname)
{
        FILE * fp, * rp;
        unsigned char const * tok;
        static uint32 tbl[TBL_LEN];
        int i;
	uint32 u;

        fp = fopen(fname, "r");
        if (fp == 0) panic("unable to open %s: %s\n", fname, strerror(errno));
        rp = fopen(rname, "w");
        if (rp == 0) panic("Unable to open `%s' for writing: %s.\n",
                           rname, strerror(errno));

        lineno = 1;

        tok = get_token(fp, fname);
        if (tok[0] != '/') panic("expected a literal, got `%s'\n", tok);

        strcpy(enc_name, (char const *) (tok+1));

        tok = get_token(fp, fname);
        if (! (tok[0] == '[' && tok[1] == 0)) {
                panic("expected `[', got `%s'\n", tok);
        }
    	report_header(rp);

        for (i = 0; i < TBL_LEN; i++) {
                tok = get_token(fp, fname);

                if (tok[0] != '/') panic("expected a literal, got `%s'\n", tok);

                u = lookup(tok+1, fname);
		tbl[i] = u ? u : 0x003f; /* QUESTION MARK */
		report_line(rp, i, tok+1, u);
        }

        tok = get_token(fp, fname);
        if (! (tok[0] == ']' && tok[1] == 0)) {
                panic("expected `]', got `%s'\n", tok);
        }

    	report_footer(rp);
        if (fclose(rp) != 0) panic("Unable to write `%s': %s.\n",
                                   rname, strerror(errno));
        fclose(fp);
        return tbl;
}

int main(int argc, char * argv[])
{
        FILE * fp;
        uint32 * tbl;
        int i;
        char * progname = argv[0];

        msglevel = 10;

        if (argc == 5 && strcmp(argv[1], "-w") == 0) {
                suppress_warnings = 1;
                --argc;
                ++argv;
        }

        if (argc != 4) panic("Usage error!\n");

    	check_adobetbl();

        tbl = parse_enc(argv[1], argv[3]);
	

        fp = fopen(argv[2], "w");
        if (fp == 0) panic("Unable to open `%s' for writing: %s.\n",
                           argv[2], strerror(errno));
        fprintf(fp, "static sint32 %s[%i] = {\n", enc_name, TBL_LEN);
        for (i = 0; i < TBL_LEN; i++) {
                fprintf(fp, "        0x%04lx,\n", tbl[i]);
        }
        fputs("};\n", fp);

        if (fclose(fp) != 0) panic("Unable to write `%s': %s.\n",
                                   argv[2], strerror(errno));

        if (suppress_warnings && num_warnings > 0) {
                fprintf(stderr, "%s: suppressed %i warnings\n",
                        progname, num_warnings);
        }

        return 0;
}
