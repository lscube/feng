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

#include "network/rtsp.h"
#include "client_events.h"

#include <sys/time.h>
#include <ev.h>

static void client_events_deregister(RTSP_Client *rtsp)
{
    feng *srv = rtsp->srv;

    ev_io_stop(srv->loop, &rtsp->ev_io_read);
    ev_async_stop(srv->loop, rtsp->ev_sig_disconnect);
    ev_timer_stop(srv->loop, rtsp->ev_timeout);

    g_free(rtsp->ev_sig_disconnect);
    g_free(rtsp->ev_timeout);
}

static void client_ev_disconnect_handler(struct ev_loop *loop, ev_async *w, int revents)
{
    RTSP_Client *rtsp = w->data;
    feng *srv = rtsp->srv;

    //Prevent from requesting disconnection again
    client_events_deregister(rtsp);

    //Close connection
    Sock_close(rtsp->sock);
    --srv->conn_count;
    srv->num_conn--;

    // Release the RTSP_Client
    srv->clients = g_slist_remove(srv->clients, rtsp);
    rtsp_client_destroy(rtsp);
    fnc_log(FNC_LOG_INFO, "[client] Client removed");
}

static void check_if_any_rtp_session_timedout(gpointer element, gpointer user_data)
{
    RTP_session *session = (RTP_session *)element;
    time_t now = time(NULL);

    if (session->pause) return;

    /* Check if we didn't send any data for more then STREAM_BYE_TIMEOUT seconds
     * this will happen if we are not receiving any more from live producer or
     * if the stored stream ended.
     */
    if ((session->track->properties->media_source == MS_live) &&
        (now - session->last_packet_send_time) >= LIVE_STREAM_BYE_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Soft stream timeout");
        RTCP_send_bye(session);
    }

    /* If we were not able to serve any packet and the client ignored our BYE
     * kick it by closing everything
     */
    if ((now - session->last_packet_send_time) >= STREAM_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Stream Timeout, client kicked off!");
        ev_async_send(session->srv->loop, session->rtsp_buffer->ev_sig_disconnect);
    }
}

static void client_ev_timeout(struct ev_loop *loop, ev_timer *w, int revents)
{
    RTSP_Client *rtsp = w->data;
    g_slist_foreach(rtsp->session->rtp_sessions, check_if_any_rtp_session_timedout, NULL);
    ev_timer_again (loop, w);
}


void client_events_register(RTSP_Client *rtsp)
{
    feng *srv = rtsp->srv;
    ev_async *ev_as_client = g_new(ev_async, 1);
    ev_timer *ev_timeout = g_new(ev_timer, 1);

    ev_as_client->data = rtsp;
    ev_async_init(ev_as_client, client_ev_disconnect_handler);
    rtsp->ev_sig_disconnect = ev_as_client;
    ev_async_start(srv->loop, ev_as_client);

    ev_timeout->data = rtsp;
    ev_init(ev_timeout, client_ev_timeout);
    ev_timeout->repeat = STREAM_TIMEOUT;
    rtsp->ev_timeout = ev_timeout;
    ev_timer_again (srv->loop, ev_timeout);
}

