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
 * @brief List of clients connected to the server
 *
 * Access to this list is limited to @ref rtsp_client.c, which
 * provides a couple of wrapper functions for common situations.
 */
static GSList *clients_list;

/**
 * @brief Lock for accessing the clients' list
 *
 * Adding, removing or iterating over the list of clients require that
 * their operation is asynchronous; this means that the operation
 * needs to be properly locked.
 *
 * Even though this seem to call for a R/W mutex rather than a simple
 * lock, the overhead is high enough that it makes no sense to
 * actually use one.
 */
static GMutex *clients_list_lock;

/**
 * @brief Threadpool to manage per-client threads
 */
static GThreadPool *client_threads;

/**
 * @brief Per-client-thread error status for libev
 *
 * Since libev does not support checking for error conditions, we've
 * got to work it around by providing our own "error flag" to turn on
 * if something goes wrong in the libev initialisation.
 *
 * Something will go wrong if you try to accept more connections than
 * your file descriptors count will allow you to.
 */
static __thread int client_ev_init_errors;

static void libev_syserr(const char *msg)
{
    fnc_perror(msg);
    client_ev_init_errors = 1;
}

static void client_loop(gpointer client_p,
                        gpointer user_data);

/**
 * @brief Initialise the clients-handling code
 */
void clients_init()
{
    clients_list_lock = g_mutex_new();
    client_threads = g_thread_pool_new(client_loop, NULL,
                                       -1, /** @TODO link it to the max number of clients */
                                       false, NULL);

    ev_set_syserr_cb(libev_syserr);
}

/**
 * @brief Send the disconnection signal to a single client
 *
 * @param element The RTSP_Client object from @ref feng::clients
 * @brief user_data unused
 */
static void client_disconnect(gpointer element,
                              ATTR_UNUSED gpointer user_data)
{
    RTSP_Client *client = (RTSP_Client*)element;

    ev_async_send(client->loop, &client->ev_sig_disconnect);
}

/**
 * @brief Disconnect and cleanup clients
 *
 * This function is not under CLEANUP_DESTRUCTOR conditional and is
 * actually called during shutdown to ensure that all the clients are
 * sent disconnections, rather than dropping connections and waiting
 * for timeout.
 */
void clients_cleanup()
{
    clients_each(client_disconnect, NULL);

#ifdef CLEANUP_DESTRUCTOR
    g_slist_free(clients_list);
#endif
}

/**
 * @brief Execute a function for each of the clients
 *
 * @param func The function to execute
 * @param user_data The value to pass as second parameter to each
 *                  call.
 *
 * This is a simple wrapper around g_slist_foreach.
 *
 * @note This function will lock the @ref clients_list_lock mutex.
 */
void clients_each(GFunc func, gpointer user_data)
{
    g_mutex_lock(clients_list_lock);
    g_slist_foreach(clients_list, func, user_data);
    g_mutex_unlock(clients_list_lock);
}

/**
 * @brief Handle client disconnection and free resources
 *
 * @param loop The event loop where the event was issued
 * @param w The async event object (Unused)
 * @param revents Unused
 *
 * This event is triggered when disconnecting clients at shutdown, and
 * simply stops the loop from proceeding.
 */
static void client_ev_disconnect_handler(struct ev_loop *loop,
                                         ATTR_UNUSED ev_async *w,
                                         ATTR_UNUSED int revents)
{
    fnc_log(FNC_LOG_INFO, "[client] Client disconnected");

    ev_unloop(loop, EVUNLOOP_ONE);
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
        ev_unloop(session->client->loop, EVUNLOOP_ONE);
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

/**
 * @brief Threadpool callback for each client
 *
 * @brief client_p The client to execute the loop for
 * @brief user_data unused
 *
 * @note This function will lock the @ref clients_list_lock mutex.
 */
static void client_loop(gpointer client_p,
                        ATTR_UNUSED gpointer user_data)
{
    RTSP_Client *client = (RTSP_Client *)client_p;
    GString *outbuf = NULL;

    struct ev_loop *loop = client->loop;
    ev_io *io;
    ev_async *async;
    ev_timer *timer;

    switch(client->socktype) {
    case RTSP_TCP:
        /* to be started/stopped when necessary */
        io = &client->ev_io_write;
        io->data = client;
        ev_io_init(io, rtsp_tcp_write_cb, client->sd, EV_WRITE);

        io = &client->ev_io_read;
        io->data = client;
        ev_io_init(io, rtsp_tcp_read_cb, client->sd, EV_READ);
        break;
#if ENABLE_SCTP
    case RTSP_SCTP:
        io = &client->ev_io_read;
        io->data = client;
        ev_io_init(io, rtsp_sctp_read_cb, client->sd, EV_READ);
        break;
#endif
    }

    ev_io_start(loop, &client->ev_io_read);

    async = &client->ev_sig_disconnect;
    ev_async_init(async, client_ev_disconnect_handler);
    ev_async_start(loop, async);

    timer = &client->ev_timeout;
    timer->data = client;
    ev_init(timer, client_ev_timeout);
    timer->repeat = STREAM_TIMEOUT;

    /* if there were no errors during libev initialisation, proceed to
     * run the loop, otherwise, start cleaning up already. We could
     * try to send something to the clients to let them know that we
     * failed, but ... it's going to be difficult at this point. */
    if ( client_ev_init_errors == 0 ) {
        g_mutex_lock(clients_list_lock);
        clients_list = g_slist_append(clients_list, client);
        g_mutex_unlock(clients_list_lock);

        ev_loop(loop, 0);

        ev_io_stop(loop, &client->ev_io_read);
        ev_io_stop(loop, &client->ev_io_write);
        ev_async_stop(loop, &client->ev_sig_disconnect);
        ev_timer_stop(loop, &client->ev_timeout);

        /* As soon as we're out of here, remove the client from the list! */
        g_mutex_lock(clients_list_lock);
        clients_list = g_slist_remove(clients_list, client);
        g_mutex_unlock(clients_list_lock);
    }

    close(client->sd);
    g_free(client->remote_host);
    feng_srv.connection_count--;

    rtsp_session_free(client->session);

    if ( client->channels )
        g_hash_table_destroy(client->channels);

    /* Remove the output queue */
    if ( client->out_queue ) {
        while( (outbuf = g_queue_pop_tail(client->out_queue)) )
            g_string_free(outbuf, TRUE);

        g_queue_free(client->out_queue);
    }

    g_byte_array_free(client->input, true);

    g_slice_free(RFC822_Request, client->pending_request);

    g_slice_free1(client->peer_len, client->peer_sa);

    ev_loop_destroy(loop);

    g_slice_free(RTSP_Client, client);

    fnc_log(FNC_LOG_INFO, "[client] Client removed");
}

RTSP_Client *rtsp_client_new()
{
    RTSP_Client *rtsp = g_slice_new0(RTSP_Client);

    rtsp->input = g_byte_array_new();

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

    rtsp->loop = ev_loop_new(EVFLAG_AUTO);

#if ENABLE_SCTP
    if ( ! listen->specific->is_sctp ) {
#endif
        rtsp->socktype = RTSP_TCP;
        rtsp->out_queue = g_queue_new();
        rtsp->write_data = rtsp_write_data_queue;
#if ENABLE_SCTP
    } else {
        rtsp->socktype = RTSP_SCTP;
        rtsp->write_data = rtsp_sctp_send_rtsp;
    }
#endif

    rtsp->local_sock = listen;

    rtsp->remote_host = neb_sa_get_host((struct sockaddr*)&sa);

    rtsp->peer_len = sa_len;
    rtsp->peer_sa = g_slice_copy(rtsp->peer_len, &sa);

    feng_srv.connection_count++;

    g_thread_pool_push(client_threads, rtsp, NULL);

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
