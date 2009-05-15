# Copyright (c) M Broek, 1997-2000
# Top-level makefile for MBSE BBS html documentation.
# $Id: Makefile,v 1.43 2007/03/01 19:12:54 mbse Exp $

include ../Makefile.global

H_BASE		= dist.html manual.css errcodes.html \
		  flow.html postfix.html ups.html doors.html \
		  intergate.html intro.html invoking.html faq.html \
		  known_bugs.html mgetty.html routing.html nodelist.html

H_FTSC		= ftsc/index.htm

H_IMAGES	= images/b_arrow.png  images/magic.png        images/nodes1.png \
		  images/connec.png   images/mbsetup0.png     images/nodes2.png \
		  images/domains.png  images/mbsetup1.6.S.png images/nodes3.png \
		  images/e_menu.png   images/mbsetup1.6.png   images/nodes4.png \
		  images/emareas.png  images/mbsetup2.png     images/nodes5.png \
		  images/nodes6.png   images/nodes7.png       images/nodes8.png \
		  images/nodes9.png   images/nodes10.png      images/doors.png \
		  images/emgroup.png  images/modems0.png      images/oneliner.png \
		  images/fdb.png      images/newfiles.png     images/protocol.png \
		  images/fegroup.png  images/newgroups.png    images/rarrow.png \
		  images/fileecho.png images/nodelist.png     images/security.png \
		  images/filefind.png images/nodelist1.png    images/tty.png \
		  images/files.png    images/nodelist2.png    images/tty1.png \
		  images/go_to.png    images/nodelist3.png    images/tty2.png \
		  images/hatch.png    images/nodelist4.png \
		  images/language.png images/nodelist5.png    images/uarrow.png \
		  images/larrow.png   images/users.png        images/doors2.png \
		  images/mbse.png     images/taskmgr.png      images/mbsebbs-s.png \
		  images/mbsebbs.png  images/users2.png       images/hand.up.png \
		  images/dream1.png   images/dream2.png       images/dream3.png \
		  images/tdream1.png  images/tdream2.png      images/tdream3.png \
		  images/route1.png   images/route2.png       images/mbmon1.png \
		  images/mbmon2.png   images/mbmon1-tn.png    images/mbmon2-tn.png \
		  images/nodes11.png

H_LICENSE	= license/copying.html license/hydracom.html license/index.htm \
		  license/jam.html

H_MENUS		= menus/control.html  menus/index.htm     menus/menu100.html \
		  menus/menu300.html  menus/menu500.html  \
		  menus/menu0.html    menus/menu200.html  menus/menu400.html

H_MISC		= misc/dropfile.html   misc/fileid.html     misc/index.htm \
		  misc/jam.html        misc/semafore.html   misc/filefind.html \
		  misc/ftpserver.html  misc/ipmailer.html   misc/outbound.html \
		  misc/usleep.html     misc/webserver.html

H_PROGS		= programs/mbfido.html     programs/mbmon.html \
		  programs/mbtoberep.html  programs/mblogin.html \
		  programs/index.htm       programs/mbcico.html \
		  programs/mbfile.html     programs/mbmsg.html \
		  programs/mbseq.html      programs/mbuser.html \
		  programs/mbaff.html      programs/mbdiff.html \
		  programs/mbindex.html    programs/mbout.html \
		  programs/mbsetup.html    programs/mbuseradd.html \
		  programs/mball.html	   programs/mbnntp.html \
		  programs/mblang.html     programs/mbsebbs.html \
		  programs/mbstat.html     programs/mbpasswd.html \
		  programs/mbtask.html

H_SETUP		= setup/archiver.html  setup/index.htm     setup/bbs.html \
		  setup/language.html setup/oneliner.html \
		  setup/emareas.html   setup/magic.html    setup/mail.html \
		  setup/protocol.html  setup/emgroup.html  setup/chat.html \
		  setup/fdb.html       setup/security.html setup/sitedoc.html \
		  setup/fegroup.html   setup/modems.html   setup/softinfo.html \
		  setup/fidonet.html   setup/tic.html      \
		  setup/fileecho.html  setup/newfiles.html setup/filefind.html \
		  setup/newgroups.html setup/files.html    setup/nodes.html \
		  setup/ttyinfo.html   setup/global.html   setup/users.html \
		  setup/hatch.html     setup/virscan.html  setup/services.html \
		  setup/domains.html   setup/taskmgr.html  setup/route.html

H_OTHER		= index.htm basic.html upgrade.html

H_IN		= index.htm.in basic.html.in upgrade.html.in

FILES		= $(H_IN) $(H_BASE) $(H_FTSC) $(H_IMAGES) $(H_LICENSE) $(H_MENUS) $(H_MISC) $(H_PROGS) $(H_SETUP) Makefile

#########################################################################################################

all depend clean depend:

install:
		@echo "Installing html documentation in ${PREFIX}/html"
		@rm -Rf ${PREFIX}/html
		@mkdir -p ${PREFIX}/html
		@mkdir -p ${PREFIX}/html/ftsc
		@mkdir -p ${PREFIX}/html/images
		@mkdir -p ${PREFIX}/html/license
		@mkdir -p ${PREFIX}/html/menus
		@mkdir -p ${PREFIX}/html/misc
		@mkdir -p ${PREFIX}/html/programs
		@mkdir -p ${PREFIX}/html/setup
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/ftsc
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/images
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/license
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/menus
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/misc
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/programs
		@${CHOWN} ${OWNER}:${GROUP} ${PREFIX}/html/setup
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_BASE}    ${PREFIX}/html
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_OTHER}   ${PREFIX}/html
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_FTSC}    ${PREFIX}/html/ftsc
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_IMAGES}  ${PREFIX}/html/images
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_LICENSE} ${PREFIX}/html/license
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_MENUS}   ${PREFIX}/html/menus
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_MISC}    ${PREFIX}/html/misc
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_PROGS}   ${PREFIX}/html/programs
		@${INSTALL} -p -c -o ${OWNER} -g ${GROUP} -m 0444 ${H_SETUP}   ${PREFIX}/html/setup

clean:
		rm -f filelist

filelist:	Makefile
		BASE=`pwd`; \
		BASE=`basename $${BASE}`; \
		(for f in ${FILES}; \
			do echo ${PACKAGE}-${VERSION}/$${BASE}/$$f; \
		done) >filelist

