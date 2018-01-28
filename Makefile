# Makefile for:  typhoon - top level makefile

MANEXT		= l
PREFIX		= /usr/local
DESTMAN		= $(PREFIX)/man/man$(MANEXT)
DESTCAT		= $(PREFIX)/man/cat$(MANEXT)
DESTOWN		= root
DESTGRP		= local
SHELL		= /bin/sh
MAKE		= make

.PHONY:		all install uninstall clean distclean

all install uninstall: include/ansi.h include/environ.h
		cd src; $(MAKE) $@
		cd util; $(MAKE) $@
		cd examples; $(MAKE) $@
		cd man; $(MAKE) $@

include/ansi.h include/environ.h:
		./configure

clean:
		cd src; $(MAKE) $@
		cd util; $(MAKE) $@
		cd examples; $(MAKE) $@
		cd man; $(MAKE) $@

distclean:	clean
		-rm -f include/ansi.h include/environ.h
		cd src; $(MAKE) $@
		cd util; $(MAKE) $@
		cd examples; $(MAKE) $@
		cd man; $(MAKE) $@
