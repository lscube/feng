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

#include <netembryo/rtsp.h>
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
 * @brief Allocate and initialise a new RTSP session object
 *
 * @param rtsp The RTSP client for which to allocate the session
 *
 * @return A pointer to a newly allocated RTSP session object
 *
 * @see rtsp_session_free();
 */
RTSP_session *rtsp_session_new(RTSP_buffer *rtsp)
{
    RTSP_session *new = rtsp->session = g_slice_new0(RTSP_session);
    
    new->srv = rtsp->srv;
    new->session_id = g_strdup_printf("%08x%08x",
                                      g_random_int(),
                                      g_random_int());

    return new;
}

/**
 * @brief Free resources for a RTSP session object
 *
 * @param session Session to free resource for
 *
 * @see rtsp_session_new()
 */
void rtsp_session_free(RTSP_session *session)
{
    if ( !session )
        return;

    /* Release mediathread resource */
    mt_resource_close(session->resource);

    g_free(session->session_id);
    g_slice_free(RTSP_session, session);
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
    RTSP_buffer *new = g_slice_new0(RTSP_buffer);

    new->srv = srv;
    new->sock = client_sock;
    new->out_queue = g_async_queue_new();

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

    // Release all RTP sessions
    g_slist_foreach(rtsp->session->rtp_sessions, schedule_remove, NULL);
    g_slist_free(rtsp->session->rtp_sessions);

    // Close connection                     
    //close(rtsp->session->fd);

    rtsp_session_free(rtsp->session);
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

  g_slice_free(RTSP_buffer, rtsp);
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
 * @param req The request structure from where to extract the URL
 * @param[out] url The netembryo Url structure where to save the buffer
 *
 * This function already takes care of sending a 400 "Bad Request" response for
 * invalid URLs or a 403 "Forbidden" reply for paths that try to exit the
 * accessible sandbox.
 *
 * @retval true The URL was properly found and extracted
 * @retval false An error was found, and a reply was already sent.
 */
gboolean rtsp_request_get_url(RTSP_Request *req, Url *url) {
  if ( !validate_url(req->object, url) ) {
      rtsp_quick_response(req, RTSP_BadRequest);
      return false;
  }

  if ( !check_forbidden_path(url) ) {
      rtsp_quick_response(req, RTSP_Forbidden);
      return false;
  }

  return true;
}
/**
 * @}
 */

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
 * @brief Check if a method has been called in an invalid state.
 *
 * @param req Request for the method
 * @param invalid_state State where the method is not valid
 *
 * If the method was called in the given invalid state, this function responds
 * to the client with a 455 "Method not allowed in this state" response, which
 * contain the Allow header as specified by RFC2326 Sections 11.3.6 and 12.4.
 */
gboolean rtsp_check_invalid_state(const RTSP_Request *req,
                                  RTSP_Server_State invalid_state) {
    static const char *const valid_states[] = {
        [RTSP_SERVER_INIT] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN",
        [RTSP_SERVER_READY] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY",
        [RTSP_SERVER_PLAYING] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE"
        /* We ignore RECORDING state since we don't support it */
    };
    RTSP_Response *response;

    if ( req->client->session->cur_state != invalid_state )
        return true;

    response = rtsp_response_new(req, RTSP_InvalidMethodInState);
    
    g_hash_table_insert(response->headers,
                        g_strdup("Allow"),
                        g_strdup(valid_states[invalid_state]));

    rtsp_response_send(response);

    return false;
}

/**
 * @}
 */
