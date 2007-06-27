/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

/** @file RTSP_state_machine.c
 * @brief Contains RTSP message dispatchment functions
 */

/**
 * Handles incoming RTSP message, validates them and then dispatches them 
 * with RTSP_state_machine
 * @param rtsp the buffer from where to read the message
 * @return ERR_NOERROR (can also mean RTSP_not_full if the message was not full)
 */
int RTSP_handler(RTSP_buffer * rtsp)
{
    unsigned short status;
    char msg[100];
    int m, op;
    int full_msg;
    RTSP_interleaved *intlvd;
    int hlen, blen;

    while (rtsp->in_size) {
        switch ((full_msg = RTSP_full_msg_rcvd(rtsp, &hlen, &blen))) {
        case RTSP_method_rcvd:
            op = RTSP_valid_response_msg(&status, msg, rtsp);
            if (op == 0) {
                // There is NOT an input RTSP message, therefore it's a request
                m = RTSP_validate_method(rtsp);
                if (m < 0) {
                    // Bad request: non-existing method
                    fnc_log(FNC_LOG_INFO, "Bad Request ");
                    send_reply(400, NULL, rtsp);
                } else
                    RTSP_state_machine(rtsp, m);
            } else {
                // There's a RTSP answer in input.
                if (op == ERR_GENERIC) {
                    // Invalid answer
                }
            }
            RTSP_discard_msg(rtsp);
            break;
        case RTSP_interlvd_rcvd:
            m = rtsp->in_buffer[1];
            for (intlvd = rtsp->interleaved;
                 intlvd && !((intlvd->proto.tcp.rtp_ch == m)
                || (intlvd->proto.tcp.rtcp_ch == m));
                 intlvd = intlvd->next);
            if (!intlvd) {    // session not found
                fnc_log(FNC_LOG_DEBUG,
                    "Interleaved RTP or RTCP packet arrived for unknown channel (%d)... discarding.\n",
                    m);
                RTSP_discard_msg(rtsp);
                break;
            }
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
            RTSP_discard_msg(rtsp);
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
void RTSP_state_machine(RTSP_buffer * rtsp, int method)
{
    char *s;
    RTSP_session *p;
    long int session_id;
    char trash[255];

    if ((s = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(s, "%254s %ld", trash, &session_id) != 2) {
            fnc_log(FNC_LOG_INFO,
                "Invalid Session number in Session header\n");
            send_reply(454, 0, rtsp);    /* Session Not Found */
            return;
        }
    }
    p = rtsp->session_list;
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
int RTSP_validate_method(RTSP_buffer * rtsp)
{
    char method[32], hdr[16];
    char object[256];
    char ver[32];
    unsigned int seq;
    int pcnt;        /* parameter count */
        char *p;
    int mid = ERR_GENERIC;

    *method = *object = '\0';
    seq = 0;

    /* parse first line of message header as if it were a request message */

    if ((pcnt =
         sscanf(rtsp->in_buffer, " %31s %255s %31s ", method,
            object, ver)) != 3)
        return ERR_GENERIC;

        p = rtsp->in_buffer;

        while ((p = strstr(p,"\n"))) {
            if ((pcnt = sscanf(p, " %15s %u ", hdr, &seq)) == 2)
                    if (strstr(hdr, HDR_CSEQ)) break;
            p++;
        }
    if (p == NULL)
        return ERR_GENERIC;

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
 * Validates an incoming response message
 * @param status where to save the state specified in the message
 * @param msg where to save the message itself
 * @param rtsp the buffer containing the message
 * @return 1 if everything was fine
 * @return 0 if the parsed message wasn't a response message
 * @return ERR_GENERIC on generic error
 */ 
int RTSP_valid_response_msg(unsigned short *status, char *msg, RTSP_buffer * rtsp)
// This routine is from BP.
{
    char ver[32], trash[15];
    unsigned int stat;
    unsigned int seq;
    int pcnt;        /* parameter count */

    *ver = *msg = '\0';
    /* assuming "stat" may not be zero (probably faulty) */
    stat = 0;

    pcnt =
        sscanf(rtsp->in_buffer, " %31s %u %s %s %u\n%255s ", ver, &stat,
           trash, trash, &seq, msg);

    /* check for a valid response token. */
    if (strncasecmp(ver, "RTSP/", 5))
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
