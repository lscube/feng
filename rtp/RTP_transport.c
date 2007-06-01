/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by LScube team <team@streaming.polito.it>
 *      - see AUTHORS for the complete list
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */


#include <config.h>

#include <fenice/rtp.h>
#include <fenice/demuxer.h>
#include <fenice/bufferpool.h>
#include <fenice/mediathread.h>

#if ENABLE_DUMP
#include <fenice/debug.h>
#endif

/**
 * @file RTP_transport.c
 * RTP packets sending and receiving with session handling
 */


/**
 * Sends pending RTP packets for the given session
 * @param session the RTP session for which to send the packets
 * @return ERR_NOERROR
 * @return ERR_ALLOC on buffer allocation errors
 * @retunr ERR_EOF on stream end
 * @return Same error values as event_buffer_low on event emission problems
 */
int RTP_send_packet(RTP_session * session)
{
    unsigned char *packet = NULL;
    unsigned int hdr_size = 0;
    RTP_header r;        // 12 bytes
    int res = ERR_GENERIC;
    double nextts;
    OMSSlot *slot = NULL;
    ssize_t psize_sent = 0;
    Track *t = r_selected_track(session->track_selector);

    if (!(slot = OMSbuff_getreader(session->cons))) {
        if ((res = event_buffer_low(session, t)) != ERR_NOERROR) {
            switch (res) {
            case ERR_EOF:
                fnc_log(FNC_LOG_INFO, "End of stream reached");
                break;
            default:
                fnc_log(FNC_LOG_FATAL, "Unable to emit event buffer low");
                break;
            }
            return res;
        }
    }

    while (slot) {
        hdr_size = sizeof(r);
        r.version = 2;
        r.padding = 0;
        r.extension = 0;
        r.csrc_len = 0;
        r.marker = slot->marker;
        r.payload = t->properties->payload_type;
        r.seq_no = htons(slot->slot_seq + session->start_seq - 1);
        r.timestamp =
            htonl(session->start_rtptime + 
                  ((slot->rtp_time) ? slot->rtp_time :
                   (slot->timestamp * t->properties->clock_rate)));
        fnc_log(FNC_LOG_VERBOSE, "[RTP] Timestamp: %u", r.timestamp);
        r.ssrc = htonl(session->ssrc);
        packet = calloc(1, slot->data_size + hdr_size);
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
            char fname[255];
            char crtp[255];
            memset(fname, 0, sizeof(fname));
            strcpy(fname, "dump_fenice.");
            strcat(fname,
                   session->current_media->description.
                   encoding_name);
            strcat(fname, ".");
            sprintf(crtp, "%d", session->transport.rtp_fd);
            strcat(fname, crtp);
            if (strcmp
                (session->current_media->description.encoding_name,
                 "MPV") == 0
                || strcmp(session->current_media->description.
                      encoding_name, "MPA") == 0)
                dump_payload(packet + 16, psize_sent - 16,
                         fname);
            else
                dump_payload(packet + 12, psize_sent - 12,
                         fname);
#endif
            session->rtcp_stats[i_server].pkt_count++;
            session->rtcp_stats[i_server].octet_count +=
                slot->data_size;
        }

        free(packet);

        OMSbuff_gotreader(session->cons);

        nextts = OMSbuff_nextts(session->cons);
        if (nextts < 0) {
            event_buffer_low(session, t);
            slot = NULL;
            session->cons->frames--;
        } else {
            slot = OMSbuff_getreader(session->cons);
            if( OMSbuff_nextts(session->cons) < 0)
                event_buffer_low(session, t);
        }
    }

    return ERR_NOERROR;
}

/**
 * Receives data from the socket linked to the session and puts it inside the session buffer
 * @param session the RTP session for which to receive the packets
 * @param proto the protocol to use (actually only rtcp is a valid option)
 * @return size of te received data or -1 on error or invalid protocol request
 */
ssize_t RTP_recv(RTP_session * session, rtp_protos proto)
{
    Sock *s = session->transport.rtcp_sock;
    struct sockaddr *sa_p = (struct sockaddr *)&(session->transport.last_stg);

    if (proto == rtcp_proto) {
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
    }
    else
        return -1;

    return session->rtcp_insize;
}

/**
 * Closes a transport linked to a session
 * @param session the RTP session for which to close the transport
 * @return always 0
 */
int RTP_transport_close(RTP_session * session) {
    port_pair pair;
    pair.RTP = get_local_port(session->transport.rtp_sock);
    pair.RTCP = get_local_port(session->transport.rtcp_sock);
    switch (session->transport.rtp_sock->socktype) {
        case UDP:
            RTP_release_port_pair(&pair);
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
RTP_session *RTP_session_destroy(RTP_session * session)
{
    RTP_session *next = session->next;

    RTP_transport_close(session);

    // Close track selector
    r_close_tracks(session->track_selector);

    // destroy consumer
    OMSbuff_unref(session->cons);


    // Deallocate memory
    free(session);

    return next;
}
