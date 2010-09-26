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
 * @brief Contains RTSP message dispatchment functions
 */

#include <stdbool.h>
#include <inttypes.h>

#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "feng.h"
#include "uri.h"

/**
 * @brief Free a request structure as parsed by rtsp_parse_request().
 *
 * @param req Request to free
 */
static void rfc822_free_request(RFC822_Request *req)
{
    if ( req == NULL )
        return;

    rfc822_headers_destroy(req->headers);
    uri_free(req->uri);
    g_free(req->method_str);
    g_free(req->object);
    g_free(req->protocol_str);
    g_slice_free(RFC822_Request, req);
}

/**
 * @brief Checks if the client required any option
 *
 * @param req The request to validate
 *
 * @retval true No requirement
 * @retval false Client required options we don't support
 *
 * Right now feng does not support any option at all, so if we see the
 * Require (RFC2326 Sec. 12.32) or Proxy-Require (Sec. 12.27) we respond to
 * the proper 551 code (Option not supported; Sec. 11.3.14).
 *
 * A 551 response contain an Unsupported header that lists the unsupported
 * options (which in our case are _all_ of them).
 */
static gboolean rtsp_check_required_options(RTSP_Client *client, RFC822_Request *req) {
    const char *require_hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Require);
    const char *proxy_require_hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Proxy_Require);
    RFC822_Response *response;

    if ( !require_hdr && !proxy_require_hdr )
        return true;

    response = rfc822_response_new(req, RTSP_OptionNotSupported);
    rfc822_headers_set(response->headers, RTSP_Header_Unsupported,
                       g_strdup_printf("%s %s",
                                       require_hdr ? require_hdr : "",
                                       proxy_require_hdr ? proxy_require_hdr : "")
                       );

    rfc822_response_send(client, response);
    return false;
}

/**
 * @brief Check the session reported by the client if any.
 *
 * @param req The request to validate
 *
 * @retval true No error (session found, or none given and none expected)
 *
 * This function checks that the server's expectations of a session are
 * satisfied. In particular, it ensures that if the client sends a Session
 * header, we are expecting that same session. If we're not expecting any
 * session or if the session differs from the expected one, we respond with a
 * 454 "Session Not Found" status.
 */
static gboolean rtsp_check_session(RTSP_Client *client, RFC822_Request *req)
{
    const char *session_hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Session);

    RTSP_session *session = client->session;

    if (/* We always accept requests without a Session header, since even when a
         * session _is_ present, the client might make a request that is not
         * tied to one.*
         */
        !session_hdr ||
        /* Otherwise, check if the session is present and corresponds. */
        (session && strcmp(session_hdr, session->session_id) == 0)
        )
        return true;

    /* At this point we either got a session when we don't expect one, or one
     * with a different id from expected, respond with a 454 "Session Not
     * Found".
     */
    rtsp_quick_response(client, req, RTSP_SessionNotFound);

    return false;
}

/**
 * @brief Handle an RTSP request coming from the client
 *
 * @param req The request to handle
 *
 * This function takes care of calling the proper method for the
 * request, and freeing it afterwards.
 */
static void rtsp_handle_request(RTSP_Client *client, RFC822_Request *req)
{
    static const rtsp_method_function methods[] = {
        [RTSP_Method_DESCRIBE] = RTSP_describe,
        [RTSP_Method_SETUP]    = RTSP_setup,
        [RTSP_Method_TEARDOWN] = RTSP_teardown,
        [RTSP_Method_OPTIONS]  = RTSP_options,
        [RTSP_Method_PLAY]     = RTSP_play,
        [RTSP_Method_PAUSE]    = RTSP_pause
    };

    /* No CSeq found */
    if ( rfc822_headers_lookup(req->headers, RTSP_Header_CSeq) == NULL ) {
        /** @todo This should be corrected for RFC! */
        rtsp_quick_response(client, req, RTSP_BadRequest);
        goto error;
    }

    if ( !rtsp_check_session(client, req) )
        goto error;

    if ( !rtsp_check_required_options(client, req) )
        goto error;

    /* We're safe to use the array of functions since rtsp_parse_request() takes
     * care of responding with an error if the method is not implemented.
     */
    methods[req->method_id](client, req);

 error:
    rfc822_free_request(req);
    client->pending_request = NULL;
}

static gboolean RTSP_handle_interleaved(RTSP_Client *rtsp) {
    uint16_t length;

    if ( rtsp->input->len < 4 )
        return false;

    length = (uint16_t)(rtsp->input->data[2]) << 8 | rtsp->input->data[3];

    g_assert_cmpint(rtsp->input->data[0], ==, '$');

    /* If we don't have enough data to complete the interleaved packet
     * for now, we ignore it and remain in interleaved status */
    if ( rtsp->input->len < (length + 4) )
        return false;

    rtsp_interleaved_receive(rtsp, rtsp->input->data[1],
                             rtsp->input->data+4, length);

    g_byte_array_remove_range(rtsp->input, 0, length+4);
    rtsp->status = RFC822_State_Begin;

    return true;
}

/**
 * @brief enforce connection limit as set by configuration
 *
 * @param req The request to handle
 *
 * This function enforces connection limit by redirecting to a twin
 * server with a 302 (Found) status or, if no server had been designed
 * in the configuration, replying with a 453 status (Not Enough
 * Bandwidth)
 */

gboolean rtsp_connection_limit(RTSP_Client *rtsp, RFC822_Request *req)
{
    if (rtsp->socket->connection_count > rtsp->socket->max_conns) {
        const char *twin = rtsp->vhost->twin;
        fnc_log(FNC_LOG_INFO, "Max connection reached");
        if (twin) {
            char *redir;
            RFC822_Response *response = rfc822_response_new(req, RTSP_Found);

            switch(req->proto) {
                case RFC822_Protocol_HTTP10:
                case RFC822_Protocol_HTTP11:
                    redir = g_strdup_printf("http://%s/%s", twin, req->uri->path);
                break;
                default:
                    redir = g_strdup_printf("rtsp://%s/%s", twin, req->uri->path);
                break;
            }

            fnc_log(FNC_LOG_INFO, "Redirecting to %s", redir);

            response->proto = req->proto;
            rfc822_headers_set(response->headers,
                               RFC822_Header_Location,
                               strdup(redir));
            rfc822_response_send(rtsp, response);
            g_free(redir);
        } else {
            rtsp_quick_response(rtsp, req, RTSP_NotEnoughBandwidth);
        }
        return false;
    }
    return true;
}

static gboolean RTSP_handle_new(RTSP_Client *rtsp) {
    if ( rtsp->input->len < 1 )
        return false;


    if ( rtsp->input->data[0] == '$' ) {
        rtsp->status = RFC822_State_Interleaved;
        return RTSP_handle_interleaved(rtsp);
    } else {
        size_t request_line_len = 0;
        RFC822_Request tmpreq = {
            .method_id = RTSP_Method__Invalid,
            .proto = RFC822_Protocol_Invalid
        };

        request_line_len = ragel_parse_request_line((char*)rtsp->input->data,
                                                    rtsp->input->len,
                                                    &tmpreq);

        switch(request_line_len) {
        case (size_t)(-1):
            rtsp_quick_response(rtsp, &tmpreq, RTSP_BadRequest);
            return false;
        case 0:
            return false;
        default:
            switch(tmpreq.proto) {
            default:
                rfc822_quick_response(rtsp, &tmpreq, RFC822_Protocol_RTSP10, RTSP_VersionNotSupported);
                return false;

            case RFC822_Protocol_HTTP_UnsupportedVersion:
                rfc822_quick_response(rtsp, &tmpreq, RFC822_Protocol_HTTP10, HTTP_VersionNotSupported);
                return false;

            case RFC822_Protocol_RTSP10:
                fnc_log(FNC_LOG_INFO, "Incoming RTSP connection accepted");
                rtsp->status = RFC822_State_RTSP_Headers;
                break;

            case RFC822_Protocol_HTTP10:
            case RFC822_Protocol_HTTP11:
                fnc_log(FNC_LOG_INFO, "Incoming HTTP connection accepted");
                rtsp->status = RFC822_State_HTTP_Headers;
                break;
            }

            /* Check if the method is a know and supported one.  Don't
             * get fooled by the RTSP_ prefix; all the Invalid and
             * Unsupported constants have the same value. And the same
             * goes for the NotImplemented response code. */
            if ( tmpreq.method_id == RTSP_Method__Invalid ||
                 tmpreq.method_id == RTSP_Method__Unsupported ) {
                rfc822_quick_response(rtsp, &tmpreq, tmpreq.proto, RTSP_NotImplemented);
                return false;
            }

            rtsp->pending_request = g_slice_dup(RFC822_Request, &tmpreq);
            g_byte_array_remove_range(rtsp->input, 0, request_line_len);
            return true;
        }
    }
}

static gboolean RTSP_handle_headers(RTSP_Client *rtsp) {
    size_t parsed_headers;
    int headers_res;

    if ( rtsp->pending_request->headers == NULL )
        rtsp->pending_request->headers = rfc822_headers_new();

    headers_res = ragel_read_rtsp_headers(rtsp->pending_request->headers,
                                          (char*)rtsp->input->data,
                                          rtsp->input->len,
                                          &parsed_headers);

    if ( headers_res == -1 ) {
        rtsp_quick_response(rtsp, rtsp->pending_request, RTSP_BadRequest);
        return false;
    }

    g_byte_array_remove_range(rtsp->input, 0, parsed_headers);

    if ( headers_res == 0 )
        return false;

    /* try parsing the URI already, since we'll most likely need it,
       and the new parser is fast enough */
    if ( rtsp->pending_request->object )
        rtsp->pending_request->uri = uri_parse(rtsp->pending_request->object);

    rtsp->status = RFC822_State_RTSP_Content;
    return rtsp_connection_limit(rtsp, rtsp->pending_request);
}

static gboolean RTSP_handle_content(RTSP_Client *rtsp) {
    const char *content_length_str =
        rfc822_headers_lookup(rtsp->pending_request->headers,
                              RFC822_Header_Content_Length);

    if ( content_length_str != NULL ) {
        /* Duh! TODO obviously! */
        g_assert_not_reached();
    }

    rtsp_handle_request(rtsp, rtsp->pending_request);

    rtsp->status = RFC822_State_Begin;
    return true;
}

/**
 * @brief Process an incremental RTSP request
 *
 * @param rtsp The RTSP client object the request is coming from
 *
 * @note This function is written in two forms, one for non-PIC (fixed
 *       address) builds, and one for PIC/PIE builds. The reason is
 *       that the function table for the states on PIE systems will
 *       require relocations and would be slower than the jump table
 *       created by the switch.
 * @note Make sure to always update both sides of the function!
 */
void RTSP_handler(RTSP_Client * rtsp)
{
#if __PIC__ || PIC
    gboolean ret;

    do {
        switch(rtsp->status) {
        case RFC822_State_Begin:
            ret = RTSP_handle_new(rtsp);
            break;
        case RFC822_State_RTSP_Headers:
            ret = RTSP_handle_headers(rtsp);
            break;
        case RFC822_State_RTSP_Content:
            ret = RTSP_handle_content(rtsp);
            break;
        case RFC822_State_Interleaved:
            ret = RTSP_handle_interleaved(rtsp);
            break;
        case RFC822_State_HTTP_Headers:
            ret = HTTP_handle_headers(rtsp);
            break;
        case RFC822_State_HTTP_Content:
            ret = HTTP_handle_content(rtsp);
            break;
        case RFC822_State_HTTP_Idle:
            ret = HTTP_handle_idle(rtsp);
            break;
        }
    } while(ret);
#else
    typedef gboolean (*state_machine_handler)(RTSP_Client *);

    static const state_machine_handler handlers[] = {
        [RFC822_State_Begin] = RTSP_handle_new,
        [RFC822_State_RTSP_Headers] = RTSP_handle_headers,
        [RFC822_State_RTSP_Content] = RTSP_handle_content,
        [RFC822_State_Interleaved] = RTSP_handle_interleaved,
        [RFC822_State_HTTP_Headers] = HTTP_handle_headers,
        [RFC822_State_HTTP_Content] = HTTP_handle_content,
        [RFC822_State_HTTP_Idle] = HTTP_handle_idle
    };

    while ( handlers[rtsp->status](rtsp) );
#endif /* PIC */
}

/**
 * @brief Process a complete RTSP request
 *
 * In SCTP (and UDP) each message carries a complete request, and
 * there is no need to keep an actual state machine. But since we
 * share the same code for both the incremental and complete
 * processing, we have to pay some bit of attention that the states
 * don't roam outside their area.
 */
gboolean rtsp_process_complete(RTSP_Client *rtsp)
{
    /* We start from no request or we're in a bad state already */
    return ( rtsp->status == RFC822_State_Begin &&
             RTSP_handle_new(rtsp) &&
             rtsp->status != RFC822_State_RTSP_Headers &&
             RTSP_handle_headers(rtsp) &&
             rtsp->status != RFC822_State_RTSP_Content &&
             RTSP_handle_content(rtsp) &&
             rtsp->status != RFC822_State_Begin
             );
}
