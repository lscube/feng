/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

/** @file
 * @brief Contains declarations of various RTSP utility structures and functions
 * 
 * Contains declaration of RTSP structures and constants for error management,
 * declaration of various RTSP requests validation functions and various
 * internal functions
 */

#include "rtsp_utils.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h> /* For SCNu64 */

#include <glib.h>

#include <fenice/utils.h>
#include <fenice/prefs.h>
#include "sdp2.h"
#include <fenice/fnc_log.h>

#undef send_reply

/**
 * @addtogroup RTSP
 * @{
 */

RTSP_Error const RTSP_Fatal_ErrAlloc = { {0, ""}, ERR_ALLOC };

/**
 * gets the reply message from a standard RTSP error code
 * @param err the code of the error
 * @return the string format of the error corresponding to the given code
 */
static const char const *get_stat(int err)
{
    static const struct {
        char token[36];
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
        "", -1}
    };
    int i;
    const RTSP_Error * err_data = get_RTSP_Error(err);

    if (err_data == NULL) {
        for (i = 0; status[i].code != err && status[i].code != -1; ++i);
        return status[i].token;
    }
    else
        return err_data->message.reply_str;
}

/**
 * Allocate and initialise a new RTSP client structure
 *
 * @param srv The feng server that accepted the connection.
 * @param client_sock The socket the client is connected to.
 *
 * @return A pointer to a newly allocated structure.
 *
 * @see rtsp_client_destroy()
 */
RTSP_buffer *rtsp_client_new(feng *srv, Sock *client_sock)
{
    RTSP_buffer *new = g_new0(RTSP_buffer, 1);

    new->srv = srv;
    new->sock = client_sock;
    new->out_queue = g_async_queue_new();

    new->session = g_new0(RTSP_session, 1);
    new->session->session_id = -1;
    new->session->srv = srv;

    return new;
}

static void interleaved_close_fds(gpointer element, gpointer user_data)
{
    RTSP_interleaved *intlvd = (RTSP_interleaved *)element;
    
    Sock_close(intlvd[0].local);
    Sock_close(intlvd[1].local);
    g_free(intlvd);
}

static void stop_ev(gpointer element, gpointer user_data)
{
    ev_io *io = element;
    feng *srv = user_data;
    ev_io_stop(srv->loop, io);
    g_free(io);
}

/**
 * Destroy and free resources for an RTSP client structure
 *
 * @param rtsp The client structure to destroy and free
 *
 * @see rtsp_client_new()
 */
void rtsp_client_destroy(RTSP_buffer *rtsp)
{
  GString *outbuf = NULL;

  if (rtsp->session != NULL) {
#if 0 // Do not use it, is just for testing...
    if (rtsp->session->resource->info->multicast[0]) {
      fnc_log(FNC_LOG_INFO,
	      "RTSP connection closed by client during"
	      " a multicast session, ignoring...");
      continue;
    }
#endif

    // Release all RTP sessions
    g_slist_foreach(rtsp->session->rtp_sessions, schedule_remove, NULL);
    g_slist_free(rtsp->session->rtp_sessions);

    // Close connection                     
    //close(rtsp->session->fd);
    // Release the mediathread resource
    mt_resource_close(rtsp->session->resource);
    // Release the RTSP session
    g_free(rtsp->session);
    rtsp->session = NULL;
    fnc_log(FNC_LOG_WARN,
	    "WARNING! RTSP connection truncated before ending operations.\n");
  }

  // close local fds
  g_slist_foreach(rtsp->interleaved, interleaved_close_fds, NULL);
  g_slist_foreach(rtsp->ev_io, stop_ev, rtsp->srv);

  g_slist_free(rtsp->interleaved);
  g_slist_free(rtsp->ev_io);

  // Remove the output queue
  g_async_queue_lock(rtsp->out_queue);
  while( (outbuf = g_async_queue_try_pop_unlocked(rtsp->out_queue)) )
    g_string_free(outbuf, TRUE);
  g_async_queue_unlock(rtsp->out_queue);
  g_async_queue_unref(rtsp->out_queue);
}

/**
 * RTSP Header and request parsing and validation functions
 * @defgroup rtsp_validation RTSP requests parsing and validation
 * @{
 */

/**
 * @brief Checks if the path required by the connection is inside the
 *        avroot.
 *
 * @param url The netembryo Url structure to validate.
 *
 * @retval RTSP_Ok The URL does not contian any forbidden sequence.
 * @retval RTSP_Forbidden The URL contains forbidden sequences that
 *         might have malicious intent.
 */
static RTSP_Error check_forbidden_path(Url *url)
{
    if ( strstr(url->path, "../") || strstr(url->path, "./") )
        return RTSP_Forbidden;

    return RTSP_Ok;
}

/**
 * Validates the url requested and sets up the connection informations
 * for the given url
 *
 * @param urlstr The string contianing the raw URL that has to be
 *               validated and split.
 * @param[out] url The netembryo Url structure to fill with the
 *                 validate Url.
 *
 * @retval RTSP_Ok The URL has been filled in url.
 * @retval RTSP_BadRequest The URL is malformed.
 */
static RTSP_Error validate_url(char *urlstr, Url * url)
{
    char *decoded_url = g_malloc(strlen(urlstr)+1);
    
    if ( Url_decode( decoded_url, urlstr, strlen(urlstr) ) < 0 )
      return RTSP_BadRequest;

    Url_init(url, decoded_url);

    g_free(decoded_url);

    if ( url->path == NULL ) {
      Url_destroy(url);
      return RTSP_BadRequest;
    }

    return RTSP_Ok;
}

/**
 * Extracts the required url from the buffer
 *
 * @param rtsp the buffer of the request from which to extract the
 *             url.
 *
 * @param[out] url_buffer the buffer where to write the url (must be
 *                        big enough).
 * 
 * @retval RTSP_Ok URL identified and copied in the buffer.
 * @retval RTSP_BadRequest URL not found in the buffer.
 */
static RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer)
{
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url_buffer)) {
        return RTSP_BadRequest;
    }

    return RTSP_Ok;
}

/**
 * @brief Takes care of extracting and validating an URL from the buffer
 *
 * @param rtsp The buffer from where to extract the URL
 * @param[out] url The netembryo Url structure where to save the buffer
 *
 * @retval RTSP_Ok The URL was found, validated and is allowed.
 * @retval RTSP_BadRequest URL not found or malformed.
 * @retval RTSP_Forbidden The URL contains forbidden character sequences.
 */
RTSP_Error rtsp_extract_validate_url(RTSP_buffer *rtsp, Url *url) {
  char urlstr[256];
  RTSP_Error error = RTSP_Ok;
  
  if ( (error = extract_url(rtsp, urlstr)).got_error )
    goto end;
  if ( (error = validate_url(urlstr, url)).got_error )
    goto end;
  if ( (error = check_forbidden_path(url)).got_error )
    goto end;

 end:
  return error;
}

/**
 * @brief Checks if the RTSP message is a request with supported
 *        options
 *
 * The HDR_REQUIRE header is not supported by feng so it will always
 * be not supported if present.
 * 
 * @param rtsp the buffer of the request to check
 *
 * @retval RTS_Ok No Require header present.
 * @retval RTSP_OptionNotSupported Require header present, not
 *         supported.
 */
RTSP_Error check_require_header(RTSP_buffer * rtsp)
{
    if (strstr(rtsp->in_buffer, HDR_REQUIRE))
        return RTSP_OptionNotSupported;

    return RTSP_Ok;
}

/**
 * @brief Gets the CSeq from the buffer
 * 
 * Search for the CSEQ header and fills rtsp->rtsp_cseq with its
 * value.
 *
 * @param rtsp the buffer of the request
 *
 * @retval RTSP_Ok No error.
 * @retval RTSP_BadRequest CSEQ header not found or not valid.
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
 * @brief Gets the ID of the requested session
 *
 * Looks for the Session ID in the RTSP buffer, and saves it in the
 * variable pointed at by @p session_id; the variable is set to zero
 * if no Session ID is found.
 *
 * @param rtsp the buffer from which to parse the Session ID
 * @param[out] session_id where to save the retrieved Session ID
 *
 * @retval RTSP_Ok No error.
 * @retval RTSP_SessionNotFound Session ID not valid.
 */
RTSP_Error get_session_id(RTSP_buffer * rtsp, guint64 * session_id)
{
    char * p;

    // Session
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%*s %"SCNu64, session_id) != 1)
            return RTSP_SessionNotFound;
    } else {
        *session_id = 0;
    }

    return RTSP_Ok;
}
/**
 * @}
 */

/**
 * @brief Print to log various informations about the user agent.
 *
 * @param rtsp the buffer containing the USER_AGENT header
 */
void log_user_agent(RTSP_buffer * rtsp)
{
    char * p, cut[256];

    if ((p = strstr(rtsp->in_buffer, HDR_USER_AGENT)) != NULL) {
        strncpy(cut, p, 255);
        cut[255] = '\0';
        if ((p = strstr(cut, RTSP_EL)) != NULL) {
            *p = '\0';
        }
        fnc_log(FNC_LOG_CLIENT, "%s", cut);
    } else
        fnc_log(FNC_LOG_CLIENT, "-");
}

/**
 * RTSP Message generation and logging functions
 * @defgroup rtsp_msg_gen RTSP Message Generation
 * @{
 */

/**
 * @brief Sends a reply message to the client
 *
 * @param err the message code
 * @param addon the text to append to the message
 * @param rtsp the buffer where to write the output message
 *
 * @retval ERR_NOERROR No error.
 * @retval ERR_ALLOC Not enough space to complete the request
 *
 * @note The return value is the one from @ref bwrite function.
 */
int send_reply(int err, const char *addon, RTSP_buffer * rtsp)
{
    GString *reply = rtsp_generate_response(err, rtsp->rtsp_cseq);
    int res;

    if (addon)
      g_string_append(reply, addon);

    res = bwrite(reply, rtsp);

    fnc_log(FNC_LOG_ERR, "%s %d - - ", get_stat(err), err);
    log_user_agent(rtsp);

    return res;
}

/**
 * @brief Redirects the client to another server
 *
 * @param rtsp the buffer of the rtsp connection
 * @param object the requested object for which the client is
 *               redirected
 *
 * @retval ERR_NOERROR No Error.
 * @retval ERR_ALLOC Not enough space to complete the request.
 *
 * @TODO This is not currently implemented for mediathread, so it
 *       results in an internal server error right now.
 * @TODO The non-mediathread code needs to be audited and cleaned up.
 */
int send_redirect_3xx(RTSP_buffer * rtsp, const char *object)
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
    //if(!g_ascii_strcasecmp(matching_descr->twin,"NONE") || !g_ascii_strcasecmp(matching_descr->twin,"")){
    if (!(matching_descr->flags & SD_FL_TWIN)) {
        send_reply(453, 0, rtsp);
        return ERR_NOERROR;
    }
    /* allocate buffer */
    mb_len = 2048;
    mb = g_malloc(mb_len);
    r = g_malloc(mb_len + 1512);
    if (!r || !mb) {
        fnc_log(FNC_LOG_ERR,
            "send_redirect(): unable to allocate memory\n");
        send_reply(500, 0, rtsp);    /* internal server error */
        if (r) {
            g_free(r);
        }
        if (mb) {
            g_free(mb);
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

    /** @TODO port to new interface! */
    bwrite(r, strlen(r), rtsp);

    g_free(mb);
    g_free(r);

    fnc_log(FNC_LOG_VERBOSE, "REDIRECT response sent.\n");
    return ERR_NOERROR;
#endif
}

/**
 * @brief Writes a GString to the output buffer of an RTSP connection
 *
 * @param buffer GString instance from which to get the data to send
 * @param rtsp where the output buffer is saved
 *
 * @retval ERR_NOERROR No error.
 *
 * @note This function destroys the buffer after completion.
 */
int bwrite(GString *buffer, RTSP_buffer * rtsp)
{
    g_async_queue_push(rtsp->out_queue, buffer);
    ev_io_start(rtsp->srv->loop, rtsp->ev_io_write);

    return ERR_NOERROR;
}

/**
 * @brief Add a timestamp to a GString
 * 
 * @param str GString instance to append the timestamp to
 *
 * Concatenates a GString instance with a time stamp in the format of
 * "Date: 23 Jan 1997 15:35:06 GMT"
 */
static void append_time_stamp(GString *str) {
  char buffer[39] = { 0, };

  time_t now = time(NULL);
  struct tm *t = gmtime(&now);

  strftime(buffer, 38, "Date: %a, %d %b %Y %H:%M:%S GMT" RTSP_EL,
	   t);
  
  g_string_append(str, buffer);
}

/**
 * @brief Generates the basic RTSP response string
 *
 * @param code The response code to use for generation.
 * @param cseq The CSeq value for the response.
 *
 * @return A new GString instance with the response heading.
 */
GString *rtsp_generate_response(guint code, guint cseq) {
  GString *response = g_string_new("");
  
  g_string_printf(response,
		  "%s %d %s" RTSP_EL
		  "CSeq: %u" RTSP_EL,
		  RTSP_VER, code, get_stat(code), cseq);

  return response;
}
/**
 * @}
 */

/**
 * @brief Generates a positive RTSP response string
 *
 * @param cseq The CSeq value for the response.
 * @param session Session-ID to provide with the response
 *                (will be omitted if zero)
 *
 * @return A new GString instance with the response heading.
 */
GString *rtsp_generate_ok_response(guint cseq, guint64 session) {
  GString *response = rtsp_generate_response(200, cseq);

  g_string_append_printf(response,
			 "Server: %s/%s" RTSP_EL,
			 PACKAGE, VERSION);

  append_time_stamp(response);
  
  if ( session != 0 )
    g_string_append_printf(response,
			    "Session: %" PRIu64 RTSP_EL,
			    session);

  return response;
}

/**
 * @}
 */
