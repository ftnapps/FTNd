## Process this file with automake to produce Makefile.in

SUBDIRS = .
noinst_PROGRAMS = mbsebbs mball mblang mbchat mbfbgen mbstat mbtoberep mbuser mbuseradd mbpasswd

mbsebbs_SOURCES = bank.c bbslist.c chat.c file.c funcs.c funcs4.c mail.c menu.c \
misc.c pinfo.c nextuser.c oneline.c page.c pwcheck.c fsedit.c \
bye.c change.c mbsebbs.c safe.c timeout.c user.c timecheck.c \
exitinfo.c filesub.c lineedit.c offline.c language.c msgutil.c \
newuser.c pop3.c email.c \
bank.h bbslist.h chat.h file.h funcs.h funcs4.h mail.h menu.h \
misc.h pinfo.h nextuser.h oneline.h page.h pwcheck.h fsedit.h \
bye.h change.h mbsebbs.h safe.h timeout.h user.h timecheck.h \
exitinfo.h filesub.h lineedit.h offline.h language.h msgutil.h \
newuser.h pop3.h email.h statetbl.h

mball_SOURCES = mball.c mball.h

mblang_SOURCES = mblang.c

mbchat_SOURCES = mbchat.c

mbfbgen_SOURCES = mbfbgen.c

mbstat_SOURCES = mbstat.c mbstat.h

mbtoberep_SOURCES = mbtoberep.c

mbuser_SOURCES = mbuser.c mbuser.h

mbuseradd_SOURCES = mbuseradd.c mbuseradd.h

mbpasswd_SOURCES = mbpasswd.c commonio.c pwio.c shadowio.c sgetpwent.c \
xmalloc.c myname.c rad64.c salt.c getdef.c encrypt.c \
mbpasswd.h commonio.h pwio.h shadowio.h sgetpwent.h \
xmalloc.h myname.h rad64.h salt.h getdef.h encrypt.h

mbsebbs_LDADD   = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libmbinet.a
mball_LDADD     = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mblang_LDADD    = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbchat_LDADD    = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbfbgen_LDADD   = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a
mbstat_LDADD    = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbtoberep_LDADD = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
mbuser_LDADD    = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a

install-exec-local:
	@if [ "$(shell whoami)" != "root" ] ; then \
		echo; echo " Must be root to install!"; echo; exit 3; \
	fi
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 6711 mbsebbs   $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mball     $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mblang    $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mbchat    $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mbfbgen   $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mbstat    $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@  -m 0711 mbtoberep $(bindir)
	$(INSTALL) -s -g root -o root -m 6711 mbuser    $(bindir)
	$(INSTALL) -s -g root -o root -m 6711 mbuseradd $(bindir)
	$(INSTALL) -s -g root -o root -m 6711 mbpasswd  $(bindir)

