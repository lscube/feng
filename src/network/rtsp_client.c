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
#include "fnc_log.h"
#include "media/demuxer.h"

#include <sys/time.h>
#include <stdbool.h>
#include <ev.h>

#define LIVE_STREAM_BYE_TIMEOUT 6
#define STREAM_TIMEOUT 12 /* This one must be big enough to permit to VLC to switch to another
                             transmission protocol and must be a multiple of LIVE_STREAM_BYE_TIMEOUT */

/**
 * @brief Handle client disconnection and free resources
 *
 * @param loop The event loop where the event was issued
 * @param w The async event object
 * @param revents Unused
 *
 * This event is triggered when a client disconnects or is forcefully
 * disconnected. It stops the other events from running, and frees all
 * the remaining resources for the client itself.
 */
static void client_ev_disconnect_handler(ATTR_UNUSED struct ev_loop *loop,
                                         ev_async *w,
                                         ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = (RTSP_Client*)w->data;
    GString *outbuf = NULL;
    feng *srv = rtsp->srv;

    ev_io_stop(srv->loop, &rtsp->ev_io_read);
    ev_io_stop(srv->loop, &rtsp->ev_io_write);
    ev_async_stop(srv->loop, &rtsp->ev_sig_disconnect);
    ev_timer_stop(srv->loop, &rtsp->ev_timeout);

    Sock_close(rtsp->sock);
    srv->connection_count--;

    rtsp_session_free(rtsp->session);

    interleaved_free_list(rtsp);

    /* Remove the output queue */
    while( (outbuf = g_queue_pop_tail(rtsp->out_queue)) )
        g_string_free(outbuf, TRUE);

    g_queue_free(rtsp->out_queue);

    g_byte_array_free(rtsp->input, true);

    g_slice_free(RTSP_Client, rtsp);

    fnc_log(FNC_LOG_INFO, "[client] Client removed");
}

static void check_if_any_rtp_session_timedout(gpointer element,
                                              ATTR_UNUSED gpointer user_data)
{
    RTP_session *session = (RTP_session *)element;
    time_t now = time(NULL);

    /* Check if we didn't send any data for more then STREAM_BYE_TIMEOUT seconds
     * this will happen if we are not receiving any more from live producer or
     * if the stored stream ended.
     */
    if ((session->track->properties.media_source == MS_live) &&
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

static void client_ev_timeout(struct ev_loop *loop, ev_timer *w,
                              ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    if(rtsp->session->rtp_sessions)
        g_slist_foreach(rtsp->session->rtp_sessions,
                        check_if_any_rtp_session_timedout, NULL);
    ev_timer_again (loop, w);
}

/**
 * @brief Handle an incoming RTSP connection
 *
 * @param loop The event loop where the incoming connection was triggered
 * @param w The watcher that triggered the incoming connection
 * @param revents Unused
 *
 * This function takes care of all the handling of an incoming RTSP
 * client connection:
 *
 * @li accept the new socket;
 *
 * @li checks that there is space for new connections for the current
 *     fork;
 *
 * @li creates and sets up the @ref RTSP_Client object.
 *
 * The newly created instance is deleted by @ref
 * client_ev_disconnect_handler.
 *
 * @internal This function should be used as callback for an ev_io
 *           listener.
 */
void rtsp_client_incoming_cb(ATTR_UNUSED struct ev_loop *loop, ev_io *w,
                             ATTR_UNUSED int revents)
{
    Sock *sock = w->data;
    feng *srv = sock->data;
    Sock *client_sock = NULL;
    RTSP_Client *rtsp;

    if ( (client_sock = Sock_accept(sock, NULL)) == NULL )
        return;

    if (srv->connection_count >= ONE_FORK_MAX_CONNECTION) {
        Sock_close(client_sock);
        return;
    }

    rtsp = g_slice_new0(RTSP_Client);
    rtsp->sock = client_sock;
    rtsp->input = g_byte_array_new();
    rtsp->out_queue = g_queue_new();
    rtsp->srv = srv;

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

    fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->connection_count);
}
