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

#ifndef FN_RTSP_H
#define FN_RTSP_H

#include <time.h>
#include <config.h>

#include <glib.h>
#include <ev.h>

#include "feng_utils.h"
#include <netembryo/wsocket.h>
#include <netembryo/rtsp.h>
#include <netembryo/url.h>

struct feng;
struct Resource;
struct RTP_transport;

/**
 * @addtogroup RTSP
 * @{
 */

/**
 * @brief Separator for the track ID on presentation URLs
 *
 * This string separates the resource URL from the track name in a
 * presentation URL, for SETUP requests for instance.
 *
 * It is represented as a preprocessor macro to be easily used both to
 * glue together the names and to use it to find the track name.
 */
#define SDP_TRACK_SEPARATOR "TrackID="

/**
 * @brief Convenience macro for the track separator in URLs
 *
 * Since outside of the SDP code we need to check for a '/' prefix,
 * just write it here once.
 */
#define SDP_TRACK_URI_SEPARATOR "/" SDP_TRACK_SEPARATOR

#define RTSP_RESERVED 4096
#define RTSP_BUFFERSIZE (65536 + RTSP_RESERVED)

/**
 * @brief RTSP server states
 *
 * These are the constants used to define the states that an RTSP session can
 * have.
 *
 * The server state machine for RTSP is defined in RFC2326 Appendix A,
 * Subsection 2.
 */
typedef enum {
    RTSP_SERVER_INIT,
    RTSP_SERVER_READY,
    RTSP_SERVER_PLAYING,
    RTSP_SERVER_RECORDING
} RTSP_Server_State;

#define RTSP_EL "\r\n"

typedef struct RTSP_session {
    char *session_id;
    RTSP_Server_State cur_state;
    int started;
    GSList *rtp_sessions; // Of type RTP_session
    // mediathread resource
    struct Resource *resource;
    char *resource_uri;
    struct feng *srv;

    /**
     * @brief List of playback requests (of type @ref RTSP_Range)
     *
     * RFC 2326 Section 10.5 defines queues of PLAY requests, which
     * allows precise editing by the client; we keep a list here to
     * make it easier to actually implement the (currently missing)
     * feature.
     */
    GQueue *play_requests;
} RTSP_session;

/**
 * @brief Structure representing a playing range
 *
 * This structure contains the software-accessible range data as
 * provided by the client with the Range header (specified by RFC 2326
 * Section 12.29).
 *
 * This structure is usually filled in by the Ragel parser in @ref
 * ragel_parser_range_header, but is also changed when PAUSE requests
 * are received.
 */
typedef struct RTSP_Range {
    /** Seconds into the stream (NTP) to start the playback at */
    double begin_time;

    /** Seconds into the stream (NTP) to stop the playback at */
    double end_time;

    /** Real-time timestamp when to start the playback */
    double playback_time;
} RTSP_Range;

typedef struct RTSP_Client {
    Sock *sock;

    /**
     * @brief Input buffer
     *
     * This is the input buffer as read straight from the sock socket;
     * GByteArray allows for automatic sizing of the array.
     */
    GByteArray *input;

    GQueue *out_queue;

    // Run-Time
    RTSP_session *session;
    struct feng *srv;

    /**
     * @brief Interleaved setup data
     *
     * Singly-linked list of @ref RTSP_interleaved instances, one per
     * RTP session linked to the RTSP session.
     */
    GSList *interleaved;

    //Events
    ev_async ev_sig_disconnect;
    ev_timer ev_timeout;

    ev_io ev_io_read;
    ev_io ev_io_write;
} RTSP_Client;

void rtsp_client_incoming_cb(struct ev_loop *loop, ev_io *w, int revents);

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
 * @brief Structure representing an incoming request
 *
 * This structure is used to access all the data related to a request
 * message. Since it also embeds the client, it's everything the
 * server needs to send a response for the request.
 */
typedef struct {
    /** The client the request comes from */
    RTSP_Client *client;

    /**
     * @brief String representing the method used
     *
     * Mostly used for logging purposes.
     */
    char *method;
    /**
     * @brief Machine-readable ID of the method
     *
     * Used by the state machine to choose the callback method.
     */
    enum RTSP_method_token method_id;

    /**
     * @brief Object of the request
     *
     * Represents the object to work on, usually the URL for the
     * request (either a resource URL or a track URL). It can be "*"
     * for methods like OPTIONS.
     */
    char *object;

    /**
     * @brief Protocol version used
     *
     * This can only be RTSP/1.0 right now. We log it here for access.log and to
     * remove more logic from the parser itself.
     *
     * @todo This could be HTTP/1.0 or 1.1 when requests come from
     *       QuickTime's proxy-passthrough. Currently that is not the
     *       case though.
     */
    char *version;

    /**
     * @brief All the headers of the request, unparsed.
     *
     * This hash table contains all the headers of the request, in
     * unparsed string form; they can used for debugging purposes or
     * simply to access the original value of an header for
     * pass-through copy.
     */
    GHashTable *headers;
} RTSP_Request;

int RTSP_handler(RTSP_Client * rtsp);

/**
 * @}
 */

/**
 * @brief Structure respresenting a response sent to the client
 * @ingroup rtsp_response
 */
typedef struct {
    /**
     * @brief Backreference to the client.
     *
     * Used to be able to send the response faster
     */
    RTSP_Client *client;

    /**
     * @brief Reference to the original request
     *
     * Used to log the request/response in access.log
     */
    const RTSP_Request *request;

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

RTSP_Response *rtsp_response_new(const RTSP_Request *req, RTSP_ResponseCode code);
void rtsp_response_send(RTSP_Response *response);

/**
 * @brief Create and send a response in a single function call
 * @ingroup rtsp_response
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
                                  RTSP_Server_State invalid_state);

void rtsp_write_cb(struct ev_loop *, ev_io *, int);
void rtsp_read_cb(struct ev_loop *, ev_io *, int);

gboolean rtsp_request_get_url(RTSP_Request *req, Url *url);
gboolean rtsp_request_check_url(RTSP_Request *req);

void rtsp_bwrite(RTSP_Client *rtsp, GString *buffer);

RTSP_session *rtsp_session_new(RTSP_Client *rtsp);
void rtsp_session_free(RTSP_session *session);
void rtsp_session_editlist_append(RTSP_session *session, RTSP_Range *range);
void rtsp_session_editlist_free(RTSP_session *session);

gboolean interleaved_setup_transport(RTSP_Client *, struct RTP_transport *,
                                     int, int);
void interleaved_rtcp_send(RTSP_Client *, int, void *, size_t);
void interleaved_free_list(RTSP_Client *);

void rtsp_do_pause(RTSP_Client *rtsp);

#endif // FN_RTSP_H
