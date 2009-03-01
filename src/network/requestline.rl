/* -*- c -*- */

%% machine requestline;

static int ragel_parse_requestline(RTSP_Request *req, char *msg) {
    int cs;
    char *p = msg, *pe = p + strlen(msg), *eof, *s;

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

        Header = (alpha|'-')+  . ':' . SP . print+ . CRLF;

        action cseq_header {
            {
                /* Discard two bytes for \r\n */
                char *tmp = g_strndup(s, p-s-2);
                req->cseq = strtol(s, NULL, 0);
                g_free(tmp);
            }
        }
        
        Cseq_Header = "CSeq: " . (digit+ >set_s) . CRLF %cseq_header;

        Known_Headers = Cseq_Header;
        Other_Headers = Header - Known_Headers;

        main := Request_Line . ( Cseq_Header | Other_Headers )+;

        write data;
        write init;
        write exec;
    }%%

    return RTSP_Ok;
}
