/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <config.h>

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

#include <fenice/rtp.h>
#include <fenice/utils.h>
#include <fenice/bufferpool.h>
#include <fenice/fnc_log.h>
#include <fenice/mediathread.h>

#if ENABLE_DUMP
#include <fenice/debug.h>
#endif

int RTP_send_packet(RTP_session * session)
{
    unsigned char *packet = NULL;
    unsigned int hdr_size = 0;
    RTP_header r;        // 12 bytes
    int res = ERR_GENERIC;
    double nextts;
    OMSSlot *slot = NULL;
    ssize_t psize_sent = 0;
    struct timespec time = {0, 27000000};
    Track *t = r_selected_track(session->track_selector);

    if (!(slot = OMSbuff_getreader(session->cons))) {
        if ((res = event_buffer_low(session, t)) != ERR_NOERROR) {
            fnc_log(FNC_LOG_FATAL, "Unable to emit event buffer low\n");
            return res;
        }
    }

    while (slot) {
        nanosleep(&time, NULL);
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
              (slot->timestamp * t->properties->clock_rate));
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
        // fnc_log(FNC_LOG_DEBUG, "*** current time=%f - next time=%f\n\n", s_time, nextts);

        if (nextts == -1) {
            // fnc_log(FNC_LOG_DEBUG, "*** time on\n");
            event_buffer_low(session, t);
            slot = NULL;
            session->cons->frames--;
        } else {
            slot = OMSbuff_getreader(session->cons);
        }
    }

    return ERR_NOERROR;
}
