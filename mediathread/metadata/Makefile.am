feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libmetadata.la

libmetadata_la_SOURCES = cpd.c

INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)

bin_PROGRAMS = cpd_test
cpd_test_SOURCES = cpd_test.c
cpd_test_CFLAGS = $(CFLAGS)
cpd_test_LDADD = libmetadata.la @GLIB_LIBS@

