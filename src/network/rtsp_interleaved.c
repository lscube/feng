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

#include <stdbool.h>

#include "rtsp.h"
#include "fenice/fnc_log.h"

/**
 * @file
 *
 * @brief Interleaved (TCP and SCTP) support
 */

static void interleaved_read_tcp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    GString *str;
    uint16_t ne_n;
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved_channel *intlvd = w->data;
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
static void interleaved_read_sctp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved_channel *intlvd = w->data;
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
#endif

static void interleaved_setup_callbacks(RTSP_buffer *rtsp, RTSP_interleaved *intlvd)
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

    intlvd->rtp.local->data = rtsp;
    intlvd->rtp.ev_io_listen.data = &intlvd->rtp;
    ev_io_init(&intlvd->rtp.ev_io_listen, cb, Sock_fd(intlvd->rtp.local), EV_READ);
    ev_io_start(rtsp->srv->loop, &intlvd->rtp.ev_io_listen);

    intlvd->rtcp.local->data = rtsp;
    intlvd->rtcp.ev_io_listen.data = &intlvd->rtcp;
    ev_io_init(&intlvd->rtcp.ev_io_listen, cb, Sock_fd(intlvd->rtcp.local), EV_READ);
    ev_io_start(rtsp->srv->loop, &intlvd->rtcp.ev_io_listen);
}

gboolean interleaved_setup_transport(RTSP_buffer *rtsp, RTP_transport *transport,
                                     int rtp_ch, int rtcp_ch) {
    RTSP_interleaved *intlvd = NULL;
    Sock *sock_pair[2][2];

    // RTP local sockpair
    if ( Sock_socketpair(sock_pair[0]) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtp\n");
        return false;
    }

    // RTCP local sockpair
    if ( Sock_socketpair(sock_pair[1]) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtcp\n");
        Sock_close(sock_pair[0][0]);
        Sock_close(sock_pair[0][1]);
        return false;
    }

    intlvd = g_slice_new0(RTSP_interleaved);

    transport->rtp_sock = sock_pair[0][0];
    intlvd->rtp.local = sock_pair[0][1];

    transport->rtcp_sock = sock_pair[1][0];
    intlvd->rtcp.local = sock_pair[1][1];

    // copy stream number in rtp_transport struct
    transport->rtp_ch = intlvd->rtp.channel = rtp_ch;
    transport->rtcp_ch = intlvd->rtcp.channel = rtcp_ch;

    interleaved_setup_callbacks(rtsp, intlvd);

    rtsp->interleaved = g_slist_prepend(rtsp->interleaved, intlvd);

    return true;
}

/**
 * @brief Compare a given RTSP_interleaved object against the RTCP channel.
 *
 * @param value A pointer to a RTSP_interleaved instance
 * @param target The integer RTCP channel to look for
 *
 * @internal This should only be called through the
 *           g_slist_find_custom() function.
 */
gboolean interleaved_rtcp_find_compare(gconstpointer value, gconstpointer target)
{
  RTSP_interleaved *i = (RTSP_interleaved *)value;
  gint m = GPOINTER_TO_INT(target);

  return (i->rtcp.channel == m);
}
