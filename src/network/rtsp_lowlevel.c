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

#include <strings.h>
#include <stdbool.h>

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "feng_utils.h"

typedef struct {
    Sock *rtp;
    Sock *rtcp;
    ev_io rtcp_reader;
} RTP_UDP_Transport;

static gboolean rtp_udp_send_pkt(Sock *sock, GByteArray *buffer)
{
    fd_set wset;
    struct timeval t;
    int written = -1;

    /*---------------SEE eventloop/rtsp_server.c-------*/
    FD_ZERO(&wset);
    t.tv_sec = 0;
    t.tv_usec = 1000;

    FD_SET(sock->fd, &wset);
    if (select(sock->fd + 1, 0, &wset, 0, &t) < 0) {
        fnc_log(FNC_LOG_ERR, "select error: %s\n", strerror(errno));
        goto end;
    }

    if (FD_ISSET(sock->fd, &wset)) {
        written = sendto(sock->fd, buffer->data, buffer->len,
                         MSG_EOR | MSG_DONTWAIT,
                         (struct sockaddr*)&sock->remote_stg,
                         sizeof(struct sockaddr_storage));

        if (written >= 0 ) {
            stats_account_sent(rtp->client, written);
        }
    }

 end:
    g_byte_array_free(buffer, true);

    return written >= 0;
}

static gboolean rtp_udp_send_rtp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_UDP_Transport *transport = rtp->transport_data;
    gboolean res = true;

    if ( (res = rtp_udp_send_pkt(transport->rtp, buffer)) ) {
        fnc_log(FNC_LOG_VERBOSE, "OUT RTP\n");
    } else {
        fnc_log(FNC_LOG_VERBOSE, "RTP Packet Lost\n");
    }

    return res;
}

static gboolean rtp_udp_send_rtcp(RTP_session *rtcp, GByteArray *buffer)
{
    RTP_UDP_Transport *transport = rtcp->transport_data;
    gboolean res = true;

    if ( (res = rtp_udp_send_pkt(transport->rtcp, buffer)) ) {
        fnc_log(FNC_LOG_VERBOSE, "OUT RTCP\n");
    } else {
        fnc_log(FNC_LOG_VERBOSE, "RTCP Packet Lost\n");
    }

    return res;
}

static void rtp_udp_close_transport(RTP_session *rtp)
{
    RTP_UDP_Transport *transport = rtp->transport_data;

    ev_io_stop(rtp->srv->loop, &transport->rtcp_reader);

    neb_sock_close(transport->rtp);
    neb_sock_close(transport->rtcp);

    g_slice_free(RTP_UDP_Transport, transport);
}

/**
 * @brief Read incoming RTCP packets from the socket
 */
static void rtcp_udp_read_cb(ATTR_UNUSED struct ev_loop *loop,
                             ev_io *w,
                             ATTR_UNUSED int revents)
{
    uint8_t buffer[RTP_DEFAULT_MTU*2] = { 0, }; //FIXME just a quick hack...
    RTP_session *rtp = w->data;
    RTP_UDP_Transport *transport = rtp->transport_data;

    int n = neb_sock_read(transport->rtcp,
                          buffer, RTP_DEFAULT_MTU*2,
                          MSG_DONTWAIT);

    if (n>0)
        rtcp_handle(rtp, buffer, n);
}

/**
 * @brief Setup unicast UDP transport sockets for an RTP session
 */
void rtp_udp_transport(RTSP_Client *rtsp,
                              RTP_session *rtp_s,
                              struct ParsedTransport *parsed)
{
    char port_buffer[8];
    RTP_UDP_Transport transport;
    ev_io *io = &transport.rtcp_reader;

    /* The client will not provide ports for us, obviously, let's
     * just ask the kernel for one, and try it to use for RTP/RTCP
     * as needed, if it fails, just get another random one.
     *
     * RFC 3550 Section 11 describe the choice of port numbers for RTP
     * applications; since we're delievering RTP as part of an RTSP
     * stream, we fall in the latest case described. We thus *can*
     * avoid using the even-odd adjacent ports pair for RTP-RTCP.
     */
    Sock *firstsock = neb_sock_bind(rtsp->sock->local_host, NULL, NULL, UDP);
    if ( firstsock == NULL )
        return;

    switch ( firstsock->local_port % 2 ) {
    case 0:
        transport.rtp = firstsock;
        snprintf(port_buffer, 8, "%d", firstsock->local_port+1);
        transport.rtcp = neb_sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP);
        if ( transport.rtcp == NULL )
            transport.rtcp = neb_sock_bind(rtsp->sock->local_host, NULL, NULL, UDP);
        break;
    case 1:
        transport.rtcp = firstsock;
        snprintf(port_buffer, 8, "%d", firstsock->local_port-1);
        transport.rtp = neb_sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP);
        if ( transport.rtp == NULL )
            transport.rtp = neb_sock_bind(rtsp->sock->local_host, NULL, NULL, UDP);
        break;
    }

    if ( transport.rtp == NULL ||
         transport.rtcp == NULL ||
         ( snprintf(port_buffer, 8, "%d", parsed->rtp_channel) != 0 &&
           neb_sock_connect (rtsp->sock->remote_host, port_buffer,
                             transport.rtp, UDP) == NULL ) ||
         ( snprintf(port_buffer, 8, "%d", parsed->rtcp_channel) != 0 &&
           neb_sock_connect (rtsp->sock->remote_host, port_buffer,
                             transport.rtcp, UDP) == NULL )
         ) {
        neb_sock_close(transport.rtp);
        neb_sock_close(transport.rtcp);
        return;
    }

    io->data = rtp_s;
    ev_io_init(io, rtcp_udp_read_cb,
               transport.rtcp->fd, EV_READ);

    rtp_s->transport_data = g_slice_dup(RTP_UDP_Transport, &transport);
    rtp_s->send_rtp = rtp_udp_send_rtp;
    rtp_s->send_rtcp = rtp_udp_send_rtcp;
    rtp_s->close_transport = rtp_udp_close_transport;

    rtp_s->transport_string = g_strdup_printf("RTP/AVP;unicast;source=%s;client_port=%d-%d;server_port=%d-%d;ssrc=%08X",
                                              rtsp->sock->local_host,
                                              transport.rtp->remote_port,
                                              transport.rtcp->remote_port,
                                              transport.rtp->local_port,
                                              transport.rtcp->local_port,
                                              rtp_s->ssrc);
}

/**
 * @brief Queue data for write in the client's output queue
 *
 * @param client The client to write the data to
 * @param data The GByteArray object to queue for sending
 *
 * @note after calling this function, the @p data object should no
 * longer be referenced by the code path.
 */
void rtsp_write_data_queue(RTSP_Client *client, GByteArray *data)
{
    g_queue_push_head(client->out_queue, data);
    ev_io_start(client->srv->loop, &client->ev_io_write);
}

void rtsp_tcp_read_cb(struct ev_loop *loop, ev_io *w,
                      ATTR_UNUSED int revents)
{
    guint8 buffer[RTSP_BUFFERSIZE + 1] = { 0, };    /* +1 to control the final '\0' */
    int read_size;
    RTSP_Client *rtsp = w->data;

    if ( (read_size = neb_sock_read(rtsp->sock,
                                    buffer,
                                    sizeof(buffer) - 1,
                                    0)
          ) <= 0 )
        goto client_close;

    stats_account_read(rtsp, read_size);

    if (rtsp->input->len + read_size > RTSP_BUFFERSIZE) {
        fnc_log(FNC_LOG_DEBUG,
                "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
        goto server_close;
    }

    g_byte_array_append(rtsp->input, (guint8*)buffer, read_size);

    RTSP_handler(rtsp);

    return;

 client_close:
    fnc_log(FNC_LOG_INFO, "RTSP connection closed by client.");
    goto disconnect;

 server_close:
    fnc_log(FNC_LOG_INFO, "RTSP connection closed by server.");
    goto disconnect;

 disconnect:
    ev_async_send(loop, &rtsp->ev_sig_disconnect);
}

void rtsp_tcp_write_cb(ATTR_UNUSED struct ev_loop *loop, ev_io *w,
                       ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    GByteArray *outpkt = (GByteArray *)g_queue_pop_tail(rtsp->out_queue);
    size_t written = 0;

    if (outpkt == NULL) {
        ev_io_stop(rtsp->srv->loop, &rtsp->ev_io_write);
        return;
    }
    written = send(rtsp->sock->fd, outpkt->data, outpkt->len, MSG_DONTWAIT);
    if ( written < outpkt->len ) {
        fnc_perror("");
        /* verify if this ever happens, as it is it'll still be popped
           from the queue */
        if ( errno == EAGAIN )
            return;
    } else {
        stats_account_sent(rtsp, written);
    }

    g_byte_array_free(outpkt, TRUE);
}
