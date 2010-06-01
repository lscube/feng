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
    /** RTP socket descriptor */
    int rtp_sd;
    /** RTCP socket descriptor */
    int rtcp_sd;
    /** RTP remote socket address */
    struct sockaddr_storage rtp_sa;
    /** RTCP remote socket address */
    struct sockaddr_storage rtcp_sa;
    ev_io rtcp_reader;
} RTP_UDP_Transport;

static gboolean rtp_udp_send_pkt(int sd, struct sockaddr *sa, GByteArray *buffer, RTSP_Client *rtsp)
{
    fd_set wset;
    struct timeval t;
    int written = -1;

    /*---------------SEE eventloop/rtsp_server.c-------*/
    FD_ZERO(&wset);
    t.tv_sec = 0;
    t.tv_usec = 1000;

    FD_SET(sd, &wset);
    if (select(sd + 1, 0, &wset, 0, &t) < 0) {
        fnc_log(FNC_LOG_ERR, "select error: %s\n", strerror(errno));
        goto end;
    }

    if (FD_ISSET(sd, &wset)) {
        written = sendto(sd, buffer->data, buffer->len,
                         MSG_EOR | MSG_DONTWAIT,
                         sa, sizeof(struct sockaddr_storage));

        if (written >= 0 ) {
            stats_account_sent(rtsp, written);
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

    if ( (res = rtp_udp_send_pkt(transport->rtp_sd, (struct sockaddr *)&transport->rtp_sa,
                                 buffer, rtp->client)) ) {
        fnc_log(FNC_LOG_VERBOSE, "OUT RTP\n");
    } else {
        fnc_log(FNC_LOG_VERBOSE, "RTP Packet Lost\n");
    }

    return res;
}

static gboolean rtp_udp_send_rtcp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_UDP_Transport *transport = rtp->transport_data;
    gboolean res = true;

    if ( (res = rtp_udp_send_pkt(transport->rtcp_sd, (struct sockaddr *)&transport->rtcp_sa,
                                 buffer, rtp->client)) ) {
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

    close(transport->rtp_sd);
    close(transport->rtcp_sd);

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

    int n = recv(transport->rtcp_sd,
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
    RTP_UDP_Transport transport = {
        .rtp_sd = -1,
        .rtcp_sd = -1
    };
    ev_io *io = &transport.rtcp_reader;

    struct sockaddr_storage sa_temp;
    struct sockaddr *sa_p = (struct sockaddr *)&sa_temp;
    socklen_t sa_len;
    int firstsd;
    in_port_t firstport, rtp_port, rtcp_port;

    memcpy(sa_p, &rtsp->sock->local_stg, sizeof(struct sockaddr_storage));

    /* The client will not provide ports for us, obviously, let's
     * just ask the kernel for one, and try it to use for RTP/RTCP
     * as needed, if it fails, just get another random one.
     *
     * RFC 3550 Section 11 describe the choice of port numbers for RTP
     * applications; since we're delievering RTP as part of an RTSP
     * stream, we fall in the latest case described. We thus *can*
     * avoid using the even-odd adjacent ports pair for RTP-RTCP.
     */
    neb_sa_set_port(sa_p, 0);

    if ( (firstsd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 )
        goto error;

    if ( bind(firstsd, sa_p, sizeof(struct sockaddr_storage)) < 0 )
        goto error;

    if ( getsockname(firstsd, sa_p, &sa_len) < 0 )
        goto error;

    firstport = neb_sa_get_port(sa_p);

    switch ( firstport % 2 ) {
    case 0:
        transport.rtp_sd = firstsd; firstsd = -1;
        rtp_port = firstport; rtcp_port = firstport+1;
        if ( (transport.rtcp_sd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 )
            goto error;

        neb_sa_set_port(sa_p, rtcp_port);

        if ( bind(transport.rtcp_sd, sa_p, sizeof(struct sockaddr_storage)) < 0 ) {
            neb_sa_set_port(sa_p, 0);
            if ( bind(transport.rtcp_sd, sa_p, sizeof(struct sockaddr_storage)) < 0 )
                goto error;

            if ( getsockname(transport.rtcp_sd, sa_p, &sa_len) < 0 )
                goto error;

            rtcp_port = neb_sa_get_port(sa_p);
        }

        break;
    case 1:
        transport.rtcp_sd = firstsd; firstsd = -1;
        rtcp_port = firstport; rtp_port = firstport-1;
        if ( (transport.rtp_sd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 )
            goto error;

        neb_sa_set_port(sa_p, rtp_port);

        if ( bind(transport.rtp_sd, sa_p, sizeof(struct sockaddr_storage)) < 0 ) {
            neb_sa_set_port(sa_p, 0);
            if ( bind(transport.rtp_sd, sa_p, sizeof(struct sockaddr_storage)) < 0 )
                goto error;

            if ( getsockname(transport.rtp_sd, sa_p, &sa_len) < 0 )
                goto error;

            rtp_port = neb_sa_get_port(sa_p);
        }

        break;
    }

    memcpy(&transport.rtp_sa, &rtsp->sock->remote_stg, sizeof(struct sockaddr_storage));
    neb_sa_set_port((struct sockaddr *)(&transport.rtp_sa), parsed->rtp_channel);

    if ( connect(transport.rtp_sd,
                 (struct sockaddr *)(&transport.rtp_sa),
                 sizeof(struct sockaddr_storage)) < 0 )
        goto error;

    memcpy(&transport.rtcp_sa, &rtsp->sock->remote_stg, sizeof(struct sockaddr_storage));
    neb_sa_set_port((struct sockaddr *)(&transport.rtcp_sa), parsed->rtcp_channel);

    if ( connect(transport.rtcp_sd,
                 (struct sockaddr *)(&transport.rtcp_sa),
                 sizeof(struct sockaddr_storage)) < 0 )
        goto error;

    io->data = rtp_s;
    ev_io_init(io, rtcp_udp_read_cb,
               transport.rtcp_sd, EV_READ);

    rtp_s->transport_data = g_slice_dup(RTP_UDP_Transport, &transport);
    rtp_s->send_rtp = rtp_udp_send_rtp;
    rtp_s->send_rtcp = rtp_udp_send_rtcp;
    rtp_s->close_transport = rtp_udp_close_transport;

    rtp_s->transport_string = g_strdup_printf("RTP/AVP;unicast;source=%s;client_port=%d-%d;server_port=%d-%d;ssrc=%08X",
                                              rtsp->sock->local_host,
                                              parsed->rtp_channel,
                                              parsed->rtcp_channel,
                                              rtp_port,
                                              rtcp_port,
                                              rtp_s->ssrc);

    return;

 error:
    fnc_perror("trying RTP transport");
    if ( firstsd >= 0 )
        close(firstsd);
    if ( transport.rtp_sd >= 0 )
        close(transport.rtp_sd);
    if ( transport.rtcp_sd >= 0 )
        close(transport.rtcp_sd);
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

    if ( (read_size = recv(rtsp->sock->fd,
                           buffer,
                           sizeof(buffer),
                           0) ) <= 0 )
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
