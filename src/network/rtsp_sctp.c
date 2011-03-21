/* *
 * This file is part of Feng
 *
 * Copyright (C) 2010 by LScube team <team@lscube.org>
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

#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if HAVE_LINUX_SOCKIOS_H
# include <linux/sockios.h>
#endif
#include <netinet/in.h>
#include <netinet/sctp.h>

#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "feng.h"

static gboolean rtsp_sctp_send_pkt(RTSP_Client *rtsp, GByteArray *buffer,
                                   const struct sctp_sndrcvinfo *sctp_info)
{
    int written = sctp_send(rtsp->sd,
                            buffer->data, buffer->len,
                            sctp_info,
                            MSG_DONTWAIT | MSG_EOR);

    stats_account_sent(rtsp, written);
    g_byte_array_free(buffer, TRUE);

    if ( written < 0 ) {
        fnc_perror("");
        return FALSE;
    } else
        return TRUE;
}

static gboolean rtp_sctp_send_rtp(RTP_session *rtp, GByteArray *buffer)
{
    return rtsp_sctp_send_pkt(rtp->client, buffer, &rtp->transport.sctp.rtp);
}

static gboolean rtp_sctp_send_rtcp(RTP_session *rtp, GByteArray *buffer)
{
    return rtsp_sctp_send_pkt(rtp->client, buffer, &rtp->transport.sctp.rtcp);
}

static void rtp_sctp_close_transport(ATTR_UNUSED RTP_session *rtp)
{
}

gboolean rtp_sctp_transport(RTSP_Client *rtsp,
                           RTP_session *rtp_s,
                           struct ParsedTransport *parsed)
{
    int max_streams;
    struct sctp_initmsg params;
    socklen_t params_size = sizeof(params);

    if ( parsed->rtp_channel == -1 )
        parsed->rtp_channel = ++rtsp->first_free_channel;
    if ( parsed->rtcp_channel == -1 )
        parsed->rtcp_channel = ++rtsp->first_free_channel;

    if (sctp_opt_info(rtsp->sd, 0, SCTP_INITMSG,
                      &params, &params_size) < 0) {
        fnc_perror("sctp_opt_info(SCTP_INITMSG)");
        return false;
    }

    feng_assert_or_retval(params_size == sizeof(params), false);

    max_streams = MIN(params.sinit_max_instreams, params.sinit_num_ostreams);

    if ( parsed->rtp_channel >= max_streams ||
         parsed->rtcp_channel >= max_streams )
        return false;

    rtsp_interleaved_register(rtsp, rtp_s, parsed->rtp_channel, parsed->rtcp_channel);

    rtp_s->transport.sctp.rtp.sinfo_stream = parsed->rtp_channel;
    rtp_s->transport.sctp.rtcp.sinfo_stream = parsed->rtp_channel;

    rtp_s->send_rtp = rtp_sctp_send_rtp;
    rtp_s->send_rtcp = rtp_sctp_send_rtcp;
    rtp_s->close_transport = rtp_sctp_close_transport;

    rtp_s->transport_string = g_strdup_printf("RTP/AVP/SCTP;server_streams=%d-%d;ssrc=%08X",
                                              parsed->rtp_channel,
                                              parsed->rtcp_channel,
                                              rtp_s->ssrc);
    return true;
}

/**
 * @brief Directly send SCTP data to the client
 *
 * @param client The client to write the data to
 * @param data The GByteArray object to queue for sending
 *
 * @note after calling this function, the @p data object should no
 * longer be referenced by the code path.
 */
void rtsp_sctp_send_rtsp(RTSP_Client *client, GByteArray *buffer)
{
    static const struct sctp_sndrcvinfo sctp_channel_zero = {
        .sinfo_stream = 0
    };

    rtsp_sctp_send_pkt(client, buffer, &sctp_channel_zero);
}

void rtsp_sctp_read_cb(struct ev_loop *loop, ev_io *w,
                       ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    struct sctp_sndrcvinfo sctp_info;
    guint size = 0;

    int disconnect = 0;

    /* start with an acceptable dimension, 1KiB; we can extend further
       in 1KiB increments if it's needed; alternatively we can decide
       to go with PAGE_SIZE chunks. */
    static const size_t buffer_chunk_size = 1024;
    size_t initial_size = buffer_chunk_size;
    GByteArray *buffer;

#ifdef SIOCINQ
    /* if ioctl(SIOCINQ) fails, do _not_ try to use it again, it would
       likely not be implemented in the OS. In such a case, just set
       this to false.
    */
    static gboolean try_siocinq = true;

    if ( try_siocinq ) {
        int pktsize;
        if ( ioctl(rtsp->sd, SIOCINQ, &pktsize) < 0 ) {
            fnc_perror("ioctl(SIOCINQ)");
            try_siocinq = false;
        } else if ( pktsize > 0 ) {
            initial_size = pktsize + 1;
        }
    }
#endif

    buffer = g_byte_array_sized_new(initial_size);

    g_byte_array_set_size(buffer, initial_size);

    do {
        int flags;

        int partial = sctp_recvmsg(rtsp->sd,
                                   &buffer->data[size], buffer->len - size,
                                   NULL, 0, &sctp_info, &flags);

        if ( partial < 0 ) {
            fnc_perror("sctp_recvmsg");
            disconnect = 1;
            goto end;
        }

        size += partial;

        if ( flags & MSG_EOR )
            break;
        else if ( size >= buffer->len ) {
            g_byte_array_set_size(buffer, buffer->len + buffer_chunk_size);
            continue;
        }
    } while ( 0 /* The loop will continue or break itself */ );

    if ( size == 0 )
        goto end;

    g_byte_array_set_size(buffer, size);

    stats_account_read(rtsp, size);

    if ( sctp_info.sinfo_stream == 0 ) {
        /* Stream 0 is always the RTSP control stream */
        rtsp->input = buffer;

        disconnect = !rtsp_process_complete(rtsp);
    } else {
        rtsp_interleaved_receive(rtsp, sctp_info.sinfo_stream,
                                 buffer->data, buffer->len);
    }

 end:
    rtsp->input = NULL;
    g_byte_array_free(buffer, TRUE);

    if ( disconnect )
        ev_unloop(loop, EVUNLOOP_ONE);
}
