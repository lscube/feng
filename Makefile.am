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
		docs \
		etc \
		avroot
		

EXTRA_DIST = 	include \
		config \
		templates \
		INSTALL-FAST
		

dist-hook:
	@for svn in `find $(distdir) -iname .svn`; do \
		rm -rf $$svn; \
	done

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
