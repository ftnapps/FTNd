## Makefile.am for mbsebbs ./lib

SUBDIRS = .

EXTRA_DIST = README ftscprod.006 mkprod.awk FAQ README.memwatch USING test.c memwatch.c.org

CONFIG_CLEAN_FILES = ftscprod.c

noinst_HEADERS = libs.h structs.h records.h mbse.h ansi.h bluewave.h

noinst_LIBRARIES = libclcomm.a libcommon.a libdbase.a libmsgbase.a libmbinet.a libmemwatch.a

libclcomm_a_SOURCES = clcomm.c client.c crc.c semafore.c signame.c clcomm.h

libcommon_a_SOURCES = ftscprod.c attach.c charconv_utf.c falists.c hdr.c msgflags.c parsedate.c rfcmsg.c unpacker.c \
batchrd.c charset.c ftn.c nodelist.c pktname.c \
charconv.c dostran.c ftnmsg.c mbfile.c nodelock.c rawio.c strcasestr.c \
charconv_hz.c execute.c expipe.c getheader.c mime.c noderecord.c rfcaddr.c strutil.c \
charconv_jp.c faddr.c gmtoffset.c packet.c rfcdate.c term.c common.h

libdbase_a_SOURCES = dbcfg.c dbdupe.c dbftn.c dbmsgs.c dbnode.c dbtic.c dbuser.c \
dbcfg.h dbdupe.h dbftn.h dbmsgs.h dbnode.h dbtic.h dbuser.h

libmsgbase_a_SOURCES = jam.h jammsg.c jammsg.h jamsys.h msg.c msg.h msgtext.c msgtext.h

libmbinet_a_SOURCES = mbinet.h nntp.c pop3.c smtp.c

libmemwatch_a_SOURCES = memwatch.c memwatch.h

BUILT_SOURCES = ftscprod.c

ftscprod.c:	ftscprod.???
		@AWK@ -F, -f mkprod.awk ftscprod.??? >ftscprod.c


