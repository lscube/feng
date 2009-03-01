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
#include <netembryo/rtsp.h>
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
    RTSP_buffer *client; //!< The client the request comes from

    char *method; //!< String of the method (used for logging, mostly)
    enum RTSP_method_token method_id; //!< ID of the method (for the state machine)

    char *object; //!< Object of the request (URL or *)

    /**
     * @brief Protocol version used
     *
     * This can only be RTSP/1.0 right now. We log it here for access.log and to
     * remove more logic from the parser itself.
     */
    char *version;

    guint64 session_id;

    /** All the headers we don't parse specifically */
    GHashTable *headers;
} RTSP_Request;

int RTSP_handler(RTSP_buffer * rtsp);

/**
 * @brief Structure respresenting a response sent to the client
 */
typedef struct {
    /**
     * @brief Backreference to the client.
     *
     * Used to be able to send the response faster
     */
    RTSP_buffer *client;

    /**
     * @brief Reference to the original request
     *
     * Used to log the request/response in access.log
     */
    RTSP_Request *request;

    /**
     * @brief The status code of the response.
     *
     * The list of valid status codes is present in RFC 2326 Section 7.1.1 and
     * they are described through Section 11.
     */
    RTSP_ResponseCode status;
    
    /**
     * @brief An hash table of headers to add to the response.
     */
    GHashTable *headers;

    /**
     * @brief An eventual body for the response, used by the DESCRIBE method.
     */
    GString *body;
} RTSP_Response;

RTSP_Response *rtsp_response_new(RTSP_Request *req, RTSP_ResponseCode code);
void rtsp_response_send(RTSP_Response *response);
void rtsp_response_free(RTSP_Response *response);

/**
 * @brief Create and send a response in a single function call
 *
 * @param req Request object to respond to
 * @param code Status code for the response
 *
 * This function is used to avoid creating and sending a new response when just
 * sending out an error response.
 */
static inline void rtsp_quick_response(RTSP_Request *req, RTSP_ResponseCode code)
{
    rtsp_response_send(rtsp_response_new(req, code));
}

gboolean rtsp_check_invalid_state(const RTSP_Request *req,
                                  enum RTSP_machine_state invalid_state);

/** 
 * RTSP low level functions, they handle message specific parsing and
 * communication.
 *
 * @defgroup rtsp_low RTSP low level functions
 * @{
 */

ssize_t rtsp_send(RTSP_buffer * rtsp);

gboolean rtsp_request_get_url(RTSP_buffer *rtsp, RTSP_Request *req, Url *url);

void rtsp_bwrite(const RTSP_buffer *rtsp, GString *buffer);

RTSP_buffer *rtsp_client_new(feng *srv, Sock *client_sock);
void rtsp_client_destroy(RTSP_buffer *rtsp);

/**
 * @}
 */

/**
 * @}
 */
#endif // FN_RTSP_H
