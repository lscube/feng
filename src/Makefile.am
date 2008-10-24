feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

feng_SOURCES = main.c fnc_log.c \
		eventloop.c eventloop.h \
		schedule.c \
		sdp_session_descr.c \
		is_valid_multicast_address.c

feng_LDADD =	../rtsp/librtsp.a \
		../rtcp/librtcp.a \
		../rtp/librtp.a \
		../mediathread/libmediath.a \
		../conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I..
