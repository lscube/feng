feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

feng_SOURCES = main.c fnc_log.c \
		eventloop.c eventloop.h \
		schedule.c

feng_LDADD =	../rtsp/librtsp.a \
		../sdp/libsdp.a \
		../rtcp/librtcp.a \
		../rtp/librtp.a \
		../mediathread/libmediath.a \
		../multicast/libmulticast.a \
		../conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I..
