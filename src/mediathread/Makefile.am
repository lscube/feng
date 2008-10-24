feniceincludedir = $(top_srcdir)/include

noinst_LIBRARIES = libmediath.a

libmediath_a_SOURCES =	cache.c \
			demuxer.c \
			InputStream.c \
			mediaparser.c \
			mediathread.c \
			mediautils.c \
			parser/mpeg12.c \
			parser/mpegaudio.c \
			parser/mpeg2t.c \
			demuxer/demuxer_sd.c \
			demuxer/demuxer_ds.c

if FENICE_FFMPEG
libmediath_a_SOURCES += parser/h264.c \
			parser/vorbis.c \
			parser/theora.c \
			parser/aac.c \
			parser/mp4ves.c \
			parser/speex.c \
			parser/h263.c \
			demuxer/demuxer_avf.c
endif


INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
