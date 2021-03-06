#    catdvi - get text from DVI files
#    Copyright (C) 1999 J.H.M. Dassen (Ray) <jdassen@wi.LeidenUniv.nl>
#    Copyright (C) 1999, 2000, 2001 Antti-Juhani Kaijanaho <gaia@iki.fi>
#    Copyright (C) 2000-2002 Bjoern Brill <brill@fs.math.uni-frankfurt.de>
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

#CC = gcc
#CFLAGS = -Wall -W -Wshadow -Wpointer-arith -Wbad-function-cast \
# 	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
# 	-Wmissing-declarations -ansi -pedantic -g

include config.mk

ifeq ($(CFG_HAS_GETOPT_LONG),yes)
GETOPT = # nothing
CPPFLAGS += -DCFG_HAS_GETOPT_LONG
else
ifeq ($(CFG_KPATHSEA_HAS_GETOPT_LONG),yes)
GETOPT = # nothing
CPPFLAGS += -DCFG_KPATHSEA_HAS_GETOPT_LONG
else
GETOPT = getopt.c getopt1.c
endif
endif

ifeq ($(CFG_HAS_KPATHSEA),yes)
FINDTFM = kpathsea.o
LDLIBS = -lkpathsea
else
$(error Sorry) # Implement a replacement for kpathsea.c yourself...
endif

ifeq ($(CFG_SHOW_PSE2UNIC_WARNINGS),yes)
PSE2UNICFLAGS = # nothing
else
PSE2UNICFLAGS = -w
endif

programs := catdvi pse2unic adobe2h tfminfo
tests := $(wildcard test/*.tex)
sources := $(wildcard *.c)
encfiles := $(wildcard enc/*.enc)

all1 : $(programs) $(tests:.tex=.dvi)

catdvi : bytesex.o catdvi.o fixword.o fntenc.o fontinfo.o outenc.o \
	page.o readdvi.o regsta.o util.o glyphops.o linebuf.o\
	density.o sparse.o pageref.o vlist.o canvas.o layout.o\
	$(GETOPT) $(FINDTFM)

# fontinfo : fontinfo.o bytesex.o util.o
tfminfo : tfminfo.o bytesex.o fixword.o fontinfo.o util.o fntenc.o $(FINDTFM)

pse2unic : pse2unic.o util.o

adobe2h : adobe2h.o

fntenc.o fntenc.d : $(encfiles:.enc=.h)

glyphops.o glyphops.d : glyphenm.h

glyphenm.h : adobe2h
	./adobe2h > glyphenm.h

config.mk : config.mk.in
	./config.status

ChangeLog :
	CVS_RSH=ssh $(CVS2CL) -U changelog-authors -W 1200 -b --gmt -w --ignore ChangeLog

.PHONY : ChangeLog

clean :
	$(RM) *.o $(programs) core tags test/*.log test/*.aux
	$(RM) enc/*.log enc/*.aux
	find . -name '*~' -or -name '*.bck' | xargs $(RM)

distclean : clean
	$(RM) *.d test/*.dvi test/*.log test/*.aux 1
	$(RM) enc/*.dvi enc/*.tex
	$(RM) glyphenm.h $(encfiles:.enc=.h)
	$(RM) config.cache config.status config.mk version.h config.log
	$(RM) -r $(package)-$(version)
	$(RM) -r test/auto

maintainer-clean : distclean
	$(RM) test/*.dvi configure ChangeLog

%.d: %.c
	set -e; $(CC) -MM $(CPPFLAGS) $< | \
	  sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	  [ -s $@ ] || rm -f $@

test/%.dvi : test/%.tex
	cd test && latex $(notdir $<)

enc/%.h enc/%.tex : enc/%.enc pse2unic
	./pse2unic $(PSE2UNICFLAGS) $< $(<:.enc=.h) $(<:.enc=.tex)

# autogenerated documentation
#
enc/%.dvi : enc/%.tex
	cd enc && latex $(notdir $<)

encreports : $(encfiles:.enc=.dvi)

catdvi.txt : catdvi.1
	man -l catdvi.1 -Tascii | col -b -x > catdvi.txt

catdvi.html: catdvi.1
	troff2html -man catdvi.1 > catdvi.html

# release packaging
#
dist : distdir=$(package)-$(version)
dist : $(sources:.c=.d) ChangeLog
	rm -rf $(distdir)
	$(RM) $(distdir).tar
	$(RM) $(distdir).tar.bz2
	$(RM) $(distdir).tar.gz
	mkdir $(distdir)
	set -e ; for d in $(shell cat Manifest | xargs -l1 dirname | sort | uniq) ;\
	do \
	    echo -n "Making directory $(distdir)/$$d ..." ;\
	    mkdir -p $(distdir)/$$d ;\
	    echo "done." ;\
	done
	set -e ; for f in $(shell cat Manifest);\
	do \
	    echo -n "Copying $$f ..." ;\
	    ln $$f $(distdir)/$$f ;\
	    echo "done." ;\
	done
	tar cf $(distdir).tar $(distdir)
	bzip2 -9k $(distdir).tar
	gzip -9 $(distdir).tar

install: catdvi
	$(INSTALL_PROGRAM) catdvi $(bindir)/catdvi
	$(INSTALL_DATA) catdvi.1 $(man1dir)/catdvi.1

uninstall:
	$(RM) $(bindir)/catdvi
	$(RM) $(man1dir)/catdvi.1

# For fun ;-)
linecount :
	rmccmt $(filter-out getopt.c getopt.h getopt1.c adobetbl.h, \
	        $(wildcard *.c *.h)) | indent | uniq | wc -l

Manifest:
	rm -f $@.new $@.edi
	./cvsparser.pl | ./manifest-filter.pl > $@.edi
	sort $@.edi -o $@.new
	mv $@.new $@
	rm -f $@.edi

.PHONY: Manifest MD5SUMS

SIGN=gpg --clearsign
MD5SUMS: Manifest
	rm -f $@.new
	set -e; for f in `egrep -v '^$@$$' Manifest `;\
	     do \
	         sum=$$(md5sum $$f | awk '{ print $$1; }'); \
	         size=$$(ls -l $$f | awk '{ print $$5; }'); \
	         printf "%s %5i %s\n" $$sum $$size $$f >> $@.new;\
	     done
	if [ "$(NOSIGN)" = "" ] ;\
	then\
		$(SIGN) $@.new ;\
		rm -f $@.new ;\
		mv $@.new.asc $@ ;\
	else\
		mv $@.new $@ ;\
	fi

CVSTAG=none
official-release : VERSION=$(shell echo $(CVSTAG) \
			           | sed -e 's/^rel_//' -e 's/_/./g')
official-release : grbdir=catdvi-release-build
official-release : 
	rm -rf $(grbdir)
	set -e ;\
	  if $$(echo $(VERSION) \
	        | egrep '^[0-9]+\.[0-9]+(\.(pre)?[0-9]+)?$$' >/dev/null);\
	  then\
	      :; \
	  else \
	      echo "Bad VERSION"; \
	      exit 1; \
	  fi
	cvs -z3 -d$(shell cat CVS/Root) checkout -r $(CVSTAG) \
	                                         -d $(grbdir) catdvi
	cd $(grbdir) && autoconf && ./configure
	make -C $(grbdir) Manifest
	make -C $(grbdir) `egrep -v '^(Manifest|MD5SUMS)$$' $(grbdir)/Manifest`
	make -C $(grbdir) -o Manifest NOSIGN=$(NOSIGN) MD5SUMS
	make -C $(grbdir) -o ChangeLog dist
	ln -f $(grbdir)/catdvi-$(VERSION).tar.gz
	ln -f $(grbdir)/catdvi-$(VERSION).tar.bz2
	rm -rf $(grbdir)


include $(sources:.c=.d)

