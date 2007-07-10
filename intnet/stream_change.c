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

#include <fenice/rtp.h>
#include <fenice/utils.h>
#include <fenice/intnet.h>
#include <string.h>

int stream_change(RTP_session *changing_session, int value)
{
	if (value == 0) {
		return ERR_NOERROR;
	} else if (value == -1) {
		if (changing_session->MinimumReached != 1) {
			changing_session->MaximumReached = 0;
	        	if (strcmp(changing_session->current_media->description.encoding_name,"MPA")==0) return downgrade_MP3(changing_session);
        		if (strcmp(changing_session->current_media->description.encoding_name,"GSM")==0) return downgrade_GSM(changing_session);
        		if (strcmp(changing_session->current_media->description.encoding_name,"L16")==0) return downgrade_L16(changing_session);
		}
	} else if (value == 1) {
		if (changing_session->MaximumReached != 1) {
			changing_session->MinimumReached = 0;
	        	if (strcmp(changing_session->current_media->description.encoding_name,"MPA")==0) return upgrade_MP3(changing_session);
        		if (strcmp(changing_session->current_media->description.encoding_name,"GSM")==0) return upgrade_GSM(changing_session);
		}
	} else if (value == -2) {
		if (changing_session->MinimumReached != 1) {
			changing_session->MaximumReached = 0;
	        	if (strcmp(changing_session->current_media->description.encoding_name,"MPA")==0) return half_MP3(changing_session);
        		if (strcmp(changing_session->current_media->description.encoding_name,"GSM")==0) return half_GSM(changing_session);
        		if (strcmp(changing_session->current_media->description.encoding_name,"L16")==0) return half_L16(changing_session);
		}
	}
	return ERR_NOERROR;
}

