
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libsdp.la
libsdp_la_SOURCES =	get_media_descr.c \
			get_SDP_descr.c \
			get_SDP_user_name.c \
			get_SDP_session_id.c \
			get_SDP_version.c 
			
libsdp_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
