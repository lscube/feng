feniceincludedir = $(top_srcdir)/include

SUBDIRS = parser \
	  demuxer \
	  metadata

noinst_LTLIBRARIES = libmediath.la

libmediath_la_LIBADD = parser/libparser.la \
		       demuxer/libdemuxer.la \
		       metadata/libmetadata.la

libmediath_la_SOURCES =	cache.c \
			demuxer.c \
			InputStream.c \
			mediaparser.c \
			mediathread.c \
			mediautils.c

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
