SUBDIRS =	mediathread \
		multicast \
		conf \
		eventloop \
		rtp \
		rtcp \
		rtsp

SUBDIRS +=	sdp \
		schedule \
		src \
		docs \
		etc \
		avroot

noinst_HEADERS = \
	include/fenice/utils.h \
	include/fenice/mediaparser.h \
	include/fenice/rtp.h \
	include/fenice/sdp2.h \
	include/fenice/demuxer_module.h \
	include/fenice/description.h \
	include/fenice/eventloop.h \
	include/fenice/fnc_log.h \
	include/fenice/mediaparser_module.h \
	include/fenice/multicast.h \
	include/fenice/prefs.h \
	include/fenice/rtcp.h \
	include/fenice/rtpptdefs.h \
	include/fenice/schedule.h \
	include/fenice/server.h \
	include/fenice/InputStream.h \
	include/fenice/mediathread.h \
	include/fenice/rtsp.h \
	include/fenice/debug.h \
	include/fenice/intnet.h \
	include/fenice/sdp_grammar.h \
	include/fenice/demuxer.h \
	include/fenice/mediautils.h \
	include/conf.h \
	include/settings.h \
	include/stream.h \
	include/configfile.h \
	include/array.h \
	include/buffer.h

EXTRA_DIST = Doxyfile RELEASE

ACLOCAL_AMFLAGS = -I m4

FENICE_AVROOT_DIR = @fenice_avroot_dir@
FENICE_LOG_FILE = `basename @fenice_log_file@`

