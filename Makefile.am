## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip no-installinfo no-installman
EXTRA_DIST = COPYING DEBUG CRON.sh FILE_ID.DIZ.in MBSE.FAQ README \
README.GoldED SETUP.sh TODO UPGRADE files.css checkbasic

SUBDIRS = @SUBDIRS@


install-exec-local:
	@./checkbasic
	@if [ "`id -un`" != "root" ] ; then \
		echo; echo " Must be root to install!"; echo; exit 3; \
	fi
	@if [ ! -d $(prefix)/bin ] ; then \
		$(mkinstalldirs) $(prefix)/bin ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/bin ; \
	fi
	@if [ ! -d $(prefix)/etc ] ; then \
		$(mkinstalldirs) $(prefix)/etc ; \
		$(mkinstalldirs) $(prefix)/etc/maptabs ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/etc ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/etc/maptabs ; \
	fi
	@if [ ! -d $(prefix)/doc ] ; then \
		$(mkinstalldirs) $(prefix)/doc ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/doc ; \
	fi
	@if [ ! -d $(prefix)/fdb ] ; then \
		$(mkinstalldirs) $(prefix)/fdb ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/fdb ; \
	fi
	@if [ ! -d $(prefix)/log ] ; then \
		$(mkinstalldirs) $(prefix)/log ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/log ; \
	fi
	@if [ ! -d $(prefix)/magic ] ; then \
		$(mkinstalldirs) $(prefix)/magic ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/magic ; \
	fi
	@if [ ! -d $(prefix)/sema ] ; then \
		$(mkinstalldirs) $(prefix)/sema ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/sema ; \
		chmod 0777 $(prefix)/sema ; \
	fi
	@if [ ! -d $(prefix)/var ] ; then \
		$(mkinstalldirs) $(prefix)/var ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/var ; \
	fi
	@if [ ! -d $(prefix)/tmp ] ; then \
		$(mkinstalldirs) $(prefix)/tmp ; \
		${CHOWN} @OWNER@.@GROUP@ $(prefix)/tmp ; \
		chmod 0777 $(prefix)/tmp ; \
	fi
	@if [ ! -d $(prefix)/dutch ] ; then \
		$(mkinstalldirs) $(prefix)/dutch ; \
		$(mkinstalldirs) $(prefix)/dutch/txtfiles ; \
		$(mkinstalldirs) $(prefix)/dutch/menus ; \
		$(mkinstalldirs) $(prefix)/dutch/macro ; \
		${CHOWN} -R @OWNER@.@GROUP@ $(prefix)/dutch ; \
	fi
	@if [ ! -d $(prefix)/english ] ; then \
		$(mkinstalldirs) $(prefix)/english ; \
		$(mkinstalldirs) $(prefix)/english/txtfiles ; \
		$(mkinstalldirs) $(prefix)/english/menus ; \
		$(mkinstalldirs) $(prefix)/english/macro ; \
		${CHOWN} -R @OWNER@.@GROUP@ $(prefix)/english ; \
	fi
	@if [ ! -d $(prefix)/italian ] ; then \
		$(mkinstalldirs) $(prefix)/italian ; \
		$(mkinstalldirs) $(prefix)/italian/txtfiles ; \
		$(mkinstalldirs) $(prefix)/italian/menus ; \
		$(mkinstalldirs) $(prefix)/italian/macro ; \
		${CHOWN} -R @OWNER@.@GROUP@ $(prefix)/italian ; \
	fi
	@if [ ! -d $(prefix)/spanish ] ; then \
		$(mkinstalldirs) $(prefix)/spanish ; \
		$(mkinstalldirs) $(prefix)/spanish/txtfiles ; \
		$(mkinstalldirs) $(prefix)/spanish/menus ; \
		$(mkinstalldirs) $(prefix)/spanish/macro ; \
		${CHOWN} -R @OWNER@.@GROUP@ $(prefix)/spanish ; \
	fi
	@if [ ! -d $(prefix)/galego ] ; then \
		$(mkinstalldirs) $(prefix)/galego ; \
		$(mkinstalldirs) $(prefix)/galego/txtfiles ; \
		$(mkinstalldirs) $(prefix)/galego/menus ; \
		$(mkinstalldirs) $(prefix)/galego/macro ; \
		${CHOWN} -R @OWNER@.@GROUP@ $(prefix)/galego ; \
	fi
	@if [ ! -d /var/spool/mbse ] ; then \
		$(mkinstalldirs) /var/spool/mbse ; \
		$(mkinstalldirs) /var/spool/mbse/nodelist ; \
		$(mkinstalldirs) /var/spool/mbse/unknown ; \
		$(mkinstalldirs) /var/spool/mbse/inbound ; \
		$(mkinstalldirs) /var/spool/mbse/outbound ; \
		$(mkinstalldirs) /var/spool/mbse/msgs; \
		$(mkinstalldirs) /var/spool/mbse/badtic ; \
		$(mkinstalldirs) /var/spool/mbse/ticqueue ; \
		$(mkinstalldirs) /var/spool/mbse/ftp ; \
		$(mkinstalldirs) /var/spool/mbse/mail ; \
		${CHOWN} -R @OWNER@.@GROUP@ /var/spool/mbse ; \
		chmod -R 0755 /var/spool/mbse ; \
	fi

