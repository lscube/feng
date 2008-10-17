feniceincludedir = $(top_srcdir)/include

noinst_LIBRARIES = libdemuxer.a

libdemuxer_a_SOURCES =	demuxer_sd.c \
			demuxer_ds.c
if FENICE_FFMPEG
libdemuxer_a_SOURCES += demuxer_avf.c
endif

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
