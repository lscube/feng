
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libstunserver.la
libstunserver_la_SOURCES =	binding_error_response.c \
				binding_response.c \
				OMSstunserver.c 
			
libstunserver_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
