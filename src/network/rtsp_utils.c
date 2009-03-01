/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h> /* For SCNu64 */

#include <netembryo/protocol_responses.h>
#include <glib.h>

#include <fenice/utils.h>
#include <fenice/prefs.h>
#include "sdp2.h"
#include "rtsp.h"
#include <fenice/fnc_log.h>

/**
 * @addtogroup RTSP
 * @{
 */

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
 * @brief Checks if the path required by the connection is inside the avroot.
 *
 * @param url The netembryo Url structure to validate.
 *
 * @retval true The URL does not contian any forbidden sequence.
 * @retval false The URL contains forbidden sequences that might have malicious
 *         intent.
 *
 * @todo This function should be moved to netembryo.
 * @todo This function does not check properly for paths.
 */
static gboolean check_forbidden_path(Url *url)
{
    if ( strstr(url->path, "../") || strstr(url->path, "./") )
        return false;

    return true;
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
 * @retval true The URL has been filled in url.
 * @retval false The URL is malformed.
 *
 * @todo This function should be moved to netembryo.
 */
static gboolean validate_url(char *urlstr, Url * url)
{
    char *decoded_url = g_uri_unescape_string(urlstr, NULL);
    if ( decoded_url == NULL )
        return false;

    Url_init(url, decoded_url);

    g_free(decoded_url);

    if ( url->path == NULL ) {
      Url_destroy(url);
      return false;
    }

    return true;
}

/**
 * @brief Takes care of extracting and validating an URL from the a request
 *        structure.
 *
 * @param rtsp Client to send the reply to
 * @param req The request structure from where to extract the URL
 * @param[out] url The netembryo Url structure where to save the buffer
 *
 * This function already takes care of sending a "Bad Request" reply for xinvalid
 * URLs or a "Forbidden" reply for paths that try to exit the accessible
 * sandbox.
 *
 * @retval true The URL was properly found and extracted
 * @retval false An error was found, and a reply was already sent.
 */
gboolean rtsp_request_get_url(RTSP_buffer *rtsp, RTSP_Request *req, Url *url) {
  if ( !validate_url(req->object, url) ) {
      rtsp_send_reply(rtsp, RTSP_BadRequest);
      return false;
  }

  if ( !check_forbidden_path(url) ) {
      rtsp_send_reply(rtsp, RTSP_Forbidden);
      return false;
  }

  return true;
}
/**
 * @}
 */

/**
 * RTSP Message generation and logging functions
 * @defgroup rtsp_msg_gen RTSP Message Generation
 * @{
 */

/**
 * @brief Sends a reply message to the client
 *
 * @param rtsp The buffer where to write the output message.
 * @param code Code of the response to send
 */
void rtsp_send_reply(const RTSP_buffer *rtsp, RTSP_ResponseCode code)
{
    GString *response = protocol_response_new(RTSP_1_0, code);

    rtsp_bwrite(rtsp, response);
    
    fnc_log(FNC_LOG_ERR, "%d - - ", code);
}

/**
 * @brief Writes a GString to the output buffer of an RTSP connection
 *
 * @param rtsp where the output buffer is saved
 * @param buffer GString instance from which to get the data to send
 *
 * @note The buffer has to be considered destroyed after calling this function
 *       (the writing thread will take care of the actual destruction).
 */
void rtsp_bwrite(const RTSP_buffer *rtsp, GString *buffer)
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
 *
 * @todo This should be moved to netembryo too.
 */
static void append_time_stamp(GString *str) {
  char buffer[37] = { 0, };

  time_t now = time(NULL);
  struct tm *t = gmtime(&now);

  strftime(buffer, 36, "Date: %a, %d %b %Y %H:%M:%S GMT", t);

  protocol_append_header(str, buffer);
}

/**
 * @brief Generates the basic RTSP response string
 *
 * @param code The response code
 * @param cseq The CSeq value for the response.
 *
 * @return A new GString instance with the response heading.
 */
GString *rtsp_generate_response(RTSP_ResponseCode code, guint cseq)
{
    GString *response = protocol_response_new(RTSP_1_0, code);
  
    protocol_append_header_uint(response, "CSeq", cseq);

    return response;
}

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
    GString *response = rtsp_generate_response(RTSP_Ok, cseq);

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

/**
 * @}
 */
