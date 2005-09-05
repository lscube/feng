
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libmediath.la

libmediath_la_SOURCES =	cache.c \
			InputStream.c \
			MediaParser.c \
			mpeg12.c \
			demuxer.c \
			demuxer_sd.c 
			
			
libmediath_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
