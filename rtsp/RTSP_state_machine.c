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

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <string.h>

/* macchina a stati dell'rtsp */
void RTSP_state_machine(RTSP_buffer * rtsp, int method)
{
	/*
	 * All state transitions are made here except when the last stream packet
	 * is sent during a PLAY.  That transition is located in stream_event().
	 */
	char *s;
	RTSP_session *p;
	long int session_id;
	char trash[255];

	if ((s = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
		if (sscanf(s, "%254s %ld", trash, &session_id) != 2) {
			printf("Invalid Session number in Session header\n");
			send_reply(454, 0, rtsp);	/* Session Not Found */
			return;
		}
	}
	p = rtsp->session_list;
	if (p == NULL) {
		return;
	}
	switch (p->cur_state) {
	case INIT_STATE:{
			switch (method) {
			case RTSP_ID_DESCRIBE:
				RTSP_describe(rtsp);
				break;
			case RTSP_ID_SETUP:
				if (RTSP_setup(rtsp, &p) == ERR_NOERROR) {
					p->cur_state = READY_STATE;
				}
				break;
			case RTSP_ID_TEARDOWN:
				if (RTSP_teardown(rtsp) == ERR_NOERROR) {
					p->cur_state = INIT_STATE;
				}
				break;
			case RTSP_ID_OPTIONS:
			        if (RTSP_options(rtsp) == ERR_NOERROR) {
					p->cur_state = INIT_STATE;
				}
				break;
			case RTSP_ID_PLAY:	/* method not valid this state. */
			case RTSP_ID_PAUSE:
				send_reply(455, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n", rtsp);
				break;
			default:
				send_reply(501, "Accept: OPTIONS, DESCRIBE, SETUP, TEARDOWN\n", rtsp);
				break;
			}
			break;
		}		/* INIT state */
	case READY_STATE:{
			switch (method) {
			case RTSP_ID_PLAY:
				if (RTSP_play(rtsp) == ERR_NOERROR) {
					p->cur_state = PLAY_STATE;
				}
				break;
			case RTSP_ID_SETUP:
				if (RTSP_setup(rtsp, &p) == ERR_NOERROR) {
					p->cur_state = READY_STATE;
				}
				break;
			case RTSP_ID_TEARDOWN:
				if (RTSP_teardown(rtsp) == ERR_NOERROR) {
					p->cur_state = INIT_STATE;
				}
				break;
			case RTSP_ID_OPTIONS:
				break;
			case RTSP_ID_PAUSE:	/* method not valid this state. */
				send_reply(455, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n", rtsp);
				break;
			case RTSP_ID_DESCRIBE:
				RTSP_describe(rtsp);
				break;
			default:
				send_reply(501, "Accept: OPTIONS, SETUP, PLAY, TEARDOWN\n", rtsp);
				break;
			}
			break;
		}		/* READY state */
	case PLAY_STATE:{
			switch (method) {
			case RTSP_ID_PLAY:
				// Feature not supported
				printf("UNSUPPORTED: Play while playing.\n");
				send_reply(551, 0, rtsp);	/* Option not supported */
				break;
			case RTSP_ID_PAUSE:
				if (RTSP_pause(rtsp) == ERR_NOERROR) {
					p->cur_state = READY_STATE;
				}
				break;
			case RTSP_ID_TEARDOWN:
				if (RTSP_teardown(rtsp) == ERR_NOERROR) {
					p->cur_state = INIT_STATE;
				}
				break;
			case RTSP_ID_OPTIONS:
				break;
			case RTSP_ID_DESCRIBE:
				RTSP_describe(rtsp);
				break;
			case RTSP_ID_SETUP:
				break;
			}
			break;
		}		/* PLAY state */
	default:{		/* invalid/unexpected current state. */
			printf("State error: unknown state=%d, method code=%d\n", p->cur_state, method);
		}
		break;
	}			/* end of current state switch */
}

