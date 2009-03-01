/* -*- c -*- */

%% machine rtsp_request;

static void ragel_parse_request(RTSP_Request *req, char *msg) {
    int cs;
    char *p = msg, *pe = p + strlen(msg), *eof, *s;

    /* Variables used for adding headers to the hash table */
    char *hdr, *hdr_val;
    size_t hdr_size, hdr_val_size;

    /* We want to express clearly which versions we support, so that we
     * can return right away if an unsupported one is found.
     */

    %%{
        SP = ' ';
        CRLF = "\r\n";

        action set_s {
            s = p;
        }

        action end_method {
            req->method = g_strndup(s, p-s);
        }
        
        Supported_Method =
            "DESCRIBE" @ { req->method_id = RTSP_ID_DESCRIBE; } |
            "OPTIONS" @ { req->method_id = RTSP_ID_OPTIONS; } |
            "PAUSE" @ { req->method_id = RTSP_ID_PAUSE; } |
            "PLAY" @ { req->method_id = RTSP_ID_PLAY; } |
            "SETUP" @ { req->method_id = RTSP_ID_SETUP; } |
            "TEARDOWN" @ { req->method_id = RTSP_ID_TEARDOWN; };

        Method = (Supported_Method | alpha+ )
            > set_s % end_method;

        action end_version {
            req->version = g_strndup(s, p-s);
        }

        Version = (alpha+ . '/' . [0-9] '.' [0-9]);

        action end_object {
            req->object = g_strndup(s, p-s);
        }
        
        Request_Line = (Supported_Method | Method) . SP 
            (print*) > set_s % end_object . SP
            Version > set_s % end_version . CRLF;

        action hdr_start {
            hdr = p;
        }

        action hdr_end {
            hdr_size = p-hdr;
        }

        action hdr_val_start {
            hdr_val = p;
        }

        action hdr_val_end {
            hdr_val_size = p-hdr_val;
        }

        Header = (alpha|'-')+ > hdr_start % hdr_end 
            . ':' . SP . print+ > hdr_val_start % hdr_val_end . CRLF;

        action save_header {
            {
                g_hash_table_insert(req->headers,
                                    g_strndup(hdr, hdr_size),
                                    g_strndup(hdr_val, hdr_val_size));
            }
        }

        main := Request_Line . ( Header @ save_header )+;

        write data;
        write init;
        write exec;
    }%%

    /** @todo this should return a boolean false when parsing fails */
}
