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

#include "feng.h"
#include "network/rtsp.h"
#include "network/rtp.h"
#include "client_events.h"
#include "fnc_log.h"
#include "mediathread/demuxer.h"

#include <sys/time.h>
#include <ev.h>

static void client_events_deregister(RTSP_Client *rtsp)
{
    feng *srv = rtsp->srv;

    ev_io_stop(srv->loop, &rtsp->ev_io_read);
    ev_async_stop(srv->loop, &rtsp->ev_sig_disconnect);
    ev_timer_stop(srv->loop, &rtsp->ev_timeout);
}

static void client_ev_disconnect_handler(struct ev_loop *loop, ev_async *w, int revents)
{
    RTSP_Client *rtsp = w->data;
    feng *srv = rtsp->srv;

    //Prevent from requesting disconnection again
    client_events_deregister(rtsp);

    //Close connection
    Sock_close(rtsp->sock);
    srv->connection_count--;

    // Release the RTSP_Client
    rtsp_client_destroy(rtsp);
    fnc_log(FNC_LOG_INFO, "[client] Client removed");
}

static void check_if_any_rtp_session_timedout(gpointer element, gpointer user_data)
{
    RTP_session *session = (RTP_session *)element;
    time_t now = time(NULL);

    /* Check if we didn't send any data for more then STREAM_BYE_TIMEOUT seconds
     * this will happen if we are not receiving any more from live producer or
     * if the stored stream ended.
     */
    if ((session->track->properties->media_source == MS_live) &&
        (now - session->last_packet_send_time) >= LIVE_STREAM_BYE_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Soft stream timeout");
        rtcp_send_sr(session, BYE);
    }

    /* If we were not able to serve any packet and the client ignored our BYE
     * kick it by closing everything
     */
    if ((now - session->last_packet_send_time) >= STREAM_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Stream Timeout, client kicked off!");
        ev_async_send(session->srv->loop, &session->client->ev_sig_disconnect);
    }
}

static void client_ev_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
    RTSP_Client *rtsp = w->data;
    g_slist_foreach(rtsp->session->rtp_sessions, check_if_any_rtp_session_timedout, NULL);
    ev_timer_again (loop, w);
}


void client_add(feng *srv, Sock *client_sock)
{
    RTSP_Client *rtsp = rtsp_client_new(srv, client_sock);

    srv->connection_count++;
    client_sock->data = srv;

    rtsp->ev_io_read.data = rtsp;
    ev_io_init(&rtsp->ev_io_read, rtsp_read_cb, Sock_fd(client_sock), EV_READ);
    ev_io_start(srv->loop, &rtsp->ev_io_read);

    /* to be started/stopped when necessary */
    rtsp->ev_io_write.data = rtsp;
    ev_io_init(&rtsp->ev_io_write, rtsp_write_cb, Sock_fd(client_sock), EV_WRITE);
    fnc_log(FNC_LOG_INFO, "Incoming RTSP connection accepted on socket: %d\n",
            Sock_fd(client_sock));

    rtsp->ev_sig_disconnect.data = rtsp;
    ev_async_init(&rtsp->ev_sig_disconnect, client_ev_disconnect_handler);
    ev_async_start(srv->loop, &rtsp->ev_sig_disconnect);

    rtsp->ev_timeout.data = rtsp;
    ev_init(&rtsp->ev_timeout, client_ev_timeout);
    rtsp->ev_timeout.repeat = STREAM_TIMEOUT;
    ev_timer_again (srv->loop, &rtsp->ev_timeout);
}
