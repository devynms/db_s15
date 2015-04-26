/* catdvi - get text from DVI files
   Copyright (C) 1999 J.H.M. Dassen (Ray) <jdassen@wi.LeidenUniv.nl>
   Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
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

#if defined CFG_HAS_GETOPT_LONG
#   include <getopt.h>
#elif defined CFG_KPATHSEA_HAS_GETOPT_LONG
#   include <kpathsea/getopt.h>
#else
#   include "getopt.h"
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "findtfm.h"
#include "readdvi.h"
#include "outenc.h"
#include "util.h"
#include "version.h"
#include "page.h"
#include "glyphops.h"

#define OPT_COPYRIGHT (UCHAR_MAX + 1)
#define OPT_VERSION (OPT_COPYRIGHT + 1)

static void print_encodings(void)
{
        enum outenc_num_t i;

        for (i = 0; i < OE_TOOBIG; i++) {
                printf("     %2i. %s\n", i, outenc_names[i]);
        }
}

int main(int argc, char *argv[]) 
{
        FILE *dvifile = NULL;
        static struct option lopt[] = {
                { "copyright",          no_argument,       0, OPT_COPYRIGHT },
                { "version",            no_argument,       0, OPT_VERSION   },
                { "help",               no_argument,       0, 'h'           },
                { "output-encoding",    required_argument, 0, 'e'           },
                { "debug",              required_argument, 0, 'd'           },
                { "list-page-numbers",  no_argument,       0, 'N'           },
                { "show-unkown-glyphs", no_argument,       0, 'U'           },
    	    	{ "sequential", 	no_argument,	   0, 's'   	    },
    	    	{ "start-page", 	required_argument, 0, 'p'   	    },
    	    	{ "last-page",	    	required_argument, 0, 'l'   	    },
                { 0, 0, 0, 0 }
        };
        static char const * shopt = "d:e:hl:p:NUs";

        setlocale(LC_ALL, "");

        msglevel = 10; /* Level of verbosity for pmesg */ 

#ifdef USE_MALLOC0
	/* Simulate malloc(0) == NULL behaviour with GNU libc for debugging */
	use_malloc0();
#endif

        setup_findtfm(argv[0]);

       /* Parse options */
        while (1) {
                int opt;

                opt = getopt_long(argc, argv, shopt, lopt, 0);
                if (opt == -1) break;

                switch (opt) {
                        long arg;
                        char * suffix;

                case OPT_VERSION:
                        printf("%s %s\n", PACKAGE, VERSION);
                        version_findtfm();
                        exit(EXIT_SUCCESS);

                case 'h':
                        printf("Usage: %s [options] [file]\n",
                               argv[0] == 0 ? PACKAGE : argv[0]);
                        puts("");
                        puts("Options:");
                        puts("  -d DEBUGLEVEL, --debug=DEBUGLEVEL");
                        puts("          Set the debug output level.  Smaller is less output.");
                        puts("  -e ENCODING, --output-encoding=ENCODING");
                        puts("          Set the output encoding.");
                        puts("          (ENCODING can be a number or name "
                             "from the table below.)");
                        puts("  -p PAGESPEC, --first-page=PAGESPEC");
                        puts("          Do not output pages before page PAGESPEC.");
                        puts("          PAGESPEC is either count0, =physicalpage or chapter:count0 .");
                        puts("  -l PAGESPEC, --last-page=PAGESPEC");
                        puts("          Do not output pages after page PAGESPEC.");
                        puts("  -N , --list-page-numbers");
                        puts("          Output physical page count, count0 value and chapter count instead");
                        puts("          of page contents.");
                        puts("  -U, --show-unknown-glyphs");
                        puts("          Show the Unicode number of unknown glyphs instead of `?'.");
                        puts("  -s, --sequential");
                        puts("          Do not attempt to reproduce the page layout; output glyphs");
                        puts("          in the order they appear in the DVI file.");
                        puts("  -h, --help");
                        puts("          Show this help page.");
                        puts("  --version");
                        puts("          Show version information.");
                        puts("  --copyright");
                        puts("          Show copyright information.");
                        puts("");
                        puts("The following output encodings are available:");
                        print_encodings();
                        puts("");
                        puts("Please, report bugs to <brill@fs.math.uni-frankfurt.de>");
                        exit(EXIT_SUCCESS);

                case 'e':
                        arg = strtol(optarg, &suffix, 10);
                        if (suffix[0] != 0) {
			    /* not a number */
			    char * upoptarg;

			    upoptarg = strupcasedup(optarg);
			    if(upoptarg == NULL) enomem();

			    for(arg = 0; arg < OE_TOOBIG; ++arg) {
			    	if(!strcmp(upoptarg, outenc_names[arg])) break;
			    }
			    free(upoptarg);

                            if(arg == OE_TOOBIG) {
			        panic("Unknown encoding `%s'.\n", optarg);
			    }
                        }
                        else if (arg < 0 || arg >= OE_TOOBIG) {
                                panic("Encoding must be between %i and %i\n",
                                      0, OE_TOOBIG - 1);
                        }
                        outenc_num = arg;
                        break;
                        
                case 'd':
                        arg = strtol(optarg, &suffix, 10);
                        if (suffix[0] != 0) {
                                panic("Debug level must be a number.\n");
                        }
                        msglevel = arg;
                        break;

                case 'l': 
		    	if(!pageref_parse(&page_last_output, optarg))
			    panic("Unable to parse last page argument.\n");
                        break;

                case 'p': 
		    	if(!pageref_parse(&page_start_output, optarg))
			    panic("Unable to parse start page argument.\n");
                        break;

                case 'N':
                        page_list_numbers = 1;
                        break;
			
                case 'U':
                        outenc_show_unicode_number = 1;
                        break;
			
		case 's':
		    	page_sequential = 1;
			break;
                        
                case OPT_COPYRIGHT:
                        puts("This is " PACKAGE ".");
                        puts("");
                        puts("Copyright 1999-2001 Antti-Juhani Kaijanaho");
                        puts("Copyright 1999 J.H.M. Dassen (Ray)");
    	    	    	puts("Copyright 2000-2002 Bjoern Brill");
                        puts("Copyright 1987-1999 Free Software Foundation, Inc.");
                        copyright_findtfm();
                        puts("");
                        puts("This program is free software; you can redistribute it and/or modify");
                        puts("it under the terms of the GNU General Public License as published by");
                        puts("the Free Software Foundation; either version 2 of the License, or");
                        puts("(at your option) any later version.");
                        puts("");
                        puts("This program is distributed in the hope that it will be useful,");
                        puts("but WITHOUT ANY WARRANTY; without even the implied warranty of");
                        puts("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
                        puts("GNU General Public License for more details.");
                        puts("");
                        puts("You should have received a copy of the GNU General Public License");
                        puts("along with this program; if not, write to the Free Software");
                        puts("Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.");
                        exit(EXIT_SUCCESS);

                default:
                        exit(EXIT_FAILURE);
                }
        }

        if ((argc-optind != 0) && (argc-optind != 1)) {
                panic("Expecting zero or one arguments; got %i.\n", argc-optind);
        }

        /* No arguments, or argument `-': read from stdin */
        if ((argc-optind == 0) || ((argc-optind == 1) && !strcmp(argv[optind], "-"))) {
                dvifile = stdin;
        } else {
                dvifile = fopen(argv[optind], "rb");
                if (!dvifile)
                        panic("Attempt to open '%s' failed: %s\n", argv[optind],
                                        strerror(errno));
        }

    	glyphops_init();
    	outenc_init();
	    /* set up some internal data structures */

        process_file(dvifile);

        fclose(dvifile);

        return 0;
}
