#include <RTSP_utils.h>

#include <stdio.h>
#include <string.h>

#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>

int check_forbidden_path(ConnectionInfo * cinfo)
{
    if ( strstr(cinfo->object, "../") || strstr(cinfo->object, "./") )
     	return 403;

    return 0;
}

int validate_url(char *url, ConnectionInfo * cinfo)
{
    switch (parse_url
        (url, cinfo->server, sizeof(cinfo->server), &cinfo->port, cinfo->object, sizeof(cinfo->object))) {
    case 1:        // bad request
        return 400;
        break;
    case -1:        // internal server error
        return 500;
        break;
    default:
        break;
    }

    if (strcmp(cinfo->server, prefs_get_hostname()) != 0) {    /* Currently this feature is disabled. */
        /* wrong server name */
        //      send_reply(404, 0 , rtsp);  /* Not Found */
        //      return ERR_NOERROR;
    }

    return 0;
}

int check_require_header(RTSP_buffer * rtsp)
{
	if (strstr(rtsp->in_buffer, HDR_REQUIRE))
        return 551;

    return 0;
}

int extract_url(RTSP_buffer * rtsp, char * url_buffer)
{
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url_buffer)) {
        return 400;
    }

    return 0;
}

int get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo)
{
    if (strstr(rtsp->in_buffer, HDR_ACCEPT) != NULL) {
        if (strstr(rtsp->in_buffer, "application/sdp") != NULL) {
            cinfo->descr_format = df_SDP_format;
        } else {
            return 551; // Add here new description formats
        }
    }

    return 0;
}

int get_cseq(RTSP_buffer * rtsp)
{
    char * p;

    if ( (p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL )
        return 400;
    else if ( sscanf(p, "%*s %d", &(rtsp->rtsp_cseq)) != 1 )
        return 400;

    return 0;
}

int get_session_description(ConnectionInfo * cinfo)
{
    int sdesc_error = sdp_session_descr(cinfo->object, cinfo->descr, sizeof(cinfo->descr));

    if ((sdesc_error))
    {
        fnc_log(FNC_LOG_ERR,"[SDP2] error");
        if (sdesc_error == ERR_NOT_FOUND)
            return 404;
        else if (sdesc_error == ERR_PARSE || sdesc_error == ERR_GENERIC ||
                 sdesc_error == ERR_ALLOC)
            return 500;
    }

    return 0;
}

int get_session_id(RTSP_buffer * rtsp, long int * session_id)
{
    char trash[255];
    char * p;

    // Session
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%254s %ld", trash, session_id) != 2)
            return 454;
    } else {
        *session_id = -1;
    }

    return 0;
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

