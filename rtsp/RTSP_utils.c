#include <RTSP_utils.h>

#include <stdio.h>
#include <string.h>

#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>

RTSP_Error const RTSP_Ok = { {0, ""}, FALSE };
RTSP_Error const RTSP_BadRequest = { {400, ""}, TRUE };
RTSP_Error const RTSP_InternalServerError = { {500, ""}, TRUE };
RTSP_Error const RTSP_Forbidden = { {403, ""}, TRUE };
RTSP_Error const RTSP_NotFound = { {404, ""}, TRUE };
RTSP_Error const RTSP_OptionNotSupported = { {551, ""}, TRUE };
RTSP_Error const RTSP_SessionNotFound = { {454, ""}, TRUE };

RTSP_Error const RTSP_Fatal_ErrAlloc = { {0, ""}, ERR_ALLOC };

void set_RTSP_Error(RTSP_Error * err, int reply_code, char * message)
{
    err->got_error = TRUE;
    err->message.reply_code = reply_code;
    strncpy(err->message.reply_str, message, MAX_REPLY_MESSAGE_LEN);
}

RTSP_Error check_forbidden_path(ConnectionInfo * cinfo)
{
    if ( strstr(cinfo->object, "../") || strstr(cinfo->object, "./") )
     	return RTSP_Forbidden;

    return RTSP_Ok;
}

RTSP_Error validate_url(char *url, ConnectionInfo * cinfo)
{
    switch (parse_url
        (url, cinfo->server, sizeof(cinfo->server), &cinfo->port, cinfo->object, sizeof(cinfo->object))) {
    case 1:        // bad request
        return RTSP_BadRequest;
        break;
    case -1:        // internal server error
        return RTSP_InternalServerError;
        break;
    default:
        break;
    }

    if (strcmp(cinfo->server, prefs_get_hostname()) != 0) {    /* Currently this feature is disabled. */
        /* wrong server name */
        //      send_reply(404, 0 , rtsp);  /* Not Found */
        //      return ERR_NOERROR;
    }

    return RTSP_Ok;
}

RTSP_Error check_require_header(RTSP_buffer * rtsp)
{
	if (strstr(rtsp->in_buffer, HDR_REQUIRE))
        return RTSP_OptionNotSupported;

    return RTSP_Ok;
}

RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer)
{
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url_buffer)) {
        return RTSP_BadRequest;
    }

    return RTSP_Ok;
}

RTSP_Error get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo)
{
    if (strstr(rtsp->in_buffer, HDR_ACCEPT) != NULL) {
        if (strstr(rtsp->in_buffer, "application/sdp") != NULL) {
            cinfo->descr_format = df_SDP_format;
        } else {
            return RTSP_OptionNotSupported; // Add here new description formats
        }
    }

    return RTSP_Ok;
}

RTSP_Error get_cseq(RTSP_buffer * rtsp)
{
    char * p;

    if ( (p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL )
        return RTSP_BadRequest;
    else if ( sscanf(p, "%*s %d", &(rtsp->rtsp_cseq)) != 1 )
        return RTSP_BadRequest;

    return RTSP_Ok;
}

RTSP_Error get_session_description(ConnectionInfo * cinfo)
{
    int sdesc_error = sdp_session_descr(cinfo->object, cinfo->descr, sizeof(cinfo->descr));

    if ((sdesc_error))
    {
        fnc_log(FNC_LOG_ERR,"[SDP2] error");
        if (sdesc_error == ERR_NOT_FOUND)
            return RTSP_NotFound;
        else if (sdesc_error == ERR_PARSE || sdesc_error == ERR_GENERIC ||
                 sdesc_error == ERR_ALLOC)
            return RTSP_InternalServerError;
    }

    return RTSP_Ok;
}

RTSP_Error get_session_id(RTSP_buffer * rtsp, long int * session_id)
{
    char * p;

    // Session
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%*s %ld", session_id) != 1)
            return RTSP_SessionNotFound;
    } else {
        *session_id = -1;
    }

    return RTSP_Ok;
}

void log_user_agent(RTSP_buffer * rtsp)
{
    char * p;

    if ((p = strstr(rtsp->in_buffer, HDR_USER_AGENT)) != NULL) {
        char cut[strlen(p)];
        strcpy(cut, p);
        p = strstr(cut, "\n");
        cut[strlen(cut) - strlen(p) - 1] = '\0';
        fnc_log(FNC_LOG_CLIENT, "%s", cut);
    } else
        fnc_log(FNC_LOG_CLIENT, "- ");
}

