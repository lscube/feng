feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libutils.la libprefs.la libdebug.la

libutils_la_SOURCES =	alloc_path_name.c \
			NTP_time.c \
			random32.c \
			get_UTC_time.c \
			is_supported_url.c \
			round.c

libutils_la_LDFLAGS = -static

libprefs_la_SOURCES =	prefs_init.c \
			prefs_get_serv_root.c \
			prefs_get_hostname.c \
			prefs_get_port.c \
			prefs_use_default.c

libprefs_la_LDFLAGS = -static

libdebug_la_SOURCES =	dump_buffer.c

libdebug_la_LDFLAGS = -static

#libplugins_la_SOURCES = load_plugins.c

#libplugins_la_LDFLAGS = -static
#libplugins_la_LIBADD = @LIBLTDL@

bin_PROGRAMS = fenice

fenice_SOURCES =	main.c 
			

fenice_LDADD =	$(top_srcdir)/eventloop/libeventloop.la \
		$(top_srcdir)/rtsp/librtsp.la \
		$(top_srcdir)/sdp/libsdp.la \
		$(top_srcdir)/schedule/libschedule.la \
		$(top_srcdir)/intnet/libintnet.la \
		$(top_srcdir)/rtcp/librtcp.la \
		$(top_srcdir)/rtp/librtp.la \
		$(top_srcdir)/mediainfo/libmediainfo.la \
		$(top_srcdir)/bufferpool/libbufferpool.la \
		$(top_srcdir)/src/libprefs.la \
		$(top_srcdir)/src/libdebug.la \
		$(top_srcdir)/src/libutils.la \
		$(top_srcdir)/md5/libmd5.la \
		$(top_srcdir)/xmlparser/libxmlparser.la \
		$(top_srcdir)/socket/libsocket.la \
		$(top_srcdir)/command_environment/libcommand.la 

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
