feniceincludedir = ../include

noinst_LTLIBRARIES = liblog.la libsafectype.la libutils.la

if FENICE_VERBOSE
noinst_LTLIBRARIES += libdebug.la
endif

libutils_la_SOURCES = sdp_utils.c

liblog_la_SOURCES = fnc_log.c 

libsafectype_la_SOURCES = safe-ctype.c 

libsafectype_la_LDFLAGS = -lm

if FENICE_VERBOSE
libdebug_la_SOURCES =	dump_buffer.c \
			dump_payload.c 
endif

.NOTPARALLEL: feng
	
bin_PROGRAMS = feng

feng_SOURCES =	main.c \
			fncheader.c

feng_LDADD =	../eventloop/libeventloop.la \
		../rtsp/librtsp.la \
		../sdp2/libsdp2.la \
		../schedule/libschedule.la \
		../rtcp/librtcp.la \
		../rtp/librtp.la \
		../mediathread/libmediath.la \
		../multicast/libmulticast.la

feng_LDADD +=	../conf/libconf.la \
		../src/libutils.la \
		../src/liblog.la \
		../src/libsafectype.la

if FENICE_VERBOSE
feng_LDADD += ../src/libdebug.la
endif

feng_LDFLAGS =

feng_LDADD += @GLIB_LIBS@ 

INCLUDES = -I$(feniceincludedir) -I..
