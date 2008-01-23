feniceincludedir = ../include

noinst_LTLIBRARIES = liblog.la libsafectype.la libutils.la libprefs.la

if FENICE_VERBOSE
noinst_LTLIBRARIES += libdebug.la
endif

libutils_la_SOURCES =	get_field.c \
			sdp_utils.c

libutils_la_LDFLAGS = -lm


liblog_la_SOURCES =	fnc_log.c 

liblog_la_LDFLAGS = -lm

libsafectype_la_SOURCES =	safe-ctype.c 

libsafectype_la_LDFLAGS = -lm

libprefs_la_SOURCES = prefs.c

libprefs_la_LDFLAGS = -static

if FENICE_VERBOSE
libdebug_la_SOURCES =	dump_buffer.c \
			dump_payload.c 

libdebug_la_LDFLAGS = -static
endif

#libplugins_la_SOURCES = load_plugins.c

#libplugins_la_LDFLAGS = -static
#libplugins_la_LIBADD = @LIBLTDL@

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

feng_LDADD +=	../conf/libcommand.la \
		../src/libprefs.la \
		../src/libutils.la \
		../src/liblog.la \
		../src/libsafectype.la

if FENICE_VERBOSE
feng_LDADD += ../src/libdebug.la
endif

feng_LDFLAGS =

feng_LDADD += @GLIB_LIBS@ 

INCLUDES = -I$(feniceincludedir) -I..
