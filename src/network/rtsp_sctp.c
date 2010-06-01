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
#include <netinet/in.h>
#include <netinet/sctp.h>

#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "netembryo.h"

typedef struct {
    struct sctp_sndrcvinfo rtp;
    struct sctp_sndrcvinfo rtcp;
} RTP_SCTP_Transport;

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
    RTP_SCTP_Transport *transport = (RTP_SCTP_Transport *)rtp->transport_data;

    return rtsp_sctp_send_pkt(rtp->client, buffer, &transport->rtp);
}

static gboolean rtp_sctp_send_rtcp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_SCTP_Transport *transport = (RTP_SCTP_Transport *)rtp->transport_data;

    return rtsp_sctp_send_pkt(rtp->client, buffer, &transport->rtcp);
}

static void rtp_sctp_close_transport(RTP_session *rtp)
{
    g_slice_free(RTP_SCTP_Transport, rtp->transport_data);
}

void rtp_sctp_transport(RTSP_Client *rtsp,
                        RTP_session *rtp_s,
                        struct ParsedTransport *parsed)
{
    if ( parsed->rtp_channel == -1 )
        parsed->rtp_channel = ++rtsp->first_free_channel;
    if ( parsed->rtcp_channel == -1 )
        parsed->rtcp_channel = ++rtsp->first_free_channel;

    /* There's a limit of 256 channels in the RFC2326 specification,
     * since a single byte is used for the $-prefixed interleaved
     * packages.
     */
    if ( parsed->rtp_channel >= NETEMBRYO_MAX_SCTP_STREAMS || parsed->rtcp_channel >= NETEMBRYO_MAX_SCTP_STREAMS )
        return;

    rtsp_interleaved_register(rtsp, rtp_s, parsed->rtp_channel, parsed->rtcp_channel);

    {
        RTP_SCTP_Transport transport = {
            .rtp = {
                .sinfo_stream = parsed->rtp_channel
            },
            .rtcp = {
                .sinfo_stream = parsed->rtcp_channel
            }
        };

        rtp_s->transport_data = g_slice_dup(RTP_SCTP_Transport, &transport);
    }

    rtp_s->send_rtp = rtp_sctp_send_rtp;
    rtp_s->send_rtcp = rtp_sctp_send_rtcp;
    rtp_s->close_transport = rtp_sctp_close_transport;

    rtp_s->transport_string = g_strdup_printf("RTP/AVP/SCTP;server_streams=%d-%d;ssrc=%08X",
                                              parsed->rtp_channel,
                                              parsed->rtcp_channel,
                                              rtp_s->ssrc);
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

    /* start with an acceptable dimension, 1KiB; we can extend further
       in 1KiB increments if it's needed; alternatively we can decide
       to go with PAGE_SIZE chunks. */
    static const size_t buffer_chunk_size = 1024;
    GByteArray *buffer = g_byte_array_sized_new(buffer_chunk_size);

    g_byte_array_set_size(buffer, buffer_chunk_size);

    do {
        int flags;

        int partial = sctp_recvmsg(rtsp->sd,
                                   &buffer->data[size], buffer->len - size,
                                   NULL, 0, &sctp_info, &flags);

        if ( partial < 0 ) {
            fnc_log(FNC_LOG_INFO, "SCTP RTSP connection closed by the client.");
            ev_async_send(loop, &rtsp->ev_sig_disconnect);
            return;
        }

        size += partial;

        if ( flags & MSG_EOR )
            break;
        else if ( size >= buffer->len ) {
            g_byte_array_set_size(buffer, buffer->len + buffer_chunk_size);
            continue;
        }
    } while ( 0 /* The loop will continue or break itself */ );

    g_byte_array_set_size(buffer, size);

    stats_account_read(rtsp, size);

    if ( sctp_info.sinfo_stream == 0 ) {
        /* Stream 0 is always the RTSP control stream */
        rtsp->input = buffer;
        RTSP_handler(rtsp);

        /* Assert that the whole buffer was consumed here */
        g_assert_cmpuint(buffer->len, ==, 0);

        rtsp->input = NULL;
    } else {
        rtsp_interleaved_receive(rtsp, sctp_info.sinfo_stream,
                                 buffer->data, buffer->len);
    }

    g_byte_array_free(buffer, TRUE);
}
