
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = librtp.la
librtp_la_SOURCES =	RTP_port.c \
                        RTP_transport.c
			
librtp_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
