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
#include <config.h>

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

uint32 send_redirect_3xx(RTSP_buffer * rtsp, char *object)
{
#if ENABLE_MEDIATHREAD
#warning Write mt equivalent
#else
	char *r;		/* get reply message buffer pointer */
	uint8 *mb;		/* message body buffer pointer */
	uint32 mb_len;
	SD_descr *matching_descr;

	if (enum_media(object, &matching_descr) != ERR_NOERROR) {
		fnc_log(FNC_LOG_ERR,
			"SETUP request specified an object file which can be damaged.\n");
		send_reply(500, 0, rtsp);	/* Internal server error */
		return ERR_NOERROR;
	}
	//if(!strcasecmp(matching_descr->twin,"NONE") || !strcasecmp(matching_descr->twin,"")){
	if (!(matching_descr->flags & SD_FL_TWIN)) {
		send_reply(453, 0, rtsp);
		return ERR_NOERROR;
	}
	/* allocate buffer */
	mb_len = 2048;
	mb = malloc(mb_len);
	r = malloc(mb_len + 1512);
	if (!r || !mb) {
		fnc_log(FNC_LOG_ERR,
			"send_redirect(): unable to allocate memory\n");
		send_reply(500, 0, rtsp);	/* internal server error */
		if (r) {
			free(r);
		}
		if (mb) {
			free(mb);
		}
		return ERR_ALLOC;
	}
	/* build a reply message */
	sprintf(r,
		"%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
		RTSP_VER, 302, get_stat(302), rtsp->rtsp_cseq, PACKAGE,
		VERSION);
	sprintf(r + strlen(r), "Location: %s" RTSP_EL, matching_descr->twin);	/*twin of the first media of the aggregate movie */

	strcat(r, RTSP_EL);


	bwrite(r, (unsigned short) strlen(r), rtsp);

	free(mb);
	free(r);

	fnc_log(FNC_LOG_VERBOSE, "REDIRECT response sent.\n");
#endif
	return ERR_NOERROR;

}
