
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libintnet.la
libintnet_la_SOURCES =	stream_change.c \
			change_check.c \
			downgrade_MP3.c \
			downgrade_GSM.c \
			downgrade_L16.c \
			upgrade_MP3.c \
			upgrade_GSM.c \
			half_MP3.c \
			half_GSM.c \
			half_L16.c \
			priority_increase.c \
			priority_decrease.c
			
libintnet_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
