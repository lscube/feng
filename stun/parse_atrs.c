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


STUNuint32 parse_atrs(OMS_STUN_PKT_DEV *pkt_dev)
{
	STUNuint16 idx;
	STUNuint32 ret = 0;

	for(idx=0;idx < pkt_dev->num_message_atrs; idx++) {
		switch(pkt_dev->stun_pkt.atrs[idx]->stun_atr_hdr.type) 
		{
			case MAPPED_ADDRESS:
				ret = parse_mapped_address(pkt_dev,idx);
			break;
			case RESPONSE_ADDRESS:
				ret = parse_response_address(pkt_dev,idx);
			break;
			case CHANGE_REQUEST:
				ret = parse_change_request(pkt_dev,idx);
			break;
			case SOURCE_ADDRESS:
				ret = parse_source_address(pkt_dev,idx);
			break;
			case CHANGED_ADDRESS:
				ret = parse_changed_address(pkt_dev,idx);
			break;
			case USERNAME:
				ret = parse_username(pkt_dev,idx);
			break;
			case PASSWORD:
				ret = parse_password(pkt_dev,idx);
			break;
			case MESSAGE_INTEGRITY:
				ret = parse_message_integrity(pkt_dev,idx);
			break;
			case ERROR_CODE:
				ret = parse_error_code(pkt_dev,idx);
			break;
			case UNKNOWN_ATTRIBUTES:
				ret = parse_unknown_attribute(pkt_dev,idx);
			break;
			case REFLECTED_FROM:
				ret = parse_reflected_from(pkt_dev,idx);
			break;
			default :
				//STUN_UNKNOWN_ATTRIBUTE_CODE;
				pkt_dev->list_unknown[idx] = 1;
				pkt_dev->num_unknown_atrs++;
			break;
		
		}
		
		if(ret!=0)
			return ret;
		
		if(!SET_ATRMASK((pkt_dev->stun_pkt.atrs[idx])->stun_atr_hdr.type, pkt_dev->set_atr_mask)) {
			fprintf(stderr,"\tThere is an unknown attribute type. Idx: %d\n",idx);
		}

	}
		

	return 0;
}

