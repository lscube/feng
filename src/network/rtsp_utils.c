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

#include <liberis/headers.h>
#include <glib.h>

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "mediathread/mediathread.h"

/**
 * @defgroup rtsp_utils Utility functions
 * @ingroup RTSP
 *
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
RTSP_session *rtsp_session_new(RTSP_Client *rtsp)
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

    /* Release all the connected RTP sessions */
    rtp_session_gslist_free(session->rtp_sessions);
    g_slist_free(session->rtp_sessions);

    /* Release mediathread resource */
    mt_resource_close(session->resource);

    g_free(session->session_id);
    g_slice_free(RTSP_session, session);
}


/**
 * RTSP Header and request parsing and validation functions
 * @defgroup rtsp_validation Requests parsing and validation
 * @ingroup rtsp_utils
 *
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
 * @brief Extract only the path from a request structure
 *
 * @param req The request structure from where to extract the URL
 *
 * @return A newly allocated string to be freed with free().
 *
 * @retval NULL The URL was not valid and a reply was already sent.
 *
 * @note the returned string is allocated by netembryo with malloc(),
 *       so it should not be freed with g_free()!
 */
char *rtsp_request_get_path(RTSP_Request *req) {
    Url url;
    char *ret;

    if ( !rtsp_request_get_url(req, &url) )
        return NULL;

    ret = url.path;
    /* Set the path to NULL so that it won't be deleted by
     * Url_destroy). */
    url.path = NULL;
    Url_destroy(&url);

    return ret;
}

/**
 * @brief Check the URL from a request structure without saving it
 *
 * @param req The request structure from where to check the URL
 *
 * @retval true The URL is valid and allowed
 * @retval false The URL is not not valid or forbidden
 *
 * @note This function will allocate and destroy the memory by itself,
 *       it's used where the actual URL is not relevant to the code
 */
gboolean rtsp_request_check_url(RTSP_Request *req) {
    Url url;

    if ( !rtsp_request_get_url(req, &url) )
        return false;

    Url_destroy(&url);

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
void rtsp_bwrite(RTSP_Client *rtsp, GString *buffer)
{
    /* Copy the GString into a GByteArray; we can avoid copying the
       data since both are transparent structures with a g_malloc'd
       data pointer.
     */
    GByteArray *outpkt = g_byte_array_new();
    outpkt->data = buffer->str;
    outpkt->len = buffer->len;

    /* make sure you don't free the actual data pointer! */
    g_string_free(buffer, false);

    g_queue_push_head(rtsp->out_queue, outpkt);
    ev_io_start(rtsp->srv->loop, &rtsp->ev_io_write);
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
                        g_strdup(eris_hdr_allow),
                        g_strdup(valid_states[invalid_state]));

    rtsp_response_send(response);

    return false;
}

/**
 * @}
 */
