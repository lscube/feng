
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libmediath.la

libmediath_la_SOURCES =	cache.c \
			demuxer.c \
			demuxer_sd.c \
			descriptionAPI.c \
			InputStream.c \
			MediaParser.c \
			mediathread.c \
			mediautils.c \
			mpeg12.c \
			mpegaudio.c \
			mpeg2t.c \
			demuxer_ds.c
if FENICE_FFMPEG
libmediath_la_SOURCES += demuxer_avf.c \
			 h264.c \
			 vorbis.c \
			 theora.c \
			 aac.c
endif

# libmediath_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
