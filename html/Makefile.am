## Process this file with automake to produce Makefile.in
# Makefile for @PACKAGE@-@VERSION@ html documentation

SUBDIRS = .

EXTRA_DIST = basic.html date.html dist.html manual.css \
flow.html postfix.html gwnews.html index.htm ups.html \
install.html intergate.html intro.html invoking.html \
known_bugs.html mgetty.html routing.html nodelist.html \
ftsc/fsc-0039.html  ftsc/fsc-0056.html  ftsc/fsc-0087.html \
ftsc/fsp-1003.html  ftsc/fsp-1009.html  ftsc/fts-0007.html \
ftsc/fsc-0046.html  ftsc/fsc-0057.html  ftsc/fsc-0091.html \
ftsc/fsp-1004.html  ftsc/fta-1005.html  ftsc/fts-0008.html \
ftsc/fsc-0048.html  ftsc/fsc-0059.html  ftsc/fsc-0092.html \
ftsc/fsp-1005.html  ftsc/fts-0001.html  ftsc/fts-0009.html \
ftsc/fsc-0049.html  ftsc/fsc-0062.html  ftsc/fsc-0093.html \
ftsc/fsp-1006.html  ftsc/fts-0004.html  ftsc/index.htm \
ftsc/fsc-0050.html  ftsc/fsc-0070.html  ftsc/fsp-1001.html \
ftsc/fsp-1007.html  ftsc/fts-0005.html  ftsc/fsc-0053.html \
ftsc/fsc-0072.html  ftsc/fsp-1002.html  ftsc/fsp-1008.html \
ftsc/fts-0006.html  ftsc/fsc-0035.html  ftsc/fsp-1010.html \
ftsc/fsp-1011.html  ftsc/ftscprod.html  ftsc/fsc-0088.html \
ftsc/fts-4001.html \
images/b_arrow.gif  images/magic.gif        images/nodes1.gif \
images/connec.gif   images/mbsetup0.gif     images/nodes2.gif \
images/domains.gif  images/mbsetup1.6.S.gif images/nodes3.gif \
images/e_menu.gif   images/mbsetup1.6.gif   images/nodes4.gif \
images/emareas.gif  images/mbsetup2.gif     images/nodes5.gif \
images/emgroup.gif  images/modems0.gif      images/oneliner.gif \
images/fdb.gif      images/newfiles.gif     images/protocol.gif \
images/fegroup.gif  images/newgroups.gif    images/rarrow.gif \
images/fileecho.gif images/nodelist.gif     images/security.gif \
images/filefind.gif images/nodelist1.gif    images/tty.gif \
images/files.gif    images/nodelist2.gif    images/tty1.gif \
images/go_to.gif    images/nodelist3.gif    images/tty2.gif \
images/hatch.gif    images/nodelist4.gif    images/tty3.gif \
images/language.gif images/nodelist5.gif    images/uarrow.gif \
images/larrow.gif   images/nodes.gif        images/users.gif \
images/mbse.jpg     images/taskmgr.gif \
license/copying.html license/hydracom.html license/index.htm \
license/jam.html \
menus/control.html  menus/index.htm     menus/menu100.html \
menus/menu300.html  menus/menu500.html  \
menus/menu0.html    menus/menu200.html  menus/menu400.html \
misc/dropfile.html   misc/fileid.html     misc/index.htm \
misc/jam.html        misc/semafore.html   misc/filefind.html \
misc/ftpserver.html  misc/ipmailer.html   misc/outbound.html \
misc/usleep.html \
programs/import.html     programs/mbchat.html \
programs/mbfido.html     programs/mbmon.html \
programs/mbtoberep.html \
programs/index.htm       programs/mbcico.html \
programs/mbfile.html     programs/mbmsg.html \
programs/mbseq.html      programs/mbuser.html \
programs/mbaff.html      programs/mbdiff.html \
programs/mbindex.html    programs/mbout.html \
programs/mbsetup.html    programs/mbuseradd.html \
programs/mball.html      programs/mbfbgen.html \
programs/mblang.html     programs/mbsebbs.html \
programs/mbstat.html     programs/mbpasswd.html \
programs/mbtask.html  	 programs/mbmail.html \
setup/archiver.html  setup/index.htm     setup/bbs.html \
setup/bbslist.html   setup/language.html setup/oneliner.html \
setup/emareas.html   setup/magic.html    setup/mail.html \
setup/protocol.html  setup/emgroup.html  setup/safe.html \
setup/fdb.html       setup/security.html setup/sitedoc.html \
setup/fegroup.html   setup/modems.html   setup/softinfo.html \
setup/fidonet.html   setup/tic.html      setup/timebank.html \
setup/fileecho.html  setup/newfiles.html setup/filefind.html \
setup/newgroups.html setup/files.html    setup/nodes.html \
setup/ttyinfo.html   setup/global.html   setup/users.html \
setup/hatch.html     setup/virscan.html  setup/services.html \
setup/domains.html   setup/taskmgr.html


install-exec-local:
	rm -Rf $(prefix)/html
	$(mkinstalldirs) $(prefix)/html
	@echo "Installing html documentation in $(prefix)/html"
	@cp -Pr $(EXTRA_DIST) $(prefix)/html
	chown -R @OWNER@.@GROUP@ $(prefix)/html
	chmod -R 0644 $(prefix)/html/*.htm*
	chmod -R 0644 $(prefix)/html/images/*.gif

