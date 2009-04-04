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

#include <netembryo/wsocket.h>

#include "mediathread/mediathread.h"

#include "bufferqueue.h"

struct feng;
struct Track;
struct RTSP_Client;
struct RTSP_session;

#define RTP_DEFAULT_PORT 5004
#define RTCP_BUFFERSIZE    1024
#define BUFFERED_FRAMES_DEFAULT 16

typedef struct {
    int RTP;
    int RTCP;
} port_pair;

typedef struct RTP_transport {
    Sock *rtp_sock;
    Sock *rtcp_sock;
    struct sockaddr_storage last_stg;
    int rtp_ch, rtcp_ch;
    ev_periodic rtp_writer;
} RTP_transport;

typedef struct RTP_session {
    /** Session paused (or never started) */
    gboolean pause;

    /** Multicast session (treated in a special way) */
    gboolean multicast;

    uint16_t start_seq;
    uint16_t seq_no;

    uint32_t start_rtptime;

    uint32_t ssrc;

    uint32_t last_packet_send_time;

    //these time vars now are now back here
    double start_time;
    double seek_time;
    double send_time;
    double last_timestamp;

    /** Resource name, including path from avroot */
    gchar *sd_filename;

    /** Pointer to the currently-selected track */
    struct Track *track;

#ifdef HAVE_METADATA
    void *metadata;
#endif

    /**
     * @brief Consumer for the track buffer queue
     *
     * This provides the interface between the RTP session and the
     * source of the data to send over the network.
     */
    BufferQueue_Consumer *consumer;

    struct feng *srv;
    struct RTSP_Client *client;

    uint32_t octet_count;
    uint32_t pkt_count;

    RTP_transport transport;
} RTP_session;

/**
 * @defgroup RTP RTP Layer
 * @{
 */

/**
 * RTP ports management functions
 * @defgroup rtp_port RTP ports management functions
 * @{
 */

void RTP_port_pool_init(struct feng *srv, int port);
int RTP_get_port_pair(struct feng *srv, port_pair * pair);
int RTP_release_port_pair(struct feng *srv, port_pair * pair);

/**
 * @}
 */

/**
 * @defgroup rtp_session RTP session management functions
 * @{
 */

RTP_session *rtp_session_new(struct RTSP_Client *, struct RTSP_session *,
                             RTP_transport *, const char *,
                             struct Track *);

void rtp_session_gslist_resume(GSList *, double);
void rtp_session_gslist_seek(GSList *, double);
void rtp_session_gslist_pause(GSList *);
void rtp_session_gslist_free(GSList *);

void rtp_session_handle_sending(RTP_session *session);

/**
 * @}
 */

/**
 * @}
 */

#endif // FN_RTP_H
