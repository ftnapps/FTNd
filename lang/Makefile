# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2000 by M. Broek.
# $Id: Makefile,v 1.10 2007/02/17 12:14:20 mbse Exp $

include ../Makefile.global

OTHER		= Makefile README Language.xref germandu.txt
SDAT		= source.en   source.nl   source.es   source.gl   source.de   source.fr   source.zh 
LDAT		= language.en language.nl language.es language.gl language.de language.fr language.zh 


all:		${LDAT}

clean:
		rm -f core filelist Makefile.bak language.*

install: ${LDAT}
		${INSTALL} -c -o ${OWNER} -g ${GROUP} -m 0444 ${LDAT} ${INTDIR}

language.en:	source.en
		../mbutils/mblang language.en source.en

language.nl:	source.nl
		../mbutils/mblang language.nl source.nl

language.es:	source.es
		../mbutils/mblang language.es source.es

language.gl:	source.gl
		../mbutils/mblang language.gl source.gl

language.de:	source.de
		../mbutils/mblang language.de source.de

language.fr:	source.fr
		../mbutils/mblang language.fr source.fr

language.zh:	source.zh
		../mbutils/mblang language.zh source.zh


filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${SDAT} ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:

