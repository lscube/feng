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

#ifndef _INTNETH
#define _INTNETH

	/*#include <time.h>
	   #include <sys/timeb.h>
	   #include <sys/socket.h>
	   #include <fenice/socket.h>
	   #include <fenice/mediainfo.h>
	 */
#include <fenice/rtp.h>
#include <fenice/rtcp.h>

	/**************************************************Intelligent Network Functions*****************************************/

int stream_change(RTP_session * changing_session, int value);
int change_check(RTP_session * changing_session);

int downgrade_GSM(RTP_session * changing_session);
int downgrade_L16(RTP_session * changing_session);
int downgrade_MP3(RTP_session * changing_session);

int upgrade_GSM(RTP_session * changing_session);
int upgrade_MP3(RTP_session * changing_session);

int half_GSM(RTP_session * changing_session);
int half_L16(RTP_session * changing_session);
int half_MP3(RTP_session * changing_session);

int priority_decrease(RTP_session * changing_session);
int priority_increase(RTP_session * changing_session);

int stream_switch(RTP_session * changing_session, media_entry * new_media);

	/**********************************************************************************************************************/
#endif
