## Process this file with automake to produce Makefile.in

SUBDIRS = .

EXTRA_DIST = maptabs.tgz paths.h.in README

noinst_PROGRAMS = mbfido mbseq mbaff mbindex mbdiff mbfile mbmsg

mbfido_SOURCES = flock.c tosspkt.c mbfido.c hatch.c maketags.c \
tracker.c makestat.c scannews.c lhash.c \
pack.c ulock.c tic.c ptic.c utic.c mover.c hash.c mkftnhdr.c \
addbbs.c magic.c fsort.c toberep.c ftn2rfc.c atoul.c ping.c \
cookie.c forward.c sendmail.c scan.c addpkt.c storenet.c storeecho.c \
areamgr.c filemgr.c notify.c mgrutil.c rollover.c bwrite.c rfc2ftn.c \
rnews.c newspost.c aliasdb.c postemail.c postnetmail.c postecho.c backalias.c \
flock.h tosspkt.h mbfido.h hatch.h maketags.h \
tracker.h makestat.h scannews.h lhash.h \
pack.h ulock.h tic.h ptic.h utic.h mover.h hash.h mkftnhdr.h \
addbbs.h magic.h fsort.h toberep.h ftn2rfc.h atoul.h ping.h \
cookie.h forward.h sendmail.h scan.h addpkt.h storenet.h storeecho.h \
areamgr.h filemgr.h notify.h mgrutil.h rollover.h bwrite.h rfc2ftn.h \
rnews.h newspost.h aliasdb.h postemail.h postnetmail.h postecho.h backalias.h

mbseq_SOURCES = mbseq.c mbseq.h

mbaff_SOURCES = announce.c fflist.c filefind.c grlist.c mbaff.c msgutil.c \
announce.h fflist.h filefind.h grlist.h mbaff.h msgutil.h

mbindex_SOURCES = mbindex.c mbindex.h

mbdiff_SOURCES = mbdiff.c mbdiff.h

mbfile_SOURCES = mbfile.c mbfile.h

mbmsg_SOURCES = post.c mbmsg.c post.h mbmsg.h

mbfido_LDADD  = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libmbinet.a
mbseq_LDADD   = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbaff_LDADD   = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a
mbindex_LDADD = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbdiff_LDADD  = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbfile_LDADD  = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbmsg_LDADD   = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a

install-exec-local:
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 4751 mbfido   $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbseq    $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbaff    $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbindex  $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbdiff   $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbfile   $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbmsg    $(bindir)
	rm -f $(bindir)/mbmail
	ln -s $(bindir)/mbfido $(bindir)/mbmail
	rm -f $(bindir)/mbnews
	ln -s $(bindir)/mbfido $(bindir)/mbnews
	(cd ${exec_prefix}; tar xfz @PACKAGE@-@VERSION@/mbfido/maptabs.tgz; $(CHOWN) @OWNER@.@GROUP@ etc/maptabs/*)

