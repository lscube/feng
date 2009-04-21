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

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"

#ifdef HAVE_SCTP
# include <netinet/sctp.h>
#endif

/**
 * @defgroup rtsp_interleaved Interleaved RTSP
 * @ingroup RTSP
 *
 * @brief Interface to deal with interleaved RTSP.
 *
 * @{
 */

/**
 * @brief Represents a single channel of an interleaved RTSP
 *        connection.
 */
typedef struct {
    /**
     * Local socket to forward the received content to.
     */
    Sock *local;

    /**
     * Channel number used for receiving
     */
    int channel;

    /**
     * libev handler for the read callback
     */
    ev_io ev_io_listen;
} RTSP_interleaved_channel;

/**
 * @brief Represent a full RTP connection inside an interleaved RTSP
 *        connection.
 */
typedef struct {
    RTSP_interleaved_channel rtp;
    RTSP_interleaved_channel rtcp;
} RTSP_interleaved;


/**
 * @file
 *
 * @brief Interleaved (TCP and SCTP) support
 */

static void interleaved_read_tcp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    GByteArray *pkt;
    uint16_t ne_n;
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved_channel *intlvd = w->data;
    RTSP_Client *rtsp = intlvd->local->data;
    int n;

    if ((n = Sock_read(intlvd->local, buffer,
                       RTSP_BUFFERSIZE, NULL, 0)) < 0) {
        fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
        return;
    }

    ne_n = htons((uint16_t)n);
    pkt = g_byte_array_sized_new(n+4);
    g_byte_array_set_size(pkt, n+4);

    pkt->data[0] = '$';
    pkt->data[1] = (uint8_t)intlvd->channel;
    memcpy(&pkt->data[2], &ne_n, sizeof(ne_n));
    memcpy(&pkt->data[4], buffer, n);

    g_queue_push_head(rtsp->out_queue, pkt);
    ev_io_start(rtsp->srv->loop, &rtsp->ev_io_write);
}

#ifdef HAVE_SCTP
static void interleaved_read_sctp_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[RTSP_BUFFERSIZE + 1];
    RTSP_interleaved_channel *intlvd = w->data;
    RTSP_Client *rtsp = intlvd->local->data;
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

static void interleaved_setup_callbacks(RTSP_Client *rtsp, RTSP_interleaved *intlvd)
{
    void (*cb)(EV_P_ struct ev_io *w, int revents);
    int i;

    switch (Sock_type(rtsp->sock)) {
        case TCP:
            cb = interleaved_read_tcp_cb;
        break;
#ifdef HAVE_SCTP
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

gboolean interleaved_setup_transport(RTSP_Client *rtsp, RTP_transport *transport,
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
 *           g_slist_find_custom() function by @ref
 *           interleaved_rtcp_send.
 */
static gboolean interleaved_rtcp_find_compare(gconstpointer value,
                                              gconstpointer target)
{
    RTSP_interleaved *i = (RTSP_interleaved *)value;
    gint m = GPOINTER_TO_INT(target);
    return (i->rtcp.channel == m);
}

/**
 * @brief Send an interleaved RTCP packet down to the RTCP handler
 *
 * @param rtsp The RTSP client where the handler can be found
 * @param channel The channel index where the package arrived
 * @param data The pointer to the data to send
 * @param len The length of the data to send
 *
 * This function is used to send data down to the actual RTCP handler
 * after it has been received from an interleaved transport (TCP or
 * SCTP alike).
 */
void interleaved_rtcp_send(RTSP_Client *rtsp, int channel,
                           void *data, size_t len)
{
    RTSP_interleaved *intlvd = NULL;
    GSList *intlvd_iter = g_slist_find_custom(rtsp->interleaved,
                                              GINT_TO_POINTER(channel),
                                              interleaved_rtcp_find_compare);

    /* We have to check if the value returned by g_slist_find_custom
     * is valid before derefencing it.
     */
    if ( intlvd_iter == NULL ) {
        fnc_log(FNC_LOG_DEBUG,
                "Interleaved RTCP packet arrived for unknown channel (%d)... discarding.\n",
                channel);
        return;
    }

    intlvd = (RTSP_interleaved *)intlvd_iter->data;

    /* We should be pretty sure this is not NULL */
    g_assert(intlvd != NULL);

    /* Write the actual data */
    Sock_write(intlvd->rtcp.local, data, len, NULL, MSG_DONTWAIT | MSG_EOR);
}

/**
 * @brief Free a RTSP_interleaved instance
 *
 * @param element Instance to destroy and free
 * @param user_data The loop to stop libev I/O
 *
 * @internal This function should only be called through
 *           g_slist_foreach() by @ref interleaved_free_list.
 *
 * @see interleaved_setup_transport
 */
static void interleaved_free(gpointer element, gpointer user_data)
{
    struct ev_loop *loop = (struct ev_loop *)user_data;
    RTSP_interleaved *intlvd = (RTSP_interleaved *)element;

    Sock_close(intlvd->rtp.local);
    ev_io_stop(loop, &intlvd->rtp.ev_io_listen);

    Sock_close(intlvd->rtcp.local);
    ev_io_stop(loop, &intlvd->rtcp.ev_io_listen);

    g_slice_free(RTSP_interleaved, element);
}

/**
 * @brief Destroy and free a list of RTSP_interleaved objects
 *
 * @param rtsp The client from which to free the list
 *
 * @see interleaved_free
 */
void interleaved_free_list(RTSP_Client *rtsp)
{
  g_slist_foreach(rtsp->interleaved, interleaved_free, rtsp->srv->loop);
  g_slist_free(rtsp->interleaved);
}

/**
 * @}
 */
