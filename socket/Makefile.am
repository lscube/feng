
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libsocket.la
libsocket_la_SOURCES =	tcp_listen.c \
			tcp_accept.c \
			tcp_connect.c \
			tcp_read.c \
			tcp_write.c \
			tcp_close.c \
			udp_open.c \
			udp_connect.c \
			udp_close.c \
			get_address.c 
			
libsocket_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
