## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip no-installinfo no-installman
EXTRA_DIST = COPYING DEBUG CRON.sh FILE_ID.DIZ.in README \
README.GoldED SETUP.sh TODO UPGRADE files.css checkbasic

SUBDIRS = @SUBDIRS@


install-exec-local:
	@./checkbasic
	@if [ "$(shell whoami)" != "root" ] ; then \
		echo; echo " Must be root to install!"; echo; exit 3; \
	fi
	$(mkinstalldirs) $(prefix)/{bin,etc,etc/maptabs,doc,fdb,home,log,magic,sema,var,tmp}
	$(mkinstalldirs) $(prefix)/{dutch,dutch/txtfiles,dutch/menus,dutch/macro}
	$(mkinstalldirs) $(prefix)/{english,english/txtfiles,english/menus,english/macro}
	$(mkinstalldirs) $(prefix)/{italian,italian/txtfiles,italian/menus,italian/macro}
	$(mkinstalldirs) $(prefix)/{spanish,spanish/txtfiles,spanish/menus,spanish/macro}
	chown @OWNER@.@GROUP@ $(prefix)/{bin,etc,etc/maptabs,doc,fdb,home,log,magic,sema,var,tmp}
	chown @OWNER@.@GROUP@ $(prefix)/{dutch,dutch/txtfiles,dutch/menus,dutch/macro}
	chown @OWNER@.@GROUP@ $(prefix)/{english,english/txtfiles,english/menus,english/macro}
	chown @OWNER@.@GROUP@ $(prefix)/{italian,italian/txtfiles,italian/menus,italian/macro}
	chown @OWNER@.@GROUP@ $(prefix)/{spanish,spanish/txtfiles,spanish/menus,spanish/macro}
	chmod 777 $(prefix)/{sema,tmp}
	$(mkinstalldirs) /var/spool/mbse
	$(mkinstalldirs) /var/spool/mbse/{nodelist,unknown,inbound,outbound,badtic,ticqueue,ftp,mail}
	chown -R @OWNER@.@GROUP@ /var/spool/mbse
	chmod -R 755 /var/spool/mbse

