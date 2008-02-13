
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libconf.la
libconf_la_SOURCES = prefs.c

noinst_LTLIBRARIES += liblight.la
liblight_la_SOURCES = array.c \
		      buffer.c \
		      stream.c \
		      configparser.c \
		      proc_open.c \
		      configfile-glue.c \
		      configfile.c

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
