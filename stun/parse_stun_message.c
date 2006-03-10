/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
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
#include <string.h>

#include <stun/stun.h>

OMS_STUN_PKT_DEV *parse_stun_message(uint8 *pkt, uint32 pktsize)
{

	struct STUN_HEADER *stun_hdr;
	OMS_STUN_PKT_DEV *pkt_dev;
	uint32 bytes_readed = 0;

	if(pktsize < 20) {
		/*header is 20 bytes length*/ 
		return NULL;
	}

	/*map STUN_HEADER*/
	stun_hdr = (struct STUN_HEADER *)pkt;	

	switch(stun_hdr->msgtype) {
		case BINDING_REQUEST:
		break;
		case BINDING_RESPONSE:
		break;
		case BINDING_ERROR_RESPONSE:
		break;
		case SHARED_SECRET_REQUEST:
			/*not implemented yet*/
			return NULL;
		break;
		case SHARED_SECRET_RESPONSE:
			/*not implemented yet*/
			return NULL;
		break;
		case SHARED_SECRET_ERROR_RESPONSE:
			/*not implemented yet*/
			return NULL;
		break;
		default:
			return NULL;
		break;
	}
	if(stun_hdr->msglen != pktsize - 20) {/*malformed message*/
		return NULL;
	}

	bytes_readed += 20;

	pkt_dev = calloc(1,sizeof(OMS_STUN_PKT_DEV));
	memcpy(pkt_dev,pkt,pktsize);
	if(stun_hdr->msglen == 0) {/*no attibutes present*/
		pkt_dev->set_atr_mask = 0;
		pkt_dev->num_message_atrs = 0;
		return pkt_dev;
	}

	/*....continue parsing all attributes...*/
	/*....*/
		
	return pkt_dev;
}
