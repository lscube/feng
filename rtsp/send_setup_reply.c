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

#include <fenice/rtsp.h>
#include <fenice/multicast.h>
#include <fenice/fnc_log.h>

int send_setup_reply(RTSP_buffer * rtsp, RTSP_session * session,
                     RTP_session * rtp_s)
{
    char r[1024];
    int w_pos = 0;

    snprintf(r, sizeof(r),
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    add_time_stamp(r, 0);
    w_pos = strlen(r);
    w_pos += snprintf(r + w_pos, sizeof(r) - w_pos, "Session: %d" RTSP_EL,
                      session->session_id);

    w_pos += snprintf(r + w_pos, sizeof(r) - w_pos, "Transport: ");
    switch (Sock_type(rtp_s->transport.rtp_sock)) {
    case UDP:
#if 0
//Temporary disable of multicast code for netembro
        // if (!(descr->flags & SD_FL_MULTICAST)) {
        if (rtp_s->transport.u.udp.is_multicast) {
            /*
               strcat(r, "Transport: RTP/AVP;multicast;");
               sprintf(temp, "destination=%s;", descr->multicast);
               strcat(r, temp);
               strcat(r, "port=");
             */
            w_pos +=
                sprintf(r + w_pos,
                    "RTP/AVP;multicast;ttl=%d;destination=%s;port=",
                    (int) DEFAULT_TTL, descr->multicast);
        } else {
#endif //Temporary disable of multicast code for netembro
            /*
               strcat(r, "Transport: RTP/AVP;unicast;client_port=");
               sprintf(temp, "%d", rtp_s->transport.u.udp.cli_ports.RTP);
               strcat(r, temp);
               strcat(r, "-");
               sprintf(temp, "%d", rtp_s->transport.u.udp.cli_ports.RTCP);
               strcat(r, temp);

               sprintf(temp, ";source=%s", get_address());
               strcat(r, temp);

               strcat(r, ";server_port=");
             */
            w_pos +=
                snprintf(r + w_pos, sizeof(r) - w_pos,
                    "RTP/AVP;unicast;client_port=%d-%d;source=%s;server_port=",
                    get_remote_port(rtp_s->transport.rtp_sock),
                    get_remote_port(rtp_s->transport.rtcp_sock),
                    get_local_host(rtsp->sock));
#if 0
        }
#endif
        /*
           sprintf(temp, "%d", rtp_s->transport.u.udp.ser_ports.RTP);
           strcat(r, temp);
           strcat(r, "-");
           sprintf(temp, "%d", rtp_s->transport.u.udp.ser_ports.RTCP);
           strcat(r, temp);
         */
        w_pos +=
            snprintf(r + w_pos, sizeof(r) - w_pos, "%d-%d",
                get_local_port(rtp_s->transport.rtp_sock),
                get_local_port(rtp_s->transport.rtcp_sock));

#if 0
        // if ((descr->flags & SD_FL_MULTICAST)) {
        if (rtp_s->transport.u.udp.is_multicast) {
            /*
               strcat(r,";ttl=");
               sprintf(ttl,"%d",(int)DEFAULT_TTL);
               strcat(r,ttl);
             */
            w_pos +=
                sprintf(r + w_pos, ";ttl=%d", (int) DEFAULT_TTL);
        }
#endif
        break;
    case LOCAL:
        if (Sock_type(rtsp->sock) == TCP) {
            w_pos += snprintf(r + w_pos, sizeof(r) - w_pos,
                 "RTP/AVP/TCP;interleaved=%d-%d",
                 rtp_s->transport.rtp_ch,
                 rtp_s->transport.rtcp_ch);
        }
        else if (Sock_type(rtsp->sock) == SCTP) {
            w_pos += snprintf(r + w_pos, sizeof(r) - w_pos,
                 "RTP/AVP/SCTP;server_streams=%d-%d",
                 rtp_s->transport.rtp_ch,
                 rtp_s->transport.rtcp_ch);
        }
        break;
    default:
        break;
    }
    snprintf(r + w_pos, sizeof(r) - w_pos, ";ssrc=%u", rtp_s->ssrc);

    strcat(r, RTSP_EL RTSP_EL);

    bwrite(r, (unsigned short) strlen(r), rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - %s ",
            r_selected_track(rtp_s->track_selector)->info->name);

    return ERR_NOERROR;
}
