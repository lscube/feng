SUBDIRS =	socket \
		md5 \
		xmlparser \
		multicast \
		command_environment \
		eventloop \
		rtp \
		rtcp \
		intnet \
		rtsp \
		mediainfo \
		sdp \
		schedule \
		src \
		docs

EXTRA_DIST = 	include \
		admin \
		templates

dist-hook:
	@for auxfile in config/*; do \
		if test -f $$auxfile; then \
			cp -p $$auxfile $(distdir)/libltdl; \
		fi; \
	done; \
	for plugin in $(distdir)/plugins/plugins/*; do \
		rm -rf $$plugin; \
	done
		
uninstall-local:
	@if test -d $(datadir)/nemesi/plugins; then\
		rmdir $(datadir)/nemesi/plugins;\
	fi
	@if test -d $(datadir)/nemesi; then\
		rmdir $(datadir)/nemesi;\
	fi


distclean-local:
	@if test -d autom4te.cache; then \
		rm -rf autom4te.cache; \
	fi

MAINTAINER_FILES = acinclude.m4 aclocal.m4 config.h.in* configure Makefile.in mkinstalldirs error.log
MAINTAINER_DIRS = config

maintainer-clean-local:
	@for mfile in $(MAINTAINER_FILES); do \
		test -f $$mfile && rm -f $$mfile; \
	done;\
	for mdir in $(MAINTAINER_DIRS); do \
		test -d $$mdir && rm -rf $$mdir; \
	done;\
	find -iname Makefile.in -exec rm \{} \;

svnclean: maintainer-clean
