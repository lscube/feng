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

#include "feng.h"
#include "rtsp.h"
#include "fnc_log.h"
#include "feng_utils.h"

#ifdef ENABLE_SCTP
# include <netinet/sctp.h>
#endif

/**
 * @brief Read data out of an RTSP client socket
 *
 * @param rtsp The client to read data from
 * @param stream Pointer to the variable to save the stream number to
 * @param buffer Pointer to the memory area to read
 * @param size The size of the memory area to read
 *
 * @return The amount of data read from the socket
 * @return -1 Error during read.
 *
 * @todo The size parameter and the return value should be size_t
 *       instead of int.
 */
static int rtsp_sock_read(RTSP_Client *rtsp, int *stream, guint8 *buffer, int size)
{
    Sock *sock = rtsp->sock;
    int n;
#ifdef ENABLE_SCTP
    struct sctp_sndrcvinfo sctp_info;
    if (sock->socktype == SCTP) {
        memset(&sctp_info, 0, sizeof(sctp_info));
        n = neb_sock_read(sock, buffer, size, &sctp_info, 0);
        *stream = sctp_info.sinfo_stream;
        fnc_log(FNC_LOG_DEBUG,
            "neb_sock_read() received %d bytes from sctp stream %d\n", n, stream);
    } else    // RTSP protocol is TCP
#endif    // ENABLE_SCTP

    n = neb_sock_read(sock, buffer, size, NULL, 0);

    return n;
}

void rtsp_interleaved(RTSP_Client *rtsp, int channel, uint8_t *data, size_t len)
{
    RTP_session *rtp = NULL;

    if ( (rtp = g_hash_table_lookup(rtsp->channels, GINT_TO_POINTER(channel))) == NULL ) {
        fnc_log(FNC_LOG_INFO, "Received interleaved message for unknown channel %d", channel);
    } else if ( channel == rtp->transport.rtcp_ch )
        rtcp_handle(rtp, data, len);
    else
        g_assert_not_reached();
}

void rtsp_read_cb(struct ev_loop *loop, ev_io *w,
                  ATTR_UNUSED int revents)
{
    guint8 buffer[RTSP_BUFFERSIZE + 1] = { 0, };    /* +1 to control the final '\0' */
    int read_size;
    gint channel = 0;
    RTSP_Client *rtsp = w->data;

    if ( (read_size = rtsp_sock_read(rtsp,
                                     &channel,
                                     buffer,
                                     sizeof(buffer) - 1)
          ) <= 0 )
        goto client_close;

    /* If we got a channel number from the socket, it means that we're
     * reading data out of some interleaved lower level protocol
     * protocol; right now this only means SCTP.
    */
    if ( channel != 0 ) {
        rtsp_interleaved(rtsp, channel, buffer, read_size);
        return;
    }

    if (rtsp->input->len + read_size > RTSP_BUFFERSIZE) {
        fnc_log(FNC_LOG_DEBUG,
                "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
        goto server_close;
    }

    g_byte_array_append(rtsp->input, (guint8*)buffer, read_size);

#ifdef HAVE_JSON
    rtsp->bytes_read += read_size;
    rtsp->srv->total_read += read_size;
#endif
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

void rtsp_write_cb(ATTR_UNUSED struct ev_loop *loop, ev_io *w,
                   ATTR_UNUSED int revents)
{
    RTSP_Client *rtsp = w->data;
    GByteArray *outpkt = (GByteArray *)g_queue_pop_tail(rtsp->out_queue);

    if (outpkt == NULL) {
        ev_io_stop(rtsp->srv->loop, &rtsp->ev_io_write);
        return;
    }

    if ( neb_sock_write(rtsp->sock, outpkt->data, outpkt->len,
                        MSG_DONTWAIT) < outpkt->len) {
        fnc_perror("");
        if ( errno == EAGAIN )
            return;
    }

#ifdef HAVE_JSON
    rtsp->bytes_sent += outpkt->len;
    rtsp->srv->total_sent += outpkt->len;
#endif
    g_byte_array_free(outpkt, TRUE);
}
