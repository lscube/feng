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
#include <netembryo/protocol_responses.h>
#include <netembryo/url.h>
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
    GSList *interleaved;
    GSList *ev_io;
    ev_io *ev_io_write;
    // Run-Time
    RTSP_session *session;
    feng *srv;
} RTSP_buffer;

typedef enum {
  df_Unsupported = -2,
  df_Unknown = -1,
  df_SDP_format = 0
} RTSP_description_format;

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
 * @brief Structure representing a request coming from the client.
 */
typedef struct {
    char *method; //!< String of the method (used for logging, mostly)
    enum RTSP_method_token method_id; //!< ID of the method (for the state machine)

    char *object; //!< Object of the request (URL or *)
    int cseq; //!< Sequence number
    guint64 session_id;

    /** All the headers we don't parse specifically */
    GHashTable *headers;
} RTSP_Request;

int RTSP_handler(RTSP_buffer * rtsp);

#define RTSP_not_full 0
#define RTSP_method_rcvd 1
#define RTSP_interlvd_rcvd 2


/** 
 * RTSP low level functions, they handle message specific parsing and
 * communication.
 *
 * @defgroup rtsp_low RTSP low level functions
 * @{
 */

void rtsp_send_reply(const RTSP_buffer *rtsp, RTSP_ResponseCode code);

ssize_t rtsp_send(RTSP_buffer * rtsp);

gboolean rtsp_request_get_url(RTSP_buffer *rtsp, RTSP_Request *req, Url *url);

void rtsp_bwrite(const RTSP_buffer *rtsp, GString *buffer);
GString *rtsp_generate_response(RTSP_ResponseCode code, guint cseq);
GString *rtsp_generate_ok_response(guint cseq, guint64 session);

RTSP_buffer *rtsp_client_new(feng *srv, Sock *client_sock);
void rtsp_client_destroy(RTSP_buffer *rtsp);

/**
 * @}
 */

/**
 * @}
 */
#endif // FN_RTSP_H
