# Makefile for MBSE BBS under Linux
# $Id: Makefile,v 1.23 2005/08/17 18:55:16 mbse Exp $

include ../Makefile.global

OTHER		= Makefile README maint midnight weekly monthly installinit.sh setup.mbse \
		  bbsdoor.sh rundoor.sh runvirtual.sh init.Debian init.RedHat init.FreeBSD init.NetBSD \
		  init.Slackware init.SuSE init.Gentoo hatch editor.in bbsjoerc bbsjmacsrc \
		  doors.bat mbsedos

####################################################################################################

all:

clean:
		rm -f core filelist Makefile.bak

install:
		@if [ "`id -un`" != "root" ] ; then \
			echo; echo " Must be root to install!"; echo; exit 3; \
		fi
		@if [ ! -x ${ETCDIR}/maint ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 maint       ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 maint       ${ETCDIR}" ; \
		else \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 maint       ${ETCDIR}/maint.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 maint       ${ETCDIR}/maint.new" ; \
		fi
		@if [ ! -x ${ETCDIR}/midnight ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 midnight    ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 midnight    ${ETCDIR}" ; \
		else \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 midnight    ${ETCDIR}/midnight.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 midnight    ${ETCDIR}/midnight.new" ; \
		fi
		@if [ ! -x ${ETCDIR}/weekly ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 weekly      ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 weekly      ${ETCDIR}" ; \
		else \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 weekly      ${ETCDIR}/weekly.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 weekly      ${ETCDIR}/weekly.new" ; \
		fi
		@if [ ! -x ${ETCDIR}/monthly ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 monthly     ${ETCDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 monthly     ${ETCDIR}" ; \
		else \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 monthly     ${ETCDIR}/monthly.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 monthly     ${ETCDIR}/monthly.new" ; \
		fi
		@if [ ! -x ${BINDIR}/hatch ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 hatch       ${BINDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 hatch       ${BINDIR}" ; \
		else  \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 hatch       ${BINDIR}/hatch.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 hatch       ${BINDIR}/hatch/new" ; \
		fi
		@if [ -x ${JOEBIN}/joe ]; then \
			echo "Installing support for joe editor" ; \
			rm -f ${JOEBIN}/bbsjoe ; \
			rm -f ${JOEBIN}/bbsjmacs ; \
			echo "${LN_S} ${JOEBIN}/joe ${JOEBIN}/bbsjoe" ; \
			${LN_S} ${JOEBIN}/joe ${JOEBIN}/bbsjoe ; \
			echo "${LN_S} ${JOEBIN}/joe ${JOEBIN}/bbsjmacs" ; \
			${LN_S} ${JOEBIN}/joe ${JOEBIN}/bbsjmacs ; \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 editor      ${BINDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 editor      ${BINDIR}" ; \
		fi
		@if [ -d ${JOELIB} ]; then \
			${INSTALL} -c -o ${ROWNER} -g ${RGROUP} -m 0644 bbsjoerc    ${JOELIB} ; \
			echo "${INSTALL} -c -o ${ROWNER} -g ${RGROUP} -m 0644 bbsjoerc    ${JOELIB}" ; \
			${INSTALL} -c -o ${ROWNER} -g ${RGROUP} -m 0644 bbsjmacsrc  ${JOELIB} ; \
			echo "${INSTALL} -c -o ${ROWNER} -g ${RGROUP} -m 0644 bbsjmacsrc  ${JOELIB}" ; \
		fi
		@if [ ! -x ${BINDIR}/bbsdoor.sh ]; then \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 bbsdoor.sh  ${BINDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 bbsdoor.sh  ${BINDIR}" ; \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 rundoor.sh  ${BINDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 rundoor.sh  ${BINDIR}" ; \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 runvirtual.sh  ${BINDIR} ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 runvirtual.sh  ${BINDIR}" ; \
		else \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 bbsdoor.sh  ${BINDIR}/bbsdoor.sh.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 bbsdoor.sh  ${BINDIR}/bbsdoor.sh.new" ; \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 rundoor.sh  ${BINDIR}/rundoor.sh.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 rundoor.sh  ${BINDIR}/rundoor.sh.new" ; \
			${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 runvirtual.sh  ${BINDIR}/runvirtual.sh.new ; \
			echo "${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0750 runvirtual.sh  ${BINDIR}/runvirtual.sh.new" ; \
		fi
		${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0640 doors.bat    ${VARDIR}/dosemu/c
		${INSTALL} -c -o ${OWNER} -g ${GROUP}  -m 0700 mbsedos      ${BINDIR}
		@bash ./installinit.sh

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:

