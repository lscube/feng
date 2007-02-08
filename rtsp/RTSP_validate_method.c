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
#include <string.h>

#include <fenice/rtsp.h>
#include <fenice/utils.h>

/* Return -1 if something doesn't work in the request */
int RTSP_validate_method(RTSP_buffer * rtsp)
{
	char method[32], hdr[16];
	char object[256];
	char ver[32];
	unsigned int seq;
	int pcnt;		/* parameter count */
        char *p;
	int mid = ERR_GENERIC;

	*method = *object = '\0';
	seq = 0;

	/* parse first line of message header as if it were a request message */

	if ((pcnt =
	     sscanf(rtsp->in_buffer, " %31s %255s %31s ", method,
		    object, ver, hdr, &seq)) != 3)
		return ERR_GENERIC;

        p = rtsp->in_buffer;

        while ((p = strstr(p,"\n"))) {
            if ((pcnt = sscanf(p, " %15s %u ", hdr, &seq)) == 2)
                    if (strstr(hdr, HDR_CSEQ)) break;
            p++;
        }
	if (p == NULL)
		return ERR_GENERIC;

	if (strcmp(method, RTSP_METHOD_DESCRIBE) == 0) {
		mid = RTSP_ID_DESCRIBE;
	}
	if (strcmp(method, RTSP_METHOD_ANNOUNCE) == 0) {
		mid = RTSP_ID_ANNOUNCE;
	}
	if (strcmp(method, RTSP_METHOD_GET_PARAMETERS) == 0) {
		mid = RTSP_ID_GET_PARAMETERS;
	}
	if (strcmp(method, RTSP_METHOD_OPTIONS) == 0) {
		mid = RTSP_ID_OPTIONS;
	}
	if (strcmp(method, RTSP_METHOD_PAUSE) == 0) {
		mid = RTSP_ID_PAUSE;
	}
	if (strcmp(method, RTSP_METHOD_PLAY) == 0) {
		mid = RTSP_ID_PLAY;
	}
	if (strcmp(method, RTSP_METHOD_RECORD) == 0) {
		mid = RTSP_ID_RECORD;
	}
	if (strcmp(method, RTSP_METHOD_REDIRECT) == 0) {
		mid = RTSP_ID_REDIRECT;
	}
	if (strcmp(method, RTSP_METHOD_SETUP) == 0) {
		mid = RTSP_ID_SETUP;
	}
	if (strcmp(method, RTSP_METHOD_SET_PARAMETER) == 0) {
		mid = RTSP_ID_SET_PARAMETER;
	}
	if (strcmp(method, RTSP_METHOD_TEARDOWN) == 0) {
		mid = RTSP_ID_TEARDOWN;
	}

	rtsp->rtsp_cseq = seq;	/* set the current method request seq. number. */
	return mid;
}
