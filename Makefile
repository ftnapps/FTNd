# Top-level makefile for MBSE BBS package
# $Id$

include Makefile.global

OTHER		= AUTHORS ChangeLog COPYING DEBUG CRON.sh FILE_ID.DIZ.in \
		  INSTALL.in Makefile Makefile.global.in NEWS \
		  ChangeLog_1998 ChangeLog_1999 ChangeLog_2000 ChangeLog_2001 \
		  ChangeLog_2002 README SETUP.sh TODO UPGRADE aclocal.m4 \
		  checkbasic config.h.in configure configure.in paths.h.in
TARFILE		= ${PACKAGE}-${VERSION}.tar.bz2

###############################################################################


all depend:
		@if [ -z ${MBSE_ROOT} ] ; then \
			echo; echo " MBSE_ROOT is not set!"; echo; exit 3; \
		else \
			for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done; \
		fi

install:
		@./checkbasic
		@if [ "`id -un`" != "root" ] ; then \
			echo; echo " Must be root to install!"; echo; exit 3; \
		fi
		@if [ -z ${PREFIX} ] ; then \
			echo; echo "PREFIX is not set!"; echo; exit 3; \
		fi
		@if [ ! -d ${PREFIX}/bin ] ; then \
			mkdir ${PREFIX}/bin ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/bin ; \
		fi
		@if [ ! -d ${PREFIX}/etc ] ; then \
			mkdir ${PREFIX}/etc ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/etc ; \
		fi
		@chmod 0775 ${PREFIX}/etc
		@if [ -f ${PREFIX}/etc/lastcall.data ] ; then \
			chmod 0660 ${PREFIX}/etc/lastcall.data ; \
		fi
		@if [ -f ${PREFIX}/etc/sysinfo.data ] ; then \
			chmod 0660 ${PREFIX}/etc/sysinfo.data ; \
		fi
		@if [ ! -d ${PREFIX}/doc ] ; then \
			mkdir ${PREFIX}/doc ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/doc ; \
		fi
		@if [ ! -d ${PREFIX}/fdb ] ; then \
			mkdir ${PREFIX}/fdb ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/fdb ; \
		fi
		@chmod 0775 ${PREFIX}/fdb
		@if [ ! -d ${PREFIX}/log ] ; then \
			mkdir ${PREFIX}/log ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/log ; \
		fi
		@chmod 0775 ${PREFIX}/log
		@if [ ! -d ${PREFIX}/magic ] ; then \
			mkdir ${PREFIX}/magic ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/magic ; \
		fi
		@if [ ! -d ${PREFIX}/sema ] ; then \
			mkdir ${PREFIX}/sema ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/sema ; \
		fi
		@chmod 0777 ${PREFIX}/sema
		@if [ ! -d ${PREFIX}/var ] ; then \
			mkdir ${PREFIX}/var ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/var ; \
		fi
		@if [ ! -d ${PREFIX}/tmp ] ; then \
			mkdir ${PREFIX}/tmp ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/tmp ; \
		fi
		@chmod 0775 ${PREFIX}/tmp
		@if [ ! -d ${PREFIX}/dutch ] ; then \
			mkdir ${PREFIX}/dutch ; \
			mkdir ${PREFIX}/dutch/txtfiles ; \
			mkdir ${PREFIX}/dutch/menus ; \
			mkdir ${PREFIX}/dutch/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/dutch ; \
		fi
		@chmod 0775 ${PREFIX}/dutch/txtfiles
		@if [ ! -d ${PREFIX}/english ] ; then \
			mkdir ${PREFIX}/english ; \
			mkdir ${PREFIX}/english/txtfiles ; \
			mkdir ${PREFIX}/english/menus ; \
			mkdir ${PREFIX}/english/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/english ; \
		fi
		@chmod 0775 ${PREFIX}/english/txtfiles
		@if [ ! -d ${PREFIX}/italian ] ; then \
			mkdir ${PREFIX}/italian ; \
			mkdir ${PREFIX}/italian/txtfiles ; \
			mkdir ${PREFIX}/italian/menus ; \
			mkdir ${PREFIX}/italian/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/italian ; \
		fi
		@chmod 0775 ${PREFIX}/italian/txtfiles
		@if [ ! -d ${PREFIX}/spanish ] ; then \
			mkdir ${PREFIX}/spanish ; \
			mkdir ${PREFIX}/spanish/txtfiles ; \
			mkdir ${PREFIX}/spanish/menus ; \
			mkdir ${PREFIX}/spanish/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/spanish ; \
		fi
		@chmod 0775 ${PREFIX}/spanish/txtfiles
		@if [ ! -d ${PREFIX}/galego ] ; then \
			mkdir ${PREFIX}/galego ; \
			mkdir ${PREFIX}/galego/txtfiles ; \
			mkdir ${PREFIX}/galego/menus ; \
			mkdir ${PREFIX}/galego/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/galego ; \
		fi
		@chmod 0775 ${PREFIX}/galego/txtfiles
		@if [ ! -d ${PREFIX}/german ] ; then \
			mkdir ${PREFIX}/german; \
			mkdir ${PREFIX}/german/txtfiles ; \
			mkdir ${PREFIX}/german/menus ; \
			mkdir ${PREFIX}/german/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/german; \
		fi
		@chmod 0775 ${PREFIX}/german/txtfiles
		@if [ ! -d ${PREFIX}/french ] ; then \
			mkdir ${PREFIX}/french; \
			mkdir ${PREFIX}/french/txtfiles ; \
			mkdir ${PREFIX}/french/menus ; \
			mkdir ${PREFIX}/french/macro ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/french; \
		fi
		@chmod 0775 ${PREFIX}/french/txtfiles
		@if [ ! -d ${PREFIX}/ftp ] ; then \
			mkdir ${PREFIX}/ftp ; \
			mkdir ${PREFIX}/ftp/pub ; \
			mkdir ${PREFIX}/ftp/incoming ; \
			mkdir ${PREFIX}/ftp/pub/local ; \
			${CHOWN} `id -un`:`id -gn` ${PREFIX}/ftp ; \
			chmod 0755 ${PREFIX}/ftp ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/ftp/pub ; \
			chmod 0755 ${PREFIX}/ftp/pub ; \
			${CHOWN} `id -un`:`id -gn` ${PREFIX}/ftp/incoming ; \
			chmod 0755 ${PREFIX}/ftp/incoming ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/ftp/pub/local ; \
			chmod 0755 ${PREFIX}/ftp/pub/local ; \
		fi
		@if [ ! -d ${PREFIX}/var/bso ] ; then \
			mkdir ${PREFIX}/var/nodelist ; \
			mkdir ${PREFIX}/var/bso ; \
			mkdir ${PREFIX}/var/bso/outbound ; \
			mkdir ${PREFIX}/var/queue ; \
			mkdir ${PREFIX}/var/msgs; \
			mkdir ${PREFIX}/var/badtic ; \
			mkdir ${PREFIX}/var/ticqueue ; \
			mkdir ${PREFIX}/var/mail ; \
			${CHOWN} -R ${OWNER}:${GROUP} ${PREFIX}/var ; \
			chmod -R 0750 ${PREFIX}/var ; \
		fi
		@if [ ! -d ${PREFIX}/var/boxes ]; then \
			mkdir ${PREFIX}/var/boxes ; \
			${CHOWN}  ${OWNER}:${GROUP} ${PREFIX}/var/boxes ; \
			chmod 0750 ${PREFIX}/var/boxes ; \
		fi
		@if [ ! -d ${PREFIX}/var/rules ]; then \
			mkdir ${PREFIX}/var/rules ; \
			${CHOWN}  ${OWNER}:${GROUP} ${PREFIX}/var/rules ; \
		fi
		@if [ ! -d ${PREFIX}/var/run ]; then \
			mkdir ${PREFIX}/var/run ; \
			${CHOWN}  ${OWNER}:${GROUP} ${PREFIX}/var/run ; \
		fi
		@if [ -d ${PREFIX}/var/inbound/tmp ]; then \
			rmdir ${PREFIX}/var/inbound/tmp ; \
			echo "Removed ${PREFIX}/var/inbound/tmp" ; \
		fi
		chmod 0770 ${PREFIX}/var/rules
		chmod 0770 ${PREFIX}/var/run
		@if [ ! -d ${PREFIX}/var/unknown ] ; then \
			mkdir ${PREFIX}/var/unknown ; \
			mkdir ${PREFIX}/var/inbound ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/var/unknown ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/var/inbound ; \
			chmod 0750 ${PREFIX}/var/unknown ; \
			chmod 0750 ${PREFIX}/var/inbound ; \
		fi
		@chmod 0770 ${PREFIX}/var
		@chmod 0770 ${PREFIX}/var/mail
		@if [ ! -d ${PREFIX}/var/arealists ] ; then \
			mkdir ${PREFIX}/var/arealists ; \
			${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/var/arealists ; \
			chmod 0750 ${PREFIX}/var/arealists ; \
		fi
		@if [ -x ${BINDIR}/mbfbgen ]; then \
			rm ${BINDIR}/mbfbgen; \
			echo "removed ${BINDIR}/mbfbgen"; \
		fi
		@if [ -x ${BINDIR}/fbutil ]; then \
			rm ${BINDIR}/fbutil ; \
			echo "removed ${BINDIR}/fbutil "; \
		fi
		@if [ -x ${BINDIR}/mbchat ]; then \
			rm ${BINDIR}/mbchat ; \
			echo "removed ${BINDIR}/mbchat"; \
		fi
		@if [ -x ${BINDIR}/mbtelnetd ]; then \
			rm ${BINDIR}/mbtelnetd ; \
			echo "removed ${BINDIR}/mbtelnetd"; \
		fi
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done

dist tar:	${TARFILE}

clean:
		rm -f .filelist core ${TARFILE} paths.h config.h
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done;

${TARFILE}:	.filelist
		cd ..; rm -f ${TARFILE}; \
		${TAR} cvTf ./${PACKAGE}-${VERSION}/.filelist - | bzip2 >${TARFILE}

crontab:
		sh ./CRON.sh

.filelist filelist:
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$$f; done) >.filelist
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} filelist && cat filelist >>../.filelist) || exit; done;

