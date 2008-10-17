feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

feng_SOURCES = main.c fnc_log.c

feng_LDADD =	../eventloop/libeventloop.a \
		../rtsp/librtsp.a \
		../sdp/libsdp.a \
		../schedule/libschedule.a \
		../rtcp/librtcp.a \
		../rtp/librtp.a \
		../mediathread/libmediath.a \
		../multicast/libmulticast.a \
		../conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I..
