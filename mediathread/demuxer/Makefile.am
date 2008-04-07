feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libdemuxer.la

libdemuxer_la_SOURCES =	demuxer_sd.c \
			demuxer_ds.c
if FENICE_FFMPEG
libdemuxer_la_SOURCES += demuxer_avf.c
endif

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
