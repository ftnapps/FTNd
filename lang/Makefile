# Makefile for FTNd under Linux
# Copyright (c) 2012 by Robert James Clay.
# Copyright (c) 1998, 2000 by M. Broek.

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
		../ftnutils/ftnlang language.en source.en

language.nl:	source.nl
		../ftnutils/ftnlang language.nl source.nl

language.es:	source.es
		../ftnutils/ftnlang language.es source.es

language.gl:	source.gl
		../ftnutils/ftnlang language.gl source.gl

language.de:	source.de
		../ftnutils/ftnlang language.de source.de

language.fr:	source.fr
		../ftnutils/ftnlang language.fr source.fr

language.zh:	source.zh
		../ftnutils/ftnlang language.zh source.zh


filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${SDAT} ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:

