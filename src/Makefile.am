feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = liblog.la

liblog_la_SOURCES = fnc_log.c 

bin_PROGRAMS = feng

feng_SOURCES =	main.c

feng_LDADD =	../eventloop/libeventloop.la \
		../rtsp/librtsp.la \
		../sdp/libsdp.la \
		../schedule/libschedule.la \
		../rtcp/librtcp.la \
		../rtp/librtp.la \
		../mediathread/libmediath.la \
		../multicast/libmulticast.la

feng_LDADD +=	../conf/liblight.la \
		liblog.la

feng_LDFLAGS =

feng_LDADD += @GLIB_LIBS@ 

INCLUDES = -I$(feniceincludedir) -I..
