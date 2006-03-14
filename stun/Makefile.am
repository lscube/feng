OMSstunincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libstun.la


libstun_la_SOURCES =	parse_atrs.c \
			parse_stun_message.c 	

libstun_la_SOURCES +=	changed-address.c \
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

libstun_la_SOURCES +=	create-address.c \
			create-changed-address.c \
			create-change-request.c \
			create-error-code.c \
			create-mapped-address.c \
			create-message-integrity.c \
			create-password.c \
			create-reflected-from.c \
			create-response-address.c \
			create-source-address.c \
			create-unknown-attributes.c \
			create-username.c

libstun_la_LDFLAGS = -static
INCLUDES = -I$(OMSstunincludedir) -I$(top_srcdir)
