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

#include <stdio.h>
#include <stdlib.h>

#include <fenice/rtsp.h>
#include <string.h>

void RTSP_msg_len(int *hdr_len, int *body_len, RTSP_buffer * rtsp)
// This routine is from OMS.
{
	int eom;		/* end of message found */
	int mb;			/* message body exists */
	int tc;			/* terminator count */
	int ws;			/* white space */
	int ml;			/* total message length including any message body */
	int bl;			/* message body length */
	char c;			/* character */
	char *p;

	eom = mb = ml = bl = 0;
	while (ml <= rtsp->in_size) {
		/* look for eol. */
		ml += strcspn(&(rtsp->in_buffer[ml]), "\r\n");
		if (ml > rtsp->in_size)
			break;
		/*
		 * work through terminaters and then check if it is the
		 * end of the message header.
		 */
		tc = ws = 0;
		while (!eom && ((ml + tc + ws) < rtsp->in_size)) {
			c = rtsp->in_buffer[ml + tc + ws];
			if (c == '\r' || c == '\n')
				tc++;
			else if ((tc < 3) && ((c == ' ') || (c == '\t')))
				ws++;	/* white space between lf & cr is sloppy, but tolerated. */
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
		if ((tc > 2) || ((tc == 2) && (rtsp->in_buffer[ml] == rtsp->in_buffer[ml + 1])))
			eom = 1;	/* must be the end of the message header */
		ml += tc + ws;

		if (eom) {
			ml += bl;	/* add in the message body length */
			break;	/* all done finding the end of the message. */
		}
		if (ml >= rtsp->in_size)
			break;
		/*
		 * check first token in each line to determine if there is
		 * a message body.
		 */
		if (!mb) {	/* content length token not yet encountered. */
			if (!strncasecmp(&(rtsp->in_buffer[ml]), HDR_CONTENTLENGTH, sizeof(HDR_CONTENTLENGTH) - 1)) {
				mb = 1;	/* there is a message body. */
				ml += sizeof(HDR_CONTENTLENGTH) - 1;
				while (ml < rtsp->in_size) {
					c = rtsp->in_buffer[ml];
					if ((c == ':') || (c == ' '))
						ml++;
					else
						break;
				}

				if (sscanf(&(rtsp->in_buffer[ml]), "%d", &bl) != 1) {
					printf("ALERT: invalid ContentLength encountered in message.");
					exit(-1);
				}
			}
		}

	}

	if (ml > rtsp->in_size) {
		printf("PANIC: buffer did not contain the entire RTSP message.");
		exit(-1);
	}

	/*
	 * go through any trailing nulls.  Some servers send null terminated strings
	 * following the body part of the message.  It is probably not strictly
	 * legal when the null byte is not included in the Content-Length count.
	 * However, it is tolerated here.
	 */
	*hdr_len = ml - bl;
	for (tc = rtsp->in_size - ml, p = &(rtsp->in_buffer[ml]); tc && (*p == '\0'); p++, bl++, tc--);
	*body_len = bl;
}

