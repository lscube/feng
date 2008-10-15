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

#ifndef FN_RTSP_H
#define FN_RTSP_H

#include <time.h>
#include <config.h>
#include <fenice/utils.h>
#include <netembryo/wsocket.h>
#include <fenice/rtp.h>
#include <fenice/rtcp.h>
#include <fenice/sdp2.h>
#include <fenice/schedule.h>

#ifdef HAVE_LIBSCTP
#include <netinet/sctp.h>
#define MAX_SCTP_STREAMS 15
#endif

#define RTSP_RESERVED 4096
#define RTSP_BUFFERSIZE (65536 + RTSP_RESERVED)

    /* FIXME move rtsp states to an enum? */
#define INIT_STATE      0
#define READY_STATE     1
#define PLAY_STATE      2

#define RTSP_VER "RTSP/1.0"

#define RTSP_EL "\r\n"
#define RTSP_RTP_AVP "RTP/AVP"


typedef struct RTSP_interleaved {
    Sock *rtp_local;
    Sock *rtcp_local;
    union {
        struct {
            int rtp_ch;
            int rtcp_ch;
        } tcp;
#ifdef HAVE_LIBSCTP
        struct {
            struct sctp_sndrcvinfo rtp;
            struct sctp_sndrcvinfo rtcp;
        } sctp;
#endif
    } proto;
    struct RTSP_interleaved *next;
} RTSP_interleaved;

typedef struct RTSP_session {
    int cur_state;
    unsigned long session_id;
    int started;
    RTP_session *rtp_session;
    // mediathread resource
    Resource *resource;
    feng *srv;
} RTSP_session;

typedef struct RTSP_buffer {
    Sock *sock;
    //unsigned int port;
    // Buffers      
    char in_buffer[RTSP_BUFFERSIZE];
    size_t in_size;
    char out_buffer[RTSP_BUFFERSIZE + MAX_DESCR_LENGTH];
    size_t out_size;
    /* vars for interleaved data:
     * interleaved pkts will be present only at the beginning of out_buffer.
     * this size is used to remenber how much data should be grouped in one
     * pkt with  MSG_MORE flag.
     * */
    RTSP_interleaved *interleaved;

    // Run-Time
    unsigned int rtsp_cseq;
    char descr[MAX_DESCR_LENGTH];
    RTSP_session *session;
    struct RTSP_buffer *next;
    feng *srv;
} RTSP_buffer;

typedef enum {
       df_SDP_format = 0
} description_format;


/**
 * @defgroup RTSP RTSP Layer
 * @{
 */
/** 
 * RTSP high level functions, they maps to the rtsp methods
 * @defgroup rtsp_high RTSP high level functions
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

#define RTSP_not_full 0
#define RTSP_method_rcvd 1
#define RTSP_interlvd_rcvd 2


/** 
 * RTSP low level functions, they handle message specific parsing and
 * communication.
 * @defgroup rtsp_low RTSP low level functions
 * @{
 */


int RTSP_full_msg_rcvd(RTSP_buffer * rtsp, int *hdr_len, int *body_len);
#define RTSP_msg_len RTSP_full_msg_rcvd

void RTSP_discard_msg(RTSP_buffer * rtsp, int len);

int send_reply(int err, char *addon, RTSP_buffer * rtsp);

#ifdef TRACE
#define send_reply(a, b, c) \
    fnc_log(FNC_LOG_INFO,"%s", __PRETTY_FUNCTION__);\
    send_reply(a,b,c);
#endif

ssize_t RTSP_send(RTSP_buffer * rtsp);

/**
 * @}
 */

/**
 * @}
 */
#endif // FN_RTSP_H
