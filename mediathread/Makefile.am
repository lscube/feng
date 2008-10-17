feniceincludedir = $(top_srcdir)/include

SUBDIRS = parser \
	  demuxer

noinst_LIBRARIES = libmediath.a

libmediath_a_SOURCES =	cache.c \
			demuxer.c \
			InputStream.c \
			mediaparser.c \
			mediathread.c \
			mediautils.c

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
