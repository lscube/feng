/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include <strings.h>

#include "rtsp.h"
#include <fenice/fnc_log.h>
#include <fenice/utils.h>

/** @file
 * @brief Contains most of lowlevel RTSP functions
 */


/**
 * Sends the rtsp output buffer through the socket
 * @param rtsp the rtsp connection to flush through the socket
 * @return the size of data sent
 */
ssize_t RTSP_send(RTSP_buffer * rtsp)
{
    int n = 0;

    GString *outpkt = (GString *)g_async_queue_try_pop(rtsp->out_queue);

    if (outpkt == NULL) {
        fnc_log(FNC_LOG_WARN, "RTSP_send called, but no data to be sent");
        return 0;
    }

    if ( (n = Sock_write(rtsp->sock, outpkt->str, outpkt->len,
          NULL, MSG_DONTWAIT)) < outpkt->len) {
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
        fnc_log(FNC_LOG_ERR, "Sock_write() error in RTSP_send()");
    }

    g_string_free(outpkt, TRUE);
    return n;
}
