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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtcp.h"

#include <fenice/utils.h>
#include <fenice/server.h>
#include <fenice/fnc_log.h>
#include <netinet/in.h>

int RTCP_send_packet(RTP_session * session, rtcp_pkt_type type)
{
    unsigned char *payload = NULL;
    RTCP_header hdr;
    size_t pkt_size = 0, hdr_s = 0, payload_s = 0;
    Track *t = r_selected_track(session->track_selector);

    hdr.version = 2;
    hdr.padding = 0;
    hdr.count = 0;
    hdr.pt = type;
    hdr_s = sizeof(hdr);
    switch (type) {
    case SR:{
        struct timespec ntp_time;
        double now;
        RTCP_header_SR hdr_sr;
        payload_s = sizeof(hdr_sr);
        hdr_sr.ssrc = htonl(session->ssrc);
        now = gettimeinseconds(&ntp_time);
        hdr_sr.ntp_timestampH =
            htonl((unsigned int) ntp_time.tv_sec + 2208988800u);
        hdr_sr.ntp_timestampL =
            htonl((((uint64_t) ntp_time.tv_nsec) << 32) / 1000000000u);
        hdr_sr.rtp_timestamp =
            htonl((unsigned int) ((now - session->start_time) *
                  t->properties->clock_rate) + session->start_rtptime);

        hdr_sr.pkt_count = htonl(session->rtcp_stats[i_server].pkt_count);
        hdr_sr.octet_count = htonl(session->rtcp_stats[i_server].octet_count);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_sr, payload_s);
        break;
    }
    case RR:{
        RTCP_header_RR hdr_rr;
        payload_s = sizeof(hdr_rr);
        hdr_rr.ssrc = htonl(session->ssrc);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_rr, payload_s);
        break;
    }
    case SDES:{
        RTCP_header_SDES hdr_sdes;
        const char *name = session->transport.rtcp_sock->local_host ? 
	  session->transport.rtcp_sock->local_host : "::";
        int hdr_sdes_s = sizeof(hdr_sdes);
        size_t name_s = strlen(name);
        payload_s = hdr_sdes_s + name_s;
        // Padding
        payload_s += (((hdr_s + payload_s) % 4) ? 1 : 0);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        hdr.count = 1;
        hdr_sdes.ssrc = htonl(session->ssrc);
        hdr_sdes.attr_name = CNAME;    // 1=CNAME
        hdr_sdes.len = name_s;
        memcpy(payload, &hdr_sdes, hdr_sdes_s);
        memcpy(payload + hdr_sdes_s, name, name_s);
        break;
    }
    case BYE:{
        RTCP_header_BYE hdr_bye;
        int hdr_bye_s = sizeof(hdr_bye);
        static const char reason[20] = "The medium is over.";
        payload_s = hdr_bye_s + sizeof(reason);
        hdr.count = 1;
        hdr_bye.ssrc = htonl(session->ssrc);
        hdr_bye.length = htonl(sizeof(reason)-1);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_bye, hdr_bye_s);
        memcpy(payload + hdr_bye_s, reason, sizeof(reason));
        break;
    }
    default:
        return ERR_NOERROR;
    }

    pkt_size += payload_s + hdr_s;
    hdr.length = htons((pkt_size >> 2) - 1);

    if (session->rtcp_outsize + pkt_size <= sizeof(session->rtcp_outbuffer)) {
        memcpy(session->rtcp_outbuffer + session->rtcp_outsize, &hdr, hdr_s);
        memcpy(session->rtcp_outbuffer + session->rtcp_outsize + hdr_s,
               payload, payload_s);
        session->rtcp_outsize += pkt_size;
    } else {
        fnc_log(FNC_LOG_VERBOSE, "Output RTCP packet lost\n");
    }
    g_free(payload);
    return ERR_NOERROR;
}
