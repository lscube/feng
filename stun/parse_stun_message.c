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

uint32 parse_stun_message(uint8 *pkt, uint32 pktsize,
				OMS_STUN_PKT_DEV **pkt_dev_pt)
{

	struct STUN_HEADER *stun_hdr;
	uint32 bytes_readed = 0;
	OMS_STUN_PKT_DEV *pkt_dev;

	if(pktsize < 20) {
		/*header is 20 bytes length*/ 
		return STUN_BAD_REQUEST_CODE;
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
			return STUN_GLOBAL_FAILURE_CODE;
		break;
		case SHARED_SECRET_RESPONSE:
			/*not implemented yet*/
			return STUN_GLOBAL_FAILURE_CODE;
		break;
		case SHARED_SECRET_ERROR_RESPONSE:
			/*not implemented yet*/
			return STUN_GLOBAL_FAILURE_CODE;
		break;
		default:
			return STUN_BAD_REQUEST_CODE;
		break;
	}
	if(stun_hdr->msglen != pktsize - 20) {/*malformed message*/
		return STUN_BAD_REQUEST_CODE;
	}


	pkt_dev = calloc(1,sizeof(OMS_STUN_PKT_DEV));
	
	if(stun_hdr->msglen == 0) {/*no attibutes present*/
		memcpy(pkt_dev,pkt,pktsize);
		pkt_dev->set_atr_mask = 0;
		pkt_dev->num_message_atrs = 0;
		*pkt_dev_pt = pkt_dev;
		return 0;
	}

	/*....continue parsing all attributes...*/
	/*copy header*/
	memcpy(pkt_dev,pkt,20);
	bytes_readed += 20;
	while(bytes_readed < pktsize && 
			pkt_dev->num_message_atrs <= STUN_MAX_MESSAGE_ATRS) {
	
		uint16 len = 0;
		
		pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs] = 
			calloc(1,sizeof(stun_atr));	
		/*copy atr_hdr (attribute header)*/	
		memcpy(&((pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs])->stun_atr_hdr),
				pkt+bytes_readed,4);
		
		bytes_readed += 4;
		
		/*read Attribute Length*/
		len = (pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs])->stun_atr_hdr.length;
		
		/*allocate the right space where i will copy the 
		 * Attribute Value*/
		pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs]->atr =
								calloc(1,len);
		
		/*copy Attribute Value*/
		memcpy(pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs]->atr,
				pkt+bytes_readed,len);
		
		bytes_readed += len;

		/*increment the number of attributes*/
		pkt_dev->num_message_atrs++;
	}
		
	*pkt_dev_pt = pkt_dev;
	
	return 0;
}
