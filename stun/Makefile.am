feniceincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libstun.la

libstun_la_SOURCES =	changed-address.c \
			change-request.c \
			error-code.c \
			mapped-address.c \
			message-integrity.c \
			password.c \
			reflected-from.c \
			response-address.c \
			source-address.c \
			unknown-attributes.c \
			username.c

libstun_la_LDFLAGS = -static
INCLUDES = -I$(feniceincludedir) -I$(top_srcdir)
