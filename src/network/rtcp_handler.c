/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#include "rtcp.h"

#include <fenice/utils.h>
#include <fenice/fnc_log.h>

/**
 * Send the rtcp payloads in queue
 */

int RTCP_flush(RTP_session * session)
{
    fd_set wset;
    struct timeval t;
    Sock *rtcp_sock = session->transport.rtcp_sock;

    /*---------------SEE eventloop/rtsp_server.c-------*/
    FD_ZERO(&wset);
    t.tv_sec = 0;
    t.tv_usec = 1000;

    if (session->rtcp_outsize > 0)
        FD_SET(Sock_fd(rtcp_sock), &wset);
    if (select(Sock_fd(rtcp_sock) + 1, 0, &wset, 0, &t) < 0) {
        fnc_log(FNC_LOG_ERR, "select error\n");
        /*send_reply(500, NULL, rtsp); */
        return ERR_GENERIC;
    }

    if (FD_ISSET(Sock_fd(rtcp_sock), &wset)) {
        if (Sock_write(rtcp_sock, session->rtcp_outbuffer,
            session->rtcp_outsize, NULL, MSG_EOR | MSG_DONTWAIT) < 0)
            fnc_log(FNC_LOG_VERBOSE, "RTCP Packet Lost\n");
        session->rtcp_outsize = 0;
        fnc_log(FNC_LOG_VERBOSE, "OUT RTCP\n");
    }

    return ERR_NOERROR;
}

int RTCP_handler(RTP_session * session)
{
    if (session->rtcp_stats[i_server].pkt_count % 29 == 1) {
        RTCP_send_packet(session, SR);
        RTCP_send_packet(session, SDES);
        return RTCP_flush(session);
    }
    return ERR_NOERROR;
}
