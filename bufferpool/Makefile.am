
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libbufferpool.la
libbufferpool_la_SOURCES = 	\
				OMSbuff_free.c \
				OMSbuff_getslot.c \
				OMSbuff_new.c \
				OMSbuff_read.c \
				OMSbuff_ref.c \
				OMSbuff_slotadd.c \
				OMSbuff_unref.c \
				OMSbuff_write.c \
				OMSbuff_isempty.c \
				OMSbuff_nextts.c \
				OMSbuff_getreader.c \
				OMSbuff_gotreader.c

libbufferpool_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
