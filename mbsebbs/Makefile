# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2001 by M. Broek.
# $Id$

include ../Makefile.global

SRCS		= bank.c commonio.c filesub.c language.c mbfbgen.c mbtoberep.c \
		  msgutil.c oneline.c pwcheck.c sgetpwent.c xmalloc.c bbslist.c \
		  email.c fsedit.c lineedit.c mblang.c mbuser.c myname.c page.c \
		  pwio.c shadowio.c bye.c encrypt.c funcs.c mail.c mbpasswd.c \
		  mbuseradd.c newuser.c pinfo.c rad64.c timecheck.c change.c \
		  exitinfo.c funcs4.c mball.c mbsebbs.c menu.c nextuser.c pop3.c \
		  safe.c timeout.c chat.c file.c getdef.c mbchat.c mbstat.c misc.c \
		  offline.c putpwent.c salt.c user.c
HDRS		= bank.h commonio.h filesub.h language.h mbsebbs.h misc.h offline.h \
		  putpwent.h salt.h timeout.h bbslist.h email.h fsedit.h lineedit.h \
		  mbstat.h msgutil.h oneline.h pwcheck.h sgetpwent.h user.h bye.h \
		  encrypt.h funcs.h mail.h mbuser.h myname.h page.h pwio.h shadowio.h \
		  xmalloc.h change.h exitinfo.h funcs4.h mball.h mbuseradd.h newuser.h \
		  pinfo.h rad64.h statetbl.h chat.h file.h getdef.h mbpasswd.h menu.h \
		  nextuser.h pop3.h safe.h timecheck.h
MBSEBBS_OBJS	= bank.o bbslist.o chat.o file.o funcs.o funcs4.o mail.o menu.o \
		  misc.o pinfo.o nextuser.o oneline.o page.o pwcheck.o fsedit.o \
		  bye.o change.o mbsebbs.o safe.o timeout.o user.o timecheck.o \
		  exitinfo.o filesub.o lineedit.o offline.o language.o msgutil.o \
		  newuser.o pop3.o email.o
MBSEBBS_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a \
		  ../lib/libdbase.a ../lib/libmbinet.a
MBALL_OBJS	= mball.o
MBALL_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBLANG_OBJS	= mblang.o
MBLANG_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBCHAT_OBJS	= mbchat.o
MBCHAT_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBFBGEN_OBJS	= mbfbgen.o
MBFBGEN_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a
MBSTAT_OBJS	= mbstat.o
MBSTAT_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBTOBEREP_OBJS	= mbtoberep.o
MBTOBEREP_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBUSER_OBJS	= mbuser.o
MBUSER_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBUSERADD_OBJS	= mbuseradd.o
MBPASSWD_OBJS	= mbpasswd.o commonio.o pwio.o shadowio.o sgetpwent.o \
		  xmalloc.o myname.o rad64.o salt.o getdef.o encrypt.o putpwent.o
OTHER		= Makefile
TARGET		= mbsebbs mball mblang mbchat mbfbgen mbstat mbtoberep mbuser mbuseradd mbpasswd

#############################################################################################################

.c.o:
		${CC} ${CFLAGS} ${INCLUDES} ${DEFINES} -c $<

all:		${TARGET}

clean: 
		rm -f ${TARGET} *.o *.h~ *.c~ core filelist Makefile.bak

mbsebbs:	${MBSEBBS_OBJS} ${MBSEBBS_LIBS}
		${CC} -o mbsebbs ${MBSEBBS_OBJS} ${LIBS} ${MBSEBBS_LIBS}
		strip mbsebbs

mball:		${MBALL_OBJS} ${MBALL_LIBS}
		${CC} -o mball   ${MBALL_OBJS} ${LIBS} ${MBALL_LIBS}
		strip mball

mblang:		${MBLANG_OBJS} ${MBLANG_LIBS}
		${CC} -o mblang  ${MBLANG_OBJS} ${LIBS} ${MBLANG_LIBS}
		strip mblang

mbchat:		${MBCHAT_OBJS} ${MBCHAT_LIBS}
		${CC} -o mbchat  ${MBCHAT_OBJS} ${LIBS} ${MBCHAT_LIBS}
		strip mbchat

mbfbgen:	${MBFBGEN_OBJS} ${MBFBGEN_LIBS}
		${CC} -o mbfbgen ${MBFBGEN_OBJS} ${LIBS} ${MBFBGEN_LIBS}
		strip mbfbgen

mbstat:		${MBSTAT_OBJS} ${MBSTAT_LIBS}
		${CC} -o mbstat ${MBSTAT_OBJS} ${LIBS} ${MBSTAT_LIBS}
		strip mbstat

mbtberep:	${MBTOBEREP_OBJS} ${MBTOBEREP_LIBS}
		${CC} -o mbtoberep ${MBTOBEREP_OBJS} ${LIBS} ${MBTOBEREP_LIBS}
		strip mbtoberep

mbuser:		${MBUSER_OBJS} ${MBUSER_LIBS}
		${CC} -o mbuser ${MBUSER_OBJS} ${LIBS} ${MBUSER_LIBS}
		strip mbuser

mbuseradd:	${MBUSERADD_OBJS}
		${CC} -o mbuseradd ${MBUSERADD_OBJS} ${LIBS}
		strip mbuseradd

mbpasswd:	${MBPASSWD_OBJS}
		${CC} -o mbpasswd ${MBPASSWD_OBJS} ${LIBS}
		strip mbpasswd

install:	all
		@if [ "`id -un`" != "root" ] ; then \
			echo; echo " Must be root to install!"; echo; exit 3; \
		fi
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 6711 mbsebbs   ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mball     ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mblang    ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mbchat    ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mbfbgen   ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mbstat    ${BINDIR}
		${INSTALL} -s -o ${OWNER} -g ${GROUP} -m 0711 mbtoberep ${BINDIR}
		${INSTALL} -s -o `id -un` -g `id -gn` -m 6711 mbuser    ${BINDIR}
		${INSTALL} -s -o `id -un` -g `id -gn` -m 6711 mbuseradd ${BINDIR}
		${INSTALL} -s -o `id -un` -g `id -gn` -m 6711 mbpasswd  ${BINDIR}

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${SRCS} ${HDRS} ${OTHER} ; do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

depend:
		@rm -f Makefile.bak; \
		mv Makefile Makefile.bak; \
		sed -e '/^# DO NOT DELETE/,$$d' Makefile.bak >Makefile; \
		${ECHO} '# DO NOT DELETE THIS LINE - MAKE DEPEND RELIES ON IT' \
		>>Makefile; \
		${ECHO} '# Dependencies generated by make depend' >>Makefile; \
		for f in ${SRCS}; \
		do \
		${ECHO} "Dependencies for $$f:\c"; \
		${ECHO} "`basename $$f .c`.o:\c" >>Makefile; \
		for h in `sed -n -e \
			's/^#[ 	]*include[ 	]*"\([^"]*\)".*/\1/p' $$f`; \
		do \
			${ECHO} " $$h\c"; \
			${ECHO} " $$h\c" >>Makefile; \
		done; \
		${ECHO} " done."; \
		${ECHO} "" >>Makefile; \
		done; \
		${ECHO} '# End of generated dependencies' >>Makefile

# DO NOT DELETE THIS LINE - MAKE DEPEND RELIES ON IT
# Dependencies generated by make depend
bank.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h bank.h funcs4.h language.h funcs.h timeout.h timecheck.h exitinfo.h
commonio.o: ../config.h commonio.h
filesub.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h filesub.h funcs.h language.h funcs4.h misc.h timeout.h exitinfo.h change.h
language.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h funcs4.h language.h
mbfbgen.o: ../lib/libs.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h
mbtoberep.o: ../lib/libs.h ../lib/structs.h
msgutil.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msgtext.h ../lib/msg.h oneline.h msgutil.h
oneline.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h oneline.h funcs.h funcs4.h language.h
pwcheck.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h pwcheck.h funcs4.h timeout.h
sgetpwent.o: ../config.h sgetpwent.h
xmalloc.o: ../config.h xmalloc.h
bbslist.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h bbslist.h funcs.h funcs4.h language.h
email.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/msgtext.h ../lib/msg.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h exitinfo.h language.h mail.h timeout.h msgutil.h funcs4.h email.h
fsedit.o: fsedit.h
lineedit.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h mail.h funcs4.h language.h timeout.h lineedit.h
mblang.o: ../lib/libs.h ../lib/structs.h ../lib/records.h
mbuser.o: ../lib/libs.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h mbuser.h
myname.o: ../config.h
page.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h funcs.h funcs4.h chat.h page.h timeout.h mail.h language.h
pwio.o: ../config.h sgetpwent.h commonio.h putpwent.h pwio.h
shadowio.o: ../config.h commonio.h shadowio.h
bye.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h funcs.h misc.h language.h bye.h
encrypt.o: ../config.h encrypt.h
funcs.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/msgtext.h ../lib/msg.h ../lib/clcomm.h funcs.h language.h funcs4.h oneline.h misc.h bye.h timeout.h timecheck.h exitinfo.h mail.h email.h
mail.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/msgtext.h ../lib/clcomm.h ../lib/msg.h mail.h funcs.h funcs4.h language.h misc.h timeout.h oneline.h exitinfo.h lineedit.h fsedit.h filesub.h msgutil.h pop3.h email.h
mbpasswd.o: ../config.h encrypt.h rad64.h myname.h xmalloc.h pwio.h shadowio.h mbpasswd.h
mbuseradd.o: ../config.h mbuseradd.h
newuser.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h funcs4.h pwcheck.h newuser.h language.h timeout.h change.h bye.h
pinfo.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h funcs4.h
rad64.o: ../config.h rad64.h
timecheck.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h timecheck.h funcs.h funcs4.h misc.h bye.h exitinfo.h language.h
change.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h change.h funcs.h funcs4.h language.h misc.h pwcheck.h timeout.h exitinfo.h bye.h
exitinfo.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h funcs.h funcs4.h language.h oneline.h misc.h bye.h timeout.h timecheck.h exitinfo.h
funcs4.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h ../lib/msg.h funcs4.h misc.h timeout.h language.h
mball.o: ../lib/libs.h ../lib/structs.h ../lib/mbse.h ../lib/records.h ../lib/common.h ../lib/dbcfg.h ../lib/clcomm.h mball.h
mbsebbs.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h mbsebbs.h user.h funcs.h funcs4.h language.h menu.h misc.h bye.h timeout.h
menu.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h oneline.h mail.h bbslist.h change.h bank.h chat.h file.h funcs.h funcs4.h misc.h nextuser.h safe.h timeout.h menu.h page.h pinfo.h bye.h timecheck.h exitinfo.h language.h offline.h email.h
nextuser.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/ansi.h ../lib/clcomm.h ../lib/common.h nextuser.h funcs.h funcs4.h language.h timeout.h
pop3.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h ../lib/msgtext.h ../lib/msg.h msgutil.h pop3.h
safe.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h exitinfo.h funcs.h funcs4.h misc.h safe.h timeout.h language.h
timeout.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h timeout.h funcs.h funcs4.h bye.h filesub.h language.h
chat.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h chat.h funcs.h funcs4.h language.h misc.h exitinfo.h
file.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h filesub.h file.h funcs.h funcs4.h language.h misc.h timeout.h exitinfo.h change.h
getdef.o: ../config.h getdef.h
mbchat.o: ../lib/libs.h ../lib/structs.h ../lib/common.h ../lib/clcomm.h
mbstat.o: ../lib/libs.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h mbstat.h
misc.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/clcomm.h ../lib/common.h funcs.h funcs4.h language.h misc.h timeout.h exitinfo.h
offline.o: ../lib/libs.h ../lib/structs.h ../lib/mbse.h ../lib/records.h ../lib/bluewave.h ../lib/common.h ../lib/clcomm.h ../lib/msgtext.h ../lib/msg.h mail.h funcs.h funcs4.h language.h file.h filesub.h exitinfo.h timeout.h msgutil.h pop3.h offline.h
putpwent.o: ../config.h putpwent.h
salt.o: ../config.h rad64.h getdef.h
user.o: ../lib/libs.h ../lib/mbse.h ../lib/structs.h ../lib/records.h ../lib/common.h ../lib/clcomm.h timeout.h user.h pwcheck.h funcs.h funcs4.h misc.h bye.h file.h mail.h change.h menu.h exitinfo.h language.h offline.h statetbl.h email.h newuser.h
# End of generated dependencies
