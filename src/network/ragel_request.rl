/* -*- c -*- */

%% machine rtsp_request;

static void ragel_parse_request(RTSP_Request *req, const char *msg) {
    int cs;
    const char *p = msg, *pe = p + strlen(msg) + 1, *s, *eof;

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
            "DESCRIBE" % { req->method_id = RTSP_ID_DESCRIBE; } |
            "OPTIONS" % { req->method_id = RTSP_ID_OPTIONS; } |
            "PAUSE" % { req->method_id = RTSP_ID_PAUSE; } |
            "PLAY" % { req->method_id = RTSP_ID_PLAY; } |
            "SETUP" % { req->method_id = RTSP_ID_SETUP; } |
            "TEARDOWN" % { req->method_id = RTSP_ID_TEARDOWN; };

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

        Header = (alpha|'-')+  . ':' . SP . print+ . CRLF;

        action parse_headers {
            fprintf(stderr, "parse_headers\n");
            req->headers = eris_parse_headers(s, fpc-s);
        }

        main := Request_Line . (( Header )+ . CRLF) > set_s % parse_headers . 0;

        write data;
        write init;
        write exec;
    }%%

    /** @todo this should return a boolean false when parsing fails */
}
