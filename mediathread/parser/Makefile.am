feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libparser.la

libparser_la_SOURCES = mpeg12.c \
			mpegaudio.c \
			mpeg2t.c

if FENICE_FFMPEG
libparser_la_SOURCES += h264.c \
			 vorbis.c \
			 theora.c \
			 aac.c \
			 mp4ves.c \
			 h263.c
endif

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
