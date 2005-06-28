
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libsocket.la
libsocket_la_SOURCES =	get_address.c \
			sock_ntop_host.c \
			tcp_accept.c \
			tcp_close.c \
			tcp_connect.c \
			tcp_listen.c \
			tcp_read.c \
			tcp_write.c \
			udp_close.c \
			udp_connect.c \
			udp_open.c
			
libsocket_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
