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
 * */

/** @file
 * @brief Contains RTSP message dispatchment functions
 */

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

int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session);

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

/**
 * @brief RTSP method tokens
 *
 * They are used to identify in which state of the state machines we
 * are.
 */
enum RTSP_method_token {
  RTSP_ID_ERROR = ERR_GENERIC,
  RTSP_ID_DESCRIBE,
  RTSP_ID_ANNOUNCE,
  RTSP_ID_GET_PARAMETERS,
  RTSP_ID_OPTIONS,
  RTSP_ID_PAUSE,
  RTSP_ID_PLAY,
  RTSP_ID_RECORD,
  RTSP_ID_REDIRECT,
  RTSP_ID_SETUP,
  RTSP_ID_SET_PARAMETER,
  RTSP_ID_TEARDOWN
};

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

static void RTSP_state_machine(RTSP_buffer * rtsp, enum RTSP_method_token method_code);
static int RTSP_valid_response_msg(unsigned short *status, char *msg,
				   RTSP_buffer * rtsp);
static enum RTSP_method_token RTSP_validate_method(RTSP_buffer * rtsp);

static gboolean find_tcp_interleaved(gconstpointer value, gconstpointer target)
{
  RTSP_interleaved *i = (RTSP_interleaved *)value;
  gint m = GPOINTER_TO_INT(target);

  return (i->proto.tcp.rtp_ch == m || i->proto.tcp.rtcp_ch == m);
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
    enum RTSP_method_token method;

    while (rtsp->in_size) {
        switch ((full_msg = RTSP_full_msg_rcvd(rtsp, &hlen, &blen))) {
        case RTSP_method_rcvd:
            op = RTSP_valid_response_msg(&status, msg, rtsp);
            if (op == 0) {
                // There is NOT an input RTSP message, therefore it's a request
                method = RTSP_validate_method(rtsp);
                if (method == RTSP_ID_ERROR) {
                    // Bad request: non-existing method
                    fnc_log(FNC_LOG_INFO, "Bad Request ");
                    send_reply(400, NULL, rtsp);
                } else
                    RTSP_state_machine(rtsp, method);
            } else {
                // There's a RTSP answer in input.
                if (op == ERR_GENERIC) {
                    // Invalid answer
                }
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
                    "Interleaved RTP or RTCP packet arrived for unknown channel (%d)... discarding.\n",
                    m);
                RTSP_discard_msg(rtsp, hlen + blen);
                break;
            }
            intlvd = list->data;
            if (m == intlvd->proto.tcp.rtcp_ch) {    // RTCP pkt arrived
                fnc_log(FNC_LOG_DEBUG,
                    "Interleaved RTCP packet arrived for channel %d (len: %d).\n",
                    m, blen);
                Sock_write(intlvd->rtcp_local, &rtsp->in_buffer[hlen],
                      blen, NULL, 0);
            } else    // RTP pkt arrived: do nothing...
                fnc_log(FNC_LOG_DEBUG,
                    "Interleaved RTP packet arrived for channel %d.\n",
                    m);
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
static void RTSP_state_machine(RTSP_buffer * rtsp, enum RTSP_method_token method)
{
    char *s;
    RTSP_session *p;
    guint64 session_id;

    if ((s = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(s, "%*s %"SCNu64, &session_id) != 1) {
            fnc_log(FNC_LOG_INFO,
                "Invalid Session number in Session header\n");
            send_reply(454, NULL, rtsp);    /* Session Not Found */
            return;
        }
    }
    p = rtsp->session;
    if (p == NULL) {
        return;
    }
    switch (p->cur_state) {
    case INIT_STATE:{
            switch (method) {
            case RTSP_ID_DESCRIBE:
                RTSP_describe(rtsp);
                break;
            case RTSP_ID_SETUP:
                if (RTSP_setup(rtsp, &p) == ERR_NOERROR) {
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
                send_reply(455,
                       "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n",
                       rtsp);
                break;
            case RTSP_ID_SET_PARAMETER:
                RTSP_set_parameter(rtsp);
                break;
            default:
                send_reply(501,
                       "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n",
                       rtsp);
                break;
            }
            break;
        }        /* INIT state */
    case READY_STATE:{
            switch (method) {
            case RTSP_ID_PLAY:
                if (RTSP_play(rtsp) == ERR_NOERROR) {
                    p->cur_state = PLAY_STATE;
                }
                break;
            case RTSP_ID_SETUP:
                if (RTSP_setup(rtsp, &p) == ERR_NOERROR) {
                    p->cur_state = READY_STATE;
                }
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
                send_reply(455,
                       "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n",
                       rtsp);
                break;
            case RTSP_ID_DESCRIBE:
                RTSP_describe(rtsp);
                break;
            default:
                send_reply(501,
                       "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n",
                       rtsp);
                break;
            }
            break;
        }        /* READY state */
    case PLAY_STATE:{
            switch (method) {
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
                p->cur_state, method);
        }
        break;
    }            /* end of current state switch */
}

/** validates an rtsp message and returns its id
 * @param rtsp the buffer containing the message
 * @return the message id or -1 if something doesn't work in the request 
 */
static enum RTSP_method_token RTSP_validate_method(RTSP_buffer * rtsp)
{
    char method[32], hdr[16];
    char object[256];
    char ver[32];
    unsigned int seq;
    int pcnt;        /* parameter count */
    char *p;
    enum RTSP_method_token mid = RTSP_ID_ERROR;

    *method = *object = '\0';
    seq = 0;

    /* parse first line of message header as if it were a request message */

    if ((pcnt =
         sscanf(rtsp->in_buffer, " %31s %255s %31s ", method,
            object, ver)) != 3)
        return RTSP_ID_ERROR;

    p = rtsp->in_buffer;

    while ((p = strstr(p,"\n"))) {
        if ((pcnt = sscanf(p, " %15s %u ", hdr, &seq)) == 2)
            if (strstr(hdr, HDR_CSEQ)) break;
        p++;
    }

    if (p == NULL)
        return RTSP_ID_ERROR;

    if (strcmp(method, RTSP_METHOD_DESCRIBE) == 0) {
        mid = RTSP_ID_DESCRIBE;
    }
    if (strcmp(method, RTSP_METHOD_ANNOUNCE) == 0) {
        mid = RTSP_ID_ANNOUNCE;
    }
    if (strcmp(method, RTSP_METHOD_GET_PARAMETERS) == 0) {
        mid = RTSP_ID_GET_PARAMETERS;
    }
    if (strcmp(method, RTSP_METHOD_OPTIONS) == 0) {
        mid = RTSP_ID_OPTIONS;
    }
    if (strcmp(method, RTSP_METHOD_PAUSE) == 0) {
        mid = RTSP_ID_PAUSE;
    }
    if (strcmp(method, RTSP_METHOD_PLAY) == 0) {
        mid = RTSP_ID_PLAY;
    }
    if (strcmp(method, RTSP_METHOD_RECORD) == 0) {
        mid = RTSP_ID_RECORD;
    }
    if (strcmp(method, RTSP_METHOD_REDIRECT) == 0) {
        mid = RTSP_ID_REDIRECT;
    }
    if (strcmp(method, RTSP_METHOD_SETUP) == 0) {
        mid = RTSP_ID_SETUP;
    }
    if (strcmp(method, RTSP_METHOD_SET_PARAMETER) == 0) {
        mid = RTSP_ID_SET_PARAMETER;
    }
    if (strcmp(method, RTSP_METHOD_TEARDOWN) == 0) {
        mid = RTSP_ID_TEARDOWN;
    }

    rtsp->rtsp_cseq = seq;    /* set the current method request seq. number. */
    return mid;
}

/**
 * @brief Validates an incoming response message
 *
 * @param status where to save the state specified in the message
 * @param msg where to save the message itself
 * @param rtsp the buffer containing the message
 *
 * @retval 1 No error
 * @retval 0 The parsed message wasn't a response message
 * @retval ERR_GENERIC Error
 */ 
static int RTSP_valid_response_msg(unsigned short *status, char *msg, RTSP_buffer * rtsp)
// This routine is from BP.
{
    char ver[32];
    unsigned int stat;
    unsigned int seq;
    int pcnt;        /* parameter count */

    *ver = *msg = '\0';
    /* assuming "stat" may not be zero (probably faulty) */
    stat = 0;

    pcnt =
        sscanf(rtsp->in_buffer, " %31s %u %*s %*s %u\n%255s ",
                ver, &stat, &seq, msg);

    /* check for a valid response token. */
    if (g_ascii_strncasecmp(ver, "RTSP/", 5))
        return 0;    /* not a response message */

    /* confirm that at least the version, status code and sequence are there. */
    if (pcnt < 3 || stat == 0)
        return 0;    /* not a response message */

    /*
     * here is where code can be added to reject the message if the RTSP
     * version is not compatible.
     */

    /* check if the sequence number is valid in this response message. */
    if (rtsp->rtsp_cseq != seq + 1) {
        fnc_log(FNC_LOG_ERR,
            "Invalid sequence number returned in response.\n");
        return ERR_GENERIC;
    }

    *status = stat;
    return 1;
}
