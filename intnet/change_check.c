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


