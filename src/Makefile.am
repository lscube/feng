feniceincludedir = ../include

noinst_LTLIBRARIES = liblog.la libsafectype.la libutils.la libprefs.la

if FENICE_VERBOSE
noinst_LTLIBRARIES += libdebug.la
endif

libutils_la_SOURCES =	NTP_time.c \
			random32.c \
			get_utc.c \
			get_field.c

libutils_la_LDFLAGS = -static -lm


liblog_la_SOURCES =	fnc_log.c 

liblog_la_LDFLAGS = -static -lm

libsafectype_la_SOURCES =	safe-ctype.c 

libsafectype_la_LDFLAGS = -static -lm

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
		../bufferpool/libbufferpool.la \
		../multicast/libmulticast.la
#		../xmlparser/libxmlparser.la
#		../intnet/libintnet.la
#		../mediainfo/libmediainfo.la
#		../sdp/libsdp.la

if FENICE_STUN
feng_LDADD += ../stunserver/libstunserver.la \
		../stun/libstun.la 
endif
		
feng_LDADD +=	../command_environment/libcommand.la \
		../src/libprefs.la \
		../src/libutils.la \
		../src/liblog.la \
		../src/libsafectype.la \
		../md5/libmd5.la \
		-lnetembryo

if FENICE_VERBOSE
feng_LDADD += ../src/libdebug.la
endif

feng_LDFLAGS =

if FENICE_EFENCE
feng_LDFLAGS += -lefence
endif


feng_LDADD += @GLIB_LIBS@

INCLUDES = -I$(feniceincludedir) -I..
