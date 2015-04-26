# -*- Makefile -*-
#    catdvi - get text from DVI files
#    Copyright (C) 1999 J.H.M. Dassen (Ray) <jdassen@wi.LeidenUniv.nl>
#    Copyright (C) 1999 Antti-Juhani Kaijanaho <gaia@iki.fi>
#    Copyright (C) 2001 Bjoern Brill <brill@fs.math.uni-frankfurt.de>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

package = catdvi
version = 0.14

CFG_HAS_GETOPT_LONG = yes
CFG_HAS_KPATHSEA = yes
CFG_KPATHSEA_HAS_GETOPT_LONG = no
CFG_SHOW_PSE2UNIC_WARNINGS = no

CC = gcc
CFLAGS = -g -O2
CPPFLAGS = 
LDFLAGS = 

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
mandir = ${prefix}/share/man
man1dir = ${mandir}/man1

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)

CVS2CL = cvs2cl
