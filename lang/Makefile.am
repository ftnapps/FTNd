## Process this file with automake to produce Makefile.in
## There are special tricks in this one ....

SUBDIRS = .
LIBS =

EXTRA_DIST = Language.xref

CONFIG_CLEAN_FILES = english.lang dutch.lang italian.lang spanish.lang galego.lang

noinst_PROGRAMS    = english.lang dutch.lang italian.lang spanish.lang galego.lang

english_lang_SOURCES = english.txt
dutch_lang_SOURCES   = dutch.txt
italian_lang_SOURCES = italian.txt
spanish_lang_SOURCES = spanish.txt
galego_lang_SOURCES  = galego.txt



install-exec-local:
	$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0444 english.lang $(sysconfdir)
	$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0444 dutch.lang   $(sysconfdir)
	$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0444 italian.lang $(sysconfdir)
	$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0444 spanish.lang $(sysconfdir)
	$(INSTALL) -o @OWNER@ -g @GROUP@ -m 0444 galego.lang  $(sysconfdir)

english.lang:   english.txt
		../mbsebbs/mblang english.lang english.txt

dutch.lang:     dutch.txt
		../mbsebbs/mblang dutch.lang dutch.txt

italian.lang:	italian.txt
		../mbsebbs/mblang italian.lang italian.txt

spanish.lang:	spanish.txt
		../mbsebbs/mblang spanish.lang spanish.txt

galego.lang:	galego.txt
		../mbsebbs/mblang galego.lang galego.txt

