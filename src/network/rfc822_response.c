/* *
 *  This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@streaming.polito.it>
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

#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include "rfc822proto.h"
#include "rtsp.h"
#include "feng.h"

#define ENDLINE "\r\n"

/**
 * @file
 * @brief Response generation and handling for RFC822-based protocols
 */

/**
 * @defgroup rfc822_response Response messages
 * @ingroup RFC822
 *
 * This module contains the functions that are used to produce
 * responses, sent by the server to the client, for RFC822-baesd
 * protocols such as RTSP and HTTP.
 *
 * RFC822 responese are comprised of status line, headers and an eventual body
 * (entity).
 *
 * RTSP protocols describe responses in RFC 2326, Section 7.
 *
 * @{
 */

/**
 * @brief Return a timestamp using HTTP Time specification
 *
 * Create a timestamp in the format of "23 Jan 1997 15:35:06 GMT".
 *
 * The timestamp format is described in RFC 2616, Section 14.18, and
 * is actually used for more than just the Date header.
 *
 * RTSP uses this value in the Date header (RFC 2326; Section 12.18).
 */
static char *http_timestamp() {
  char buffer[31] = { 0, };

  time_t now = time(NULL);
  struct tm *t = gmtime(&now);

  strftime(buffer, 30, "%a, %d %b %Y %H:%M:%S GMT", t);

  return g_strdup(buffer);
}

/**
 * @brief Create a new RFC822_Response structure
 *
 * @param req The request to respond to
 * @param status_code The status code of the response
 *
 * @return A new RFC822_Response object
 *
 * This method creates a new object for the response and adds some of the basic
 * headers common to all responses:
 *
 * @li Server (RFC 2326; Sec. 12.36)
 * @li Date (RFC 2326; Sec. 12.18)
 * @li CSeq (RFC 2326; Sec. 12.17)
 * @li Session (if applicable) (Sec. 12.37)
 * @li Timestamp (if present) (Sec. 12.38)
 *
 * The headers CSeq and Timestamp that are just copied over from the request are
 * taken through its headers hash table. Session is copied over if present, but
 * it might be added by the SETUP method function too.
 */
RFC822_Response *rfc822_response_new(const RFC822_Request *req, int status_code)
{
    RFC822_Response *response = g_slice_new0(RFC822_Response);
    const char *hdr;

    response->proto = req->proto;
    response->request = req;
    response->status = status_code;
    response->headers = rfc822_headers_new();
    response->body = NULL;

    rfc822_headers_set(response->headers,
                       RFC822_Header_Server, g_strdup(feng_signature));
    rfc822_headers_set(response->headers,
                       RFC822_Header_Date, http_timestamp());

    if ( (hdr = rfc822_headers_lookup(req->headers, RTSP_Header_CSeq)) )
        rfc822_headers_set(response->headers,
                           RTSP_Header_CSeq, g_strdup(hdr));

    if ( (hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Session)) )
        rfc822_headers_set(response->headers,
                           RTSP_Header_Session, g_strdup(hdr));

    if ( (hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Timestamp)) )
        rfc822_headers_set(response->headers,
                           RTSP_Header_Timestamp, g_strdup(hdr));

    return response;
}

/**
 * @brief Frees an RFC822_Response object deallocating all its content
 *
 * @param response The response to free
 */
static void rfc822_response_free(RFC822_Response *response)
{
    rfc822_headers_destroy(response->headers);
    if ( response->body )
        g_string_free(response->body, true);
    g_slice_free(RFC822_Response, response);
}

/**
 * @brief Iterating function used by rfc822_response_send
 *
 * @param hdr_name_p Pointer to the header name string
 * @param hdr_value_p Pointer to the header value string
 * @param response_str_p Pointer to the response GString
 *
 * This function is used by g_hash_table_foreach to append the various headers
 * to the final response string.
 */
static void rfc822_response_append_headers(gpointer hdr_code_p,
                                           gpointer hdr_value_p,
                                           gpointer response_str_p)
{
    int hdr_code = GPOINTER_TO_INT(hdr_code_p);
    const char *const hdr_value = hdr_value_p;
    GString *response_str = response_str_p;

    g_string_append_printf(response_str, "%s: %s" ENDLINE,
                           rfc822_header_to_string(hdr_code), hdr_value);
}

/**
 * @brief Finalise, send and free an response object
 *
 * @param response The response to send
 *
 * This method generates an RTSP response message, following the indication of
 * RFC 2326 Section 7.
 */
void rfc822_response_send(RTSP_Client *client, RFC822_Response *response)
{
    GString *str = g_string_new("");

    /* Generate the status line, see RFC 2326 Sec. 7.1 */
    g_string_printf(str, "%s %d %s" ENDLINE,
                    rfc822_proto_to_string(response->proto),
                    response->status,
                    rfc822_response_reason(response->proto, response->status));

    /* Append the headers */
    g_hash_table_foreach(response->headers,
                         rfc822_response_append_headers,
                         str);

    /* If there is a body we need to calculate its length and append that to the
     * headers, see RFC 2326 Sec. 12.14. */
    if ( response->body )
        g_string_append_printf(str, "%s: %zu" ENDLINE,
                               rfc822_header_to_string(RFC822_Header_Content_Length),
                               response->body->len + 2);

    g_string_append(str, ENDLINE);

    if ( response->body ) {
        g_string_append(str, response->body->str);
        /* Make sure we add a final ENDLINE here since the body string might
         * not have it at all. */
        g_string_append(str, ENDLINE);
    }

    /* Now the whole response is complete, we can queue it to be sent away. */
    rtsp_write_string(client, str);

    /* Log the access */
    accesslog_log(client, response);

    /* After we did output to access.log, we can free the response since it's no
     * longer necessary. */
    rfc822_response_free(response);
}

/**
 * @}
 */
