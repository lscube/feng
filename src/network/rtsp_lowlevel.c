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

#include "rtsp.h"
#include <fenice/fnc_log.h>
#include <fenice/utils.h>

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
static int rtsp_sock_read(RTSP_Client *rtsp, int *stream, char *buffer, int size)
{
    Sock *sock = rtsp->sock;
    int n;
#ifdef HAVE_SCTP
    struct sctp_sndrcvinfo sctp_info;
    if (Sock_type(sock) == SCTP) {
        memset(&sctp_info, 0, sizeof(sctp_info));
        n = Sock_read(sock, buffer, size, &sctp_info, 0);
        *stream = sctp_info.sinfo_stream;
        fnc_log(FNC_LOG_DEBUG,
            "Sock_read() received %d bytes from sctp stream %d\n", n, stream);
    } else    // RTSP protocol is TCP
#endif    // HAVE_SCTP

    n = Sock_read(sock, buffer, size, NULL, 0);

    return n;
}

void rtsp_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    char buffer[RTSP_BUFFERSIZE + 1] = { 0, };    /* +1 to control the final '\0' */
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
        interleaved_rtcp_send(rtsp, channel, buffer, read_size);
        return;
    }

    if (rtsp->input->len + read_size > RTSP_BUFFERSIZE) {
        fnc_log(FNC_LOG_DEBUG,
                "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
        goto server_close;
    }

    g_byte_array_append(rtsp->input, buffer, read_size);
    if (RTSP_handler(rtsp) == ERR_GENERIC) {
        fnc_log(FNC_LOG_ERR, "Invalid input message.\n");
        goto server_close;
    }

    return;

 client_close:
    fnc_log(FNC_LOG_INFO, "RTSP connection closed by client.");
    goto disconnect;

 server_close:
    fnc_log(FNC_LOG_INFO, "RTSP connection closed by server.");
    goto disconnect;

 disconnect:
    ev_async_send(loop, rtsp->ev_sig_disconnect);
}

void rtsp_write_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    RTSP_Client *rtsp = w->data;
    GString *outpkt = (GString *)g_async_queue_try_pop(rtsp->out_queue);

    if (outpkt == NULL) {
        ev_io_stop(rtsp->srv->loop, &rtsp->ev_io_write);
        return 0;
    }

    if ( Sock_write(rtsp->sock, outpkt->str, outpkt->len,
                    NULL, MSG_DONTWAIT) < outpkt->len) {
        switch (errno) {
            case EACCES:
                fnc_log(FNC_LOG_ERR, "EACCES error");
                break;
            case EAGAIN:
                fnc_log(FNC_LOG_ERR, "EAGAIN error");
                return 0; // Don't close socket if tx buffer is full!
                break;
            case EBADF:
                fnc_log(FNC_LOG_ERR, "EBADF error");
                break;
            case ECONNRESET:
                fnc_log(FNC_LOG_ERR, "ECONNRESET error");
                break;
            case EDESTADDRREQ:
                fnc_log(FNC_LOG_ERR, "EDESTADDRREQ error");
                break;
            case EFAULT:
                fnc_log(FNC_LOG_ERR, "EFAULT error");
                break;
            case EINTR:
                fnc_log(FNC_LOG_ERR, "EINTR error");
                break;
            case EINVAL:
                fnc_log(FNC_LOG_ERR, "EINVAL error");
                break;
            case EISCONN:
                fnc_log(FNC_LOG_ERR, "EISCONN error");
                break;
            case EMSGSIZE:
                fnc_log(FNC_LOG_ERR, "EMSGSIZE error");
                break;
            case ENOBUFS:
                fnc_log(FNC_LOG_ERR, "ENOBUFS error");
                break;
            case ENOMEM:
                fnc_log(FNC_LOG_ERR, "ENOMEM error");
                break;
            case ENOTCONN:
                fnc_log(FNC_LOG_ERR, "ENOTCONN error");
                break;
            case ENOTSOCK:
                fnc_log(FNC_LOG_ERR, "ENOTSOCK error");
                break;
            case EOPNOTSUPP:
                fnc_log(FNC_LOG_ERR, "EOPNOTSUPP error");
                break;
            case EPIPE:
                fnc_log(FNC_LOG_ERR, "EPIPE error");
                break;
            default:
                break;
        }
        fnc_log(FNC_LOG_ERR, "Sock_write() error in rtsp_send()");
    }

    g_string_free(outpkt, TRUE);
}
