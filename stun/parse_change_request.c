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

#if DEBUG_STUN
	#include <stdio.h> /*printf*/
	#include <arpa/inet.h> /*ntohs*/
#endif

#include <stun/stun.h>

STUNuint32 parse_change_request(OMS_STUN_PKT_DEV *pkt_dev,STUNuint32 idx)
{
#if 0
#if DEBUG_STUN
	printf("idx: %d\n", idx);
	printf("ATR_TYPE: %d\n", ntohs((pkt_dev->stun_pkt.atrs[idx])->stun_atr_hdr.type));
	printf("ATR_LEN: %d\n", ntohs((pkt_dev->stun_pkt.atrs[idx])->stun_atr_hdr.length));
	printf("sizeof ATR_LEN: %d\n", sizeof((pkt_dev->stun_pkt.atrs[idx])));
	printf("flagsAB ntohl: %d\n", ntohl(((struct STUN_ATR_CHANGE_REQUEST *)((pkt_dev->stun_pkt.atrs[idx])->atr))->flagsAB));
#endif
#endif

	return parse_address(pkt_dev,idx);
}
