OMSstunincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libstun.la


libstun_la_SOURCES =	free_pkt_dev.c \
			parse_atrs.c \
			parse_stun_message.c 	


#libstun_la_SOURCES +=	parse-changed-address.c \
#			parse-change-request.c \
#			parse-error-code.c \
#			parse-mapped-address.c \
#			parse-message-integrity.c \
#			parse-password.c \
#			parse-reflected-from.c \
#			parse-response-address.c \
#			parse-source-address.c \
#			parse-unknown-attributes.c \
#			parse-username.c

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
