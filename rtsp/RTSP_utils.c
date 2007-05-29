/*
 *  $Id: $
 *
 *  Copyright (C) 2007 by team@streaming.polito.it
 *
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libnms is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libnms; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/** @file RTSP_utils.c
 * @brief Contains declarations of various RTSP utility structures and functions
 * 
 * Contains declaration of RTSP structures and constants for error management,
 * declaration of various RTSP requests validation functions and various
 * internal functions
 */

#include <RTSP_utils.h>

#include <stdio.h>
#include <string.h>

#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>

RTSP_Error const RTSP_Ok = { {200, "OK"}, FALSE };
RTSP_Error const RTSP_BadRequest = { {400, "Bad Request"}, TRUE };
RTSP_Error const RTSP_Forbidden = { {403, "Forbidden"}, TRUE };
RTSP_Error const RTSP_NotFound = { {404, "Not Found"}, TRUE };
RTSP_Error const RTSP_SessionNotFound = { {454, "Session Not Found"}, TRUE };
RTSP_Error const RTSP_InternalServerError = { {500, "Internal Server Error"}, TRUE };
RTSP_Error const RTSP_OptionNotSupported = { {551, "Option not supported"}, TRUE };

RTSP_Error const RTSP_Fatal_ErrAlloc = { {0, ""}, ERR_ALLOC };

//! number of currently active connections
extern int num_conn;

/**
 * sets an RTSP_Error to a specific error
 * @param err the pointer to the error variable to edit
 * @param reply_code the code of RTSP reply message
 * @param message the content of the RTSP reply message
 */
void set_RTSP_Error(RTSP_Error * err, int reply_code, char * message)
{
    err->got_error = TRUE;
    err->message.reply_code = reply_code;
    strncpy(err->message.reply_str, message, MAX_REPLY_MESSAGE_LEN);
}

/**
 * gets the reply message from a standard RTSP error code
 * @param err the code of the error
 * @return the string format of the error corresponding to the given code
 */
char const *get_stat(int err)
{
    struct {
        char *token;
        int code;
    } status[] = {
        {
        "Continue", 100}, {
        "Created", 201}, {
        "Accepted", 202}, {
        "Non-Authoritative Information", 203}, {
        "No Content", 204}, {
        "Reset Content", 205}, {
        "Partial Content", 206}, {
        "Multiple Choices", 300}, {
        "Moved Permanently", 301}, {
        "Moved Temporarily", 302}, {
        "Unauthorized", 401}, {
        "Payment Required", 402}, {
        "Method Not Allowed", 405}, {
        "Not Acceptable", 406}, {
        "Proxy Authentication Required", 407}, {
        "Request Time-out", 408}, {
        "Conflict", 409}, {
        "Gone", 410}, {
        "Length Required", 411}, {
        "Precondition Failed", 412}, {
        "Request Entity Too Large", 413}, {
        "Request-URI Too Large", 414}, {
        "Unsupported Media Type", 415}, {
        "Bad Extension", 420}, {
        "Invalid Parameter", 450}, {
        "Parameter Not Understood", 451}, {
        "Conference Not Found", 452}, {
        "Not Enough Bandwidth", 453}, {
        "Method Not Valid In This State", 455}, {
        "Header Field Not Valid for Resource", 456}, {
        "Invalid Range", 457}, {
        "Parameter Is Read-Only", 458}, {
        "Unsupported transport", 461}, {
        "Not Implemented", 501}, {
        "Bad Gateway", 502}, {
        "Service Unavailable", 503}, {
        "Gateway Time-out", 504}, {
        "RTSP Version Not Supported", 505}, {
        "Extended Error:", 911}, {
        NULL, -1}
    };
    int i;

    switch (err)
    {
        case 200:
            return RTSP_Ok.message.reply_str;
        case 400:
            return RTSP_BadRequest.message.reply_str;
        case 403:
            return RTSP_Forbidden.message.reply_str;
        case 404:
            return RTSP_NotFound.message.reply_str;
        case 454:
            return RTSP_SessionNotFound.message.reply_str;
        case 500:
            return RTSP_InternalServerError.message.reply_str;
        case 551:
            return RTSP_OptionNotSupported.message.reply_str;
        default:
            for (i = 0; status[i].code != err && status[i].code != -1; ++i);
            return status[i].token;
    }

}

/**
 * Checks if the path required by the connection is inside the avroot
 * @param cinfo the connection informations containing the path to be checked
 * @return RTSP_Ok or RTSP_Forbidden if the path is out of avroot
 */
RTSP_Error check_forbidden_path(ConnectionInfo * cinfo)
{
    if ( strstr(cinfo->object, "../") || strstr(cinfo->object, "./") )
     	return RTSP_Forbidden;

    return RTSP_Ok;
}

/**
 * Validates the url requested and sets up the connection informations for the given url
 * @param url the url of the request
 * @param cinfo where the connection informations retrieved from the url should be placed
 * @return RTSP_Ok or RTSP_BadRequest if the url is malformed (might return RTSP_InternalServerError on url parsing errors)
 */
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

/**
 * Checks if the RTSP message is a request of the supported options
 * actually HDR_REQUIRE header is not supported by feng.
 * @param rtsp the buffer of the request to check
 * @return RTSP_Ok if it is not present a REQUIRE header, RTSP_OptionNotSupported else
 */
RTSP_Error check_require_header(RTSP_buffer * rtsp)
{
	if (strstr(rtsp->in_buffer, HDR_REQUIRE))
        return RTSP_OptionNotSupported;

    return RTSP_Ok;
}

/**
 * Extracts the required url from the buffer
 * @param rtsp the buffer of the request from which to extract the url
 * @param url_buffer the buffer where to write the url (must be big enough)
 * @return RTSP_Ok or RTSP_BadRequest if no url is present
 */
RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer)
{
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url_buffer)) {
        return RTSP_BadRequest;
    }

    return RTSP_Ok;
}

/**
 * Gets the required media description format from the RTSP request
 * @param rtsp the buffer of the request
 * @param cinfo the connection informations where to store the description format
 * @return RTSP_Ok or RTSP_OptionNotSupported if the required format is not SDP
 */
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

/**
 * Gets the CSeq from the buffer
 * @param rtsp the buffer of the request
 * @return RTSP_Ok or RTSP_BadRequest if there is not a CSEQ header
 * @return or it is not possible to parse it
 */
RTSP_Error get_cseq(RTSP_buffer * rtsp)
{
    char * p;

    if ( (p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL )
        return RTSP_BadRequest;
    else if ( sscanf(p, "%*s %d", &(rtsp->rtsp_cseq)) != 1 )
        return RTSP_BadRequest;

    return RTSP_Ok;
}

/**
 * Gets the session description, the description will be saved in the given cinfo
 * @param the connection for which is required to retrieve the description
 * @return RTSP_Ok or RTSP_InternalServerError on various errors
 * @return might return RTSP_NotFound both if the file doesn't exist or
 * @return if a demuxer is not available for the given file
 */
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

/**
 * Gets the id of the requested session
 * @param rtsp the buffer from which to parse the session id
 * @param session_id where to save the retrieved session id
 * @return RTSP_Ok or RTSP_SessionNotFound if it is not possible to parse the id
 */
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

/**
 * prints out various informations about the user agent
 * @param rtsp the buffer containing the USER_AGENT header
 */
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


// ------------------------------------------------------
// Message Functions
// ------------------------------------------------------

/**
 * Sends a reply message to the client
 * @param err the message code
 * @param addon the text to append to the message
 * @param rtsp the buffer where to write the output message
 * @return ERR_NOERROR or ERR_ALLOC if it was not possible to allocate enough space
 */
int send_reply(int err, char *addon, RTSP_buffer * rtsp)
{
    unsigned int len;
    char *b;
    char *p;
    int res;
    char method[32];
    char object[256];
    char ver[32];


    if (addon != NULL) {
        len = 256 + strlen(addon);
    } else {
        len = 256;
    }

    b = (char *) malloc(len);
    if (b == NULL) {
        fnc_log(FNC_LOG_ERR,
            "send_reply(): memory allocation error.\n");
        return ERR_ALLOC;
    }
    memset(b, 0, sizeof(b));
    sprintf(b, "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL, RTSP_VER, err,
        get_stat(err), rtsp->rtsp_cseq);
    //---patch coerenza con rfc in caso di errore
    // strcat(b, "\r\n");
    strcat(b, RTSP_EL);

    res = bwrite(b, (unsigned short) strlen(b), rtsp);
    free(b);

    sscanf(rtsp->in_buffer, " %31s %255s %31s ", method, object, ver);
    fnc_log(FNC_LOG_ERR, "%s %s %s %d - - ", method, object, ver, err);
    if ((p = strstr(rtsp->in_buffer, HDR_USER_AGENT)) != NULL) {
        char cut[strlen(p)];
        strcpy(cut, p);
        cut[strlen(cut) - 1] = '\0';
        fnc_log(FNC_LOG_CLIENT, "%s", cut);
    }

    return res;
}

/**
 * Redirects the client to another server
 * @param rtsp the buffer of the rtsp connection
 * @param object the requested object for which the client is redirected
 * @return ERR_NOERROR or ERR_ALLOC if it was not possible to allocate enough space
 */
int send_redirect_3xx(RTSP_buffer * rtsp, char *object)
{
#if ENABLE_MEDIATHREAD
#warning Write mt equivalent
        send_reply(500, 0, rtsp);       /* Internal server error */
        return ERR_NOERROR;
#else
    char *r;        /* get reply message buffer pointer */
    uint8 *mb;        /* message body buffer pointer */
    uint32 mb_len;
    SD_descr *matching_descr;

    if (enum_media(object, &matching_descr) != ERR_NOERROR) {
        fnc_log(FNC_LOG_ERR,
            "SETUP request specified an object file which can be damaged.\n");
        send_reply(500, 0, rtsp);    /* Internal server error */
        return ERR_NOERROR;
    }
    //if(!strcasecmp(matching_descr->twin,"NONE") || !strcasecmp(matching_descr->twin,"")){
    if (!(matching_descr->flags & SD_FL_TWIN)) {
        send_reply(453, 0, rtsp);
        return ERR_NOERROR;
    }
    /* allocate buffer */
    mb_len = 2048;
    mb = malloc(mb_len);
    r = malloc(mb_len + 1512);
    if (!r || !mb) {
        fnc_log(FNC_LOG_ERR,
            "send_redirect(): unable to allocate memory\n");
        send_reply(500, 0, rtsp);    /* internal server error */
        if (r) {
            free(r);
        }
        if (mb) {
            free(mb);
        }
        return ERR_ALLOC;
    }
    /* build a reply message */
    sprintf(r,
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 302, get_stat(302), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    sprintf(r + strlen(r), "Location: %s" RTSP_EL, matching_descr->twin);    /*twin of the first media of the aggregate movie */

    strcat(r, RTSP_EL);


    bwrite(r, (unsigned short) strlen(r), rtsp);

    free(mb);
    free(r);

    fnc_log(FNC_LOG_VERBOSE, "REDIRECT response sent.\n");
#endif
    return ERR_NOERROR;

}

/**
 * Writes a buffer to the output buffer of an RTSP connection
 * @param buffer the buffer from which to get the data to send
 * @param len the size of the data to send
 * @param rtsp where the output buffer is saved
 * @return ERR_NOERROR or ERR_ALLOC if it was not possible to allocate enough space
 */
int bwrite(char *buffer, unsigned short len, RTSP_buffer * rtsp)
{
    if ((rtsp->out_size + len) > (int) sizeof(rtsp->out_buffer)) {
        fnc_log(FNC_LOG_ERR,
            "bwrite(): not enough free space in out message buffer.\n");
        return ERR_ALLOC;
    }
    memcpy(&(rtsp->out_buffer[rtsp->out_size]), buffer, len);
    rtsp->out_buffer[rtsp->out_size + len] = '\0';
    rtsp->out_size += len;
    return ERR_NOERROR;
}

/**
 * Checks if the number of active connections is greater then the maximum of supported
 * return ERR_NOERROR or ERR_GENERIC if the number of connections is greater then the maximum
 */
int max_connection()
{
    if (num_conn <= prefs_get_max_session())
        return ERR_NOERROR;

    return ERR_GENERIC;
}

/**
 * Adds a timestamp to a buffer
 * @param b the buffer where to write the timestamp
 * @crlf whenever to put the message terminator or not
 */
void add_time_stamp(char *b, int crlf)
{
    struct tm *t;
    time_t now;

    /*!
     * concatenates a null terminated string with a
     * time stamp in the format of "Date: 23 Jan 1997 15:35:06 GMT"
     */
    now = time(NULL);
    t = gmtime(&now);
    strftime(b + strlen(b), 38, "Date: %a, %d %b %Y %H:%M:%S GMT" RTSP_EL,
         t);
    if (crlf)
        strcat(b, "\r\n");    /* add a message header terminator (CRLF) */
}

/**
 * Parses an url giving back the server, the port and the requested file
 * @param url the url to parse
 * @param server where to save the server address
 * @param server_len the maximum size of the server address buffer
 * @param port where to save the connection port
 * @param file_name where to save the requested file name
 * @param file_name_len the maximum size of the file name buffer
 * @return 0 if the URL is valid, 1 if the URL is not valid, -1 on internal error (some buffer too small)
 */
int parse_url(const char *url, char *server, size_t server_len,
          unsigned short *port, char *file_name, size_t file_name_len)
// Note: this routine comes from OMS
{
    /* expects format '[rtsp://server[:port/]]filename' */

    int not_valid_url = 1;
    /* copy url */
    char *full = malloc(strlen(url) + 1);
    strcpy(full, url);
    if (strncmp(full, "rtsp://", 7) == 0) {
        char *token;
        int has_port = 0;
        /* BEGIN Altered by Ed Hogan, Trusted Info. Sys. Inc. */
        /* Need to look to see if it has a port on the first host or not. */
        char *aSubStr = malloc(strlen(url) + 1);
        strcpy(aSubStr, &full[7]);
        if (strchr(aSubStr, '/')) {
            int len = 0;
            unsigned short i = 0;
            char *end_ptr;
            end_ptr = strchr(aSubStr, '/');
            len = end_ptr - aSubStr;
            for (; (i < strlen(url)); i++)
                aSubStr[i] = 0;
            strncpy(aSubStr, &full[7], len);
        }
        if (strchr(aSubStr, ':'))
            has_port = 1;
        free(aSubStr);
        /* END   Altered by Ed Hogan, Trusted Info. Sys. Inc. */

        token = strtok(&full[7], " :/\t\n");
        if (token) {
            strncpy(server, token, server_len);
            if (server[server_len - 1]) {
                free(full);
                return -1;    // internal error
            }
            if (has_port) {
                char *port_str =
                    strtok(&full[strlen(server) + 7 + 1],
                       " /\t\n");
                if (port_str)
                    *port = (unsigned short) atol(port_str);
                else
                    *port = FENICE_RTSP_PORT_DEFAULT;
            } else
                *port = FENICE_RTSP_PORT_DEFAULT;
            /* don't require a file name */
            not_valid_url = 0;
            token = strtok(NULL, " ");
            if (token) {
                strncpy(file_name, token, file_name_len);
                if (file_name[file_name_len - 1]) {
                    free(full);
                    return -1;    // internal error
                }
            } else
                file_name[0] = '\0';
        }
    } else {
        /* try just to extract a file name */
        char *token = strtok(full, " \t\n");
        if (token) {
            strncpy(file_name, token, file_name_len);
            if (file_name[file_name_len - 1]) {
                free(full);
                return -1;    // internal error
            }
            server[0] = '\0';
            not_valid_url = 0;
        }
    }
    free(full);
    return not_valid_url;
}
