SUBDIRS = mediathread conf .

feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

BUILT_SOURCES = \
	network/ragel_request_line.c \
	network/ragel_transport.c \
	network/ragel_range.c \
	liberis/headers_parser.c

dist_feng_SOURCES = \
	liberis/headers.h \
	liberis/utils.c liberis/utils.h \
	network/ragel_parsers.h \
	network/rtcp.c \
	network/rtp.c network/rtp.h \
	network/rtp_port.c \
	network/rtsp.h \
	network/rtsp_interleaved.c \
	network/rtsp_lowlevel.c \
	network/rtsp_method_describe.c \
	network/rtsp_method_options.c \
	network/rtsp_method_pause.c \
	network/rtsp_method_play.c \
	network/rtsp_method_setup.c \
	network/rtsp_method_teardown.c \
	network/rtsp_response.c \
	network/rtsp_state_machine.c \
	network/rtsp_utils.c \
	network/sdp2.c network/sdp2.h \
	\
	bufferqueue.c bufferqueue.h \
	client_events.h client_events.c \
	fnc_log.c fnc_log.h \
	incoming.c incoming.h \
	feng.h \
	feng_utils.h \
	main.c \
	\
	$(BUILT_SOURCES)

feng_LDADD = \
		mediathread/libmediath.a \
		conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I.. -I$(srcdir)/nework

if HAVE_RAGEL

include $(top_srcdir)/ragel.make

$(BUILT_SOURCES): network

network:
	$(mkdir_p) $(builddir)/network

CLEANFILES = $(BUILT_SOURCES)
endif

DISTCLEANFILES = $(BUILT_SOURCES)
