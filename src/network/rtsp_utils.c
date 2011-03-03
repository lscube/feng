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

#include <glib.h>

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "uri.h"
#include "media/demuxer.h"

/**
 * @defgroup rtsp_utils Utility functions
 * @ingroup RTSP
 *
 * @{
 */

/**
 * @brief Append a range to an RTSP session's editlist
 *
 * @param session The RTSP session to append the range to
 * @param range The range to append
 *
 * This functin forms the basis used for editlist handling as defined
 * by RFC 2326 Section 10.5.
 */
void rtsp_session_editlist_append(RTSP_session *session, RTSP_Range *range)
{
    g_queue_push_tail(session->play_requests, range);
}

/**
 * @brief Free the single range in an editlist
 *
 * @param element The Range to free
 * @param user_data Unused
 *
 * @internal This should only be called through g_list_foreach by @ref
 *           rtsp_session_editlist_free.
 */
static void rtsp_range_free(gpointer element,
                            ATTR_UNUSED gpointer user_data)
{
    g_slice_free(RTSP_Range, element);
}

/**
 * @brief Free the editlist of a given session
 *
 * @param session The RTSP session to free the editlist of
 */
void rtsp_session_editlist_free(RTSP_session *session)
{
    g_queue_foreach(session->play_requests, rtsp_range_free, NULL);
    g_queue_clear(session->play_requests);
}

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

    new->session_id = g_strdup_printf("%08x%08x",
                                      g_random_int(),
                                      g_random_int());
    new->play_requests = g_queue_new();

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

    rtsp_session_editlist_free(session);
    g_queue_free(session->play_requests);

    g_free(session->resource_uri);
    r_close(session->resource);

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
 * @param url The URI structure to validate.
 *
 * @retval true The URL does not contian any forbidden sequence.
 * @retval false The URL contains forbidden sequences that might have malicious
 *         intent.
 *
 * @todo This function should be moved to netembryo.
 * @todo This function does not check properly for paths.
 */
static gboolean check_forbidden_path(URI *uri)
{
    if ( strstr(uri->path, "../") || strstr(uri->path, "./") )
        return false;

    return true;
}

/**
 * @brief Takes care of extracting and validating an URL from the a request
 *        structure.
 *
 * @param req The request structure from where to extract the URL
 *
 * @retval true The URL is valid and allowed
 * @retval false The URL is not not valid or forbidden
 *
 * This function already takes care of sending a 400 "Bad Request" response for
 * invalid URLs or a 403 "Forbidden" reply for paths that try to exit the
 * accessible sandbox.
 *
 * @retval true The URL was properly found and extracted
 * @retval false An error was found, and a reply was already sent.
 */
gboolean rfc822_request_check_url(RTSP_Client *client, RFC822_Request *req)
{
    if ( req->uri == NULL ) {
        rtsp_quick_response(client, req, RTSP_BadRequest);
        return false;
    }

    if ( !check_forbidden_path(req->uri) ) {
        rtsp_quick_response(client, req, RTSP_Forbidden);
        return false;
    }

    return true;
}

/**
 * @}
 */

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
gboolean rtsp_check_invalid_state(RTSP_Client *client,
                                  const RFC822_Request *req,
                                  RTSP_Server_State invalid_state) {
    static const char *const valid_states[] = {
        [RTSP_SERVER_INIT] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN",
        [RTSP_SERVER_READY] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY",
        [RTSP_SERVER_PLAYING] = "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE"
        /* We ignore RECORDING state since we don't support it */
    };
    RFC822_Response *response;
    const int currstate = client->session ? client->session->cur_state : RTSP_SERVER_INIT;

    if ( currstate != invalid_state )
        return true;

    response = rfc822_response_new(req, RTSP_InvalidMethodInState);

    rfc822_headers_set(response->headers,
                       RTSP_Header_Allow,
                       g_strdup(valid_states[invalid_state]));

    rfc822_response_send(client, response);

    return false;
}

/**
 * @}
 */
