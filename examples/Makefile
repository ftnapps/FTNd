# Copyright (c) M. Broek, 1997-2001
# $Id$

include ../Makefile.global

OTHER		= Makefile header.txt footer.txt menus-le.tar menus-be.tar txtfiles.tar

#####################################################################################

install:
		@if [ ! -f ${ETCDIR}/header.txt ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0644 header.txt ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0644 header.txt ${ETCDIR}" ; \
		fi
		@if [ ! -f ${ETCDIR}/footer.txt ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0644 footer.txt ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0644 footer.txt ${ETCDIR}" ; \
		fi
		@if [ ! -f ${PREFIX}/english/menus/main.mnu ]; then \
			tar xfC menus-le.tar ${PREFIX}/english/menus ; \
			echo "Installing default english menus for a Little-Endian system" ; \
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

