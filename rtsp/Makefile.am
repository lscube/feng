
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = librtsp.la
librtsp_la_SOURCES =	RTSP_utils.c \
			RTSP_state_machine.c \
			RTSP_lowlevel.c \
			RTSP_setup.c \
			RTSP_play.c \
			RTSP_pause.c \
			RTSP_teardown.c \
			RTSP_options.c \
			RTSP_describe.c
			 
						
librtsp_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
