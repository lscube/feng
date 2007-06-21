SUBDIRS =	bufferpool \
		mediathread \
		multicast \
		command_environment \
		eventloop \
		rtp \
		rtcp \
		rtsp

SUBDIRS +=	sdp2 \
		schedule \
		src \
		docs \
		etc \
		avroot

EXTRA_DIST = 	include \
		config \
		debian \
		INSTALL-FAST

FENICE_AVROOT_DIR = $(DESTDIR)@fenice_avroot_dir@
FENICE_LOG_FILE = `basename @fenice_log_file@`

dist-hook:
	@for svn in `find $(distdir) -iname .svn`; do \
		rm -rf $$svn; \
	done

distclean-local:
	@if test -d autom4te.cache; then \
		rm -rf autom4te.cache; \
	fi
