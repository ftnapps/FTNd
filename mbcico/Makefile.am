## Process this file with automake to produce Makefile.in

EXTRA_DIST = README
SUBDIRS = .

noinst_PROGRAMS = mbcico mbout

mbcico_SOURCES = zmmisc.c zmrle.c zmrecv.c zmsend.c binkp.c \
xmsend.c xmrecv.c m7recv.c m7send.c hydra.c \
answer.c chat.c dial.c dietifna.c emsidat.c filelist.c \
openfile.c openport.c opentcp.c rdoptions.c yoohoo.c \
recvbark.c respfreq.c sendbark.c tcp.c tcpproto.c wazoo.c \
filetime.c ftsc.c atoul.c portsel.c \
ttyio.c lutil.c scanout.c emsi.c ulock.c \
callstat.c session.c call.c callall.c mbcico.c \
zmodem.h binkp.h config.h statetbl.h \
xmsend.h xmrecv.h m7recv.h m7send.h hydra.h \
answer.h chat.h dial.h dietifna.h emsidat.h filelist.h \
openfile.h openport.h opentcp.h rdoptions.h yoohoo.h \
recvbark.h respfreq.h sendbark.h tcp.h tcpproto.h wazoo.h \
filetime.h ftsc.h atoul.h portsel.h \
ttyio.h lutil.h scanout.h emsi.h ulock.h \
callstat.h session.h call.h callall.h mbcico.h

mbout_SOURCES = outstat.c nlinfo.c mbout.c scanout.c callstat.c \
outstat.h nlinfo.h scanout.h callstat.h

LDADD = ../lib/libmemwatch.a ../lib/libclcomm.a ../lib/libcommon.a \
../lib/libmsgbase.a ../lib/libdbase.a

install-exec-local:
	$(mkinstalldirs) $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 4751 mbcico $(bindir)
	$(INSTALL) -s -o @OWNER@ -g @GROUP@ -m 0711 mbout  $(bindir)


