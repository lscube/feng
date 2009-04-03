/**
 * Copyright (c) 2004, Jan Kneschke, incremental
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the 'incremental' nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FN_CONF_H
#define FN_CONF_H
typedef enum { T_CONFIG_UNSET,
                T_CONFIG_STRING,
                T_CONFIG_SHORT,
                T_CONFIG_BOOLEAN,
                T_CONFIG_ARRAY,
                T_CONFIG_LOCAL,
                T_CONFIG_DEPRECATED,
                T_CONFIG_UNSUPPORTED
} config_values_type_t;

typedef enum { T_CONFIG_SCOPE_UNSET,
                T_CONFIG_SCOPE_SERVER,
                T_CONFIG_SCOPE_CONNECTION
} config_scope_type_t;

typedef struct {
        const char *key;
        void *destination;

        config_values_type_t type;
        config_scope_type_t scope;
} config_values_t;

typedef struct {
	short port;
	conf_buffer *bindhost;

	conf_buffer *errorlog_file;
	int errorlog_use_syslog;

    /* FIXME feng specific stuff */
        short first_udp_port;
        short buffered_frames;
        short loglevel;

//	unsigned short dont_daemonize;
//	conf_buffer *changeroot;
	conf_buffer *username;
	conf_buffer *groupname;

//	conf_buffer *pid_file;

//	conf_buffer *event_handler;

	conf_buffer *modules_dir;
//	conf_buffer *network_backend;
	array *modules;
	array *upload_tempdirs;

//	unsigned short max_worker;
	unsigned short max_fds;
	unsigned short max_conns;
//	unsigned short max_request_size;

//	unsigned short log_request_header_on_error;
//	unsigned short log_state_handling;
/*
	enum { STAT_CACHE_ENGINE_UNSET,
			STAT_CACHE_ENGINE_NONE,
			STAT_CACHE_ENGINE_SIMPLE,
#ifdef HAVE_FAM_H
			STAT_CACHE_ENGINE_FAM
#endif
	} stat_cache_engine;
	unsigned short enable_cores;
*/
} server_config;

typedef struct specific_config {
	array *mimetypes;

	/* virtual-servers */
	conf_buffer *document_root;
	conf_buffer *server_name;
/*
	conf_buffer *error_handler;
	conf_buffer *server_tag;
	conf_buffer *dirlist_encoding;
	conf_buffer *errorfile_prefix;

	unsigned short max_keep_alive_requests;
	unsigned short max_keep_alive_idle;
	unsigned short max_read_idle;
	unsigned short max_write_idle;
	unsigned short use_xattr;
	unsigned short follow_symlink;
	unsigned short range_requests;


	unsigned short log_file_not_found;
	unsigned short log_request_header;
	unsigned short log_request_handling;
	unsigned short log_response_header;
	unsigned short log_condition_handling;
*/

	/* server wide */
	conf_buffer *ssl_pemfile;
	conf_buffer *ssl_ca_file;
	conf_buffer *ssl_cipher_list;
	int ssl_use_sslv2;
	int is_ssl;

	int use_ipv6;

        int is_sctp;
        unsigned short sctp_max_streams;

        conf_buffer *cpd_port;
        conf_buffer *cpd_db_host;
        conf_buffer *cpd_db_user;
        conf_buffer *cpd_db_password;
        conf_buffer *cpd_db_name;

//	int allow_http11;
/*	unsigned short etag_use_inode;
	unsigned short etag_use_mtime;
	unsigned short etag_use_size;
	unsigned short force_lowercase_filenames;
        unsigned short max_request_size;
*/
//	unsigned short kbytes_per_second; /* connection kb/s limit */

	/* configside */
//	unsigned short global_kbytes_per_second; /*  */

//	off_t  global_bytes_per_second_cnt;
	/* server-wide traffic-shaper
	 *
	 * each context has the counter which is inited once
	 * a second by the global_kbytes_per_second config-var
	 *
	 * as soon as global_kbytes_per_second gets below 0
	 * the connected conns are "offline" a little bit
	 *
	 * the problem:
	 * we somehow have to loose our "we are writable" signal
	 * on the way.
	 *
	 */
//	off_t *global_bytes_per_second_cnt_ptr; /*  */

#ifdef USE_OPENSSL
	SSL_CTX *ssl_ctx;
#endif
} specific_config;

#endif // FN_CONF_H
