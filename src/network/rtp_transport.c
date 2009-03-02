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


#include <config.h>

#include "rtp.h"
#include "mediathread/mediathread.h"
#include "mediathread/demuxer.h"
#include <fenice/fnc_log.h>
#include <sys/time.h>

#if ENABLE_DUMP
#include <fenice/debug.h>
#endif

/**
 * @file
 * RTP packets sending and receiving with session handling
 */

/**
 * Calculate RTP time from media timestamp or using pregenerated timestamp
 * depending on what is available
 * @param session RTP session of the packet
 * @param clock_rate RTP clock rate (depends on media)
 * @param slot Slot of which calculate timestamp
 * @return RTP Timestamp (in local endianess)
 */
static inline uint32_t RTP_calc_rtptime(RTP_session *session, int clock_rate, BPSlot *slot) {
    uint32_t calc_rtptime = (uint32_t)((slot->timestamp - session->seek_time) * clock_rate);
    return (session->start_rtptime + (slot->rtp_time ? slot->rtp_time : calc_rtptime));
}

static inline double calc_send_time(RTP_session *session, BPSlot *slot) {
    double last_timestamp = (slot->last_timestamp - session->seek_time);
    return (last_timestamp - session->send_time)/slot->pkt_num;
}

/**
 * Sends pending RTP packets for the given session
 * @param session the RTP session for which to send the packets
 * @retval ERR_NOERROR
 * @retval ERR_ALLOC Buffer allocation errors
 * @retval ERR_EOF End of stream
 * @return Same error values as event_buffer_low on event emission problems
 */
int RTP_send_packet(RTP_session * session)
{
    unsigned char *packet = NULL;
    unsigned int hdr_size = 0;
    RTP_header r;        // 12 bytes
    int res = ERR_NOERROR, bp_frames = 0;
    BPSlot *slot = NULL;
    ssize_t psize_sent = 0;
    feng *srv = session->srv;
    Track *t = r_selected_track(session->track_selector);
    uint32_t now=time(NULL);

    if ((slot = bp_getreader(session->cons))) {
        if (!(session->pause && t->properties->media_source == MS_live)) {
            hdr_size = sizeof(r);
            r.version = 2;
            r.padding = 0;
            r.extension = 0;
            r.csrc_len = 0;
            r.marker = slot->marker & 0x1;
            r.payload = t->properties->payload_type & 0x7f;
            r.seq_no = htons(session->seq_no += slot->seq_delta);
            r.timestamp = htonl(RTP_calc_rtptime(session,
                                t->properties->clock_rate, slot));
            fnc_log(FNC_LOG_VERBOSE, "[RTP] Timestamp: %u", ntohl(r.timestamp));
            r.ssrc = htonl(session->ssrc);
            packet = g_malloc0(slot->data_size + hdr_size);
            if (packet == NULL) {
                return ERR_ALLOC;
            }
            memcpy(packet, &r, hdr_size);
            memcpy(packet + hdr_size, slot->data, slot->data_size);

            if ((psize_sent =
                 Sock_write(session->transport.rtp_sock, packet,
                 slot->data_size + hdr_size, NULL, MSG_DONTWAIT
                 | MSG_EOR)) < 0) {
                fnc_log(FNC_LOG_DEBUG, "RTP Packet Lost\n");
            } else {
#if ENABLE_DUMP
	        GString *fname = g_string_new("dump_fenice.");

		g_string_append(fname,
				session->current_media->description.
				encoding_name);
		g_string_append_printf(fname,
				       ".%d",
				       session->transport.rtp_fd);
                if (strcmp
                    (session->current_media->description.encoding_name,
                    "MPV") == 0
                    || strcmp(session->current_media->description.
                      encoding_name, "MPA") == 0)
                dump_payload(packet + 16, psize_sent - 16,
                         fname.str);
                else
                    dump_payload(packet + 12, psize_sent - 12,
                         fname.str);

		g_string_free(fname, TRUE);
#endif
                if (!session->send_time) {
                    session->send_time = slot->timestamp - session->seek_time;
                }
                bp_frames = (fabs(slot->last_timestamp - slot->timestamp) /
                    t->properties->frame_duration) + 1;
                session->send_time += calc_send_time(session, slot);
                session->rtcp_stats[i_server].pkt_count += slot->seq_delta;
                session->rtcp_stats[i_server].octet_count += slot->data_size;

                session->last_live_packet_send_time = now;
            }
            g_free(packet);
        } else {
#warning Remove as soon as feng is fixed
            usleep(1000);
        }
        bp_gotreader(session->cons);
    }
    if (!slot || bp_frames <= srv->srvconf.buffered_frames) {
        res = event_buffer_low(session, t);
    }
    switch (res) {
        case ERR_NOERROR:
            break;
        case ERR_EOF:
            if (slot) {
                //Wait to empty feng before sending BYE packets.
                res = ERR_NOERROR;
            }
        break;
        default:
            fnc_log(FNC_LOG_FATAL, "Unable to emit event buffer low");
            break;
    }
    return res;
}

/**
 * Receives data from the socket linked to the session and puts it inside the session buffer
 * @param session the RTP session for which to receive the packets
 * @return size of te received data or -1 on error or invalid protocol request
 */
ssize_t RTP_recv(RTP_session * session)
{
    Sock *s = session->transport.rtcp_sock;
    struct sockaddr *sa_p = (struct sockaddr *)&(session->transport.last_stg);

    switch (s->socktype) {
        case UDP:
            session->rtcp_insize = Sock_read(s, session->rtcp_inbuffer,
                     sizeof(session->rtcp_inbuffer),
                     sa_p, 0);
            break;
        case LOCAL:
            session->rtcp_insize = Sock_read(s, session->rtcp_inbuffer,
                     sizeof(session->rtcp_inbuffer),
                     NULL, 0);
            break;
        default:
            session->rtcp_insize = -1;
            break;
    }

    return session->rtcp_insize;
}

/**
 * Closes a transport linked to a session
 * @param session the RTP session for which to close the transport
 * @return always 0
 */
static int RTP_transport_close(RTP_session * session) {
    port_pair pair;
    pair.RTP = get_local_port(session->transport.rtp_sock);
    pair.RTCP = get_local_port(session->transport.rtcp_sock);
    switch (session->transport.rtp_sock->socktype) {
        case UDP:
            RTP_release_port_pair(session->srv, &pair);
        default:
            break;
    }
    Sock_close(session->transport.rtp_sock);
    Sock_close(session->transport.rtcp_sock);
    return 0;
}

/**
 * Deallocates an RTP session, closing its tracks and transports
 * @param session the RTP session to remove
 * @return the subsequent session
 */
void RTP_session_destroy(RTP_session * session)
{
    RTP_transport_close(session);

    // Close track selector
    r_close_tracks(session->track_selector);

    // destroy consumer
    bp_unref(session->cons);

    // Deallocate memory
    g_free(session->sd_filename);
    g_free(session);
}
