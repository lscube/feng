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
#include <strings.h>
#include <inttypes.h> /* For SCNu64 */

#include <liberis/headers.h>

#include "rtsp.h"
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

/**
 * RTSP high level functions, mapping to the actual RTSP methods
 * @defgroup rtsp_methods RTSP Method functions
 * @ingroup RTSP
 *
 * The declaration of these functions are in rtsp_state_machine.c
 * because the awareness of their existance outside their own
 * translation unit has to be limited to the state machine function
 * itself.
 *
 * @{
 */

typedef void (*rtsp_method_function)(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_describe(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_setup(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_play(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_pause(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_teardown(RTSP_buffer * rtsp, RTSP_Request *req);

void RTSP_options(RTSP_buffer * rtsp, RTSP_Request *req);

/**
 * @}
 */

extern size_t ragel_parse_request_line(const char *msg, const size_t length, RTSP_Request *req);

/**
 * @brief Free a request structure as parsed by rtsp_parse_request().
 *
 * @param req Request to free
 */
static void rtsp_free_request(RTSP_Request *req)
{
    if ( req == NULL )
        return;

    if ( req->headers )
        g_hash_table_destroy(req->headers);

    g_free(req->version);
    g_free(req->method);
    g_free(req->object);
    g_slice_free(RTSP_Request, req);
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
static gboolean check_required_options(RTSP_Request *req) {
    const char *require_hdr =
        g_hash_table_lookup(req->headers, eris_hdr_require);
    const char *proxy_require_hdr =
        g_hash_table_lookup(req->headers, eris_hdr_proxy_require);
    RTSP_Response *response;

    if ( !require_hdr && !proxy_require_hdr )
        return true;

    response = rtsp_response_new(req, RTSP_OptionNotSupported);
    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_unsupported),
                        g_strdup_printf("%s %s",
                                        require_hdr ? require_hdr : "",
                                        proxy_require_hdr ? proxy_require_hdr : "")
                        );

    rtsp_response_send(response);
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
static gboolean check_session(RTSP_Request *req)
{
    const char *session_hdr =
        g_hash_table_lookup(req->headers, eris_hdr_session);

    RTSP_session *session = req->client->session;

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
    rtsp_quick_response(req, RTSP_SessionNotFound);

    return false;
}

/**
 * @brief Parse a request using Ragel functions
 *
 * @param rtsp The client connection the request come
 *
 * @return A new request structure or NULL in case of error
 *
 * @note In case of error, the response is sent to the client before returning.
 */
static RTSP_Request *rtsp_parse_request(RTSP_buffer *rtsp)
{
    RTSP_Request *req = g_slice_new0(RTSP_Request);
    size_t message_length = strlen(rtsp->input->data),
        request_line_length, headers_length;

    req->client = rtsp;
    req->method_id = RTSP_ID_ERROR;

    request_line_length = ragel_parse_request_line(rtsp->input->data, message_length, req);

    /* If the parser returns zero, it means that the request line
     * couldn't be parsed. Since a request line that cannot be parsed
     * might mean that the client is speaking the wrong protocol, we
     * just ignore the whole thing. */
    if ( request_line_length == 0 )
        goto error;

    /* Check for supported RTSP version.
     *
     * It is important to check for this for the first thing, this because this
     * is the failsafe mechanism that allows for somewhat-incompatible changes
     * to be made to the protocol.
     *
     * While we could check for this after accepting the method, if a client
     * uses a method of a RTSP version we don't support, we want to make it
     * clear to the client it should not be using that version at all.
     *
     * @todo This needs to be changed to something different, since
     *       for supporting the QuickTime tunneling of RTP/RTSP over
     *       HTTP proxy we have to accept (limited) HTTP requests too.
     */
    if ( strcmp(req->version, "RTSP/1.0") != 0 ) {
        rtsp_quick_response(req, RTSP_VersionNotSupported);
        goto error;
    }

    /* Now we actually go around parsing headers. The reason why we
     * want this done here is that once we know the protocol is the
     * one we support, all the requests need to be responded with the
     * right rules, which include repeating of some headers. */
    headers_length = eris_parse_headers(rtsp->input->data + request_line_length,
                                        message_length - request_line_length,
                                        &req->headers);


    /* Error during headers parsing */
    if ( headers_length == 0 ) {
        req->headers = NULL;
        rtsp_quick_response(req, RTSP_BadRequest);
        goto error;
    }

    /* Check if the method is a know and supported one */
    if ( req->method_id == RTSP_ID_ERROR ) {
        rtsp_quick_response(req, RTSP_NotImplemented);
        goto error;
    }

    /* No CSeq found */
    if ( g_hash_table_lookup(req->headers, eris_hdr_cseq) == NULL ) {
        /** @todo This should be corrected for RFC! */
        rtsp_quick_response(req, RTSP_BadRequest);
        goto error;
    }

    if ( !check_session(req) )
        goto error;

    if ( !check_required_options(req) )
        goto error;

    return req;

 error:
    rtsp_free_request(req);
    return NULL;
}

typedef enum {
    RTSP_not_full,
    RTSP_method_rcvd,
    RTSP_interlvd_rcvd
} rtsp_rcvd_status;

/**
 * Recieves an RTSP message and puts it into the buffer
 *
 * @param rtsp Buffer to receive the data from.
 * @param hdr_len where to save the header length
 * @param body_len where to save the message body length
 *
 * @retval -1 Error happened.
 * @retval RTSP_not_full A full RTSP message is *not* present in
 *         RTSP_buffer::input yet.
 * @retval RTSP_method_rcvd A full RTSP message is present in
 *         RTSP_buffer::input and is ready to be handled.
 * @retval RTSP_interlvd_rcvd A complete RTP/RTCP interleaved packet
 *         is present.
 */
static rtsp_rcvd_status RTSP_full_msg_rcvd(RTSP_buffer * rtsp,
                                           int *hdr_len, int *body_len)
{
    int eomh;          /*! end of message header found */
    int mb;            /*! message body exists */
    int tc;            /*! terminator count */
    int ws;            /*! white space */
    unsigned int ml;   /*! total message length including any message body */
    int bl;            /*! message body length */
    char c;            /*! character */
    int control;
    char *p;

    // is there an interleaved RTP/RTCP packet?
    if (rtsp->input->data[0] == '$') {
        uint16_t *intlvd_len = (uint16_t *) &rtsp->input->data[2];

        if ((bl = ntohs(*intlvd_len)) + 4 <= rtsp->input->len) {
            if (hdr_len)
                *hdr_len = 4;
            if (body_len)
                *body_len = bl;
            return RTSP_interlvd_rcvd;
        } else {
            fnc_log(FNC_LOG_DEBUG,
                "Non-complete Interleaved RTP or RTCP packet arrived.");
            return RTSP_not_full;
        }

    }
    eomh = mb = ml = bl = 0;
    while (ml <= rtsp->input->len) {
        /* look for eol. */
        control = strcspn(rtsp->input->data + ml, RTSP_EL);
        if (control > 0) {
            ml += control;
        } else {
            return ERR_GENERIC;
        }

        if (ml > rtsp->input->len)
            return RTSP_not_full;    /* haven't received the entire message yet. */
        /*
         * work through terminaters and then check if it is the
         * end of the message header.
         */
        tc = ws = 0;
        while (!eomh && ((ml + tc + ws) < rtsp->input->len)) {
            c = rtsp->input->data[ml + tc + ws];
            if (c == '\r' || c == '\n')
                tc++;
            else if ((tc < 3) && ((c == ' ') || (c == '\t')))
                ws++;    /* white space between lf & cr is sloppy, but tolerated. */
            else
                break;
        }
        /*
         * cr,lf pair only counts as one end of line terminator.
         * Double line feeds are tolerated as end marker to the message header
         * section of the message.  This is in keeping with RFC 2068,
         * section 19.3 Tolerant Applications.
         * Otherwise, CRLF is the legal end-of-line marker for all HTTP/1.1
         * protocol compatible message elements.
         */
        if ((tc > 2) || ((tc == 2) &&
                         (rtsp->input->data[ml] == rtsp->input->data[ml + 1])))
            eomh = 1;    /* must be the end of the message header */
        ml += tc + ws;

        if (eomh) {
            ml += bl;    /* add in the message body length, if collected earlier */
            if (ml <= rtsp->input->len)
                break;    /* all done finding the end of the message. */
        }

        if (ml >= rtsp->input->len)
            return RTSP_not_full;    /* haven't received the entire message yet. */

        /*
         * check first token in each line to determine if there is
         * a message body.
         */
        if (!mb) {    /* content length token not yet encountered. */
            if (!g_ascii_strncasecmp (rtsp->input->data + ml, HDR_CONTENTLENGTH,
                 RTSP_BUFFERSIZE - ml)) {
                mb = 1;    /* there is a message body. */
                ml += strlen(HDR_CONTENTLENGTH);
                while (ml < rtsp->input->len) {
                    c = rtsp->input->data[ml];
                    if ((c == ':') || (c == ' '))
                        ml++;
                    else
                        break;
                }

                if (sscanf(rtsp->input->data + ml, "%d", &bl) != 1) {
                    fnc_log(FNC_LOG_ERR,
                        "RTSP_full_msg_rcvd(): Invalid ContentLength.");
                    return ERR_GENERIC;
                }
            }
        }
    }

    if (hdr_len)
        *hdr_len = ml - bl;

    if (body_len) {
        /*
         * go through any trailing nulls.  Some servers send null terminated strings
         * following the body part of the message.  It is probably not strictly
         * legal when the null byte is not included in the Content-Length count.
         * However, it is tolerated here.
         */
        for (tc = rtsp->input->len - ml, p = &(rtsp->input->data[ml]);
             tc && (*p == '\0'); p++, bl++, tc--);
        *body_len = bl;
    }

    return RTSP_method_rcvd;
}

/**
 * @brief Handle a request coming from the client
 *
 * @param rtsp The client the request comes from
 *
 * This function takes care of parsing and getting a request from the client,
 * and freeing it afterward.
 */
static void rtsp_handle_request(RTSP_buffer *rtsp)
{
    static const rtsp_method_function methods[] = {
        [RTSP_ID_DESCRIBE] = RTSP_describe,
        [RTSP_ID_SETUP]    = RTSP_setup,
        [RTSP_ID_TEARDOWN] = RTSP_teardown,
        [RTSP_ID_OPTIONS]  = RTSP_options,
        [RTSP_ID_PLAY]     = RTSP_play,
        [RTSP_ID_PAUSE]    = RTSP_pause
    };

    RTSP_Request *req = rtsp_parse_request(rtsp);

    if ( !req ) return;

    /* We're safe to use the array of functions since rtsp_parse_request() takes
     * care of responding with an error if the method is not implemented.
     */

    methods[req->method_id](rtsp, req);

    rtsp_free_request(req);
}

static void rtsp_handle_interleaved(RTSP_buffer *rtsp, int blen, int hlen)
{
    RTSP_interleaved *channel;

    int m = rtsp->input->data[1];
    GSList *channel_it = g_slist_find_custom(rtsp->interleaved,
                                             GINT_TO_POINTER(m),
                                             interleaved_rtcp_find_compare);

    if (!channel_it) {    // session not found
        fnc_log(FNC_LOG_DEBUG,
                "Interleaved RTP or RTCP packet arrived for unknown channel (%d)... discarding.\n",
                m);
        return;
    }

    channel = channel_it->data;

    fnc_log(FNC_LOG_DEBUG,
            "Interleaved RTCP packet arrived for channel %d (len: %d).\n",
            m, blen);
    Sock_write(channel->rtcp.local, &rtsp->input->data[hlen],
               blen, NULL, MSG_DONTWAIT | MSG_EOR);
}

/**
 * Handles incoming RTSP message, validates them and then dispatches them
 * with RTSP_state_machine
 * @param rtsp the buffer from where to read the message
 * @return ERR_NOERROR (can also mean RTSP_not_full if the message was not full)
 */
int RTSP_handler(RTSP_buffer * rtsp)
{
    while (rtsp->input->len) {
        int hlen, blen;
        rtsp_rcvd_status full_msg = RTSP_full_msg_rcvd(rtsp, &hlen, &blen);

        switch (full_msg) {
        case RTSP_method_rcvd:
            rtsp_handle_request(rtsp);
            break;
        case RTSP_interlvd_rcvd:
            rtsp_handle_interleaved(rtsp, blen, hlen);
            break;
        default:
            return full_msg;
        }

        g_byte_array_remove_range(rtsp->input, 0, hlen+blen);
    }
    return ERR_NOERROR;
}
