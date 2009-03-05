feniceincludedir = $(top_srcdir)/include
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)/src

noinst_LIBRARIES = libnetwork.a

libnetwork_a_SOURCES = \
		rtcp.h \
		rtcp_handler.c \
		rtcp_recv_packet.c \
		rtcp_send_packet.c \
		rtp.h \
		rtp_port.c \
		rtp_transport.c \
		rtpptdefs.h \
		rtsp.h \
		rtsp_lowlevel.c \
		rtsp_method_describe.c \
		rtsp_method_options.c \
		rtsp_method_pause.c \
		rtsp_method_play.c \
		rtsp_method_setup.c \
		rtsp_method_teardown.c \
		rtsp_response.c \
		rtsp_state_machine.c \
		rtsp_utils.c \
		sdp2.h \
		sdp_session_descr.c

rtsp_state_machine.o: ragel_request_line.c
rtsp_method_setup.o: ragel_transport.c
rtsp_method_play.o: ragel_range.c

%.c: %.rl
	$(RAGEL) $< -o $@

CLEANFILES = \
	ragel_request_line.c \
	ragel_transport.c \
	ragel_range.c
