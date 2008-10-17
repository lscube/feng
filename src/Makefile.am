feniceincludedir = $(top_srcdir)/include

noinst_LIBRARIES = liblog.a

liblog_a_SOURCES = fnc_log.c 

bin_PROGRAMS = feng

feng_SOURCES =	main.c

feng_LDADD =	../eventloop/libeventloop.a \
		../rtsp/librtsp.a \
		../sdp/libsdp.a \
		../schedule/libschedule.a \
		../rtcp/librtcp.a \
		../rtp/librtp.a \
		../mediathread/libmediath.a \
		../mediathread/demuxer/libdemuxer.a \
		../mediathread/parser/libparser.a \
		../multicast/libmulticast.a

feng_LDADD +=	../conf/liblight.a \
		liblog.a

feng_LDFLAGS =

feng_LDADD += @GLIB_LIBS@ 

INCLUDES = -I$(feniceincludedir) -I..
