## Process this file with automake to produce Makefile.in

SUBDIRS = .

install-exec-local:
	@if [ "`id -un`" != "root" ] ; then \
		echo; echo " Must be root to install!"; echo; exit 3; \
	fi
	@if [ ! -x $(sysconfdir)/maint ]; then \
		$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 maint       $(sysconfdir) ; \
		echo "$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 maint       $(sysconfdir)" ; \
	fi
	@if [ ! -x $(sysconfdir)/midnight ]; then \
		$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 midnight    $(sysconfdir) ; \
		echo "$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 midnight    $(sysconfdir)" ; \
	fi
	@if [ ! -x $(sysconfdir)/weekly ]; then \
		$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 weekly      $(sysconfdir) ; \
		echo "$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 weekly      $(sysconfdir)" ; \
	fi
	@if [ ! -x $(sysconfdir)/monthly ]; then \
		$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 monthly     $(sysconfdir) ; \
		echo "$(INSTALL) -o @OWNER@ -g @GROUP@  -m 0711 monthly     $(sysconfdir)" ; \
	fi
	@bash ./installinit.sh

EXTRA_DIST = README maint midnight weekly monthly installinit.sh rc rc.shutdown mbse.start mbse.stop


