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

#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

#include <ev.h>

#include "feng.h"
#include "network/rtsp.h"
#include "network/rtp.h"
#include "network/netembryo.h"
#include "fnc_log.h"
#include "media/demuxer.h"

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

    ev_io_stop(feng_srv.loop, &rtsp->ev_io_read);
    ev_io_stop(feng_srv.loop, &rtsp->ev_io_write);
    ev_async_stop(feng_srv.loop, &rtsp->ev_sig_disconnect);
    ev_timer_stop(feng_srv.loop, &rtsp->ev_timeout);

    close(rtsp->sd);
    g_free(rtsp->remote_host);
    feng_srv.connection_count--;

    rtsp_session_free(rtsp->session);

    if ( rtsp->channels )
        g_hash_table_destroy(rtsp->channels);

    /* Remove the output queue */
    if ( rtsp->out_queue ) {
        while( (outbuf = g_queue_pop_tail(rtsp->out_queue)) )
            g_string_free(outbuf, TRUE);

        g_queue_free(rtsp->out_queue);
    }

    g_byte_array_free(rtsp->input, true);

    feng_srv.clients = g_slist_remove(feng_srv.clients, rtsp);

    g_slice_free(RFC822_Request, rtsp->pending_request);

    g_slice_free1(rtsp->peer_len, rtsp->peer_sa);

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
    if ((session->track->properties.media_source == LIVE_SOURCE) &&
        (now - session->last_packet_send_time) >= LIVE_STREAM_BYE_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Soft stream timeout");
        rtcp_send_sr(session, BYE);
    }

    /* If we were not able to serve any packet and the client ignored our BYE
     * kick it by closing everything
     */
    if ((now - session->last_packet_send_time) >= STREAM_TIMEOUT) {
        fnc_log(FNC_LOG_INFO, "[client] Stream Timeout, client kicked off!");
        ev_async_send(feng_srv.loop, &session->client->ev_sig_disconnect);
    }
}

static void client_ev_timeout(struct ev_loop *loop, ev_timer *w,
                              ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    if(rtsp->session && rtsp->session->rtp_sessions)
        g_slist_foreach(rtsp->session->rtp_sessions,
                        check_if_any_rtp_session_timedout, NULL);
    ev_timer_again (loop, w);
}

RTSP_Client *rtsp_client_new()
{
    RTSP_Client *rtsp = g_slice_new0(RTSP_Client);

    rtsp->input = g_byte_array_new();

    feng_srv.clients = g_slist_append(feng_srv.clients, rtsp);

    return rtsp;
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
    Feng_Listener *listen = w->data;
    int client_sd = -1;
    struct sockaddr_storage sa;
    socklen_t sa_len = sizeof(struct sockaddr_storage);

    ev_io *io;
    ev_async *async;
    ev_timer *timer;
    RTSP_Client *rtsp;

    if ( (client_sd = accept(listen->fd, (struct sockaddr*)&sa, &sa_len)) < 0 ) {
        fnc_perror("accept failed");
        return;
    }

// Paranoid safeguard
    if (feng_srv.connection_count >= ONE_FORK_MAX_CONNECTION*2) {
        close(client_sd);
        return;
    }

    fnc_log(FNC_LOG_INFO, "Incoming connection accepted on socket: %d",
            client_sd);

    rtsp = rtsp_client_new();
    rtsp->sd = client_sd;

    if ( ! listen->specific->is_sctp ) {
        rtsp->socktype = RTSP_TCP;
        rtsp->out_queue = g_queue_new();
        rtsp->write_data = rtsp_write_data_queue;

        /* to be started/stopped when necessary */
        io = &rtsp->ev_io_write;
        io->data = rtsp;
        ev_io_init(io, rtsp_tcp_write_cb, rtsp->sd, EV_WRITE);

        io = &rtsp->ev_io_read;
        io->data = rtsp;
        ev_io_init(io, rtsp_tcp_read_cb, rtsp->sd, EV_READ);
    } else {
#ifdef ENABLE_SCTP
        rtsp->socktype = RTSP_SCTP;
        rtsp->write_data = rtsp_sctp_send_rtsp;

        io = &rtsp->ev_io_read;
        io->data = rtsp;
        ev_io_init(io, rtsp_sctp_read_cb, rtsp->sd, EV_READ);
#else
        g_assert_not_reached();
#endif
    }

    rtsp->local_sock = listen;

    rtsp->remote_host = neb_sa_get_host((struct sockaddr*)&sa);

    rtsp->peer_len = sa_len;
    rtsp->peer_sa = g_slice_copy(rtsp->peer_len, &sa);

    feng_srv.connection_count++;

    ev_io_start(feng_srv.loop, &rtsp->ev_io_read);

    async = &rtsp->ev_sig_disconnect;
    async->data = rtsp;
    ev_async_init(async, client_ev_disconnect_handler);
    ev_async_start(feng_srv.loop, async);

    timer = &rtsp->ev_timeout;
    timer->data = rtsp;
    ev_init(timer, client_ev_timeout);
    timer->repeat = STREAM_TIMEOUT;

    fnc_log(FNC_LOG_INFO, "Connection reached: %d", feng_srv.connection_count);
}

/**
 * @brief Write a GString to the RTSP socket of the client
 *
 * @param client The client to write the data to
 * @param string The data to send out as string
 *
 * @note after calling this function, the @p string object should no
 * longer be referenced by the code path.
 */
void rtsp_write_string(RTSP_Client *client, GString *string)
{
    /* Copy the GString into a GByteArray; we can avoid copying the
       data since both are transparent structures with a g_malloc'd
       data pointer.
     */
    GByteArray *outpkt = g_byte_array_new();
    outpkt->data = (guint8*)string->str;
    outpkt->len = string->len;

    /* make sure you don't free the actual data pointer! */
    g_string_free(string, false);

    client->write_data(client, outpkt);
}
