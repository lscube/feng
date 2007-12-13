/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>
#include <fenice/utils.h>

/** @file RTSP_lowlevel.c
 * @brief Contains most of lowlevel RTSP functions
 */

/**
 * Initializes an RTSP buffer binding it to a socket
 * @param rtsp the buffer to inizialize
 * @param rtsp_sock the socket to link to the buffer
 */
void RTSP_initserver(RTSP_buffer * rtsp, Sock *rtsp_sock)
{
    rtsp->sock = rtsp_sock;
    rtsp->session_list = (RTSP_session *) calloc(1, sizeof(RTSP_session));
    rtsp->session_list->session_id = -1;
}

/**
 * Sends the rtsp output buffer through the socket
 * @param rtsp the rtsp connection to flush through the socket
 * @return the size of data sent
 */
ssize_t RTSP_send(RTSP_buffer * rtsp)
{
    int n = 0;

    if (!rtsp->out_size) {
        fnc_log(FNC_LOG_WARN, "RTSP_send called, but no data to be sent\n");
        return 0;
    }

    if ( (n = Sock_write(rtsp->sock, rtsp->out_buffer, rtsp->out_size,
          NULL, MSG_DONTWAIT)) < 0) {
        switch (errno) {
            case EACCES:
                fnc_log(FNC_LOG_ERR, "EACCES error\n");
                break;
            case EAGAIN:
                fnc_log(FNC_LOG_ERR, "EAGAIN error\n");
                return 0; // Don't close socket if tx buffer is full!
                break;
            case EBADF:
                fnc_log(FNC_LOG_ERR, "EBADF error\n");
                break;
            case ECONNRESET:
                fnc_log(FNC_LOG_ERR, "ECONNRESET error\n");
                break;
            case EDESTADDRREQ:
                fnc_log(FNC_LOG_ERR, "EDESTADDRREQ error\n");
                break;
            case EFAULT:
                fnc_log(FNC_LOG_ERR, "EFAULT error\n");
                break;
            case EINTR:
                fnc_log(FNC_LOG_ERR, "EINTR error\n");
                break;
            case EINVAL:
                fnc_log(FNC_LOG_ERR, "EINVAL error\n");
                break;
            case EISCONN:
                fnc_log(FNC_LOG_ERR, "EISCONN error\n");
                break;
            case EMSGSIZE:
                fnc_log(FNC_LOG_ERR, "EMSGSIZE error\n");
                break;
            case ENOBUFS:
                fnc_log(FNC_LOG_ERR, "ENOBUFS error\n");
                break;
            case ENOMEM:
                fnc_log(FNC_LOG_ERR, "ENOMEM error\n");
                break;
            case ENOTCONN:
                fnc_log(FNC_LOG_ERR, "ENOTCONN error\n");
                break;
            case ENOTSOCK:
                fnc_log(FNC_LOG_ERR, "ENOTSOCK error\n");
                break;
            case EOPNOTSUPP:
                fnc_log(FNC_LOG_ERR, "EOPNOTSUPP error\n");
                break;
            case EPIPE:
                fnc_log(FNC_LOG_ERR, "EPIPE error\n");
                break;
            default:
                break;
        }
        fnc_log(FNC_LOG_ERR, "Sock_write() error in RTSP_send()\n");
        return n;
    }
    //remove tx bytes from buffer
    memmove(rtsp->out_buffer, rtsp->out_buffer + n, (rtsp->out_size -= n));
    return n;
}

/** 
 * Removes a message from the input buffer
 * @param len the size of the message to remove
 * @param rtsp the buffer from which to remove the message
 */
void RTSP_remove_msg(int len, RTSP_buffer * rtsp)
{
    rtsp->in_size -= len;
    if (rtsp->in_size && len) {    /* discard the message from the in_buffer. */
        memmove(rtsp->in_buffer, &(rtsp->in_buffer[len]),
            RTSP_BUFFERSIZE - len);
        memset(&(rtsp->in_buffer[len]), 0, RTSP_BUFFERSIZE - len);
    }
}

/**
 * Removes the last message from the rtsp buffer
 * @param rtsp the buffer from which to discard the message
 */
void RTSP_discard_msg(RTSP_buffer * rtsp)
{
    int hlen, blen;

    if (RTSP_msg_len(rtsp, &hlen, &blen) > 0)
        RTSP_remove_msg(hlen + blen, rtsp);
}

/**
 * Recieves an RTSP message and puts it into the buffer
 * @param hdr_len where to save the header length
 * @param body_len where to save the message body length
 * @return -1 on ERROR
 * @return RTSP_not_full (0) if a full RTSP message is NOT present in the in_buffer yet.
 * @return RTSP_method_rcvd (1) if a full RTSP message is present in the in_buffer and is
 * ready to be handled.
 * @return RTSP_interlvd_rcvd (2) if a complete RTP/RTCP interleaved packet is present.  
 * terminate on really ugly cases.
 */
int RTSP_full_msg_rcvd(RTSP_buffer * rtsp, int *hdr_len, int *body_len)
{
    int eomh;          /*! end of message header found */
    int mb;            /*! message body exists */
    int tc;            /*! terminator count */
    int ws;            /*! white space */
    unsigned int ml;   /*! total message length including any message body */
    int bl;            /*! message body length */
    char c;            /*! character */
    int control;
    char *p;

    // is there an interleaved RTP/RTCP packet?
    if (rtsp->in_buffer[0] == '$') {
        uint16_t *intlvd_len = (uint16_t *) & rtsp->in_buffer[2];

        if ((bl = ntohs(*intlvd_len)) <= rtsp->in_size) {
            if (hdr_len)
                *hdr_len = 4;
            if (body_len)
                *body_len = bl;
            return RTSP_interlvd_rcvd;
        } else {
            fnc_log(FNC_LOG_DEBUG,
                "Non-complete Interleaved RTP or RTCP packet arrived.\n");
            return RTSP_not_full;
        }

    }
    eomh = mb = ml = bl = 0;
    while (ml <= rtsp->in_size) {
        /* look for eol. */
        control = strcspn(&(rtsp->in_buffer[ml]), "\r\n");
        if (control > 0)
            ml += control;
        else
            return ERR_GENERIC;

        if (ml > rtsp->in_size)
            return RTSP_not_full;    /* haven't received the entire message yet. */
        /*
         * work through terminaters and then check if it is the
         * end of the message header.
         */
        tc = ws = 0;
        while (!eomh && ((ml + tc + ws) < rtsp->in_size)) {
            c = rtsp->in_buffer[ml + tc + ws];
            if (c == '\r' || c == '\n')
                tc++;
            else if ((tc < 3) && ((c == ' ') || (c == '\t')))
                ws++;    /* white space between lf & cr is sloppy, but tolerated. */
            else
                break;
        }
        /*
         * cr,lf pair only counts as one end of line terminator.
         * Double line feeds are tolerated as end marker to the message header
         * section of the message.  This is in keeping with RFC 2068,
         * section 19.3 Tolerant Applications.
         * Otherwise, CRLF is the legal end-of-line marker for all HTTP/1.1
         * protocol compatible message elements.
         */
        if ((tc > 2)
            || ((tc == 2)
            && (rtsp->in_buffer[ml] == rtsp->in_buffer[ml + 1])))
            eomh = 1;    /* must be the end of the message header */
        ml += tc + ws;

        if (eomh) {
            ml += bl;    /* add in the message body length, if collected earlier */
            if (ml <= rtsp->in_size)
                break;    /* all done finding the end of the message. */
        }

        if (ml >= rtsp->in_size)
            return RTSP_not_full;    /* haven't received the entire message yet. */

        /*
         * check first token in each line to determine if there is
         * a message body.
         */
        if (!mb) {    /* content length token not yet encountered. */
            if (!strncasecmp
                (&(rtsp->in_buffer[ml]), HDR_CONTENTLENGTH,
                 strlen(HDR_CONTENTLENGTH))) {
                mb = 1;    /* there is a message body. */
                ml += strlen(HDR_CONTENTLENGTH);
                while (ml < rtsp->in_size) {
                    c = rtsp->in_buffer[ml];
                    if ((c == ':') || (c == ' '))
                        ml++;
                    else
                        break;
                }

                if (sscanf(&(rtsp->in_buffer[ml]), "%d", &bl) !=
                    1) {
                    fnc_log(FNC_LOG_ERR,
                        "RTSP_full_msg_rcvd(): Invalid ContentLength encountered in message.\n");
                    return ERR_GENERIC;
                }
            }
        }
    }

    if (hdr_len)
        *hdr_len = ml - bl;

    if (body_len) {
        /*
         * go through any trailing nulls.  Some servers send null terminated strings
         * following the body part of the message.  It is probably not strictly
         * legal when the null byte is not included in the Content-Length count.
         * However, it is tolerated here.
         */
        for (tc = rtsp->in_size - ml, p = &(rtsp->in_buffer[ml]);
             tc && (*p == '\0'); p++, bl++, tc--);
        *body_len = bl;
    }

    return RTSP_method_rcvd;
}
