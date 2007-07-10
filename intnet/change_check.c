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

#include <fenice/intnet.h>

int change_check(RTP_session *changing_session)
{
	unsigned int res;

	if ((res = RTCP_get_RR_received(changing_session)) == changing_session->PreviousCount) {
		return 0;
	}
	changing_session->PreviousCount = res;
// printf("RTCP received = %d, Fract lost = %f\n", res, RTCP_get_fract_lost(changing_session));
	if (RTCP_get_fract_lost(changing_session) > 0.03) {
 		return -2;
	} else if (RTCP_get_jitter(changing_session) > 10) {
		return -1;
	} else if ((RTCP_get_fract_lost(changing_session) < 0.02) && (RTCP_get_jitter(changing_session) < 10 )) {
		return 1;
	} else {
		return 0;
	}
}


