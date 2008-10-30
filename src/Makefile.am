SUBDIRS = mediathread network .

feniceincludedir = $(top_srcdir)/include

bin_PROGRAMS = feng

feng_SOURCES = main.c fnc_log.c \
		eventloop.c eventloop.h \
		schedule.c \
		is_valid_multicast_address.c

feng_LDADD =	network/libnetwork.a \
		mediathread/libmediath.a \
		../conf/liblight.a

INCLUDES = -I$(feniceincludedir) -I..
