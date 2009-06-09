feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

BUILT_SOURCES = \
	network/ragel_request_line.c \
	network/ragel_transport.c \
	network/ragel_range.c \
	liberis/headers_parser.c

dist_feng_SOURCES = $(BUILT_SOURCES) \
	bufferqueue.c bufferqueue.h \
	fnc_log.c fnc_log.h \
	incoming.c incoming.h \
	feng.h \
	feng_utils.h \
	main.c \
	conf/array.c \
	conf/array.h \
	conf/buffer.c \
	conf/buffer.h \
	conf/conf.h \
	conf/stream.c \
	conf/stream.h \
	conf/configparser.c \
	conf/configparser.h \
	conf/proc_open.c \
	conf/proc_open.h \
	conf/configfile-glue.c \
	conf/configfile.c \
	conf/configfile.h \
	conf/data_array.c \
	conf/data_count.c \
	conf/data_integer.c \
	conf/data_config.c \
	conf/data_string.c \
	conf/settings.h \
	\
	liberis/headers.h \
	liberis/utils.c liberis/utils.h \
	\
	network/ragel_parsers.h \
	network/rtcp.c \
	network/rtp.c network/rtp.h \
	network/rtp_port.c \
	network/rtsp.h \
	network/rtsp_client.c \
	network/rtsp_interleaved.c \
	network/rtsp_lowlevel.c \
	network/rtsp_method_describe.c \
	network/rtsp_method_options.c \
	network/rtsp_method_pause.c \
	network/rtsp_method_play.c \
	network/rtsp_method_setup.c \
	network/rtsp_method_teardown.c \
	network/rtsp_response.c \
	network/rtsp_state_machine.c \
	network/rtsp_utils.c \
	\
	media/demuxer.c \
	media/demuxer.h \
	media/demuxer_module.h \
	media/mediaparser.c \
	media/mediaparser.h \
	media/mediaparser_module.h \
	media/mediautils.c \
	media/resource.c \
	media/parser/mpeg12.c \
	media/parser/mpegaudio.c \
	media/parser/mpeg2t.c \
	media/demuxer/demuxer_ds.c

if FENG_CPD
dist_feng_SOURCES += media/metadata/cpd.c media/metadata/cpd.h
endif

if FENICE_FFMPEG
dist_feng_SOURCES += media/parser/h264.c \
		     media/parser/vorbis.c \
		     media/parser/theora.c \
		     media/parser/aac.c \
		     media/parser/mp4ves.c \
		     media/parser/speex.c \
		     media/parser/h263.c \
		     media/parser/amr.c \
		     media/demuxer/demuxer_avf.c
endif

if LIVE_STREAMING
dist_feng_SOURCES += media/demuxer/demuxer_sd.c
endif

INCLUDES = -I$(feniceincludedir) -I.. -I$(srcdir)/nework

if HAVE_RAGEL

include $(top_srcdir)/ragel.make

CLEANFILES = $(BUILT_SOURCES)
endif

DISTCLEANFILES = $(BUILT_SOURCES)
