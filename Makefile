# Top-level makefile for MBSE BBS package
# $Id$

include Makefile.global

OTHER		= AUTHORS ChangeLog COPYING DEBUG CRON.sh FILE_ID.DIZ.in \
		  INSTALL.in MBSE.FAQ Makefile Makefile.global.in NEWS \
		  README README.GoldED README.isdn SETUP.sh TODO UPGRADE \
		  aclocal.m4 \
		  checkbasic config.h.in configure configure.in files.css
TARFILE		= ${PACKAGE}-${VERSION}.tar.gz

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
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/bin ; \
		fi
		@if [ ! -d ${PREFIX}/etc ] ; then \
			mkdir ${PREFIX}/etc ; \
			mkdir ${PREFIX}/etc/maptabs ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/etc ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/etc/maptabs ; \
		fi
		@chmod 0775 ${PREFIX}/etc
		@if [ ! -d ${PREFIX}/doc ] ; then \
			mkdir ${PREFIX}/doc ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/doc ; \
		fi
		@if [ ! -d ${PREFIX}/fdb ] ; then \
			mkdir ${PREFIX}/fdb ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/fdb ; \
		fi
		@chmod 0775 ${PREFIX}/fdb
		@if [ ! -d ${PREFIX}/log ] ; then \
			mkdir ${PREFIX}/log ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/log ; \
		fi
		@chmod 0775 ${PREFIX}/log
		@if [ ! -d ${PREFIX}/magic ] ; then \
			mkdir ${PREFIX}/magic ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/magic ; \
		fi
		@if [ ! -d ${PREFIX}/sema ] ; then \
			mkdir ${PREFIX}/sema ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/sema ; \
		fi
		@chmod 0777 ${PREFIX}/sema
		@if [ ! -d ${PREFIX}/var ] ; then \
			mkdir ${PREFIX}/var ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/var ; \
		fi
		@if [ ! -d ${PREFIX}/tmp ] ; then \
			mkdir ${PREFIX}/tmp ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/tmp ; \
		fi
		@chmod 0775 ${PREFIX}/tmp
		@if [ ! -d ${PREFIX}/dutch ] ; then \
			mkdir ${PREFIX}/dutch ; \
			mkdir ${PREFIX}/dutch/txtfiles ; \
			mkdir ${PREFIX}/dutch/menus ; \
			mkdir ${PREFIX}/dutch/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/dutch ; \
		fi
		@if [ ! -d ${PREFIX}/english ] ; then \
			mkdir ${PREFIX}/english ; \
			mkdir ${PREFIX}/english/txtfiles ; \
			mkdir ${PREFIX}/english/menus ; \
			mkdir ${PREFIX}/english/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/english ; \
		fi
		@if [ ! -d ${PREFIX}/italian ] ; then \
			mkdir ${PREFIX}/italian ; \
			mkdir ${PREFIX}/italian/txtfiles ; \
			mkdir ${PREFIX}/italian/menus ; \
			mkdir ${PREFIX}/italian/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/italian ; \
		fi
		@if [ ! -d ${PREFIX}/spanish ] ; then \
			mkdir ${PREFIX}/spanish ; \
			mkdir ${PREFIX}/spanish/txtfiles ; \
			mkdir ${PREFIX}/spanish/menus ; \
			mkdir ${PREFIX}/spanish/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/spanish ; \
		fi
		@if [ ! -d ${PREFIX}/galego ] ; then \
			mkdir ${PREFIX}/galego ; \
			mkdir ${PREFIX}/galego/txtfiles ; \
			mkdir ${PREFIX}/galego/menus ; \
			mkdir ${PREFIX}/galego/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/galego ; \
		fi
		@if [ ! -d ${PREFIX}/german ] ; then \
			mkdir ${PREFIX}/german; \
			mkdir ${PREFIX}/german/txtfiles ; \
			mkdir ${PREFIX}/german/menus ; \
			mkdir ${PREFIX}/german/macro ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/german; \
		fi
		@if [ ! -d ${PREFIX}/ftp ] ; then \
			mkdir ${PREFIX}/ftp ; \
			mkdir ${PREFIX}/ftp/pub ; \
			mkdir ${PREFIX}/ftp/incoming ; \
			${CHOWN} `id -un`.`id -gn` ${PREFIX}/ftp ; \
			chmod 0755 ${PREFIX}/ftp ; \
			${CHOWN} ${OWNER}.${GROUP} ${PREFIX}/ftp/pub ; \
			chmod 0755 ${PREFIX}/ftp/pub ; \
			${CHOWN} `id -un`.`id -gn` ${PREFIX}/ftp/incoming ; \
			chmod 0755 ${PREFIX}/ftp/incoming ; \
		fi
		@if [ ! -d ${PREFIX}/var/bso ] ; then \
			mkdir ${PREFIX}/var/nodelist ; \
			mkdir ${PREFIX}/var/bso ; \
			mkdir ${PREFIX}/var/bso/unknown ; \
			mkdir ${PREFIX}/var/bso/inbound ; \
			mkdir ${PREFIX}/var/bso/outbound ; \
			mkdir ${PREFIX}/var/msgs; \
			mkdir ${PREFIX}/var/badtic ; \
			mkdir ${PREFIX}/var/ticqueue ; \
			mkdir ${PREFIX}/var/mail ; \
			${CHOWN} -R ${OWNER}.${GROUP} ${PREFIX}/var ; \
			chmod -R 0750 ${PREFIX}/var ; \
		fi
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done

dist tar:	${TARFILE}

clean:
		rm -f .filelist core ${TARFILE}
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done;

${TARFILE}:	.filelist
		cd ..; rm -f ${TARFILE}; \
		${TAR} cvTf ./${PACKAGE}-${VERSION}/.filelist - | gzip >${TARFILE}

crontab:
		sh ./CRON.sh

.filelist filelist:
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$$f; done) >.filelist
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} filelist && cat filelist >>../.filelist) || exit; done;

