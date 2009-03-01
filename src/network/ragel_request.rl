/* -*- c -*- */

%% machine rtsp_request;

static int ragel_parse_request(RTSP_Request *req, char *msg) {
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
            "SET_PARAMETER" @ { req->method_id = RTSP_ID_SET_PARAMETER; } |
            "TEARDOWN" @ { req->method_id = RTSP_ID_TEARDOWN; };

        action method_not_implemented {
            return RTSP_NotImplemented;
        }

        Other_Method = (alpha+ - Supported_Method) %method_not_implemented;
        Method = (Supported_Method | Other_Method )
            > set_s % end_method;

        action version_not_supported {
            return RTSP_VersionNotSupported;
        }
        
        Supported_RTSP_Version = "RTSP/1.0";
        RTSP_Version = (alpha . '/' . [0-9] '.' [0-9]) - 
            Supported_RTSP_Version % version_not_supported;

        action end_object {
            req->object = g_strndup(s, p-s);
        }
        
        Request_Line = (Supported_Method | Method) . SP 
            (print*) > set_s % end_object . SP
            (Supported_RTSP_Version | RTSP_Version) . CRLF;

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

        action cseq_header {
            {
                /* Discard two bytes for \r\n */
                char *tmp = g_strndup(s, p-s-2);
                req->cseq = strtol(s, NULL, 0);
                g_free(tmp);
            }
        }
        
        Cseq_Header = "CSeq: " . (digit+ >set_s) . CRLF %cseq_header;

        action session_header {
            {
                /* Discard two bytes for \r\n */
                char *tmp = g_strndup(s, p-s-2);
                req->session_id = g_ascii_strtoull(s, NULL, 0);
                g_free(tmp);
            }
        }

        Session_Header = "Session: " . (digit+ >set_s) . CRLF %session_header;
        
        Known_Headers = Cseq_Header | Session_Header;
        Other_Headers = Header - Known_Headers;

        action save_header {
            {
                g_hash_table_insert(req->headers,
                                    g_strndup(hdr, hdr_size),
                                    g_strndup(hdr_val, hdr_val_size));
            }
        }

        main := Request_Line . ( Known_Headers | Other_Headers @ save_header )+;

        write data;
        write init;
        write exec;
    }%%

    return RTSP_Ok;
}
