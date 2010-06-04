ACLOCAL_AMFLAGS = -I m4

FENG_AVROOT_DIR = $(feng_avroot_dir)
FENG_LOG_FILE = `basename $(feng_log_file)`

avrootdir = $(feng_avroot_dir)

dist_avroot_DATA = \
	avroot/h264.ds \
	avroot/h264_scramble.ds \
	avroot/test.mov

feng_confdir = $(feng_conf_dir)
dist_feng_conf_DATA = etc/feng.conf

dist_man_MANS = docs/feng.1

dist_doc_DATA = AUTHORS ChangeLog NEWS README RELEASE TODO

if CHANGELOG_REGEN
ChangeLog:
	-$(AM_V_GEN)GIT_DIR="${srcdir}/.git" git log --pretty > $@

# Always regenerate!
.PHONY: ChangeLog
endif

distcheck-hook:
	@test -s "${srcdir}"/ChangeLog || { echo "distcheck: empty ChangeLog found; make sure you have GIT installed" >/dev/stderr; false; }

bin_PROGRAMS = feng

RAGEL_SOURCES = \
	src/network/ragel_request_line.c \
	src/network/ragel_transport.c \
	src/network/ragel_range.c \
	src/network/ragel_headers.c \
	src/network/ragel_uri.c

BUILT_SOURCES = $(RAGEL_SOURCES) \
	src/network/rfc822proto.c \
	src/network/rfc822proto-constants.h

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@

dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

dist_feng_SOURCES = $(RAGEL_SOURCES) \
	\
	src/accesslog.c \
	src/bufferqueue.c src/bufferqueue.h \
	src/fnc_log.c src/fnc_log.h \
	src/incoming.c src/incoming.h \
	src/feng.h \
	src/main.c \
	src/conf/array.c \
	src/conf/array.h \
	src/conf/buffer.c \
	src/conf/buffer.h \
	src/conf/conf.h \
	src/conf/stream.c \
	src/conf/stream.h \
	src/conf/configparser.c \
	src/conf/configparser.h \
	src/conf/proc_open.c \
	src/conf/proc_open.h \
	src/conf/configfile-glue.c \
	src/conf/configfile.c \
	src/conf/configfile.h \
	src/conf/data_array.c \
	src/conf/data_integer.c \
	src/conf/data_config.c \
	src/conf/data_string.c \
	src/conf/settings.h \
	\
	src/network/netembryo.h \
	src/network/neb_sockaddr.c \
	src/network/ragel_parsers.h \
	src/network/rfc822proto.c \
	src/network/rfc822proto-constants.h \
	src/network/rfc822proto.h \
	src/network/rfc822_response.c \
	src/network/rtcp.c \
	src/network/rtp.c network/rtp.h \
	src/network/rtsp.h \
	src/network/rtsp_client.c \
	src/network/rtsp_lowlevel.c \
	src/network/rtsp_method_describe.c \
	src/network/rtsp_method_options.c \
	src/network/rtsp_method_pause.c \
	src/network/rtsp_method_play.c \
	src/network/rtsp_method_setup.c \
	src/network/rtsp_method_teardown.c \
	src/network/rtsp_state_machine.c \
	src/network/rtsp_utils.c \
	src/network/http_tunnelling.c \
	src/network/rtsp_interleaved.c \
	src/network/uri.c src/network/uri.h \
	\
	src/media/demuxer.c \
	src/media/demuxer.h \
	src/media/demuxer_module.h \
	src/media/mediaparser.c \
	src/media/mediaparser.h \
	src/media/mediaparser_module.h \
	src/media/mediautils.c \
	src/media/resource.c \
	src/media/parser/mpeg12.c \
	src/media/parser/mpegaudio.c \
	src/media/parser/mpeg2t.c \
	src/media/demuxer/demuxer_ds.c

if FENG_FFMPEG
dist_feng_SOURCES += src/media/parser/h264.c \
		     src/media/parser/vorbis.c \
		     src/media/parser/theora.c \
		     src/media/parser/aac.c \
		     src/media/parser/mp4ves.c \
		     src/media/parser/speex.c \
		     src/media/parser/h263.c \
		     src/media/parser/amr.c \
		     src/media/demuxer/demuxer_avf.c
endif

if LIVE_STREAMING
dist_feng_SOURCES += src/media/demuxer/demuxer_sd.c
endif

if HAVE_JSON
dist_feng_SOURCES += src/statistics.c
endif

if ENABLE_SCTP
dist_feng_SOURCES += src/network/rtsp_sctp.c
endif

if AVOID_TABLES
src/network/rfc822proto.c: src/network/rfc822proto-source-switch.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^
else
src/network/rfc822proto.c: src/network/rfc822proto-source-table.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^
endif

src/network/rfc822proto-constants.h: src/network/rfc822proto-header.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^

src/network/rfc822proto-statemachine.rl: src/network/rfc822proto-statemachine.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^

src/network/ragel_headers.rl: src/network/rfc822proto-statemachine.rl

src/network/rfc822proto-requestline.rl: src/network/rfc822proto-requestline.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^

src/network/ragel_request_line.c: src/network/rfc822proto-statemachine.rl src/network/rfc822proto-requestline.rl

tests/rfc822proto/rfc822proto-test.rl: tests/rfc822proto/rfc822proto-test.xsl src/network/rfc822proto.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $^

tests/rfc822proto/rfc822proto-test.c: src/network/rfc822proto-statemachine.rl

INCLUDES = -I. -I$(srcdir)/src -Isrc -I$(srcdir)/src/network -Isrc/network

AM_RAGELFLAGS = -I$(srcdir)/src/network -Isrc/network

if HAVE_RAGEL
include $(top_srcdir)/ragel.make

CLEANFILES = $(RAGEL_SOURCES)
endif

if BUILD_TESTS
TESTS = tests/testsuite

check_PROGRAMS = $(TESTS)

BUILT_SOURCES += tests/gtestmain.c \
	tests/rfc822proto/rfc822proto-test.c

tests/gtestmain.c: $(tests_testsuite_SOURCES) $(srcdir)/tests/genmain.awk Makefile.am
	@$(mkdir_p) $(dir $@)
	$(AM_V_GEN)$(EXUBERANT_CTAGS) -f - --file-scope=no $(filter %.c,$^) | \
		sort -k 2 | $(EGREP) '^test_' | \
		$(GAWK) -f $(srcdir)/tests/genmain.awk \
		> $@
endif

tests_testsuite_SOURCES = \
	src/bufferqueue.c \
	src/network/ragel_request_line.c \
	src/network/ragel_headers.c \
	src/network/ragel_transport.c \
	src/network/ragel_uri.c \
	src/network/uri.c \
	tests/bufferqueue/test-1.c \
	tests/bufferqueue/test-2.c \
	tests/bufferqueue/test-3.c \
	tests/bufferqueue/test-4.c \
	tests/bufferqueue/test-5.c \
	tests/rfc822proto/rfc822proto-test.c \
	tests/rfc822proto/request_line.c \
	tests/rfc822proto/headers.c \
	tests/rfc822proto/transport_header.c \
	tests/uri.c \
	tests/gtest-extra.h

# tests_testsuite_CFLAGS = -DFENG_BQ_DEBUG

dist_tests_testsuite_SOURCES = tests/gtestmain.c

EXTRA_DIST = tests/genmain.awk

DISTCLEANFILES = $(BUILT_SOURCES)
MAINTAINERCLEANFILES = ChangeLog
