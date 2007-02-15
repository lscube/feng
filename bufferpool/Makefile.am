
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libbufferpool.la
libbufferpool_la_SOURCES = 	fnc_ipc_name.c \
				OMSbuff_addpage.c \
				OMSbuff_dump.c \
				OMSbuff_free.c \
				OMSbuff_getslot.c \
				OMSbuff_new.c \
				OMSbuff_read.c \
				OMSbuff_ref.c \
				OMSbuff_sync.c \
				OMSbuff_shm_addpage.c \
				OMSbuff_shm_create.c \
				OMSbuff_shm_destroy.c \
				OMSbuff_shm_map.c \
				OMSbuff_shm_remap.c \
				OMSbuff_shm_unmap.c \
				OMSbuff_unref.c \
				OMSbuff_write.c \
				OMSbuff_isempty.c \
				OMSbuff_nextts.c \
				OMSbuff_getreader.c \
				OMSbuff_gotreader.c

libbufferpool_la_LDFLAGS = -static $(SHM_LIB)
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
