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
#include <netembryo/url.h>

#include "rfc822proto.h"
#include "rtp.h"

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

struct RTSP_Client;

typedef void (*rtsp_write_data)(struct RTSP_Client *client, GByteArray *data);

typedef struct RTSP_Client {
    Sock *sock;

    /**
     * @brief Input buffer
     *
     * This is the input buffer as read straight from the sock socket;
     * GByteArray allows for automatic sizing of the array.
     */
    GByteArray *input;

    /**
     * @brief Status of the connected client for the parser
     */
    RFC822_Parser_State status;

    /**
     * @brief Current request being parsed
     *
     * @note The pointer is only valid when RTSP_Client::status is
     *       either Parser_Headers or Parser_Content.
     */
    RFC822_Request *pending_request;

    GQueue *out_queue;

    /**
     * @brief Hash table for interleaved and SCTP channels
     */
    GHashTable *channels;

    // Run-Time
    RTSP_session *session;
    struct feng *srv;

    rtsp_write_data write_data;

    //Events
    ev_async ev_sig_disconnect;
    ev_timer ev_timeout;

    ev_io ev_io_read;
    ev_io ev_io_write;
} RTSP_Client;

void rtsp_write_data_direct(RTSP_Client *client, GByteArray *data);
void rtsp_write_data_base64(RTSP_Client *client, GByteArray *data);
void rtsp_write_string(RTSP_Client *client, GString *str);

void rtsp_client_incoming_cb(struct ev_loop *loop, ev_io *w, int revents);

void RTSP_handler(RTSP_Client * rtsp);

/**
 * RTSP high level functions, mapping to the actual RTSP methods
 *
 * @defgroup rtsp_methods Method functions
 *
 * @{
 */

typedef void (*rtsp_method_function)(RTSP_Client * rtsp, RFC822_Request *req);

void RTSP_describe(RTSP_Client * rtsp, RFC822_Request *req);
void RTSP_setup(RTSP_Client * rtsp, RFC822_Request *req);
void RTSP_play(RTSP_Client * rtsp, RFC822_Request *req);
void RTSP_pause(RTSP_Client * rtsp, RFC822_Request *req);
void RTSP_teardown(RTSP_Client * rtsp, RFC822_Request *req);
void RTSP_options(RTSP_Client * rtsp, RFC822_Request *req);
/**
 * @}
 */

static inline void rtsp_quick_response(struct RTSP_Client *client, RFC822_Request *req, int code)
{
    /* We want to make sure we respond with an RTSP protocol every
       time if we called this function in particular. */
    RFC822_Protocol proto;
    switch ( req->proto ) {
    case RFC822_Protocol_RTSP10:
        proto = req->proto;
        break;
    default:
        proto = RFC822_Protocol_RTSP10;
        break;
    }

    rfc822_quick_response(client, req, proto, code);
}

gboolean rtsp_check_invalid_state(RTSP_Client *client,
                                  const RFC822_Request *req,
                                  RTSP_Server_State invalid_state);

void rtsp_write_cb(struct ev_loop *, ev_io *, int);
void rtsp_read_cb(struct ev_loop *, ev_io *, int);

void rtsp_interleaved(RTSP_Client *rtsp, int channel, uint8_t *data, size_t len);

RTSP_session *rtsp_session_new(RTSP_Client *rtsp);
void rtsp_session_free(RTSP_session *session);
void rtsp_session_editlist_append(RTSP_session *session, RTSP_Range *range);
void rtsp_session_editlist_free(RTSP_session *session);

void rtsp_do_pause(RTSP_Client *rtsp);

/**
 * @defgroup ragel Ragel parsing
 *
 * @brief Functions and data structure for parsing of RTSP protocol.
 *
 * This group enlists all the shared data structure between the
 * parsers written using Ragel (http://www.complang.org/ragel/) and
 * the users of those parsers, usually the method handlers (see @ref
 * rtsp_methods).
 *
 * @{
 */

/**
 * @defgroup ragel_transport Transport: header parsing
 *
 * @{
 */

/**
 * @brief Structure filled by the ragel parser of the transport header.
 *
 * @internal
 */
struct ParsedTransport {
    RTP_Protocol protocol;
    //! Mode for UDP transmission, here is easier to access
    enum { TransportUnicast, TransportMulticast } mode;
    union {
        union {
            struct {
                uint16_t port_rtp;
                uint16_t port_rtcp;
            } Unicast;
            struct {
            } Multicast;
        } UDP;
        struct {
            uint16_t ich_rtp;  //!< Interleaved channel for RTP
            uint16_t ich_rtcp; //!< Interleaved channel for RTCP
        } TCP;
        struct {
            uint16_t ch_rtp;  //!< SCTP channel for RTP
            uint16_t ch_rtcp; //!< SCTP channel for RTCP
        } SCTP;
    } parameters;
};

gboolean check_parsed_transport(struct RTSP_Client *rtsp,
                                struct RTP_transport *rtp_t,
                                struct ParsedTransport *transport);


gboolean ragel_parse_transport_header(struct RTSP_Client *rtsp,
                                      struct RTP_transport *rtp_t,
                                      const char *header);
/**
 *@}
 */

/**
 * @defgroup ragel_range Range: header parsing
 *
 * @{
 */

struct RTSP_Range;

gboolean ragel_parse_range_header(const char *header,
                                  RTSP_Range *range);

/**
 *@}
 */

size_t ragel_parse_request_line(const char *msg, const size_t length, RFC822_Request *req);

int ragel_read_rtsp_headers(GHashTable *headers, const char *msg,
                            size_t length, size_t *read_size);
int ragel_read_http_headers(GHashTable *headers, const char *msg,
                            size_t length, size_t *read_size);
/**
 *@}
 */

/**
 * @}
 */

#endif // FN_RTSP_H
