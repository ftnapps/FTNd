# Top-level makefile for MBSE BBS package
# $Id: Makefile,v 1.73 2007/09/01 15:35:47 mbse Exp $

include Makefile.global

OTHER		= AUTHORS ChangeLog COPYING DEBUG CRON.sh FILE_ID.DIZ.in \
		  INSTALL.in Makefile Makefile.global.in NEWS cpuflags \
		  ChangeLog_1998 ChangeLog_1999 ChangeLog_2000 ChangeLog_2001 \
		  ChangeLog_2002 ChangeLog_2003 ChangeLog_2004 ChangeLog_2005 \
		  README SETUP.sh \
		  TODO UPGRADE aclocal.m4 checkbasic config.h.in configure \
		  configure.ac \
		  paths.h.in README.Gentoo README.Ubuntu
TARFILE		= ${PACKAGE}-${VERSION}.tar.bz2

###############################################################################


all depend:
		@if [ -z ${MBSE_ROOT} ] ; then \
			echo; echo " MBSE_ROOT is not set!"; echo; exit 3; \
		else \
			for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done; \
		fi

help:
		@echo "         Help for MBSE BBS make:"
		@echo ""
		@echo "make [all]                 Compile all sources"
		@echo "make install               Install everything (must be root)"
		@echo "make depend                Update source dependencies"
		@echo "make dist                  Create distribution archive"
		@echo "make clean                 Clean sourcetree and configuration"
		@echo "make crontab               Install default crontab for mbse"
		@echo "make filelist              Create filelist for make dist"
		@echo ""

install:
		@./checkbasic
		@if [ "`id -un`" != "root" ] ; then \
			echo; echo " Must be root to install!"; echo; exit 3; \
		fi
		@if [ -z ${PREFIX} ] ; then \
			echo; echo "PREFIX is not set!"; echo; exit 3; \
		fi
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0755 ${PREFIX}
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/bin
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/etc
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/etc/dosemu
		@if [ -f ${PREFIX}/etc/lastcall.data ] ; then \
			chmod 0660 ${PREFIX}/etc/lastcall.data ; \
		fi
		@if [ -f ${PREFIX}/etc/sysinfo.data ] ; then \
			chmod 0660 ${PREFIX}/etc/sysinfo.data ; \
		fi
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/log
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/tmp
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/home
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0755 ${PREFIX}/share
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0755 ${PREFIX}/share/doc
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0755 ${PREFIX}/share/doc/html
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0755 ${PREFIX}/share/doc/tags
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/en
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/es
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/nl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/de
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/gl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/menus/zh
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/en
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/es
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/nl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/de
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/gl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/macro/zh
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/en
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/es
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/nl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/de
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/gl
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/share/int/txtfiles/zh
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0775 ${PREFIX}/ftp
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0775 ${PREFIX}/ftp/pub
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0775 ${PREFIX}/ftp/pub/local
		@${INSTALL} -d -o ${ROWNER} -g ${RGROUP} -m 0750 ${PREFIX}/ftp/incoming
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0775 ${PREFIX}/var
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/arealists
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/badtic
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/boxes
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/bso
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/bso/outbound
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/boxes
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/dosemu
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/dosemu/c
		@if [ ! -d ${PREFIX}/var/fdb ] && [ -d ${PREFIX}/fdb ]; then \
			echo "Migrate files database..." ; \
			${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/fdb ; \
			mv ${PREFIX}/fdb/file*.data ${PREFIX}/var/fdb ; \
			echo "...done. You may remove ${PREFIX}/fdb" ; \
		fi
		@if [ ! -d ${PREFIX}/var/magic ] && [ -d ${PREFIX}/magic ]; then \
			echo "Migrate magic filenames..." ; \
			${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/magic ; \
			mv ${PREFIX}/magic/* ${PREFIX}/var/magic ; \
			rmdir ${PREFIX}/magic ; \
			echo "...done." ; \
		fi
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/fdb
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/hatch
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/inbound
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/magic
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/mail
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/msgs
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/nodelist
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/queue
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/rules
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0770 ${PREFIX}/var/run
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0775 ${PREFIX}/var/sema
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/ticqueue
		@${INSTALL} -d -o ${OWNER} -g ${GROUP} -m 0750 ${PREFIX}/var/unknown
		@chmod 0775 ${PREFIX}/var
		@chmod 0775 ${PREFIX}/var/sema
		@if [ -x ${BINDIR}/mbtelnetd ]; then \
			rm ${BINDIR}/mbtelnetd ; \
			echo "removed ${BINDIR}/mbtelnetd"; \
		fi
		@for d in ${SUBDIRS}; do (cd $$d && ${MAKE} -w $@) || exit; done
		@if [ -d ${PREFIX}/doc ] ; then \
			echo; echo "If there is nothing important in ${PREFIX}/doc" ; \
			echo "you may remove that obsolete directory." ; \
		fi
		@for d in de en es fr gl it nl; do \
		    if [ -d ${PREFIX}/share/int/$$d ] ; then \
		    	rmdir ${PREFIX}/share/int/$$d ; \
			echo "Removed directory ${PREFIX}/share/int/$$d" ; \
		    fi ; \
		done
		@rm -f ${PREFIX}/etc/charset.bin
		@rm -f ${PREFIX}/bin/mbcharsetc
		@rm -rf ${PREFIX}/sema
		@rm -rf ${PREFIX}/tmp/arc


dist tar:	${TARFILE}

clean:
		rm -f .filelist core ${TARFILE} paths.h config.h
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} $@) || exit; done;

${TARFILE}:	.filelist
		cd ..; ln -s ${PACKAGE} ${PACKAGE}-${VERSION} ; rm -f ${TARFILE}; \
		${TAR} cvTf ./${PACKAGE}-${VERSION}/.filelist - | bzip2 >${TARFILE} ; \
		rm -f ${PACKAGE}-${VERSION}

crontab:
		sh ./CRON.sh

.filelist filelist:
		(for f in ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$$f; done) >.filelist
		for d in ${SUBDIRS}; do (cd $$d && ${MAKE} filelist && cat filelist >>../.filelist) || exit; done;

