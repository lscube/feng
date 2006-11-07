/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS) Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <time.h>
#include <stun/stun.h>
#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

void OMSstunserverActions(OMSStunServer *omsss)
{
	OMS_STUN_PKT_DEV *pkt_dev = NULL;
	// Fake timespec for fake nanosleep. See below.
	struct timespec ts = { 0, 0 };
	int32 n = 0 ;
	uint8 buffer[STUN_MAX_MESSAGE_SIZE];
	uint32 buffer_size = STUN_MAX_MESSAGE_SIZE;
	uint32 ret = 0;
	struct sockaddr_storage stg;

	while (1) {
		n = 0;
		if ( (n = Sock_read( (omsss->sock[0]).sock, buffer, buffer_size, &stg)) > 0 ) { 
			if ( (ret = parse_stun_message(buffer, n, &pkt_dev) ) != 0 ) {
				binding_error_response(ret, omsss,0);
			}
			else
				response(pkt_dev, omsss, 0, stg);
		}
		else if ( (n = Sock_read( (omsss->sock[2]).sock, buffer, buffer_size, &stg)) > 0 ) { 
			if ( (ret = parse_stun_message(buffer, n, &pkt_dev) ) != 0 ) {
				binding_error_response(ret, omsss,2);
			}
			else
				response(pkt_dev, omsss, 2, stg);
		}
		// Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
		nanosleep(&ts, NULL);
		
		if (pkt_dev != NULL)
			free_pkt_dev(pkt_dev);
		pkt_dev = NULL;
	}
}
