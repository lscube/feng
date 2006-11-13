
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libeventloop.la
libeventloop_la_SOURCES =	eventloop.c \
				schedule_connections.c \
				add_client.c \
				rtsp_set_fdsets.c \
				rtsp_server.c
			
libeventloop_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
