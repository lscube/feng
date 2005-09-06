feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = liblog.la libutils.la libprefs.la

if FENICE_VERBOSE
noinst_LTLIBRARIES += libdebug.la
endif

libutils_la_SOURCES =	alloc_path_name.c \
			NTP_time.c \
			random32.c \
			get_UTC_time.c \
			get_field.c \
			is_supported_url.c

libutils_la_LDFLAGS = -static -lm


liblog_la_SOURCES =	fnc_log.c 

liblog_la_LDFLAGS = -static -lm


libprefs_la_SOURCES =	prefs_init.c \
			prefs_get_serv_root.c \
			prefs_get_hostname.c \
			prefs_get_port.c \
			prefs_get_max_session.c \
			prefs_get_log.c \
			prefs_use_default.c

libprefs_la_LDFLAGS = -static

if FENICE_VERBOSE
libdebug_la_SOURCES =	dump_buffer.c \
			dump_payload.c 

libdebug_la_LDFLAGS = -static
endif

#libplugins_la_SOURCES = load_plugins.c

#libplugins_la_LDFLAGS = -static
#libplugins_la_LIBADD = @LIBLTDL@

.NOTPARALLEL: fenice
	
bin_PROGRAMS = fenice

fenice_SOURCES =	main.c \
			fncheader.c

fenice_LDADD =	$(top_srcdir)/eventloop/libeventloop.la \
		$(top_srcdir)/rtsp/librtsp.la \
		$(top_srcdir)/sdp/libsdp.la \
		$(top_srcdir)/schedule/libschedule.la \
		$(top_srcdir)/intnet/libintnet.la \
		$(top_srcdir)/rtcp/librtcp.la \
		$(top_srcdir)/rtp/librtp.la \
		$(top_srcdir)/mediainfo/libmediainfo.la \
		$(top_srcdir)/bufferpool/libbufferpool.la \
		$(top_srcdir)/xmlparser/libxmlparser.la \
		$(top_srcdir)/multicast/libmulticast.la \
		$(top_srcdir)/socket/libsocket.la \
		$(top_srcdir)/command_environment/libcommand.la \
		$(top_srcdir)/src/libprefs.la \
		$(top_srcdir)/src/libutils.la \
		$(top_srcdir)/src/liblog.la \
		$(top_srcdir)/md5/libmd5.la

if FENICE_VERBOSE
fenice_LDADD += $(top_srcdir)/src/libdebug.la
endif

if FENICE_EFENCE
fenice_LDFLAGS = -lefence
endif

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
