
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = librtsp.la
librtsp_la_SOURCES =	RTSP_state_machine.c \
			RTSP_handler.c \
			RTSP_initserver.c \
			RTSP_full_msg_rcvd.c \
			RTSP_msg_len.c \
			RTSP_remove_msg.c \
			RTSP_valid_response_msg.c \
			RTSP_discard_msg.c \
			RTSP_validate_method.c \
			RTSP_describe.c \
			RTSP_setup.c \
			RTSP_play.c \
			RTSP_pause.c \
			RTSP_teardown.c \
			RTSP_options.c \
			get_stat.c \
			send_reply.c \
			bwrite.c \
			add_time_stamp.c \
			send_describe_reply.c \
			send_setup_reply.c \
			send_play_reply.c \
			send_pause_reply.c \
			send_teardown_reply.c \
			send_options_reply.c  \
			send_redirect_3xx.c  \
			max_connection.c  \
			parse_url.c 
			 
						
librtsp_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
