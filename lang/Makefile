# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2000 by M. Broek.
# $Id$

include ../Makefile.global

OTHER		= Makefile README Language.xref
SDAT		= english.txt  dutch.txt  italian.txt  spanish.txt  galego.txt  german.txt  germandu.txt
LDAT		= english.lang dutch.lang italian.lang spanish.lang galego.lang german.lang germandu.lang

all:		${LDAT}

clean:
		rm -f core filelist Makefile.bak *.lang

install: ${LDAT}
		${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0444 ${LDAT} ${ETCDIR}

english.lang:	english.txt
		../mbsebbs/mblang english.lang english.txt

dutch.lang:	dutch.txt
		../mbsebbs/mblang dutch.lang dutch.txt

italian.lang:	italian.txt
		../mbsebbs/mblang italian.lang italian.txt

spanish.lang:	spanish.txt
		../mbsebbs/mblang spanish.lang spanish.txt

galego.lang:	galego.txt
		../mbsebbs/mblang galego.lang galego.txt

german.lang:	german.txt
		../mbsebbs/mblang german.lang german.txt

germandu.lang:	germandu.txt
		../mbsebbs/mblang germandu.lang germandu.txt

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${SDAT} ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:

