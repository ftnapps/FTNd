# Copyright (c) M. Broek, 1997-2001
# $Id$

include ../Makefile.global

OTHER		= Makefile etc.tar menus.tar txtfiles.tar

#####################################################################################

install:
		@if [ ! -f ${ETCDIR}/footer.txt ]; then \
			tar xfC etc.tar ${ETCDIR} ; \
			echo "Installing default etc files" ; \
		fi
		@if [ ! -f ${PREFIX}/english/menus/main.mnu ]; then \
			tar xfC menus.tar ${PREFIX}/english/menus ; \
			echo "Installing default english menus" ; \
		fi
		@if [ ! -f ${PREFIX}/english/txtfiles/main.ans ]; then \
			tar xfC txtfiles.tar ${PREFIX}/english/txtfiles ; \
			echo "Installing default english txtfiles" ; \
		fi

clean:
		rm -f filelist Makefile.bak

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist
all:

depend:

