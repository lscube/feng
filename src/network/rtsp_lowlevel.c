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

#include <strings.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "netembryo.h"

typedef struct {
    /** RTP socket descriptor */
    int rtp_sd;
    /** RTCP socket descriptor */
    int rtcp_sd;
    /** RTP remote socket address */
    struct sockaddr *rtp_sa;
    /** RTCP remote socket address */
    struct sockaddr *rtcp_sa;
    /* Length of the sockaddr structures above */
    socklen_t sa_len;
    ev_io rtcp_reader;
} RTP_UDP_Transport;

static gboolean rtp_udp_send_pkt(int sd, struct sockaddr *sa, GByteArray *buffer, RTSP_Client *rtsp)
{
    int written = -1;
    struct pollfd p = { sd, POLLOUT, 0};

    if (poll(&p, 1, 1) < 0) {
        fnc_perror("poll");
        goto end;
    }

    if (p.revents & POLLOUT) {
        written = sendto(sd, buffer->data, buffer->len,
                         MSG_EOR | MSG_DONTWAIT,
                         sa, sizeof(struct sockaddr_storage));
        if (written >= 0 ) {
            stats_account_sent(rtsp, written);
        } else {
            fnc_perror("sendto");
        }
    }

 end:
    g_byte_array_free(buffer, true);

    return written >= 0;
}

static gboolean rtp_udp_send_rtp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_UDP_Transport *transport = rtp->transport_data;

    return rtp_udp_send_pkt(transport->rtp_sd, transport->rtp_sa,
                            buffer, rtp->client);
}

static gboolean rtp_udp_send_rtcp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_UDP_Transport *transport = rtp->transport_data;

    return rtp_udp_send_pkt(transport->rtcp_sd, transport->rtcp_sa,
                            buffer, rtp->client);
}

static void rtp_udp_close_transport(RTP_session *rtp)
{
    RTP_UDP_Transport *transport = rtp->transport_data;

    ev_io_stop(rtp->client->loop, &transport->rtcp_reader);

    close(transport->rtp_sd);
    close(transport->rtcp_sd);

    g_slice_free1(transport->sa_len, transport->rtp_sa);
    g_slice_free1(transport->sa_len, transport->rtcp_sa);
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
        .rtp_sa = NULL,
        .rtcp_sd = -1,
        .rtcp_sa = NULL
    };
    ev_io *io = &transport.rtcp_reader;
    char *source = NULL;
    struct sockaddr_storage sa;
    socklen_t sa_len = rtsp->local_len;
    struct sockaddr *sa_p = (struct sockaddr*) &sa;
    int firstsd;
    in_port_t firstport, rtp_port, rtcp_port;

    memcpy(sa_p, &rtsp->local_sa, sa_len);

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

    if ( (firstsd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 ) {
        fnc_perror("socket 1");
        goto error;
    }

    if ( bind(firstsd, sa_p, sa_len) < 0 ) {
        fnc_perror("bind 1");
        goto error;
    }

    if ( getsockname(firstsd, sa_p, &sa_len) < 0 ) {
        fnc_perror("getsockname 1");
        goto error;
    }

    firstport = neb_sa_get_port(sa_p);

    switch ( firstport % 2 ) {
    case 0:
        transport.rtp_sd = firstsd; firstsd = -1;
        rtp_port = firstport; rtcp_port = firstport+1;
        if ( (transport.rtcp_sd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 ) {
            fnc_perror("socket 2");
            goto error;
        }

        neb_sa_set_port(sa_p, rtcp_port);

        if ( bind(transport.rtcp_sd, sa_p, sa_len) < 0 ) {
            fnc_perror("bind 2");

            neb_sa_set_port(sa_p, 0);
            if ( bind(transport.rtcp_sd, sa_p, sa_len) < 0 ) {
                fnc_perror("bind 3");
                goto error;
            }

            if ( getsockname(transport.rtcp_sd, sa_p, &sa_len) < 0 ) {
                fnc_perror("getsockname 2");
                goto error;
            }

            rtcp_port = neb_sa_get_port(sa_p);
        }

        break;
    case 1:
        transport.rtcp_sd = firstsd; firstsd = -1;
        rtcp_port = firstport; rtp_port = firstport-1;
        if ( (transport.rtp_sd = socket(sa_p->sa_family, SOCK_DGRAM, 0)) < 0 ) {
            fnc_perror("socket 3");
            goto error;
        }

        neb_sa_set_port(sa_p, rtp_port);

        if ( bind(transport.rtp_sd, sa_p, sa_len) < 0 ) {
            fnc_perror("bind 4");
            neb_sa_set_port(sa_p, 0);
            if ( bind(transport.rtp_sd, sa_p, sa_len) < 0 ) {
                fnc_perror("bind 5");
                goto error;
            }

            if ( getsockname(transport.rtp_sd, sa_p, &sa_len) < 0 ) {
                fnc_perror("getsockname 3");
                goto error;
            }

            rtp_port = neb_sa_get_port(sa_p);
        }

        break;
    }

    transport.sa_len = rtsp->peer_len;

    transport.rtp_sa = g_slice_copy(transport.sa_len, rtsp->peer_sa);
    neb_sa_set_port(transport.rtp_sa, parsed->rtp_channel);

    if ( connect(transport.rtp_sd,
                 transport.rtp_sa,
                 transport.sa_len) < 0 ) {
        fnc_perror("connect 1");
        goto error;
    }

    transport.rtcp_sa = g_slice_copy(transport.sa_len, rtsp->peer_sa);
    neb_sa_set_port(transport.rtcp_sa, parsed->rtcp_channel);

    if ( connect(transport.rtcp_sd,
                 transport.rtcp_sa,
                 transport.sa_len) < 0 ) {
        fnc_perror("connect 2");
        goto error;
    }

    io->data = rtp_s;
    ev_io_init(io, rtcp_udp_read_cb,
               transport.rtcp_sd, EV_READ);

    rtp_s->transport_data = g_slice_dup(RTP_UDP_Transport, &transport);
    rtp_s->send_rtp = rtp_udp_send_rtp;
    rtp_s->send_rtcp = rtp_udp_send_rtcp;
    rtp_s->close_transport = rtp_udp_close_transport;

    source = neb_sa_get_host((struct sockaddr*) &sa);

    rtp_s->transport_string = g_strdup_printf("RTP/AVP;unicast;source=%s;client_port=%d-%d;server_port=%d-%d;ssrc=%08X",
                                              rtsp->local_host,
                                              parsed->rtp_channel,
                                              parsed->rtcp_channel,
                                              rtp_port,
                                              rtcp_port,
                                              rtp_s->ssrc);

    free(source);

    return;

 error:
    if ( transport.rtp_sa != NULL )
        g_slice_free1(transport.sa_len, transport.rtp_sa);
    if ( transport.rtcp_sa != NULL )
        g_slice_free1(transport.sa_len, transport.rtcp_sa);
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
    ev_io_start(client->loop, &client->ev_io_write);
}

void rtsp_tcp_read_cb(struct ev_loop *loop, ev_io *w,
                      ATTR_UNUSED int revents)
{
    guint8 buffer[RTSP_BUFFERSIZE + 1] = { 0, };    /* +1 to control the final '\0' */
    int read_size;
    RTSP_Client *rtsp = w->data;

    if ( (read_size = recv(rtsp->sd,
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
    ev_unloop(loop, EVUNLOOP_ONE);
}

void rtsp_tcp_write_cb(ATTR_UNUSED struct ev_loop *loop, ev_io *w,
                       ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    GByteArray *outpkt = (GByteArray *)g_queue_pop_tail(rtsp->out_queue);
    size_t written = 0;

    if (outpkt == NULL) {
        ev_io_stop(loop, &rtsp->ev_io_write);
        return;
    }
    written = send(rtsp->sd, outpkt->data, outpkt->len, MSG_DONTWAIT);
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
