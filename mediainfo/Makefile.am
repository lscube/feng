
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libmediainfo.la
libmediainfo_la_SOURCES =	enum_media.c \
				media_is_empty.c \
				mediacpy.c \
				default_selection_criterion.c \
				search_media.c \
				get_media_entry.c \
				msec2tick.c \
				parse_SD_file.c \
				validate_stream.c \
				calculate_skip.c \
				read_dim.c \
				load_MPA.c \
				load_GSM.c \
				load_L16.c \
				load_MPV.c \
				load_MP4ES.c \
				load_MP2T.c \
				get_frame.c \
				read_PCM.c \
				read_MP3.c \
				read_GSM.c \
				read_H26L.c \
				read_MPEG_system.c \
				read_MPEG_ts.c \
				read_MPEG_video.c \
				read_MPEG4ES_video.c \
				read_MPEG_utils.c \
				mediaopen.c \
				mediaclose.c
			
			
libmediainfo_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
