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

        action set_describe {
            req->method_id = RTSP_ID_DESCRIBE;
        }

        action set_options {
            req->method_id = RTSP_ID_OPTIONS;
        }

        action set_pause {
            req->method_id = RTSP_ID_PAUSE;
        }

        action set_play {
            req->method_id = RTSP_ID_PLAY;
        }

        action set_setup {
            req->method_id = RTSP_ID_SETUP;
        }

        action set_set_parameter {
            req->method_id = RTSP_ID_SET_PARAMETER;
        }

        action set_teardown {
            req->method_id = RTSP_ID_TEARDOWN;
        }
        
        action start_method {
            s = p;
        }

        action end_method {
            req->method = g_strndup(s, p-s);
        }

        Supported_Method =
            "DESCRIBE" @ set_describe |
            "OPTIONS" @ set_options |
            "PAUSE" @ set_pause |
            "PLAY" @ set_play |
            "SETUP" @ set_setup |
            "SET_PARAMETER" @ set_setup |
            "TEARDOWN" @ set_teardown;

        action method_not_implemented {
            return RTSP_NotImplemented;
        }

        Other_Method = (alpha+ - Supported_Method) %method_not_implemented;
        Method = (Supported_Method | Other_Method )
            > start_method % end_method;

        action version_not_supported {
            return RTSP_VersionNotSupported;
        }
        
        Supported_RTSP_Version = "RTSP/1.0";
        RTSP_Version = (alpha . '/' . [0-9] '.' [0-9]) - 
            Supported_RTSP_Version % version_not_supported;

        action start_object {
            s = p;
        }

        action end_object {
            req->object = g_strndup(s, p-s);
        }
        
        Request_Line = (Supported_Method | Method) . SP 
            (print*) > start_object % end_object . SP
            (Supported_RTSP_Version | RTSP_Version) . CRLF;

        main := Request_Line;

        write data;
        write init;
        write exec;
    }%%

    return RTSP_Ok;
}
