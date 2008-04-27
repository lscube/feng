feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = liblog.la libsafectype.la libutils.la

libutils_la_SOURCES = sdp_utils.c

liblog_la_SOURCES = fnc_log.c 

libsafectype_la_SOURCES = safe-ctype.c 

libsafectype_la_LDFLAGS = -lm

.NOTPARALLEL: feng
	
bin_PROGRAMS = feng

feng_SOURCES =	main.c \
			fncheader.c

feng_LDADD =	../eventloop/libeventloop.la \
		../rtsp/librtsp.la \
		../sdp/libsdp.la \
		../schedule/libschedule.la \
		../rtcp/librtcp.la \
		../rtp/librtp.la \
		../mediathread/libmediath.la \
		../multicast/libmulticast.la

feng_LDADD +=	../conf/liblight.la \
		../src/libutils.la \
		../src/liblog.la \
		../src/libsafectype.la

feng_LDFLAGS =

feng_LDADD += @GLIB_LIBS@ 

INCLUDES = -I$(feniceincludedir) -I..
