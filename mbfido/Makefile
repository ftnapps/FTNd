# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2001 by M. Broek.
# $Id$

include ../Makefile.global

SRCS		= addbbs.c backalias.c flock.c hatch.c mbdiff.c mgrutil.c \
		  ptic.c sendmail.c tracker.c addpkt.c bwrite.c forward.c lhash.c \
		  mbfido.c mkftnhdr.c ping.c rfc2ftn.c storeecho.c unpack.c aliasdb.c \
		  fsort.c magic.c mbfile.c mover.c post.c rnews.c storenet.c \
		  utic.c announce.c fflist.c ftn2rfc.c makestat.c mbindex.c msgutil.c \
		  postecho.c rollover.c tic.c areamgr.c filefind.c grlist.c maketags.c \
		  mbmsg.c newspost.c postemail.c scan.c toberep.c atoul.c filemgr.c \
		  hash.c mbaff.c mbseq.c notify.c postnetmail.c scannews.c tosspkt.c \
		  mbfkill.c mbfutil.c mbfindex.c mbfcheck.c mbfpack.c mbflist.c mbfadopt.c \
		  mbfimport.c virscan.c mbftoberep.c mbfmove.c mbfdel.c bounce.c \
		  createm.c createf.c msgflags.c dirsession.c queue.c dirlock.c
HDRS		= addbbs.h backalias.h flock.h hatch.h mbdiff.h mgrutil.h \
		  postnetmail.h scannews.h tosspkt.h addpkt.h bwrite.h forward.h \
		  lhash.h mbfido.h mkftnhdr.h ptic.h sendmail.h tracker.h \
		  aliasdb.h fsort.h magic.h mbfile.h mover.h ping.h rfc2ftn.h \
		  storeecho.h unpack.h announce.h fflist.h ftn2rfc.h makestat.h mbindex.h \
		  msgutil.h post.h rnews.h storenet.h utic.h areamgr.h filefind.h \
		  grlist.h maketags.h mbmsg.h newspost.h postecho.h rollover.h tic.h \
		  atoul.h filemgr.h hash.h mbaff.h mbseq.h notify.h postemail.h scan.h toberep.h \
		  mbfkill.h mbfutil.h mbfindex.h mbfcheck.h mbfpack.h mbflist.h mbfadopt.h \
		  mbfimport.h virscan.h mbftoberep.h mbfmove.h mbfdel.h bounce.h \
		  createm.h createf.h msgflags.h dirsession.h queue.h dirlock.h
MBFIDO_OBJS	= flock.o tosspkt.o mbfido.o hatch.o maketags.o virscan.o tracker.o \
		  makestat.o scannews.o lhash.o bounce.o unpack.o tic.o ptic.o \
		  utic.o mover.o hash.o mkftnhdr.o addbbs.o magic.o fsort.o toberep.o \
		  ftn2rfc.o atoul.o ping.o forward.o sendmail.o scan.o addpkt.o \
		  storenet.o storeecho.o areamgr.o filemgr.o notify.o mgrutil.o rollover.o \
		  bwrite.o rfc2ftn.o rnews.o newspost.o aliasdb.o postemail.o postnetmail.o \
		  postecho.o backalias.o createm.o createf.o msgflags.o dirsession.o \
		  queue.o dirlock.o
MBSEQ_OBJS	= mbseq.o
MBAFF_OBJS	= announce.o fflist.o filefind.o grlist.o mbaff.o msgutil.o
MBINDEX_OBJS	= mbindex.o
MBDIFF_OBJS	= mbdiff.o
MBFILE_OBJS	= mbfile.o mbfkill.o mbfutil.o mbfindex.o mbfcheck.o mbfpack.o mbflist.o mbfadopt.o \
		  mbfimport.o virscan.o mbftoberep.o mbfmove.o mbfdel.o
MBMSG_OBJS	= post.o mbmsg.o
MBFIDO_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a \
		  ../lib/libdbase.a ../lib/libdiesel.a ../lib/libmbinet.a
MBSEQ_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBAFF_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a \
		  ../lib/libdbase.a ../lib/libdiesel.a
MBINDEX_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBDIFF_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a
MBFILE_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libdbase.a ../lib/libdiesel.a
MBMSG_LIBS	= ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a ../lib/libmsgbase.a ../lib/libdbase.a
OTHER		= Makefile README maptabs.tgz
TARGET		= mbfido mbseq mbaff mbindex mbdiff mbfile mbmsg

####################################################################################################################

.c.o:
		${CC} ${CFLAGS} ${INCLUDES} ${DEFINES} -c $<

all:		${TARGET}

mbfido:		${MBFIDO_OBJS} ${MBFIDO_LIBS}
		${CC} -o mbfido ${MBFIDO_OBJS} ${LIBS} ${MBFIDO_LIBS}
		strip mbfido

mbseq:		${MBSEQ_OBJS} ${MBSEQ_LIBS}
		${CC} -o mbseq ${MBSEQ_OBJS} ${LIBS} ${MBSEQ_LIBS}
		strip mbseq

mbaff:		${MBAFF_OBJS} ${MBAFF_LIBS}
		${CC} -o mbaff ${MBAFF_OBJS} ${LIBS} ${MBAFF_LIBS}
		strip mbaff

mbindex:	${MBINDEX_OBJS} ${MBINDEX_LIBS}
		${CC} -o mbindex ${MBINDEX_OBJS} ${LIBS} ${MBINDEX_LIBS}
		strip mbindex

mbdiff:		${MBDIFF_OBJS} ${MBDIFF_LIBS}
		${CC} -o mbdiff ${MBDIFF_OBJS} ${LIBS} ${MBDIFF_LIBS}
		strip mbdiff

mbfile:		${MBFILE_OBJS} ${MBFILE_LIBS}
		${CC} -o mbfile ${MBFILE_OBJS} ${LIBS} ${MBFILE_LIBS}
		strip mbfile

mbmsg:		${MBMSG_OBJS} ${MBMSG_LIBS}
		${CC} -o mbmsg ${MBMSG_OBJS} ${LIBS} ${MBMSG_LIBS}
		strip mbmsg

clean: 
		rm -f ${TARGET} *.o *.h~ *.c~ core filelist Makefile.bak

install:	all
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 4751 mbfido  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbseq   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbaff   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbindex ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbdiff  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbfile  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0711 mbmsg   ${BINDIR}
		rm -f ${BINDIR}/mbmail
		ln -s ${BINDIR}/mbfido ${BINDIR}/mbmail
		rm -f ${BINDIR}/mbnews
		ln -s ${BINDIR}/mbfido ${BINDIR}/mbnews
		(cp maptabs.tgz ${PREFIX}; cd ${PREFIX}; tar xfz maptabs.tgz; ${CHOWN} ${OWNER}.${GROUP} etc/maptabs/*; rm ${PREFIX}/maptabs.tgz)
		chmod 0775 ${PREFIX}/etc

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${SRCS} ${HDRS} ${OTHER} ;do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; done) >filelist

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
addbbs.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h tic.h fsort.h addbbs.h
backalias.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h backalias.h
flock.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/clcomm.h flock.h
hatch.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbtic.h utic.h rollover.h hatch.h
mbdiff.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbdiff.h
mgrutil.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbnode.h ../lib/diesel.h sendmail.h rollover.h addpkt.h createm.h createf.h mgrutil.h
ptic.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/dbtic.h ../lib/clcomm.h ../lib/dbnode.h ../lib/dbdupe.h unpack.h mover.h toberep.h tic.h utic.h addbbs.h magic.h forward.h rollover.h ptic.h magic.h createf.h virscan.h
sendmail.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/dbnode.h ../lib/clcomm.h ../lib/dbmsgs.h addpkt.h rollover.h sendmail.h
tracker.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbnode.h ../lib/dbftn.h tracker.h
addpkt.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/clcomm.h ../lib/common.h ../lib/dbnode.h ../lib/dbmsgs.h addpkt.h
bwrite.o: ../config.h ../lib/libs.h ../lib/memwatch.h bwrite.h
forward.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbnode.h ../lib/dbtic.h ../lib/diesel.h tic.h sendmail.h rollover.h mgrutil.h forward.h
lhash.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/clcomm.h lhash.h
mbfido.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/mbse.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbdupe.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/dbuser.h ../lib/dbftn.h ../lib/dbtic.h ../lib/msg.h ../lib/mberrors.h flock.h tosspkt.h unpack.h tic.h fsort.h scan.h mbfido.h tracker.h notify.h rollover.h hatch.h scannews.h maketags.h makestat.h newspost.h rnews.h mgrutil.h backalias.h rfc2ftn.h dirsession.h dirlock.h queue.h
mkftnhdr.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h atoul.h hash.h msgflags.h aliasdb.h mkftnhdr.h
ping.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbtic.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h sendmail.h postnetmail.h ping.h
rfc2ftn.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h ../lib/dbdupe.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h postnetmail.h postecho.h msgflags.h rfc2ftn.h
storeecho.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbmsgs.h ../lib/dbuser.h rollover.h storeecho.h
unpack.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h flock.h unpack.h
aliasdb.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h aliasdb.h
fsort.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/clcomm.h fsort.h
magic.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbtic.h tic.h utic.h magic.h
mbfile.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfkill.h mbfadopt.h mbfindex.h mbfcheck.h mbfpack.h mbflist.h mbfimport.h mbftoberep.h mbfmove.h mbfdel.h mbfutil.h mbfile.h
mover.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h tic.h mover.h
post.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/msg.h ../lib/msgtext.h post.h
rnews.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h ../lib/dbdupe.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/msg.h ../lib/msgtext.h ../lib/mberrors.h rfc2ftn.h mbfido.h ../paths.h rnews.h
storenet.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbmsgs.h ../lib/dbuser.h msgflags.h rollover.h storenet.h
utic.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h tic.h mover.h tic.h utic.h
announce.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h grlist.h msgutil.h announce.h
fflist.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/clcomm.h ../lib/msg.h fflist.h
ftn2rfc.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/dbftn.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/common.h ../lib/clcomm.h rollover.h aliasdb.h postemail.h backalias.h msgflags.h ftn2rfc.h
makestat.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/diesel.h ../lib/msg.h mgrutil.h makestat.h
mbindex.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/dbftn.h ../lib/mberrors.h mbindex.h
msgutil.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h msgutil.h
postecho.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h ftn2rfc.h postecho.h storeecho.h addpkt.h rollover.h
rollover.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h rollover.h
tic.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/dbftn.h ../lib/clcomm.h unpack.h ptic.h fsort.h tic.h
areamgr.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h ../lib/diesel.h sendmail.h mgrutil.h scan.h createm.h areamgr.h
filefind.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h fflist.h filefind.h msgutil.h
grlist.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/clcomm.h grlist.h
maketags.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h maketags.h
mbmsg.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/dbcfg.h ../lib/mberrors.h post.h mbmsg.h
newspost.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h newspost.h
postemail.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/dbuser.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h postemail.h
scan.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/msg.h ../lib/clcomm.h ../lib/msgtext.h ../lib/dbnode.h ../lib/dbmsgs.h addpkt.h tracker.h ftn2rfc.h rfc2ftn.h postemail.h scan.h
toberep.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h tic.h toberep.h
atoul.o: ../config.h ../lib/libs.h ../lib/memwatch.h atoul.h
filemgr.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbtic.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h ../lib/diesel.h sendmail.h mgrutil.h createf.h filemgr.h
hash.o: ../config.h ../lib/libs.h ../lib/memwatch.h hash.h lhash.h
mbaff.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/msg.h ../lib/mberrors.h announce.h filefind.h mbaff.h
mbseq.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h mbseq.h
notify.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbnode.h filemgr.h areamgr.h sendmail.h mgrutil.h notify.h
postnetmail.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/common.h ../lib/records.h ../lib/dbcfg.h ../lib/dbuser.h ../lib/dbnode.h ../lib/dbftn.h ../lib/clcomm.h tracker.h addpkt.h storenet.h ftn2rfc.h areamgr.h filemgr.h ping.h bounce.h postemail.h
scannews.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/mbinet.h ../lib/dbdupe.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h storeecho.h rfc2ftn.h scannews.h
tosspkt.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbmsgs.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h tosspkt.h postnetmail.h postecho.h rollover.h createm.h
mbfkill.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfkill.h mbfutil.h
mbfutil.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbfile.h
mbfindex.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/diesel.h ../lib/mberrors.h mbfutil.h mbfindex.h
mbfcheck.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbfcheck.h
mbfpack.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbfpack.h
mbflist.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbflist.h
mbfadopt.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h virscan.h mbfutil.h mbflist.h
mbfimport.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h virscan.h mbfutil.h mbfimport.h
virscan.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h virscan.h
mbftoberep.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h mbfutil.h mbftoberep.h
mbfmove.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbfmove.h
mbfdel.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbcfg.h ../lib/mberrors.h mbfutil.h mbfmove.h
bounce.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h ../lib/msgtext.h ../lib/dbcfg.h ../lib/dbnode.h ../lib/dbtic.h ../lib/dbdupe.h ../lib/dbuser.h ../lib/dbftn.h sendmail.h postnetmail.h ping.h
createm.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/msg.h mgrutil.h createm.h
createf.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h mgrutil.h createf.h
msgflags.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/clcomm.h msgflags.h
dirsession.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h dirsession.h
queue.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h ../lib/dbnode.h ../lib/dbftn.h fsort.h dirsession.h queue.h
dirlock.o: ../config.h ../lib/libs.h ../lib/memwatch.h ../lib/structs.h ../lib/users.h ../lib/records.h ../lib/common.h ../lib/clcomm.h flock.h dirlock.h
# End of generated dependencies
