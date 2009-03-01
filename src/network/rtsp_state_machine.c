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

#include "rtsp.h"
#include <fenice/utils.h>
#include <fenice/fnc_log.h>


/** 
 * RTSP high level functions, they maps to the rtsp methods
 * @defgroup rtsp_high RTSP high level functions
 * @ingroup RTSP
 *
 * The declaration of these functions are in rtsp_state_machine.c
 * because the awareness of their existance outside their own
 * translation unit has to be limited to the state machine function
 * itself.
 *
 * @{
 */

int RTSP_describe(RTSP_buffer * rtsp);

int RTSP_setup(RTSP_buffer * rtsp);

int RTSP_play(RTSP_buffer * rtsp);

int RTSP_pause(RTSP_buffer * rtsp);

int RTSP_teardown(RTSP_buffer * rtsp);

int RTSP_options(RTSP_buffer * rtsp);

int RTSP_set_parameter(RTSP_buffer * rtsp);

int RTSP_handler(RTSP_buffer * rtsp);

/**
 * @}
 */

/**
 * @defgroup rtsp_method_strings RTSP Method strings
 * @ingroup RTSP
 * @{
 */
#define RTSP_METHOD_MAXLEN 15
#define RTSP_METHOD_DESCRIBE        "DESCRIBE"
#define RTSP_METHOD_ANNOUNCE        "ANNOUNCE"
#define RTSP_METHOD_GET_PARAMETERS  "GET_PARAMETERS"
#define RTSP_METHOD_OPTIONS         "OPTIONS"
#define RTSP_METHOD_PAUSE           "PAUSE"
#define RTSP_METHOD_PLAY            "PLAY"
#define RTSP_METHOD_RECORD          "RECORD"
#define RTSP_METHOD_REDIRECT        "REDIRECT"
#define RTSP_METHOD_SETUP           "SETUP"
#define RTSP_METHOD_SET_PARAMETER   "SET_PARAMETER"
#define RTSP_METHOD_TEARDOWN        "TEARDOWN"
/**
 * @}
 */

#include "ragel_request.c"

/**
 * @brief Free a request structure as parsed by rtsp_parse_request().
 *
 * @param req Request to free
 */
static void rtsp_free_request(RTSP_Request *req)
{
    if ( req == NULL )
        return;
    
    g_hash_table_destroy(req->headers);
    g_free(req->method);
    g_free(req->object);
    g_free(req);
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
    RTSP_Request *req = g_new0(RTSP_Request, 1);
    RTSP_ResponseCode status;
    char *headers;
    int pcnt;

    req->method_id = RTSP_ID_ERROR;
    req->cseq = -1;
    req->headers = g_hash_table_new_full(g_str_hash, g_str_equal,
                                         g_free, g_free);

    status = ragel_parse_request(req, rtsp->in_buffer);

    if ( status != RTSP_Ok ) {
        rtsp_send_reply(rtsp, status);
        goto error;
    }

    /* No CSeq found! */
    if ( req->cseq == -1 ) {
        /** @todo This should be corrected for RFC! */
        rtsp_send_reply(rtsp, RTSP_BadRequest);
        goto error;
    }

    /* Check if the session header match with the expected one */
    if ( req->session_id != 0 && /* We might not have a session id at all */
         rtsp->session->session_id != req->session_id ) {
        rtsp_send_reply(rtsp, RTSP_SessionNotFound);
        goto error;
    }

    fnc_log(FNC_LOG_CLIENT, "%s %s RTSP/1.0\nUser-Agent: %s",
            req->method, req->object,
            g_hash_table_lookup(req->headers, "User-Agent"));

    return req;

 error:
    rtsp_free_request(req);
    return NULL;
}

/**
 * Removes the last message from the rtsp buffer
 * @param rtsp the buffer from which to discard the message
 * @param len Length of data to remove.
 */
static void RTSP_discard_msg(RTSP_buffer * rtsp, int len)
{
    if (len > 0 && rtsp->in_size >= len) {    /* discard the message from the in_buffer. */
        memmove(rtsp->in_buffer, rtsp->in_buffer + len,
            rtsp->in_size - len);
        rtsp->in_size -= len;
        memset(rtsp->in_buffer + rtsp->in_size, 0, len);
    }
}

/**
 * Recieves an RTSP message and puts it into the buffer
 *
 * @param rtsp Buffer to receive the data from.
 * @param hdr_len where to save the header length
 * @param body_len where to save the message body length
 *
 * @retval -1 Error happened.
 * @retval RTSP_not_full A full RTSP message is *not* present in
 *         rtsp->in_buffer yet.
 * @retval RTSP_method_rcvd A full RTSP message is present in
 *         rtsp->in_buffer and is ready to be handled.
 * @retval RTSP_interlvd_rcvd A complete RTP/RTCP interleaved packet
 *         is present.
 */
static int RTSP_full_msg_rcvd(RTSP_buffer * rtsp, int *hdr_len, int *body_len)
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
    if (rtsp->in_buffer[0] == '$') {
        uint16_t *intlvd_len = (uint16_t *) & rtsp->in_buffer[2];

        if ((bl = ntohs(*intlvd_len)) + 4 <= rtsp->in_size) {
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
    while (ml <= rtsp->in_size) {
        /* look for eol. */
        control = strcspn(rtsp->in_buffer + ml, RTSP_EL);
        if (control > 0) {
            ml += control;
        } else {
            return ERR_GENERIC;
        }

        if (ml > rtsp->in_size)
            return RTSP_not_full;    /* haven't received the entire message yet. */
        /*
         * work through terminaters and then check if it is the
         * end of the message header.
         */
        tc = ws = 0;
        while (!eomh && ((ml + tc + ws) < rtsp->in_size)) {
            c = rtsp->in_buffer[ml + tc + ws];
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
                         (rtsp->in_buffer[ml] == rtsp->in_buffer[ml + 1])))
            eomh = 1;    /* must be the end of the message header */
        ml += tc + ws;

        if (eomh) {
            ml += bl;    /* add in the message body length, if collected earlier */
            if (ml <= rtsp->in_size)
                break;    /* all done finding the end of the message. */
        }

        if (ml >= rtsp->in_size)
            return RTSP_not_full;    /* haven't received the entire message yet. */

        /*
         * check first token in each line to determine if there is
         * a message body.
         */
        if (!mb) {    /* content length token not yet encountered. */
            if (!g_ascii_strncasecmp (rtsp->in_buffer + ml, HDR_CONTENTLENGTH,
                 RTSP_BUFFERSIZE - ml)) {
                mb = 1;    /* there is a message body. */
                ml += strlen(HDR_CONTENTLENGTH);
                while (ml < rtsp->in_size) {
                    c = rtsp->in_buffer[ml];
                    if ((c == ':') || (c == ' '))
                        ml++;
                    else
                        break;
                }

                if (sscanf(rtsp->in_buffer + ml, "%d", &bl) != 1) {
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
        for (tc = rtsp->in_size - ml, p = &(rtsp->in_buffer[ml]);
             tc && (*p == '\0'); p++, bl++, tc--);
        *body_len = bl;
    }

    return RTSP_method_rcvd;
}

static void RTSP_state_machine(RTSP_buffer * rtsp, RTSP_Request *req);

static gboolean 
find_tcp_interleaved(gconstpointer value, gconstpointer target)
{
  RTSP_interleaved *i = (RTSP_interleaved *)value;
  gint m = GPOINTER_TO_INT(target);

  return (i[1].channel == m);
}

/**
 * Handles incoming RTSP message, validates them and then dispatches them 
 * with RTSP_state_machine
 * @param rtsp the buffer from where to read the message
 * @return ERR_NOERROR (can also mean RTSP_not_full if the message was not full)
 */
int RTSP_handler(RTSP_buffer * rtsp)
{
    unsigned short status;
    char msg[256];
    int m, op;
    int full_msg;
    RTSP_interleaved *intlvd;
    GSList *list;
    int hlen, blen;

    while (rtsp->in_size) {
        switch ((full_msg = RTSP_full_msg_rcvd(rtsp, &hlen, &blen))) {
        case RTSP_method_rcvd: {
                RTSP_Request *req = rtsp_parse_request(rtsp);
                if ( req )
                    RTSP_state_machine(rtsp, req);
                rtsp_free_request(req);
            }
            RTSP_discard_msg(rtsp, hlen + blen);
            break;
        case RTSP_interlvd_rcvd:
            m = rtsp->in_buffer[1];
	    list = g_slist_find_custom(rtsp->interleaved,
                                       GINT_TO_POINTER(m),
                                       find_tcp_interleaved);
            if (!list) {    // session not found
                fnc_log(FNC_LOG_DEBUG,
                        "Interleaved RTP or RTCP packet arrived"
                        "for unknown channel (%d)... discarding.",
                        m);
                RTSP_discard_msg(rtsp, hlen + blen);
                break;
            }
            intlvd = list->data;
            fnc_log(FNC_LOG_DEBUG,
                    "Interleaved RTCP packet arrived for"
                    "channel %d (len: %d).",
                    m, blen);
            Sock_write(intlvd->local, &rtsp->in_buffer[hlen], blen, NULL, 0);
            RTSP_discard_msg(rtsp, hlen + blen);
            break;
        default:
            return full_msg;
            break;
        }
    }
    return ERR_NOERROR;
}

/**
 * All state transitions are made here except when the last stream packet
 * is sent during a PLAY.  That transition is located in stream_event().
 * @param rtsp the buffer containing the message to dispatch
 * @param method the id of the method to execute
 */
static void RTSP_state_machine(RTSP_buffer * rtsp, RTSP_Request *req)
{
    RTSP_session *p = rtsp->session;
    
    rtsp->rtsp_cseq = req->cseq;

    switch (p->cur_state) {
    case INIT_STATE:{
        /*
        static const ProtocolReply InvalidMethods =
            { 455, true, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n" };
        static const ProtocolReply NotImplemented =
            { 501, true, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n" };
        */

            switch (req->method_id) {
            case RTSP_ID_DESCRIBE:
                RTSP_describe(rtsp);
                break;
            case RTSP_ID_SETUP:
                if (RTSP_setup(rtsp) == ERR_NOERROR) {
                    p->cur_state = READY_STATE;
                }
                break;
            case RTSP_ID_TEARDOWN:
                RTSP_teardown(rtsp);
                break;
            case RTSP_ID_OPTIONS:
                if (RTSP_options(rtsp) == ERR_NOERROR) {
                    p->cur_state = INIT_STATE;
                }
                break;
            case RTSP_ID_PLAY:    /* method not valid this state. */
            case RTSP_ID_PAUSE:
                rtsp_send_reply(rtsp, RTSP_InvalidMethodInState);
                break;
            case RTSP_ID_SET_PARAMETER:
                RTSP_set_parameter(rtsp);
                break;
            default:
                rtsp_send_reply(rtsp, RTSP_NotImplemented);
                break;
            }
            break;
        }        /* INIT state */
    case READY_STATE:{
        /*
        static const ProtocolReply InvalidMethods =
            { 455, true, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n" };
        static const ProtocolReply NotImplemented =
            { 501, true, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n" };
        */

            switch (req->method_id) {
            case RTSP_ID_PLAY:
                if (RTSP_play(rtsp) == ERR_NOERROR) {
                    p->cur_state = PLAY_STATE;
                }
                break;
            case RTSP_ID_SETUP:
                RTSP_setup(rtsp);
                break;
            case RTSP_ID_TEARDOWN:
                RTSP_teardown(rtsp);
                break;
            case RTSP_ID_OPTIONS:
                break;
            case RTSP_ID_SET_PARAMETER:
                RTSP_set_parameter(rtsp);
                break;
            case RTSP_ID_PAUSE:    /* method not valid this state. */
                rtsp_send_reply(rtsp, RTSP_InvalidMethodInState);
                break;
            case RTSP_ID_DESCRIBE:
                RTSP_describe(rtsp);
                break;
            default:
                rtsp_send_reply(rtsp, RTSP_NotImplemented);
                break;
            }
            break;
        }        /* READY state */
    case PLAY_STATE:{
            switch (req->method_id) {
            case RTSP_ID_PLAY:
                // This is a seek
                fnc_log(FNC_LOG_INFO, "EXPERIMENTAL: Seek.");
                RTSP_play(rtsp);
                break;
            case RTSP_ID_PAUSE:
                if (RTSP_pause(rtsp) == ERR_NOERROR) {
                    p->cur_state = READY_STATE;
                }
                break;
            case RTSP_ID_TEARDOWN:
                RTSP_teardown(rtsp);
                break;
            case RTSP_ID_OPTIONS:
                break;
            case RTSP_ID_DESCRIBE:
                RTSP_describe(rtsp);
                break;
            case RTSP_ID_SETUP:
                break;
            case RTSP_ID_SET_PARAMETER:
                RTSP_set_parameter(rtsp);
                break;
            }
            break;
        }        /* PLAY state */
    default:{        /* invalid/unexpected current state. */
            fnc_log(FNC_LOG_ERR,
                "State error: unknown state=%d, method code=%d\n",
                p->cur_state, req->method_id);
        }
        break;
    }            /* end of current state switch */
}
