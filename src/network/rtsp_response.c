/* *
 *  This file is part of Feng
 *
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 *
 * Feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdbool.h>

#include <liberis/headers.h>

#include "rtsp.h"

/**
 * @file
 * @brief RTSP response generation
 */

/**
 * @defgroup rtsp_response Response messages
 * @ingroup RTSP
 *
 * This module contains the functions that are used to produce RTSP response
 * that the server sends to the client.
 *
 * RTSP responses are comprised of status line, headers and an eventual body
 * (entity), and are described by RFC2326, Section 7.
 *
 * @{
 */

/**
 * @brief Return a timestamp using HTTP Time specification
 *
 * Create a timestamp in the format of "23 Jan 1997 15:35:06 GMT".
 *
 * The timestamp format is described in RFC2616, Section 14.18, and is actually
 * used for more than just the Date: header.
 */
static char *rtsp_timestamp() {
  char buffer[31] = { 0, };

  time_t now = time(NULL);
  struct tm *t = gmtime(&now);

  strftime(buffer, 30, "%a, %d %b %Y %H:%M:%S GMT", t);

  return g_strdup(buffer);
}

/**
 * @brief Create a new RTSP_Response structure
 *
 * @param req The request to respond to
 * @param code The status code of the response
 *
 * @return A new RTSP_Response object
 *
 * This method creates a new object for the response and adds some of the basic
 * headers common to all responses:
 *
 * @li Server (Sec. 12.36)
 * @li Date (Sec. 12.18)
 * @li CSeq (Sec. 12.17)
 * @li Session (if applicable) (Sec. 12.37)
 * @li Timestamp (if present) (Sec. 12.38)
 *
 * The headers CSeq and Timestamp that are just copied over from the request are
 * taken through its headers hash table. Session is copied over if present, but
 * it might be added by the SETUP method function too.
 */
RTSP_Response *rtsp_response_new(const RTSP_Request *req, RTSP_ResponseCode code)
{
    static const char server_header[] = PACKAGE "/" VERSION;
    RTSP_Response *response = g_slice_new0(RTSP_Response);
    char *hdr;

    response->client = req->client;
    response->request = req;
    response->status = code;
    response->headers = g_hash_table_new_full(g_str_hash, g_str_equal,
                                              g_free, g_free);
    response->body = NULL;

    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_server), g_strdup(server_header));
    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_date), rtsp_timestamp());

    if ( (hdr = g_hash_table_lookup(req->headers, eris_hdr_cseq)) )
        g_hash_table_insert(response->headers,
                            g_strdup(eris_hdr_cseq), g_strdup(hdr));

    if ( (hdr = g_hash_table_lookup(req->headers, eris_hdr_session)) )
        g_hash_table_insert(response->headers,
                            g_strdup(eris_hdr_session), g_strdup(hdr));

    if ( (hdr = g_hash_table_lookup(req->headers, eris_hdr_timestamp)) )
        g_hash_table_insert(response->headers,
                            g_strdup(eris_hdr_timestamp), g_strdup(hdr));

    return response;
}

/**
 * @brief Frees an RTSP_Response object deallocating all its content
 *
 * @param response The response to free
 */
void rtsp_response_free(RTSP_Response *response)
{
    g_hash_table_destroy(response->headers);
    if ( response->body )
        g_string_free(response->body, true);
    g_slice_free(RTSP_Response, response);
}

/**
 * @brief Iterating function used by rtsp_response_send
 *
 * @param hdr_name_p Pointer to the header name string
 * @param hdr_value_p Pointer to the header value string
 * @param response_str_p Pointer to the response GString
 *
 * This function is used by g_hash_table_foreach to append the various headers
 * to the final response string.
 */
static void rtsp_response_append_headers(gpointer hdr_name_p,
                                         gpointer hdr_value_p,
                                         gpointer response_str_p)
{
    const char *const hdr_name = hdr_name_p;
    const char *const hdr_value = hdr_value_p;
    GString *response_str = response_str_p;

    g_string_append_printf(response_str, "%s: %s" RTSP_EL,
                           hdr_name, hdr_value);
}

/**
 * @brief Log an RTSP access to the proper log
 *
 * @param response Response to log to the access log
 *
 * Like Apache and most of the web servers, we log the access through CLF
 * (Common Log Format).
 *
 * @todo Create a true access.log file
 *
 * Right now the access is just logged to stderr for debug purposes.  It
 * should also be possible to let the user configure the log format.
 *
 * @todo This should use an Apache-compatible user setting to decide the output
 *       format, and parse that line.
 */
static void rtsp_log_access(RTSP_Response *response)
{
    const char *referer =
        g_hash_table_lookup(response->request->headers, eris_hdr_referer);
    const char *useragent =
        g_hash_table_lookup(response->request->headers, eris_hdr_user_agent);
    char *response_length = response->body ?
        g_strdup_printf("%zd", response->body->len) : NULL;

    fprintf(stderr, "%s - - [%s], \"%s %s %s\" %d %s %s %s\n",
            response->client->sock->remote_host,
            (const char*)g_hash_table_lookup(response->headers, eris_hdr_date),
            response->request->method, response->request->object,
            response->request->version,
            response->status, response_length ? response_length : "-",
            referer ? referer : "-",
            useragent ? useragent : "-");

    g_free(response_length);
}

/**
 * @brief Finalise, send and free an response object
 *
 * @param response The response to send
 *
 * This method generates an RTSP response message, following the indication of
 * RFC 2326 Section 7.
 */
void rtsp_response_send(RTSP_Response *response)
{
    GString *str = g_string_new("");

    /* Generate the status line, see RFC 2326 Sec. 7.1 */
    g_string_printf(str, "%s %d %s" RTSP_EL,
                    "RTSP/1.0", response->status,
                    rtsp_reason_phrase(response->status));

    /* Append the headers */
    g_hash_table_foreach(response->headers,
                         rtsp_response_append_headers,
                         str);

    /* If there is a body we need to calculate its length and append that to the
     * headers, see RFC 2326 Sec. 12.14. */
    if ( response->body )
        g_string_append_printf(str, "%s: %zu" RTSP_EL,
                               eris_hdr_content_length,
                               response->body->len + 2);

    g_string_append(str, RTSP_EL);

    if ( response->body ) {
        g_string_append(str, response->body->str);
        /* Make sure we add a final RTSP_EL here since the body string might
         * not have it at all. */
        g_string_append(str, RTSP_EL);
    }

    /* Now the whole response is complete, we can queue it to be sent away. */
    rtsp_bwrite(response->client, str);

    /* Log the access */
    rtsp_log_access(response);

    /* After we did output to access.log, we can free the response since it's no
     * longer necessary. */
    rtsp_response_free(response);
}

/**
 * @}
 */
