## Process this file with automake to produce Makefile.in

SUBDIRS = .

noinst_PROGRAMS = mbsetup

mbsetup_SOURCES = screen.c mutil.c ledit.c m_bbs.c m_global.c m_mail.c m_tic.c \
m_fido.c m_archive.c m_virus.c m_tty.c m_users.c m_protocol.c \
m_limits.c m_lang.c m_ol.c m_node.c m_marea.c m_mgroup.c m_ff.c \
m_ticarea.c m_magic.c m_menu.c m_new.c m_modem.c grlist.c \
m_fgroup.c m_farea.c m_fdb.c m_hatch.c stlist.c mbsetup.c \
m_ngroup.c m_service.c m_domain.c m_task.c \
screen.h mutil.h ledit.h m_bbs.h m_global.h m_mail.h m_tic.h \
m_fido.h m_archive.h m_virus.h m_tty.h m_users.h m_protocol.h \
m_limits.h m_lang.h m_ol.h m_node.h m_marea.h m_mgroup.h m_ff.h \
m_ticarea.h m_magic.h m_menu.h m_new.h m_modem.h grlist.h \
m_fgroup.h m_farea.h m_fdb.h m_hatch.h stlist.h \
m_ngroup.h m_service.h m_domain.h m_task.h

LDADD = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a

install-exec-local:
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbsetup $(bindir)

