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

typedef struct HTTP_Tunnel_Pair {
    RTSP_Client *client;
    gint base64_state;
    gint base64_save;
} HTTP_Tunnel_Pair;

static GHashTable *http_tunnel_pairs;

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
    pair->client = rtsp_client_new(client->srv);
    pair->client->sock = client->sock;
    pair->client->write_data = rtsp_write_data_base64;
    memcpy(&pair->client->ev_io_write, &client->ev_io_write, sizeof(client->ev_io_write));
    memcpy(&pair->client->ev_sig_disconnect, &client->ev_sig_disconnect, sizeof(client->ev_sig_disconnect));

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
                                          rtsp->input->data,
                                          rtsp->input->len,
                                          &parsed_headers);

    if ( headers_res == -1 ) {
        rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, RTSP_BadRequest);
        return false;
    }

    g_byte_array_remove_range(rtsp->input, 0, parsed_headers);

    if ( headers_res == 0 )
        return false;

    if ( rtsp->pending_request->method_id == HTTP_Method_POST ) {
        const char *http_session = rfc822_headers_lookup(rtsp->pending_request->headers, HTTP_Header_x_sessioncookie);

        if ( http_session == NULL ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        if ( (rtsp->pair = g_hash_table_lookup(http_tunnel_pairs, http_session)) == NULL ) {
            rfc822_quick_response(rtsp, rtsp->pending_request, RFC822_Protocol_HTTP10, HTTP_BadRequest);
            return false;
        }

        rtsp->status = RFC822_State_HTTP_Content;
    } else {
        if ( http_tunnel_create_pair(rtsp, rtsp->pending_request) )
            rtsp->status = RFC822_State_HTTP_Idle;
        // Maybe we should disconnect if this is false
        return false;
    }
    return true;
}

/**
 * @brief Write data to the RTSP socket of the client (base64-encoded)
 *
 * @param client The client to write the data to
 * @param data The GByteArray object to queue for sending
 *
 * @note after calling this function, the @p ataobject should no
 * longer be referenced by the code path.
 *
 * This is used by the RTSP-over-HTTP tunnel implementation.
 */
void rtsp_write_data_base64(RTSP_Client *client, GByteArray *data)
{
    GByteArray *encoded = g_byte_array_new();
    encoded->data = (guint8*)g_base64_encode(data->data, data->len);
    encoded->len = strlen((char*)encoded->data);

    g_byte_array_free(data, true);

    rtsp_write_data_direct(client, encoded);
}

gboolean HTTP_handle_content(RTSP_Client *rtsp)
{
    gsize decoded_length = (rtsp->input->len / 4) * 3 + 6, actual_decoded_length;
    GByteArray *decoded_input = rtsp->pair->client->input;
    guint8 *outbuf = decoded_input->data + decoded_input->len;

    g_byte_array_set_size(decoded_input, decoded_input->len + decoded_length);

    actual_decoded_length = g_base64_decode_step(rtsp->input->data,
                                                 rtsp->input->len,
                                                 outbuf,
                                                 &rtsp->pair->base64_state,
                                                 &rtsp->pair->base64_save);

    decoded_input->len -= (decoded_length - actual_decoded_length);

    RTSP_handler(rtsp->pair->client);

    return false;
}

gboolean HTTP_handle_idle(RTSP_Client *rtsp)
{
    return false;
}

void http_tunnel_initialise()
{
    http_tunnel_pairs = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, http_tunnel_destroy_pair);
}
