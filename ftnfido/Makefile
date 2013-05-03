# Makefile for FTNd ftnfido under Linux
# Copyright (c) 2012-2013 by Robert James Clay.
# Copyright (c) 1998, 2001 by M. Broek.

include ../Makefile.global

SRCS		= addbbs.c backalias.c flock.c hatch.c ftndiff.c mgrutil.c qualify.c \
		  ptic.c sendmail.c tracker.c addpkt.c bwrite.c forward.c lhash.c \
		  ftnfido.c mkftnhdr.c ping.c rfc2ftn.c storeecho.c unpack.c aliasdb.c \
		  fsort.c magic.c ftnfile.c mover.c post.c rnews.c storenet.c ftnfrearc.c \
		  utic.c announce.c fflist.c ftn2rfc.c makestat.c ftnindex.c msgutil.c \
		  postecho.c rollover.c tic.c areamgr.c filefind.c grlist.c maketags.c \
		  ftnmsg.c newspost.c postemail.c scan.c toberep.c atoul.c filemgr.c \
		  hash.c ftnaff.c ftnseq.c notify.c postnetmail.c scannews.c tosspkt.c \
		  ftnfkill.c ftnfutil.c ftnfindex.c ftnfcheck.c ftnfpack.c ftnflist.c ftnfadopt.c \
		  ftnfimport.c ftnfsort.c ftnftoberep.c ftnfmove.c ftnfdel.c bounce.c \
		  createm.c createf.c msgflags.c dirsession.c queue.c dirlock.c msg.c orphans.c
HDRS		= addbbs.h backalias.h flock.h hatch.h ftndiff.h mgrutil.h qualify.h \
		  postnetmail.h scannews.h tosspkt.h addpkt.h bwrite.h forward.h \
		  lhash.h ftnfido.h mkftnhdr.h ptic.h sendmail.h tracker.h \
		  aliasdb.h fsort.h magic.h ftnfile.h mover.h ping.h rfc2ftn.h \
		  storeecho.h unpack.h announce.h fflist.h ftn2rfc.h makestat.h ftnindex.h \
		  msgutil.h post.h rnews.h storenet.h utic.h areamgr.h filefind.h \
		  grlist.h maketags.h ftnmsg.h newspost.h postecho.h rollover.h tic.h ftnfrearc.h \
		  atoul.h filemgr.h hash.h ftnaff.h ftnseq.h notify.h postemail.h scan.h toberep.h \
		  ftnfkill.h ftnfutil.h ftnfindex.h ftnfcheck.h ftnfpack.h ftnflist.h ftnfadopt.h \
		  ftnfimport.h ftnfsort.h ftnftoberep.h ftnfmove.h ftnfdel.h bounce.h \
		  createm.h createf.h msgflags.h dirsession.h queue.h dirlock.h msg.h orphans.h
FTNFIDO_OBJS	= flock.o tosspkt.o ftnfido.o hatch.o maketags.o tracker.o \
		  makestat.o scannews.o lhash.o bounce.o unpack.o tic.o ptic.o \
		  utic.o mover.o hash.o mkftnhdr.o addbbs.o magic.o fsort.o toberep.o \
		  ftn2rfc.o atoul.o ping.o forward.o sendmail.o scan.o addpkt.o \
		  storenet.o storeecho.o areamgr.o filemgr.o notify.o mgrutil.o rollover.o \
		  bwrite.o rfc2ftn.o rnews.o newspost.o aliasdb.o postemail.o postnetmail.o \
		  postecho.o backalias.o createm.o createf.o msgflags.o dirsession.o \
		  queue.o dirlock.o qualify.o msg.o orphans.o
FTNSEQ_OBJS	= ftnseq.o
FTNAFF_OBJS	= announce.o fflist.o filefind.o grlist.o ftnaff.o msgutil.o toberep.o
FTNINDEX_OBJS	= ftnindex.o
FTNDIFF_OBJS	= ftndiff.o
FTNFILE_OBJS	= ftnfile.o ftnfkill.o ftnfutil.o ftnfindex.o ftnfcheck.o ftnfpack.o ftnflist.o ftnfadopt.o \
		  ftnfimport.o ftnftoberep.o ftnfmove.o ftnfdel.o ftnfsort.o ftnfrearc.o
FTNMSG_OBJS	= post.o ftnmsg.o
FTNFIDO_LIBS	= ../lib/libftnd.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libdiesel.a ../lib/libftninet.a ../lib/libnodelist.a
FTNSEQ_LIBS	= ../lib/libftnd.a ../lib/libdbase.a
FTNAFF_LIBS	= ../lib/libftnd.a ../lib/libmsgbase.a ../lib/libdbase.a ../lib/libdiesel.a
FTNINDEX_LIBS	= ../lib/libftnd.a ../lib/libdbase.a
FTNDIFF_LIBS	= ../lib/libftnd.a ../lib/libdbase.a
FTNFILE_LIBS	= ../lib/libftnd.a ../lib/libdbase.a ../lib/libdiesel.a
FTNMSG_LIBS	= ../lib/libftnd.a ../lib/libmsgbase.a ../lib/libdbase.a
OTHER		= Makefile README
TARGET		= ftnfido ftnseq ftnaff ftnindex ftndiff ftnfile ftnmsg

####################################################################################################################

.c.o:
		${CC} ${CFLAGS} ${INCLUDES} ${DEFINES} -c $<

all:		${TARGET}

ftnfido:		${FTNFIDO_OBJS} ${FTNFIDO_LIBS}
		${CC} -o ftnfido ${FTNFIDO_OBJS} ${LDFLAGS} ${LIBS} ${FTNFIDO_LIBS}

ftnseq:		${FTNSEQ_OBJS} ${FTNSEQ_LIBS}
		${CC} -o ftnseq ${FTNSEQ_OBJS} ${LDFLAGS} ${LIBS} ${FTNSEQ_LIBS}

ftnaff:		${FTNAFF_OBJS} ${FTNAFF_LIBS}
		${CC} -o ftnaff ${FTNAFF_OBJS} ${LDFLAGS} ${LIBS} ${FTNAFF_LIBS}

ftnindex:	${FTNINDEX_OBJS} ${FTNINDEX_LIBS}
		${CC} -o ftnindex ${FTNINDEX_OBJS} ${LDFLAGS} ${LIBS} ${FTNINDEX_LIBS}

ftndiff:		${FTNDIFF_OBJS} ${FTNDIFF_LIBS}
		${CC} -o ftndiff ${FTNDIFF_OBJS} ${LDFLAGS} ${LIBS} ${FTNDIFF_LIBS}

ftnfile:		${FTNFILE_OBJS} ${FTNFILE_LIBS}
		${CC} -o ftnfile ${FTNFILE_OBJS} ${LDFLAGS} ${LIBS} ${FTNFILE_LIBS}

ftnmsg:		${FTNMSG_OBJS} ${FTNMSG_LIBS}
		${CC} -o ftnmsg ${FTNMSG_OBJS} ${LDFLAGS} ${LIBS} ${FTNMSG_LIBS}

clean: 
		rm -f ${TARGET} *.o *.h~ *.c~ core filelist Makefile.bak

install:	all
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 4750 ftnfido  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0750 ftnseq   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 ftnaff   ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 ftnindex ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 ftndiff  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0700 ftnfile  ${BINDIR}
		${INSTALL} -c -s -o ${OWNER} -g ${GROUP} -m 0750 ftnmsg   ${BINDIR}
		rm -f ${BINDIR}/ftnmail
		${LN_S}  ${BINDIR}/ftnfido ${BINDIR}/ftnmail
		rm -f ${BINDIR}/ftnnews
		${LN_S}  ${BINDIR}/ftnfido ${BINDIR}/ftnnews

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
addbbs.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h orphans.h tic.h fsort.h qualify.h addbbs.h
backalias.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h backalias.h
flock.o: ../config.h ../lib/ftndlib.h flock.h
hatch.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h utic.h rollover.h hatch.h
ftndiff.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftndiff.h
mgrutil.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/diesel.h sendmail.h rollover.h addpkt.h createm.h createf.h mgrutil.h
qualify.o: ../config.h ../lib/ftndlib.h qualify.h
ptic.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h unpack.h mover.h toberep.h orphans.h tic.h utic.h magic.h forward.h rollover.h ptic.h magic.h createf.h qualify.h addbbs.h
sendmail.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h addpkt.h rollover.h sendmail.h
tracker.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/nodelist.h ../lib/ftnddb.h tracker.h
addpkt.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h addpkt.h
bwrite.o: ../config.h ../lib/ftndlib.h bwrite.h
forward.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/diesel.h orphans.h tic.h sendmail.h rollover.h mgrutil.h forward.h
lhash.o: ../config.h ../lib/ftndlib.h lhash.h
ftnfido.o: ../config.h ../lib/ftndlib.h ../lib/ftnd.h ../lib/users.h ../lib/nodelist.h ../lib/ftnddb.h ../lib/msg.h flock.h tosspkt.h unpack.h orphans.h tic.h fsort.h scan.h ftnfido.h tracker.h notify.h rollover.h hatch.h scannews.h maketags.h makestat.h newspost.h rnews.h mgrutil.h backalias.h rfc2ftn.h dirsession.h dirlock.h queue.h msg.h createm.h
mkftnhdr.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h atoul.h hash.h msgflags.h aliasdb.h mkftnhdr.h
ping.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h sendmail.h postnetmail.h ping.h
rfc2ftn.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftninet.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h postnetmail.h postecho.h msgflags.h rfc2ftn.h
storeecho.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h rollover.h storeecho.h
unpack.o: ../config.h ../lib/ftndlib.h flock.h unpack.h
aliasdb.o: ../config.h ../lib/ftndlib.h aliasdb.h
fsort.o: ../config.h ../lib/ftndlib.h fsort.h
magic.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h orphans.h tic.h utic.h magic.h
ftnfile.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfkill.h ftnfadopt.h ftnfindex.h ftnfcheck.h ftnfpack.h ftnflist.h ftnfimport.h ftnftoberep.h ftnfmove.h ftnfdel.h ftnfutil.h ftnfsort.h ftnfile.h ftnfrearc.h
mover.o: ../config.h ../lib/ftndlib.h orphans.h tic.h mover.h
post.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h post.h
rnews.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftninet.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h rfc2ftn.h ftnfido.h ../paths.h rnews.h
storenet.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h msgflags.h rollover.h storenet.h
ftnfrearc.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfmove.h
utic.o: ../config.h ../lib/ftndlib.h orphans.h tic.h mover.h tic.h utic.h
announce.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h grlist.h msgutil.h toberep.h announce.h
fflist.o: ../config.h ../lib/ftndlib.h ../lib/msg.h fflist.h
ftn2rfc.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h rollover.h aliasdb.h postemail.h backalias.h msgflags.h rfc2ftn.h ftn2rfc.h
makestat.o: ../config.h ../lib/ftndlib.h ../lib/diesel.h ../lib/msg.h mgrutil.h makestat.h
ftnindex.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnindex.h
msgutil.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h msgutil.h
postecho.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h ftn2rfc.h postecho.h storeecho.h addpkt.h rollover.h qualify.h
rollover.o: ../config.h ../lib/ftndlib.h rollover.h
tic.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h unpack.h fsort.h orphans.h ptic.h mover.h tic.h
areamgr.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h ../lib/diesel.h sendmail.h mgrutil.h scan.h createm.h areamgr.h
filefind.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h ../lib/diesel.h fflist.h filefind.h msgutil.h
grlist.o: ../config.h ../lib/ftndlib.h grlist.h
maketags.o: ../config.h ../lib/ftndlib.h maketags.h
ftnmsg.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/ftnddb.h post.h ftnmsg.h
newspost.o: ../config.h ../lib/ftndlib.h ../lib/ftninet.h newspost.h
postemail.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/ftninet.h ../lib/msg.h postemail.h
scan.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h addpkt.h tracker.h ftn2rfc.h rfc2ftn.h rollover.h postemail.h scan.h
toberep.o: ../config.h ../lib/ftndlib.h orphans.h tic.h toberep.h
atoul.o: ../config.h ../lib/ftndlib.h atoul.h
filemgr.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h ../lib/diesel.h sendmail.h mgrutil.h createf.h filemgr.h
hash.o: ../config.h ../lib/ftndlib.h hash.h lhash.h
ftnaff.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h announce.h filefind.h ftnaff.h
ftnseq.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnseq.h
notify.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h filemgr.h areamgr.h sendmail.h mgrutil.h notify.h
postnetmail.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msg.h tracker.h addpkt.h storenet.h ftn2rfc.h areamgr.h filemgr.h ping.h bounce.h postemail.h
scannews.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftninet.h ../lib/ftnddb.h ../lib/msg.h ../lib/msgtext.h mkftnhdr.h hash.h rollover.h storeecho.h rfc2ftn.h scannews.h
tosspkt.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h tosspkt.h postnetmail.h postecho.h rollover.h createm.h
ftnfkill.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfkill.h ftnfutil.h
ftnfutil.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfile.h
ftnfindex.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/diesel.h ftnfutil.h ftnfindex.h
ftnfcheck.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfcheck.h
ftnfpack.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfpack.h
ftnflist.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnflist.h
ftnfadopt.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnflist.h
ftnfimport.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfimport.h
ftnfsort.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfsort.h
ftnftoberep.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnftoberep.h
ftnfmove.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfmove.h
ftnfdel.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ftnfutil.h ftnfmove.h
bounce.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/msg.h ../lib/msgtext.h ../lib/ftnddb.h sendmail.h postnetmail.h ping.h
createm.o: ../config.h ../lib/ftndlib.h ../lib/msg.h ../lib/users.h ../lib/ftnddb.h mgrutil.h createm.h
createf.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h mgrutil.h createf.h
msgflags.o: ../config.h ../lib/ftndlib.h msgflags.h
dirsession.o: ../config.h ../lib/ftndlib.h dirsession.h
queue.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h fsort.h dirsession.h queue.h
dirlock.o: ../config.h ../lib/ftndlib.h flock.h dirlock.h
msg.o: ../config.h ../lib/ftndlib.h ../lib/users.h ../lib/ftnddb.h ../lib/msgtext.h ../lib/msg.h msgflags.h msg.h
orphans.o: ../config.h ../lib/ftndlib.h orphans.h
# End of generated dependencies
