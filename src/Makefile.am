SUBDIRS = mediathread conf .

feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

BUILT_SOURCES = \
	network/ragel_request_line.c \
	network/ragel_transport.c \
	network/ragel_range.c

dist_feng_SOURCES = \
	network/rtcp.h \
	network/rtcp_handler.c \
	network/rtcp_send_packet.c \
	network/rtp.h \
	network/rtp_port.c \
	network/rtp_transport.c \
	network/rtsp.h \
	network/rtsp_lowlevel.c \
	network/rtsp_method_describe.c \
	network/rtsp_method_options.c \
	network/rtsp_method_pause.c \
	network/rtsp_method_play.c \
	network/rtsp_method_setup.c network/rtsp_method_setup.h \
	network/rtsp_method_teardown.c \
	network/rtsp_response.c \
	network/rtsp_state_machine.c \
	network/rtsp_utils.c \
	network/sdp2.h \
	network/sdp_session_descr.c \
	\
	client_events.h client_events.c \
	eventloop.c eventloop.h \
	fnc_log.c \
	is_valid_multicast_address.c \
	main.c \
	schedule.c \
	\
	$(BUILT_SOURCES)

feng_LDADD = \
		mediathread/libmediath.a \
		conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I..

if HAVE_RAGEL

include $(top_srcdir)/ragel.make

$(BUILT_SOURCES): network

network:
	$(mkdir_p) $(builddir)/network

CLEANFILES = $(BUILT_SOURCES)
endif

DISTCLEANFILES = $(BUILT_SOURCES)
