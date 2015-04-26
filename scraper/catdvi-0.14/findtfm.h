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

#ifndef FINDTFM_H
#define FINDTFM_H

/* Call this first, and pass argv[0] as progname. */
void setup_findtfm(char const * progname);

/* This prints out the copyright and information on the code that is
   used to look up the TFM files. */
void copyright_findtfm(void);
void version_findtfm(void);

/* Find the name of the TFM file corresponding to the font named. */
char const * find_tfm(char const * fontname);


#endif /* FINDTFM_H */
