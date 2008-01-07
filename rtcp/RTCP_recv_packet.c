/* *
 *  This file is part of Feng
 *
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it>
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
#include <netinet/in.h>

#include <fenice/rtcp.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

int RTCP_recv_packet(RTP_session * session)
{
    short len = 0;
    for (len = 0; len < session->rtcp_insize;
         len +=
         (ntohs(*((short *) &(session->rtcp_inbuffer[len + 2]))) + 1) * 4) {
        switch (session->rtcp_inbuffer[1 + len]) {
        case SR:{
            int ssrc_count, i;
            unsigned char tmp[4];
            fnc_log(FNC_LOG_VERBOSE, "RTCP SR packet received\n");
            session->rtcp_stats[i_client].SR_received += 1;
            session->rtcp_stats[i_client].pkt_count =
                *((int *) &(session->rtcp_inbuffer[20 + len]));
            session->rtcp_stats[i_client].octet_count =
                *((int *) &(session->rtcp_inbuffer[24 + len]));
            ssrc_count = session->rtcp_inbuffer[0 + len] & 0x1f;
            for (i = 0; i < ssrc_count; ++i) {
                session->rtcp_stats[i_client].fract_lost =
                    session->rtcp_inbuffer[32 + len];
                tmp[0] = 0;
                tmp[1] = session->rtcp_inbuffer[33 + len];
                tmp[2] = session->rtcp_inbuffer[34 + len];
                tmp[3] = session->rtcp_inbuffer[35 + len];
                session->rtcp_stats[i_client].pkt_lost = ntohl(*((int *) tmp));
                session->rtcp_stats[i_client].highest_seq_no =
                    ntohl(session->rtcp_inbuffer[36 + len]);
                session->rtcp_stats[i_client].jitter =
                    ntohl(session->rtcp_inbuffer[40 + len]);
                session->rtcp_stats[i_client].last_SR =
                    ntohl(session->rtcp_inbuffer[44 + len]);
                session->rtcp_stats[i_client].delay_since_last_SR =
                    ntohl(session->rtcp_inbuffer[48 + len]);
            }
            break;
        }
        case RR:{
            int ssrc_count, i;
            unsigned char tmp[4];
            fnc_log(FNC_LOG_VERBOSE, "RTCP RR packet received\n");
            session->rtcp_stats[i_client].RR_received += 1;
            ssrc_count = session->rtcp_inbuffer[0 + len] & 0x1f;
            for (i = 0; i < ssrc_count; ++i) {
                session->rtcp_stats[i_client].fract_lost =
                    session->rtcp_inbuffer[12 + len];
                tmp[0] = 0;
                tmp[1] = session->rtcp_inbuffer[13 + len];
                tmp[2] = session->rtcp_inbuffer[14 + len];
                tmp[3] = session->rtcp_inbuffer[15 + len];
                session->rtcp_stats[i_client].pkt_lost =
                    ntohl(*((int *) tmp));
                session->rtcp_stats[i_client].
                    highest_seq_no =
                    ntohl(session->rtcp_inbuffer[16 + len]);
                session->rtcp_stats[i_client].jitter =
                    ntohl(session->rtcp_inbuffer[20 + len]);
                session->rtcp_stats[i_client].last_SR =
                    ntohl(session->rtcp_inbuffer[24 + len]);
                session->rtcp_stats[i_client].delay_since_last_SR =
                    ntohl(session->rtcp_inbuffer[28 + len]);
            }
            break;
        }
        case SDES:{
            fnc_log(FNC_LOG_VERBOSE, "RTCP SDES packet received\n");
            switch (session->rtcp_inbuffer[8]) {
            case CNAME:
                session->rtcp_stats[1].dest_SSRC =
                    ntohs(*((int *)&(session->rtcp_inbuffer[4])));
                break;
            case NAME:
                break;
            case EMAIL:
                break;
            case PHONE:
                break;
            case LOC:
                break;
            case TOOL:
                break;
            case NOTE:
                break;
            case PRIV:
                break;
            }
            break;
        }
        case BYE:
            fnc_log(FNC_LOG_VERBOSE, "RTCP BYE packet received\n");
            break;
        case APP:
            fnc_log(FNC_LOG_VERBOSE, "RTCP APP packet received\n");
            break;
        default:
            fnc_log(FNC_LOG_VERBOSE, "Unknown RTCP received and ignored.\n");
            return ERR_NOERROR;
        }
    }
    return ERR_NOERROR;
}
