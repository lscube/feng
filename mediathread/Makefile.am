
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libmediath.la

libmediath_la_SOURCES =	cache.c \
			demuxer.c \
			demuxer_sd.c \
			demuxer_avf.c \
			descriptionAPI.c \
			InputStream.c \
			MediaParser.c \
			mediathread.c \
			mediautils.c \
			mpeg12.c \
			h264.c \
			mpegaudio.c 
			
			
libmediath_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
