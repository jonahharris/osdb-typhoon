# Makefile for:  man - manual pages for typhoon

ROFF		= nroff
RFLAGS		= -man
MANEXT		= l
PREFIX		= /usr/local
DESTMAN		= $(PREFIX)/man/man$(MANEXT)
DESTCAT		= $(PREFIX)/man/cat$(MANEXT)
DESTOWN		= root
DESTGRP		= local
SHELL		= /bin/sh
MANPAGES	= d_close.3 d_crget.3 d_crread.3 d_crset.3 d_dbdpath.3 \
		  d_dbfpath.3 d_dbget.3 d_dbset.3 d_delete.3 d_fillnew.3 \
		  d_keyfind.3 d_keyfrst.3 d_keylast.3 d_keynext.3 d_keyprev.3 \
		  d_keyread.3 d_open.3 d_recfrst.3 d_reclast.3 d_recnext.3 \
		  d_recprev.3 d_recread.3 d_recwrite.3 d_setfiles.3 ddlp.1 \
		  d_getsequence.3
CATPAGES	= d_close.cat d_crget.cat d_crread.cat d_crset.cat \
		  d_dbdpath.cat d_dbfpath.cat d_dbget.cat d_dbset.cat \
		  d_delete.cat d_fillnew.cat d_keyfind.cat d_keyfrst.cat \
		  d_keylast.cat d_keynext.cat d_keyprev.cat d_keyread.cat \
		  d_open.cat d_recfrst.cat d_reclast.cat d_recnext.cat \
		  d_recprev.cat d_recread.cat d_recwrite.cat d_setfiles.cat \
		  d_getsequence.cat ddlp.cat

.DEFAULT:
		co $@

.PHONY:		all install uninstall clean

.3.cat:
		$(ROFF) $(RFLAGS) $< | col > $@

.SUFFIXES:	.cat .3

all:		$(CATPAGES)

$(CATPAGES):	$(MANPAGES)

install:	$(CATPAGES)
		-for manpage in $(MANPAGES); do \
			basepage=`expr $$manpage : '\(.*\)\.[^.]*'`; \
:			cp $$manpage $(DESTMAN)/$$basepage.$(MANEXT); \
:			chmod 644 $(DESTMAN)/$$basepage.$(MANEXT); \
:			chown $(DESTOWN) $(DESTMAN)/$$basepage.$(MANEXT); \
:			chgrp $(DESTGRP) $(DESTMAN)/$$basepage.$(MANEXT); \
			compress -c $$basepage.cat \
				>$(DESTCAT)/$$basepage.$(MANEXT).Z; \
			chmod 644 $(DESTCAT)/$$basepage.$(MANEXT)*; \
			chown $(DESTOWN) $(DESTCAT)/$$basepage.$(MANEXT)*; \
			chgrp $(DESTGRP) $(DESTCAT)/$$basepage.$(MANEXT)*; \
		done

uninstall:
		-for manpage in $(MANPAGES); do \
			basepage=`expr $$manpage : '\(.*\)\.[^.]*'`; \
			rm -f $(DESTMAN)/$$basepage.$(MANEXT)*; \
			rm -f $(DESTCAT)/$$basepage.$(MANEXT)*; \
		done

clean:
		-rm -f $(CATPAGES)

distclean:	clean

### Do NOT edit this or the following lines.
