# Makefile for MBSE BBS under Linux
# Copyright (c) 1998, 2001 by M. Broek.
# $Id: Makefile,v 1.74 2008/02/17 17:50:14 mbse Exp $

include ../Makefile.global

SRCS		= addbbs.c backalias.c flock.c hatch.c mbdiff.c mgrutil.c qualify.c \
		  ptic.c sendmail.c tracker.c addpkt.c bwrite.c forward.c lhash.c \
		  mbfido.c mkftnhdr.c ping.c rfc2ftn.c storeecho.c unpack.c aliasdb.c \
		  fsort.c magic.c mbfile.c mover.c post.c rnews.c storenet.c mbfrearc.c \
		  utic.c announce.c fflist.c ftn2rfc.c makestat.c mbindex.c msgutil.c \
		  postecho.c rollover.c tic.c areamgr.c filefind.c grlist.c maketags.c \
		  mbmsg.c newspost.c postemail.c scan.c toberep.c atoul.c filemgr.c \
		  hash.c mbaff.c mbseq.c notify.c postnetmail.c scannews.c tosspkt.c \
		  mbfkill.c mbfutil.c mbfindex.c mbfcheck.c mbfpack.c mbflist.c mbfadopt.c \
		  mbfimport.c mbfsort.c mbftoberep.c mbfmove.c mbfdel.c bounce.c \
		  createm.c createf.c msgflags.c dirsession.c queue.c dirlock.c msg.c orphans.c
HDRS		= addbbs.h backalias.h flock.h hatch.h mbdiff.h mgrutil.h qualify.h \
		  postnetmail.h scannews.h tosspkt.h addpkt.h bwrite.h forward.h \
		  lhash.h mbfido.h mkftnhdr.h ptic.h sendmail.h tracker.h \
		  aliasdb.h fsort.h magic.h mbfile.h mover.h ping.h rfc2ftn.h \
		  storeecho.h unpack.h announce.h fflist.h ftn2rfc.h makestat.h mbindex.h \
		  msgutil.h post.h rnews.h storenet.h utic.h areamgr.h filefind.h \
		  grlist.h maketags.h mbmsg.h newspost.h postecho.h rollover.h tic.h mbfrearc.h \
		  atoul.h filemgr.h hash.h mbaff.h mbseq.h notify.h postemail.h scan.h toberep.h \
		  mbfkill.h mbfutil.h mbfindex.h mbfcheck.h mbfpack.h mbflist.h mbfadopt.h \
		  mbfimport.h mbfsort.h mbftoberep.h mbfmove.h mbfdel.h bounce.h \
		  createm.h createf.h msgflags.h dirsession.h queue.h dirlock.h msg.h orphans.h
MBFIDO_OBJS	= flock.o tosspkt.o mbfido.o hatch.o maketags.o tracker.o \
		  makestat.o scannews.o lhash.o bounce.o unpack.o tic.o ptic.o \
		  utic.o mover.o hash.o mkftnhdr.o addbbs.o magic.o fsort.o toberep.o \
		  ftn2rfc.o atoul.o ping.o forward.o sendmail.o scan.o addpkt.o \
		  storenet.o storeecho.o areamgr.o filemgr.o notify.o mgrutil.o rollover.o \
		  bwrite.o rfc2ftn.o rnews.o newspost.o aliasdb.o postemail.o postnetmail.o \
		  postecho.o backalias.o createm.o createf.o msgflags.o dirsession.o \
		  queue.o dirlock.o qualify.o msg.o orphans.o
MBSEQ_OBJS	= mbseq.o
MBAFF_OBJS	= announce.o fflist.o filefind.o grlist.o mbaff.o msgutil.o toberep.o
MBINDEX_OBJS	= mbindex.o
MBDIFF_OBJS	= mbdiff.o
MBFILE_OBJS	= mbfile.o mbfkill.o mbfutil.o mbfindex.o mbfcheck.o mbfpack.o mbflist.o mbfadopt.o \
		  mbfimport.o mbftoberep.o mbfmove.o mbfdel.o mbfsort.o mbfrearc.o
MBMSG_OBJS	= post.o mbmsg.o
MBFIDO_LIBS	= ../lib/libmbse.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libdiesel.a ../lib/libmbinet.a ../lib/libnodelist.a
MBSEQ_LIBS	= ../lib/libmbse.a ../lib/libdbase.a
MBAFF_LIBS	= ../lib/libmbse.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libdiesel.a
MBINDEX_LIBS	= ../lib/libmbse.a ../lib/libdbase.a
MBDIFF_LIBS	= ../lib/libmbse.a ../lib/libdbase.a
MBFILE_LIBS	= ../lib/libmbse.a ../lib/libdbase.a ../lib/libdiesel.a
MBMSG_LIBS	= ../lib/libmbse.a ../lib/libmsgbase.a ../lib/libdbase.a
OTHER		= Makefile README
TARGET		= mbfido mbseq mbaff mbindex mbdiff mbfile mbmsg

####################################################################################################################

.c.o:
		${CC} ${CFLAGS} ${INCLUDES} ${DEFINES} -c $<

all:		${TARGET}

mbfido:		${MBFIDO_OBJS} ${MBFIDO_LIBS}
		${CC} -o mbfido ${MBFIDO_OBJS} ${LDFLAGS} ${LIBS} ${MBFIDO_LIBS}

mbseq:		${MBSEQ_OBJS} ${MBSEQ_LIBS}
		${CC} -o mbseq ${MBSEQ_OBJS} ${LDFLAGS} ${LIBS} ${MBSEQ_LIBS}

mbaff:		${MBAFF_OBJS} ${MBAFF_LIBS}
		${CC} -o mbaff ${MBAFF_OBJS} ${LDFLAGS} ${LIBS} ${MBAFF_LIBS}

mbindex:	${MBINDEX_OBJS} ${MBINDEX_LIBS}
		${CC} -o mbindex ${MBINDEX_OBJS} ${LDFLAGS} ${LIBS} ${MBINDEX_LIBS}

mbdiff:		${MBDIFF_OBJS} ${MBDIFF_LIBS}
		${CC} -o mbdiff ${MBDIFF_OBJS} ${LDFLAGS} ${LIBS} ${MBDIFF_LIBS}

mbfile:		${MBFILE_OBJS} ${MBFILE_LIBS}
		${CC} -o mbfile ${MBFILE_OBJS} ${LDFLAGS} ${LIBS} ${MBFILE_LIBS}

mbmsg:		${MBMSG_OBJS} ${MBMSG_LIBS}
		${CC} -o mbmsg ${MBMSG_OBJS} ${LDFLAGS} ${LIBS} ${MBMSG_LIBS}

clean: 
		rm -f ${TARGET} *.o *.h~ *.c~ core filelist Makefile.bak

install:	all
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 4750 mbfido  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0750 mbseq   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 mbaff   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 mbindex ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 mbdiff  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 mbfile  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0750 mbmsg   ${BINDIR}
		rm -f ${BINDIR}/mbmail
		${LN_S}  ${BINDIR}/mbfido ${BINDIR}/mbmail
		rm -f ${BINDIR}/mbnews
		${LN_S}  ${BINDIR}/mbfido ${BINDIR}/mbnews

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
addbbs.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h orphans.h tic.h fsort.h qualify.h addbbs.h
backalias.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h backalias.h
flock.o: ../config.h ../lib/mbselib.h flock.h
hatch.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h utic.h rollover.h hatch.h
mbdiff.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbdiff.h
mgrutil.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/diesel.h sendmail.h rollover.h addpkt.h createm.h createf.h mgrutil.h
qualify.o: ../config.h ../lib/mbselib.h qualify.h
ptic.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h unpack.h mover.h toberep.h orphans.h tic.h utic.h magic.h forward.h rollover.h ptic.h magic.h createf.h qualify.h addbbs.h
sendmail.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h addpkt.h rollover.h sendmail.h
tracker.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/nodelist.h ../lib/mbsedb.h tracker.h
addpkt.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h addpkt.h
bwrite.o: ../config.h ../lib/mbselib.h bwrite.h
forward.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/diesel.h orphans.h tic.h sendmail.h rollover.h mgrutil.h forward.h
lhash.o: ../config.h ../lib/mbselib.h lhash.h
mbfido.o: ../config.h ../lib/mbselib.h ../lib/mbse.h ../lib/users.h ../lib/nodelist.h ../lib/mbsedb.h ../lib/msg.h flock.h tosspkt.h unpack.h orphans.h tic.h fsort.h scan.h mbfido.h tracker.h notify.h rollover.h hatch.h scannews.h maketags.h makestat.h newspost.h rnews.h mgrutil.h backalias.h rfc2ftn.h dirsession.h dirlock.h queue.h msg.h createm.h
mkftnhdr.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h atoul.h hash.h msgflags.h aliasdb.h mkftnhdr.h
ping.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h sendmail.h postnetmail.h ping.h
rfc2ftn.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbinet.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h postnetmail.h postecho.h msgflags.h rfc2ftn.h
storeecho.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h rollover.h storeecho.h
unpack.o: ../config.h ../lib/mbselib.h flock.h unpack.h
aliasdb.o: ../config.h ../lib/mbselib.h aliasdb.h
fsort.o: ../config.h ../lib/mbselib.h fsort.h
magic.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h orphans.h tic.h utic.h magic.h
mbfile.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfkill.h mbfadopt.h mbfindex.h mbfcheck.h mbfpack.h mbflist.h mbfimport.h mbftoberep.h mbfmove.h mbfdel.h mbfutil.h mbfsort.h mbfile.h mbfrearc.h
mover.o: ../config.h ../lib/mbselib.h orphans.h tic.h mover.h
post.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h post.h
rnews.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbinet.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h rfc2ftn.h mbfido.h ../paths.h rnews.h
storenet.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h msgflags.h rollover.h storenet.h
mbfrearc.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfmove.h
utic.o: ../config.h ../lib/mbselib.h orphans.h tic.h mover.h tic.h utic.h
announce.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h grlist.h msgutil.h toberep.h announce.h
fflist.o: ../config.h ../lib/mbselib.h ../lib/msg.h fflist.h
ftn2rfc.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h rollover.h aliasdb.h postemail.h backalias.h msgflags.h rfc2ftn.h ftn2rfc.h
makestat.o: ../config.h ../lib/mbselib.h ../lib/diesel.h ../lib/msg.h mgrutil.h makestat.h
mbindex.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbindex.h
msgutil.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h msgutil.h
postecho.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h ftn2rfc.h postecho.h storeecho.h addpkt.h rollover.h qualify.h
rollover.o: ../config.h ../lib/mbselib.h rollover.h
tic.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h unpack.h fsort.h orphans.h ptic.h mover.h tic.h
areamgr.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h ../lib/diesel.h sendmail.h mgrutil.h scan.h createm.h areamgr.h
filefind.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h fflist.h filefind.h msgutil.h
grlist.o: ../config.h ../lib/mbselib.h grlist.h
maketags.o: ../config.h ../lib/mbselib.h maketags.h
mbmsg.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/mbsedb.h post.h mbmsg.h
newspost.o: ../config.h ../lib/mbselib.h ../lib/mbinet.h newspost.h
postemail.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/mbinet.h ../lib/msg.h postemail.h
scan.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h addpkt.h tracker.h ftn2rfc.h rfc2ftn.h rollover.h postemail.h scan.h
toberep.o: ../config.h ../lib/mbselib.h orphans.h tic.h toberep.h
atoul.o: ../config.h ../lib/mbselib.h atoul.h
filemgr.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h ../lib/diesel.h sendmail.h mgrutil.h createf.h filemgr.h
hash.o: ../config.h ../lib/mbselib.h hash.h lhash.h
mbaff.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h announce.h filefind.h mbaff.h
mbseq.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbseq.h
notify.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h filemgr.h areamgr.h sendmail.h mgrutil.h notify.h
postnetmail.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msg.h tracker.h addpkt.h storenet.h ftn2rfc.h areamgr.h filemgr.h ping.h bounce.h postemail.h
scannews.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbinet.h ../lib/mbsedb.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h storeecho.h rfc2ftn.h scannews.h
tosspkt.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h tosspkt.h postnetmail.h postecho.h rollover.h createm.h
mbfkill.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfkill.h mbfutil.h
mbfutil.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfile.h
mbfindex.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/diesel.h mbfutil.h mbfindex.h
mbfcheck.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfcheck.h
mbfpack.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfpack.h
mbflist.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbflist.h
mbfadopt.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbflist.h
mbfimport.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfimport.h
mbfsort.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfsort.h
mbftoberep.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbftoberep.h
mbfmove.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfmove.h
mbfdel.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mbfutil.h mbfmove.h
bounce.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/mbsedb.h sendmail.h postnetmail.h ping.h
createm.o: ../config.h ../lib/mbselib.h ../lib/msg.h ../lib/users.h ../lib/mbsedb.h mgrutil.h createm.h
createf.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h mgrutil.h createf.h
msgflags.o: ../config.h ../lib/mbselib.h msgflags.h
dirsession.o: ../config.h ../lib/mbselib.h dirsession.h
queue.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h fsort.h dirsession.h queue.h
dirlock.o: ../config.h ../lib/mbselib.h flock.h dirlock.h
msg.o: ../config.h ../lib/mbselib.h ../lib/users.h ../lib/mbsedb.h ../lib/msgtext.h ../lib/msg.h msgflags.h msg.h
orphans.o: ../config.h ../lib/mbselib.h orphans.h
# End of generated dependencies
