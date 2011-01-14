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

#include <stdbool.h>

#include "feng.h"
#include "network/rtsp.h"

static GHashTable *http_tunnel_pairs;

#ifdef CLEANUP_DESTRUCTOR
static void CLEANUP_DESTRUCTOR http_tunnel_cleanup()
{
    if ( http_tunnel_pairs )
        g_hash_table_destroy(http_tunnel_pairs);
}
#endif

/**
 * @brief Write data to the hidden HTTP socket of the client
 *
 * @param client The client to write the data to
 * @param data The GByteArray object to queue for sending
 *
 * @note after calling this function, the @p ataobject should no
 * longer be referenced by the code path.
 *
 * This is used by the RTSP-over-HTTP tunnel implementation.
 */
static void rtsp_write_data_http(RTSP_Client *client, GByteArray *data)
{
    g_queue_push_head(client->pair->http_client->out_queue, data);
    ev_io_start(client->loop, &client->pair->http_client->ev_io_write);
}

static gboolean http_tunnel_create_pair(RTSP_Client *client, RFC822_Request *req)
{
    const char *http_session = rfc822_headers_lookup(req->headers, HTTP_Header_x_sessioncookie);
    HTTP_Tunnel_Pair *pair = NULL;
    RFC822_Response *response = NULL;

    if ( http_session == NULL ) {
        rfc822_quick_response(client, req, RFC822_Protocol_HTTP10, HTTP_BadRequest);
        return false;
    }

    if ( (pair = g_hash_table_lookup(http_tunnel_pairs, http_session)) != NULL ) {
        rfc822_quick_response(client, req, RFC822_Protocol_HTTP10, HTTP_BadRequest);
        return false;
    }

    pair = g_slice_new0(HTTP_Tunnel_Pair);
    pair->http_client = client;
    client->pair = pair;

    g_hash_table_insert(http_tunnel_pairs, strdup(http_session), pair);

    response = rfc822_response_new(req, HTTP_Ok);

    rfc822_headers_set(response->headers,
                       HTTP_Header_Connection,
                       strdup("close"));

    rfc822_headers_set(response->headers,
                       HTTP_Header_Date,
                       strdup("Tue, 8 Jun 2004 15:04:35 GMT"));

    rfc822_headers_set(response->headers,
                       HTTP_Header_Cache_Control,
                       strdup("no-store"));

    rfc822_headers_set(response->headers,
                       HTTP_Header_Pragma,
                       strdup("no-cache"));

    rfc822_headers_set(response->headers,
                       HTTP_Header_Content_Type,
                       strdup("application/x-rtsp-tunnelled"));

    rfc822_response_send(client, response);

    return true;
}

static void http_tunnel_destroy_pair(gpointer ptr)
{
    g_slice_free(HTTP_Tunnel_Pair, ptr);
}

gboolean HTTP_handle_headers(RTSP_Client *rtsp)
{
    size_t parsed_headers;
    int headers_res;

    if ( rtsp->pending_request->headers == NULL )
        rtsp->pending_request->headers = rfc822_headers_new();

    headers_res = ragel_read_http_headers(rtsp->pending_request->headers,
                                          (char*)rtsp->input->data,
                                          rtsp->input->len,
                                          &parsed_headers);

    if ( headers_res == -1 ) {
        rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, RTSP_BadRequest);
        return false;
    }

    g_byte_array_remove_range(rtsp->input, 0, parsed_headers);

    if ( headers_res == 0 )
        return false;

    if (!rtsp_connection_limit(rtsp, rtsp->pending_request))
        return false;

#ifdef HAVE_JSON
    if ( rtsp->pending_request->method_id == HTTP_Method_GET &&
         strstr(rtsp->pending_request->object, "stats") ) {

        feng_send_statistics(rtsp);
        return false;
    }
#endif
    if ( rtsp->pending_request->method_id == HTTP_Method_POST ) {
        const char *http_session = rfc822_headers_lookup(rtsp->pending_request->headers, HTTP_Header_x_sessioncookie);
        gpointer tmpptr;

        if ( http_session == NULL ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        if ( (rtsp->pair = g_hash_table_lookup(http_tunnel_pairs, http_session)) == NULL ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        /* let's be sure that the other connection has reached the
           idle state, otherwise the client has been too eager to
           connect. */
        if ( rtsp->pair->http_client->status != RFC822_State_HTTP_Idle ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        /* we should also ensure that there is not data waiting to be
           parsed, as we expect the client to send nothing on that
           connection from then on */
        if ( rtsp->pair->http_client->input->len != 0 ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        /* re-use the current object to be used for the HTTP tunnel;
           we change the callback and set the tunnel, and switch the
           input buffers around for convenience */
        rtsp->pair->rtsp_client = rtsp;
        rtsp->write_data = rtsp_write_data_http;

        /* this will start from scratch */
        rtsp->status = RFC822_State_Begin;

        tmpptr = rtsp->input;
        rtsp->input = rtsp->pair->http_client->input;
        rtsp->pair->http_client->input = tmpptr;

        /* get the http_client ready to read the data */
        rtsp->pair->http_client->status = RFC822_State_HTTP_Content;

        /* we run it here so that it starts getting some data at least */
        RTSP_handler(rtsp->pair->http_client);

        return false;
    } else {
        if ( http_tunnel_create_pair(rtsp, rtsp->pending_request) )
            rtsp->status = RFC822_State_HTTP_Idle;
        // Maybe we should disconnect if this is false
        return true;
    }
    return true;
}

gboolean HTTP_handle_content(RTSP_Client *rtsp)
{
    gsize decoded_length = (rtsp->input->len / 4) * 3 + 6, actual_decoded_length;
    GByteArray *decoded_input = rtsp->pair->rtsp_client->input;
    gsize prev_size = decoded_input->len;
    guint8 *outbuf;

    g_byte_array_set_size(decoded_input, decoded_input->len + decoded_length);

    /* This way if it's empty it'll be allocated */
    outbuf = decoded_input->data + prev_size;

    actual_decoded_length = g_base64_decode_step((gchar*)rtsp->input->data,
                                                 rtsp->input->len,
                                                 outbuf,
                                                 &rtsp->pair->base64_state,
                                                 &rtsp->pair->base64_save);

    decoded_input->len -= (decoded_length - actual_decoded_length);
    g_byte_array_set_size(rtsp->input, 0);

    RTSP_handler(rtsp->pair->rtsp_client);

    return false;
}

gboolean HTTP_handle_idle(ATTR_UNUSED RTSP_Client *rtsp)
{
    /* make sure to send the one queued answer we have */
    while ( g_queue_get_length(rtsp->out_queue) > 0 )
        ev_run(rtsp->loop, EVRUN_ONCE);

    ev_unloop(rtsp->loop, EVUNLOOP_ONE);
    return false;
}

void http_tunnel_initialise()
{
    http_tunnel_pairs = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, http_tunnel_destroy_pair);
}
