## Process this file with automake to produce Makefile.in

SUBDIRS = .

EXTRA_DIST = issue

noinst_PROGRAMS = mbtask

mbtask_SOURCES = mbtask.c mbtask.h signame.c signame.h taskutil.c taskutil.h \
taskcomm.c taskcomm.h taskstat.c taskstat.h taskdisk.c taskdisk.h \
taskregs.c taskregs.h taskinfo.c taskinfo.h outstat.c outstat.h \
scanout.c scanout.h nodelist.c nodelist.h callstat.c callstat.h libs.h

#  LDADD = ../lib/libmemwatch.a

install-exec-local:
	@if [ "`id -un`" != "root" ] ; then \
		echo; echo " Must be root to install!"; echo; exit 3; \
	fi
	$(INSTALL) -s -o `id -un` -g `id -gn` -m 6711 mbtask $(bindir)
	@if [ -f $(sysconfdir)/mbsed.conf ]; then \
		rm $(sysconfdir)/mbsed.conf ; \
	fi
	@if [ -f $(sysconfdir)/client.conf ]; then \
		rm $(sysconfdir)/client.conf ; \
	fi
	@if [ -f $(bindir)/mbsed ]; then \
		rm $(bindir)/mbsed ; \
	fi
	@if [ ! -f $(sysconfdir)/issue ]; then \
		$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0644 issue       $(sysconfdir) ; \
		echo "$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0644 issue       $(sysconfdir)"; \
	fi

