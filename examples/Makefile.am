## Process this file with automake to produce Makefile.in

SUBDIRS = .

EXTRA_DIST = etc.tar menus.tar txtfiles.tar

install-exec-local:
	@if [ ! -f $(sysconfdir)/mareas.data ]; then \
		tar xfC etc.tar $(sysconfdir) ; \
		echo "Installing default databases" ; \
	fi
	@if [ ! -f $(prefix)/english/menus/main.mnu ]; then \
		tar xfC menus.tar $(prefix)/english/menus ; \
		echo "Installing default english menus" ; \
	fi
	@if [ ! -f $(prefix)/english/txtfiles/main.ans ]; then \
		tar xfC txtfiles.tar $(prefix)/english/txtfiles ; \
		echo "Installing default english txtfiles" ; \
	fi

