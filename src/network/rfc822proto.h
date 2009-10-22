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

#ifndef RFC822_PROTO_H__
#define RFC822_PROTO_H__

struct RTSP_Client;

#include <glib.h>
#include <netembryo/url.h>
#include "rfc822proto-constants.h"

/**
 * @brif State constant for the RFC822 protocol parser
 */
typedef enum RFC822_Parser_State {
    /** Initial state */
    RFC822_State_Begin,
    /** Interleaved packet found, reading content */
    RFC822_State_Interleaved,
    /** RTSP request line found, reading headers */
    RFC822_State_RTSP_Headers,
    /** RTSP request with content, reading content */
    RFC822_State_RTSP_Content,
    /** RTSP request read, handling */
    RFC822_State_RTSP_Handler,
    /** HTTP request line found, reading headers */
    RFC822_State_HTTP_Headers,
    /** HTTP headers read, reading content */
    RFC822_State_HTTP_Content,
    /** HTTP request read, handling */
    RFC822_State_HTTP_Handler,
    /** HTTP GET request read, idling (output only) */
    RFC822_State_HTTP_Idle
} RFC822_Parser_State;

typedef struct RFC822_Request {
    /** State of the current request parsing */
    RFC822_Parser_State state;

    /** Protocol used by the request */
    RFC822_Protocol proto;

    /**
     * @brief Method used by the request
     *
     * @note This is a generic integer because we have to handle
     *        multiple $proto_Method enumerators; the actual domain of
     *        this variable depends on RFC822_Request::proto.
     */
    int method_id;

    /** Method of the request (string) */
    char *method_str;

    /** Object of the request */
    char *object;

    /** Protocol of the request (string) */
    char *protocol_str;

    /**
     * @brief Hash table of the known/supported headers in the request.
     *
     * This hash table contains all the headers read from the request
     * that feng will use. Unsupported, unknown headers will not be
     * read into this hash table!
     */
    GHashTable *headers;
} RFC822_Request;

gboolean rfc822_request_get_url(struct RTSP_Client *client, RFC822_Request *req, Url *url);
gboolean rfc822_request_check_url(struct RTSP_Client *client, RFC822_Request *req);

typedef struct RFC822_Response {
    /** Protocol used by the response */
    RFC822_Protocol proto;

    /**
     * @brief Status code of the response
     *
     * @note This is a generic integer because we have to handle
     *        multiple $proto_Method enumerators; the actual domain of
     *        this variable depends on RFC822_Response::proto.
     */
    int status;

    /** Hash table of headers to add to the response */
    GHashTable *headers;

    /** Eventual body for the response */
    GString *body;

    /** Original request triggering the response */
    const RFC822_Request *request;
} RFC822_Response;

RFC822_Response *rfc822_response_new(const RFC822_Request *request,
                                     int status_code);
void rfc822_response_send(struct RTSP_Client *client, RFC822_Response *response);

/**
 * @brief Create and send a response in a single function call
 * @ingroup rtsp_response
 *
 * @param req Request object to respond to
 * @param proto The protocol to use for the response
 * @param code Status code for the response
 *
 * This function is used to avoid creating and sending a new response when just
 * sending out an error response.
 */
static inline void rfc822_quick_response(struct RTSP_Client *client, RFC822_Request *req,
                                         RFC822_Protocol proto, int code)
{
    RFC822_Response *response = rfc822_response_new(req, code);
    response->proto = proto;
    rfc822_response_send(client, response);
}


/**
 * @brief Creates a new hash table for handling RFC822 headers.
 *
 * @return A new GHashTable object with direct hash and g_free as
 *         value cleanup function.
 */
static inline GHashTable *rfc822_headers_new()
{
    return g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                 NULL, g_free);
}

/**
 * @brief Sets an header in the hash table to a given value
 *
 * @param headers The hash table (as returned by @ref rfc822_headers_new)
 * @param hdr The constant code of the header
 * @param value A g_malloc'd value to set the header to
 *
 * @note The hdr parameter is a generic integer because the proper
 *       autogenerated enumeration constant will have to be used
 *       depending on the request.
 *
 * @note The value given will be freed with g_free() when removing it
 *       from the hash table (or with @ref rfc822_headers_destroy.
 */
static inline void rfc822_headers_set(GHashTable *headers, RFC822_Header hdr, char *value)
{
    return g_hash_table_insert(headers,
                               GINT_TO_POINTER(hdr),
                               value);
}

/**
 * @brief Gets the value of an header in the hash table
 *
 * @param headers The hash table (as returned by @ref rfc822_headers_new)
 * @param hdr The constant code of the header
 *
 * @return The raw content string straight from the hash table
 *
 * @note The hdr parameter is a generic integer because the proper
 *       autogenerated enumeration constant will have to be used
 *       depending on the request.
 */
static inline const char *rfc822_headers_lookup(GHashTable *headers, RFC822_Header hdr)
{
    if ( headers == NULL )
        return NULL;

    return g_hash_table_lookup(headers,
                               GINT_TO_POINTER(hdr));
}

/**
 * @brief Destroys headers hash tables created by @ref rfc822_headers_new
 *
 * @param headers The hash table to destroy (may be NULL)
 *
 * This is a simple wrapper around g_hash_table_destroy(), but it
 * checks for NULL values, to be compatible with free() and g_free().
 */
static inline void rfc822_headers_destroy(GHashTable *headers)
{
    if ( headers )
        g_hash_table_destroy(headers);
}

#endif /* RFC822_PROTO_H__ */
