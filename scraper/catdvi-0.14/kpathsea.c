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

#include <kpathsea/kpathsea.h>
#include <stdio.h>
#include "findtfm.h"
#include "version.h"

void setup_findtfm(char const * progname)
{
        kpse_set_program_name(progname, PACKAGE);
        kpse_init_format(kpse_tfm_format);
        kpse_set_program_enabled(kpse_tfm_format, 1, kpse_src_cmdline);
}

void copyright_findtfm(void)
{
        /* This is for kpathsea 3.2 */
        puts("Copyright Karl Berry, Olaf Weber and others (kpathsea library)");
}

void version_findtfm(void)
{
        extern char * kpathsea_version_string;
        puts(kpathsea_version_string);
}

char const * find_tfm(char const * fontname)
{
        return kpse_find_tfm(fontname);
}
