## Process this file with automake to produce Makefile.in

SUBDIRS = .
noinst_PROGRAMS = mbmon
mbmon_SOURCES = mutil.c mbmon.c common.c mutil.h mbmon.h common.h

mbmon_LDADD = ../lib/libmemwatch.a

install-exec-local:
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbmon $(bindir)

