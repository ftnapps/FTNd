# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2001 by M. Broek.
# $Id$

include ../Makefile.global

OTHER		= Makefile README maint midnight weekly monthly installinit.sh rc rc.shutdown \
		  mbse.start mbse.stop bbsdoor.sh rundoor.sh

####################################################################################################

all:

clean:
		rm -f core filelist Makefile.bak

install:
		@if [ "`id -un`" != "root" ] ; then \
			echo; echo " Must be root to install!"; echo; exit 3; \
		fi
		@if [ ! -x ${ETCDIR}/maint ]; then \
			${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 maint       ${ETCDIR} ; \
			echo "${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 maint       ${ETCDIR}" ; \
		fi
		@if [ ! -x ${ETCDIR}/midnight ]; then \
			${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 midnight    ${ETCDIR} ; \
			echo "${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 midnight    ${ETCDIR}" ; \
		fi
		@if [ ! -x ${ETCDIR}/weekly ]; then \
			${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 weekly      ${ETCDIR} ; \
			echo "${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 weekly      ${ETCDIR}" ; \
		fi
		@if [ ! -x ${ETCDIR}/monthly ]; then \
			${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 monthly     ${ETCDIR} ; \
			echo "${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0711 monthly     ${ETCDIR}" ; \
		fi
		${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0755 bbsdoor.sh  ${BINDIR}
		${INSTALL} -o ${OWNER} -g ${GROUP}  -m 0755 rundoor.sh  ${BINDIR}
		@bash ./installinit.sh

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:

