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

/**
 * @file eventloop.c
 * Network main loop
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <ev.h>

#include "eventloop.h"
#include "network/rtsp.h"
#include <fenice/utils.h>

#include <fenice/schedule.h>

static GPtrArray *io_watchers; //!< keep track of ev_io allocated

static inline
int rtsp_sock_read(Sock *sock, int *stream, char *buffer, int size)
{
    int n;
#ifdef HAVE_LIBSCTP
    struct sctp_sndrcvinfo sctp_info;
    if (Sock_type(sock) == SCTP) {
        memset(&sctp_info, 0, sizeof(sctp_info));
        n = Sock_read(sock, buffer, size, &sctp_info, 0);
        *stream = sctp_info.sinfo_stream;
        fnc_log(FNC_LOG_DEBUG,
            "Sock_read() received %d bytes from sctp stream %d\n", n, stream);
    } else    // RTSP protocol is TCP
#endif    // HAVE_LIBSCTP

    n = Sock_read(sock, buffer, size, NULL, 0);

    return n;
}

static void
interleaved_read_tcp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    GString *str;
    uint16_t ne_n;
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved *intlvd = w->data;
    RTSP_buffer *rtsp = intlvd->local->data;
    int n;

    if ((n = Sock_read(intlvd->local, buffer,
                       RTSP_BUFFERSIZE, NULL, 0)) < 0) {
        fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
        return;
    }

    ne_n = htons((uint16_t)n);
    str = g_string_sized_new(n+4);

    g_string_append_c(str, '$');
    g_string_append_c(str, (unsigned char)intlvd->channel);
    g_string_append_len(str, (gchar *)&ne_n, 2);
    g_string_append_len(str, (const gchar *)buffer, n);

    g_async_queue_push(rtsp->out_queue, str);
    ev_io_start(rtsp->srv->loop, &rtsp->ev_io_write);
}

#ifdef HAVE_LIBSCTP
static void
interleaved_read_sctp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved *intlvd = w->data;
    RTSP_buffer *rtsp = intlvd->local->data;
    struct sctp_sndrcvinfo sctp_info;
    int n;

    if ((n = Sock_read(intlvd->local, buffer,
                       RTSP_BUFFERSIZE, NULL, 0)) < 0) {
        fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
        return;
    }

    sctp_info.sinfo_stream = intlvd->channel;
    Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR);
}

static gboolean
find_sctp_interleaved(gconstpointer value, gconstpointer target)
{
    RTSP_interleaved *i = (RTSP_interleaved *)value;
    gint m = GPOINTER_TO_INT(target);
    return (i[1].channel == m);
}

#endif

void eventloop_local_callbacks(RTSP_buffer *rtsp, RTSP_interleaved *intlvd)
{
    void (*cb)(EV_P_ struct ev_io *w, int revents);
    int i;

    switch (Sock_type(rtsp->sock)) {
        case TCP:
            cb = interleaved_read_tcp_cb;
        break;
#ifdef HAVE_LIBSCTP
        case SCTP:
            cb = interleaved_read_sctp_cb;
        break;
        default:
            // Shouldn't be possible
        return;
#endif
    }

    /* Let's hope this is unrolled by GCC, shall we? */
    for (i = 0; i < 2; i++) {
        Sock *sock = intlvd[i].local;
        sock->data = rtsp;
        rtsp->ev_io_listen[i].data = intlvd + i;
        ev_io_init(&rtsp->ev_io_listen[i], cb, Sock_fd(sock), EV_READ);
        ev_io_start(rtsp->srv->loop, &rtsp->ev_io_listen[i]);
    }
}

static void rtsp_write_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    RTSP_buffer *rtsp = w->data;
    rtsp_send(rtsp);
}

static void rtsp_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[RTSP_BUFFERSIZE + 1];    /* +1 to control the final '\0' */
    int n;
    gint m = 0;
    RTSP_buffer *rtsp = w->data;

    memset(buffer, 0, sizeof(buffer));
    n = rtsp_sock_read(rtsp->sock, &m, buffer, sizeof(buffer) - 1);

    if (n > 0) {
        if (m == 0) {
            if (rtsp->in_size + n > RTSP_BUFFERSIZE) {
                fnc_log(FNC_LOG_DEBUG,
                    "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
                n = -1;
            }
            memcpy(&(rtsp->in_buffer[rtsp->in_size]), buffer, n);
            rtsp->in_size += n;
            if (RTSP_handler(rtsp) == ERR_GENERIC) {
                fnc_log(FNC_LOG_ERR, "Invalid input message.\n");
                n = -1;
            }
        } else {    /* if (rtsp->proto == SCTP && m != 0) */
#ifdef HAVE_LIBSCTP
        RTSP_interleaved *intlvd =
            g_slist_find_custom(rtsp->interleaved, GINT_TO_POINTER(m),
                                find_sctp_interleaved)->data;
        if (intlvd) {
            Sock_write(intlvd->local, buffer, n, NULL, 0);
        } else {
            fnc_log(FNC_LOG_DEBUG, "Unknown rtcp stream %d ", m);
        }
#endif    // HAVE_LIBSCTP
        }
    }

    if (n > 0)
        return;
    else if (n == 0)
        fnc_log(FNC_LOG_INFO, "RTSP connection closed by client.");
    else if (n < 0)
        fnc_log(FNC_LOG_INFO, "RTSP connection closed by server.");

    ev_async_send(loop, rtsp->ev_sig_disconnect);
}

static void add_client(feng *srv, Sock *client_sock)
{
    RTSP_buffer *rtsp = rtsp_client_new(srv, client_sock);

    srv->clients = g_slist_prepend(srv->clients, rtsp);
    client_sock->data = srv;

    rtsp->ev_io_read.data = rtsp;
    g_ptr_array_add(io_watchers, &rtsp->ev_io_read);
    ev_io_init(&rtsp->ev_io_read, rtsp_read_cb, Sock_fd(client_sock), EV_READ);
    ev_io_start(srv->loop, &rtsp->ev_io_read);

    /* to be started/stopped when necessary */
    rtsp->ev_io_write.data = rtsp;
    ev_io_init(&rtsp->ev_io_write, rtsp_write_cb, Sock_fd(client_sock), EV_WRITE);
    fnc_log(FNC_LOG_INFO, "Incoming RTSP connection accepted on socket: %d\n",
            Sock_fd(client_sock));

    client_events_register(rtsp);
}

/**
 * @brief Search function for connections in the clients list
 * @param value Current entry
 * @param compare Socket to find in the list
 */
static gboolean
connections_compare_socket(gconstpointer value, gconstpointer compare)
{
    RTSP_buffer *p = (RTSP_buffer*)value;
    return !!Sock_compare((Sock *)compare, p->sock);
}

/**
 * Accepts the new connection if possible.
 */

static void
incoming_connection_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    Sock *sock = w->data;
    feng *srv = sock->data;
    Sock *client_sock = NULL;
    GSList *p = NULL;

    client_sock = Sock_accept(sock, NULL);

    // Handle a new connection
    if (!client_sock)
        return;

    p = g_slist_find_custom(srv->clients, client_sock,
                            connections_compare_socket);

    if ( p == NULL ) {
        if (srv->conn_count < ONE_FORK_MAX_CONNECTION) {
            ++srv->conn_count;
            add_client(srv, client_sock);
        } else {
            Sock_close(client_sock);
        }
        srv->num_conn++;
        fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->num_conn);

        return;
    }

    Sock_close(client_sock);

    fnc_log(FNC_LOG_INFO, "Connection found: %d\n", Sock_fd(client_sock));
}

/**
 * Bind to the defined listening port
 */

int feng_bind_port(feng *srv, char *host, char *port, specific_config *s)
{
    int is_sctp = s->is_sctp;
    Sock *sock;
    ev_io *ev_io_listen = g_new(ev_io, 1);

    if (is_sctp)
        sock = Sock_bind(host, port, NULL, SCTP, NULL);
    else
        sock = Sock_bind(host, port, NULL, TCP, NULL);
    if(!sock) {
        fnc_log(FNC_LOG_ERR,"Sock_bind() error for port %s.", port);
        fprintf(stderr,
                "[fatal] Sock_bind() error in main() for port %s.\n",
                port);
        return 1;
    }

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    if(Sock_listen(sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }
    sock->data = srv;
    ev_io_listen->data = sock;
    g_ptr_array_add(io_watchers, ev_io_listen);
    ev_io_init(ev_io_listen, incoming_connection_cb,
               Sock_fd(sock), EV_READ);
    ev_io_start(srv->loop, ev_io_listen);
    return 0;
}

/**
 * @brief Initialise data structure needed for eventloop
 */
void eventloop_init(feng *srv)
{
    io_watchers = g_ptr_array_new();
    srv->loop = ev_default_loop(0);
    srv->clients = NULL;
}

/**
 * Main loop waiting for clients
 * @param srv server instance variable.
 */

void eventloop(feng *srv)
{
    ev_loop (srv->loop, 0);
}

/**
 * @brief Cleanup data structure needed by the eventloop
 */
void eventloop_cleanup(feng *srv) {
    g_ptr_array_free(io_watchers, TRUE);
}
