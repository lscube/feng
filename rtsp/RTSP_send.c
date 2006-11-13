/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */
#include <string.h>
#include <errno.h>

#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>
 
ssize_t RTSP_send(RTSP_buffer * rtsp)
{
	int n = 0;
	size_t to_send;
	// char *buffer;
#ifdef HAVE_SCTP_FENICE
	struct sctp_sndrcvinfo sctp_info;
	// int m = 0;
#endif

 	if (!rtsp->out_size) {
 		fnc_log(FNC_LOG_WARN, "RTSP_send called, but no data to be sent\n");
 		return 0;
 	}

	to_send = rtsp->out_size - rtsp->out_sent;
		
	// TODO: find che interleaved channel to be used and, in the TCP case, build the pkt.
		
	switch (rtsp->proto) {
#ifdef HAVE_SCTP_FENICE
	case SCTP: // TODO: choose right stream
		memset(&sctp_info, 0, sizeof(sctp_info));
		if ( (n = sctp_send(rtsp->fd, rtsp->out_buffer + rtsp->out_sent, to_send,
							&sctp_info, MSG_DONTWAIT | MSG_NOSIGNAL)) < 0) {
			fnc_log(FNC_LOG_ERR, "sctp_send() error in RTSP_send()\n");
			return n;
		}
		break;
#endif // HAVE_SCTP_FENICE
	case TCP:
		if ( (n = send(rtsp->fd, rtsp->out_buffer + rtsp->out_sent, to_send, MSG_DONTWAIT | MSG_NOSIGNAL)) < 0) {
			switch (errno) {
				case EACCES:
					fnc_log(FNC_LOG_ERR, "EACCES error\n");
					break;
				case EAGAIN:
					fnc_log(FNC_LOG_ERR, "EAGAIN error\n");
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
			fnc_log(FNC_LOG_ERR, "send() error in RTSP_send()\n");
			return n;
		}
		break;
	default:
		return ERR_GENERIC;
		break;
	}

	if ( (rtsp->out_sent += n) == rtsp->out_size )
		rtsp->out_sent = rtsp->out_size = 0;
	return n;
}
