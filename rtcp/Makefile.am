
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = librtcp.la
librtcp_la_SOURCES =	RTCP_send_packet.c \
			RTCP_recv_packet.c \
			RTCP_handler.c \
			RTCP_get_pkt_lost.c \
			RTCP_get_fract_lost.c \
			RTCP_get_jitter.c \
			RTCP_get_RR_received.c \
			RTCP_get_total_packet.c \
			RTCP_get_total_payload_octet.c 
			
librtcp_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
