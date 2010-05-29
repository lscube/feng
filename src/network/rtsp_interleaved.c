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

#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"

void rtsp_interleaved_register(RTSP_Client *rtsp, RTP_session *rtp_s,
                               ATTR_UNUSED int rtp_channel, int rtcp_channel)
{
    if ( rtsp->channels == NULL )
        rtsp->channels = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                               NULL, NULL);

    g_hash_table_insert(rtsp->channels, GINT_TO_POINTER(rtcp_channel),
                        rtp_s);

    /* Keep the following commented out until we actually need to
     * _read_ rtp data
     */
#if 0
    g_hash_table_insert(rtsp->channels, GINT_TO_POINTER(rtp_channel),
                        rtp_s);

#endif
}

void rtsp_interleaved_receive(RTSP_Client *rtsp, int channel, uint8_t *data, size_t len)
{
    RTP_session *rtp = NULL;

    if ( (rtp = g_hash_table_lookup(rtsp->channels, GINT_TO_POINTER(channel))) == NULL )
        fnc_log(FNC_LOG_INFO, "Received interleaved message for unknown channel %d", channel);
    else /* we currently only support inbound RTCP; find a solution before supporting inbound RTP */
        rtcp_handle(rtp, data, len);
}

typedef struct {
    int rtp;
    int rtcp;
} RTP_Interleaved_Transport;

static gboolean rtp_interleaved_send_pkt(RTSP_Client *rtsp, GByteArray *buffer, int channel)
{
    const uint16_t ne_n = htons((uint16_t)buffer->len);
    const uint8_t interleaved_preamble[4] = { '$', channel, 0, 0 };

    /* This might not be the best choice here, to be honest… a
     * scatter-gather approach might work best, but for now, let's
     * stick with this…
     */
    g_byte_array_prepend(buffer, interleaved_preamble, 4);
    memcpy(&buffer->data[2], &ne_n, sizeof(uint16_t));

    /* pass the bucket down; it might be direct RTSP or HTTP-tunnelled */
    rtsp->write_data(rtsp, buffer);

    /* no stats accounting because the write_data function will take care of it */
    return TRUE;
}

static gboolean rtp_interleaved_send_rtp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_Interleaved_Transport *transport = (RTP_Interleaved_Transport *)rtp->transport_data;

    return rtp_interleaved_send_pkt(rtp->client, buffer, transport->rtp);
}

static gboolean rtp_interleaved_send_rtcp(RTP_session *rtp, GByteArray *buffer)
{
    RTP_Interleaved_Transport *transport = (RTP_Interleaved_Transport *)rtp->transport_data;

    return rtp_interleaved_send_pkt(rtp->client, buffer, transport->rtcp);
}

static void rtp_interleaved_close_transport(RTP_session *rtp)
{
    g_slice_free(RTP_Interleaved_Transport, rtp->transport_data);
}

void rtp_interleaved_transport(RTSP_Client *rtsp,
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
    if ( parsed->rtp_channel >= 256 || parsed->rtcp_channel >= 256 )
        return;

    rtsp_interleaved_register(rtsp, rtp_s, parsed->rtp_channel, parsed->rtcp_channel);

    {
        RTP_Interleaved_Transport transport = {
            .rtp = parsed->rtp_channel,
            .rtcp = parsed->rtcp_channel
        };

        rtp_s->transport_data = g_slice_dup(RTP_Interleaved_Transport, &transport);
    }

    rtp_s->send_rtp = rtp_interleaved_send_rtp;
    rtp_s->send_rtcp = rtp_interleaved_send_rtcp;
    rtp_s->close_transport = rtp_interleaved_close_transport;

    rtp_s->transport_string = g_strdup_printf("RTP/AVP/TCP;interleaved=%d-%d;ssrc=%08X",
                                              parsed->rtp_channel,
                                              parsed->rtcp_channel,
                                              rtp_s->ssrc);
}
