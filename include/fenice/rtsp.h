/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

#ifndef _RTSPH
#define _RTSPH

#include <time.h>
#include <config.h>
#include <fenice/utils.h>
#include <netembryo/wsocket.h>
#include <fenice/rtp.h>
#include <fenice/rtcp.h>
#include <fenice/sdp2.h>
#include <fenice/schedule.h>

#define RTSP_RESERVED 4096
#define RTSP_BUFFERSIZE (65536 + RTSP_RESERVED)

    /* FIXME move rtsp states to an enum? */
#define INIT_STATE      0
#define READY_STATE     1
#define PLAY_STATE      2

#define RTSP_VER "RTSP/1.0"

#define RTSP_EL "\r\n"
#define RTSP_RTP_AVP "RTP/AVP"


typedef struct _RTSP_interleaved {
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
    struct _RTSP_interleaved *next;
} RTSP_interleaved;

typedef struct _RTSP_session {
    int cur_state;
    unsigned long session_id;
    int started;
    RTP_session *rtp_session;
    struct _RTSP_session *next;
    // mediathread resource
    Resource *resource;
} RTSP_session;

typedef struct _RTSP_buffer {
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
    RTSP_session *session_list;
    struct _RTSP_buffer *next;
} RTSP_buffer;

typedef enum {
       df_SDP_format = 0
} description_format;


/**
 * @defgroup RTSP
 * @{
 */
/** 
 * RTSP high level functions, they maps to the rtsp methods
 * @defgroup rtsp_high RTSP high level functions
 * @{
 */

void RTSP_state_machine(RTSP_buffer * rtsp, int method_code);

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

void RTSP_discard_msg(RTSP_buffer * rtsp);

void RTSP_remove_msg(int len, RTSP_buffer * rtsp);

int RTSP_valid_response_msg(unsigned short *status, char *msg,
                RTSP_buffer * rtsp);

int RTSP_validate_method(RTSP_buffer * rtsp);

void RTSP_initserver(RTSP_buffer * rtsp, Sock *rtsp_sock);

int send_reply(int err, char *addon, RTSP_buffer * rtsp);

ssize_t RTSP_send(RTSP_buffer * rtsp);

#if 0 // To support multiple session per socket...
static inline RTSP_session *rtsp_session_from_id(RTSP_buffer *rtsp,
                                                 unsigned long session_id )
{
    RTSP_session *rtsp_sess;

    for (rtsp_sess = rtsp->session_list;
         rtsp_sess != NULL;
         rtsp_sess = rtsp_sess->next)
            if (rtsp_sess->session_id == session_id) break;

    return rtsp_sess;
}
static inline rtsp_session_list_free( RTSP_buffer *rtsp )
{
    RTSP_session *cur, *tmp;
    for (cur = rtsp->session_list;
         cur != NULL;) {
         tmp = cur;
         cur = cur->next;
         free(tmp);
    }
}
#endif 

/**
 * @}
 */

/**
 * @}
 */
#endif
