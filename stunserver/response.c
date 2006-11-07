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

#include <stun/stun.h>
#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

void response(OMS_STUN_PKT_DEV *pkt_dev, OMSStunServer *omsss, uint32 idx_sock, struct sockaddr_storage stg)
{
	if(ntohs((pkt_dev->stun_pkt).stun_hdr.msgtype) != BINDING_REQUEST) {
		fnc_log(FNC_LOG_DEBUG,"send STUN_BAD_REQUEST\n");	
		binding_error_response(STUN_BAD_REQUEST,omsss, idx_sock);
	}
	else if(pkt_dev->num_unknown_atrs > 0) {
		fnc_log(FNC_LOG_DEBUG,"send STUN_UNKNOW_ATTRIBUTE\n");	
		binding_error_response(STUN_UNKNOWN_ATTRIBUTE,omsss,idx_sock);
	} 
	else {
		fnc_log(FNC_LOG_DEBUG,"send response to idx=%d\n",idx_sock);	
		binding_response(pkt_dev,omsss,idx_sock,stg);
	}
}

