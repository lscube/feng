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

/**
 * @addtogroup RTSP
 * @{
 */

/**
 * @file
 * @brief feng-wide RTSP definitions
 */

#ifndef FN_RTSP_H
#define FN_RTSP_H

#include <time.h>
#include <config.h>

#include <glib.h>

#include <fenice/utils.h>
#include <netembryo/wsocket.h>
#include "rtp.h"
#include "rtcp.h"
#include "sdp2.h"
#include <fenice/schedule.h>

#ifdef HAVE_LIBSCTP
#include <netinet/sctp.h>
#define MAX_SCTP_STREAMS 15
#endif

#define RTSP_RESERVED 4096
#define RTSP_BUFFERSIZE (65536 + RTSP_RESERVED)

/**
 * @brief state of the state machine
 */
enum RTSP_machine_state {
  INIT_STATE,
  READY_STATE,
  PLAY_STATE
};

#define RTSP_VER "RTSP/1.0"

#define RTSP_EL "\r\n"
#define RTSP_RTP_AVP "RTP/AVP"

typedef struct RTSP_interleaved {
    Sock *local;
    int channel;
} RTSP_interleaved;

typedef struct RTSP_session {
    enum RTSP_machine_state cur_state;
    guint64 session_id;
    int started;
    GSList *rtp_sessions; // Of type RTP_session
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
    GAsyncQueue *out_queue;
    GSList *interleaved_rtp;
    GSList *interleaved_rtcp;
    GSList *interleaved_ev_io;

    // Run-Time
    unsigned int rtsp_cseq;
    RTSP_session *session;
    feng *srv;
} RTSP_buffer;

typedef enum {
  df_Unsupported = -2,
  df_Unknown = -1,
  df_SDP_format = 0
} RTSP_description_format;


int RTSP_handler(RTSP_buffer * rtsp);

#define RTSP_not_full 0
#define RTSP_method_rcvd 1
#define RTSP_interlvd_rcvd 2


/** 
 * RTSP low level functions, they handle message specific parsing and
 * communication.
 * @defgroup rtsp_low RTSP low level functions
 * @{
 */


int send_reply(int err, const char *addon, RTSP_buffer * rtsp);

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
