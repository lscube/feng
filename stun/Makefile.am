OMSstunincludedir = $(top_srcdir)/include

noinst_LTLIBRARIES = libstun.la


libstun_la_SOURCES =	free_pkt_dev.c \
			create_transactionID.c \
			parse_atrs.c \
			parse_stun_message.c \
			stun_rand.c


libstun_la_SOURCES +=	parse_address.c \
			parse_change_request.c \
			parse_error_code.c \
			parse_message_integrity.c \
			parse_password.c \
			parse_unknown_attributes.c \
			parse_username.c

libstun_la_SOURCES +=	add_stun_atr_hdr.c \
			create-address.c \
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
