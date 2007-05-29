/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h> /*htons*/
#include <stun/stun.h>

STUNuint32 parse_stun_message(STUNuint8 *pkt, STUNuint32 pktsize,
				OMS_STUN_PKT_DEV **pkt_dev_pt)
{

	struct STUN_HEADER *stun_hdr;
	STUNuint32 bytes_read = 0;
	OMS_STUN_PKT_DEV *pkt_dev;

	if(pktsize < sizeof(struct STUN_HEADER)) { /*malformed message*/
		/*header is 20 bytes length*/ 
		return STUN_BAD_REQUEST;
	}

	/*map STUN_HEADER*/
	stun_hdr = (struct STUN_HEADER *)pkt;	
	

	switch(ntohs(stun_hdr->msgtype)) {
		case BINDING_REQUEST:
//#if DEBUG_STUN
//		fprintf(stderr,">>>received BINDING_REQUEST\n");			
//#endif
		break;
		case BINDING_RESPONSE:
//#if DEBUG_STUN
//			fprintf(stderr,">>>received BINDING_RESPONSE\n");			
//#endif
		break;
		case BINDING_ERROR_RESPONSE:
//#if DEBUG_STUN
//			fprintf(stderr,">>>received BINDING_ERROR_RESPONSE\n");			
//#endif
		break;
		case SHARED_SECRET_REQUEST:
			/*not implemented yet*/
			return STUN_GLOBAL_FAILURE;
		break;
		case SHARED_SECRET_RESPONSE:
			/*not implemented yet*/
			return STUN_GLOBAL_FAILURE;
		break;
		case SHARED_SECRET_ERROR_RESPONSE:
			/*not implemented yet*/
			return STUN_GLOBAL_FAILURE;
		break;
		default:
			return STUN_BAD_REQUEST;
		break;
	}

#if 0
#if DEBUG_STUN
	printf("stun_hdr \
			\tmsgtype: %d\n \
			\t\tmsglen: %d\n \
			\t\tpktsize: %d\n \
			\t\tsizeof STUN_HEADER: %d\n", ntohs(stun_hdr->msgtype), ntohs(stun_hdr->msglen),pktsize,  sizeof(struct STUN_HEADER));
#endif
#endif //0

	if(ntohs(stun_hdr->msglen) != pktsize - sizeof(struct STUN_HEADER)) {/*malformed message*/		
		return STUN_BAD_REQUEST;
	}


	pkt_dev = calloc(1,sizeof(OMS_STUN_PKT_DEV));
	
	/*copy header*/
	memcpy(&(pkt_dev->stun_pkt.stun_hdr), pkt, sizeof(struct STUN_HEADER));

	if(ntohs(stun_hdr->msglen) == 0) {/*no attibutes present*/
		pkt_dev->num_message_atrs = 0;
		*pkt_dev_pt = pkt_dev;
		return 0;
	}

	/*....continue adding all attributes...*/
	bytes_read += sizeof(struct STUN_HEADER);
	while(bytes_read < pktsize && 
			pkt_dev->num_message_atrs <= STUN_MAX_MESSAGE_ATRS) {
	
		STUNuint16 len = 0;
		
		pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs] = 
			calloc(1,sizeof(stun_atr));
		/*copy atr_hdr (attribute header)*/	
		memcpy(&((pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs])->stun_atr_hdr),
				pkt+bytes_read,sizeof(struct STUN_ATR_HEADER));
		
		bytes_read += sizeof(struct STUN_ATR_HEADER);
		
		/*read Attribute Length*/
		len = ntohs((pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs])->stun_atr_hdr.length);
		
		/*allocate the right space where i will copy the 
		 * Attribute Value*/
		pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs]->atr =
								calloc(1,len);
		
		/*copy Attribute Value*/
		memcpy(pkt_dev->stun_pkt.atrs[pkt_dev->num_message_atrs]->atr,
				pkt+bytes_read,len);
		
		bytes_read += len;

		/*increment the number of attributes*/
		pkt_dev->num_message_atrs++;
	}
		
	*pkt_dev_pt = pkt_dev;
	
	return parse_atrs(pkt_dev);
}
