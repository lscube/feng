
feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libschedule.la
libschedule_la_SOURCES =	schedule_init.c \
				schedule_start.c \
				schedule_stop.c \
				schedule_do.c \
				schedule_resume.c \
				schedule_add.c \
				schedule_remove.c
			
libschedule_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
