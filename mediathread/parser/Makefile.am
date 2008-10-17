feniceincludedir = $(top_srcdir)/include

noinst_LIBRARIES = libparser.a

libparser_a_SOURCES = mpeg12.c \
			mpegaudio.c \
			mpeg2t.c \
			mpeg.h

if FENICE_FFMPEG
libparser_a_SOURCES += h264.c \
			vorbis.c \
			theora.c \
			aac.c \
			mp4ves.c \
			speex.c \
			h263.c
endif

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
