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

#ifndef FN_RTP_H
#define FN_RTP_H

#include <glib.h>

#include <time.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ev.h>

#if ENABLE_SCTP
# include <netinet/in.h>
# include <netinet/sctp.h>
#endif

#include "bufferqueue.h"

struct feng;
struct Track;
struct RTSP_Client;
struct RTSP_Range;
struct RTSP_session;
struct RTP_session;

#define RTP_DEFAULT_PORT 5004
#define BUFFERED_FRAMES_DEFAULT 16
#define RTP_DEFAULT_MTU 1500

typedef gboolean (*rtp_send_cb)(struct RTP_session *client, GByteArray *data);
typedef void (*rtp_close_cb)(struct RTP_session *rtp);

typedef struct RTP_session {
    /** Multicast session (treated in a special way) */
    gboolean multicast;

    uint16_t start_seq;
    uint16_t seq_no;

    uint32_t start_rtptime;

    uint32_t ssrc;

    uint32_t last_packet_send_time;

    struct RTSP_Range *range;
    double send_time;
    double last_timestamp;

    /** URI of the resouce for RTP-Info */
    char *uri;

    /** Pointer to the currently-selected track */
    struct Track *track;

    /**
     * @brief Consumer for the track buffer queue
     *
     * This provides the interface between the RTP session and the
     * source of the data to send over the network.
     */
    BufferQueue_Consumer *consumer;

    struct RTSP_Client *client;

    uint32_t octet_count;
    uint32_t pkt_count;

    rtp_send_cb send_rtp;
    rtp_send_cb send_rtcp;
    rtp_close_cb close_transport;

    ev_periodic rtp_writer;

    /**
     * @brief String representing the Transport header to report
     *
     * It's basically the one chosen between the options provided by
     * the client, as indicated by RFC2326, Section 12.39. We could
     * skip over it if the client only proposed a single transport,
     * but since we need to tell them where to get the data from, and
     * send it to, we always report it.
     *
     * This gets freed right after responding to the SETUP request, so
     * don't rely on it anywhere else.
     */
    char *transport_string;

    /**
     * @brief Private data for the transport
     */
    union {
        struct {
            int rtp;
            int rtcp;
        } tcp;

        struct {
            /** RTP socket descriptor */
            int rtp_sd;
            /** RTCP socket descriptor */
            int rtcp_sd;
            /** RTP remote socket address */
            struct sockaddr *rtp_sa;
            /** RTCP remote socket address */
            struct sockaddr *rtcp_sa;
            ev_io rtcp_reader;
        } udp;

#if ENABLE_SCTP
        struct {
            struct sctp_sndrcvinfo rtp;
            struct sctp_sndrcvinfo rtcp;
        } sctp;
#endif

    } transport;
} RTP_session;

/**
 * @defgroup RTP RTP Layer
 * @{
 */

/**
 * @defgroup rtp_session RTP session management functions
 * @{
 */

typedef enum {
    RTP_UDP,
    RTP_TCP,
    RTP_SCTP,
    _RTP_PROTOCOL_MAX
} RTP_Protocol;

/**
 * @brief Structure filled by the ragel parser of the transport header.
 *
 * @internal
 */
struct ParsedTransport {
    RTP_Protocol protocol;
    //! Mode for UDP transmission, here is easier to access
    enum { TransportUnicast, TransportMulticast } mode;
    int rtp_channel;
    int rtcp_channel;
};


gboolean rtp_udp_transport(struct RTSP_Client *rtsp,
                           struct RTP_session *rtp_s,
                           struct ParsedTransport *parsed);
gboolean rtp_interleaved_transport(struct RTSP_Client *rtsp,
                                   struct RTP_session *rtp_s,
                                   struct ParsedTransport *parsed);
gboolean rtp_sctp_transport(struct RTSP_Client *rtsp,
                            struct RTP_session *rtp_s,
                            struct ParsedTransport *parsed);

void rtsp_interleaved_register(struct RTSP_Client *rtsp,
                               struct RTP_session *rtp_s,
                               int rtp_channel, int rtcp_channel);

RTP_session *rtp_session_new(struct RTSP_Client *,
                             const char *, struct Track *,
                             GSList *transports);

void rtp_session_gslist_resume(GSList *, struct RTSP_Range *range);
void rtp_session_gslist_pause(GSList *);
void rtp_session_gslist_free(GSList *);

void rtp_session_handle_sending(RTP_session *session);

/**
 * @}
 */

/**
 * @addtogroup rtcp
 * @{
 */

typedef enum {
    SR = 200,
    RR = 201,
    SDES = 202,
    BYE = 203,
    APP = 204
} rtcp_pkt_type;

gboolean rtcp_send_sr(RTP_session *session, rtcp_pkt_type type);
void rtcp_handle(RTP_session *session, uint8_t *packet, size_t len);

/**
 * @}
 */

/**
 * @}
 */

#endif // FN_RTP_H
