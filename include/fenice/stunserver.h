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

#ifndef _STUNSERVERH	
#define _STUNSERVERH
/*RFC-3489*/

#include <fenice/types.h>
#include <netembryo/wsocket.h>
#include <stun/stun.h>

#define STUN_DEFAULT_PORT1 3478
#define OMS_STUN_DEFAULT_PORT2 3479 /*it shall be changeable*/


#define PRIMARY 0
#define SECONDARY 1
#define CHANGE_IDX(idx) ((++idx)%2) /*idx of port or addr*/
typedef struct STUN_SERVER_CONFIG_PAIR {
	char *addr[2];/*addr[PRIMARY] and addr[SECONDARY]*/
        char *port[2];/*port[PRIMARY] and port[SECONDARY]*/
} OMSStunServerConfigPair;

typedef struct STUN_SOCK_PAIR {
	uint16 addr_idx; /* = PRIMARY or SECONDARY*/
	uint16 port_idx; /* = PRIMARY or SECONDARY*/
	Sock *sock;
} OMSStunSockPair;

#define NUM_SOCKSPAIR 4
typedef struct STUN_SERVER {
	OMSStunSockPair *socks_pair[NUM_SOCKSPAIR];	/*receives from:*/
							/*socks[0], socks[1]*/
		  	  	  			/*sends to all*/
	OMSStunServerConfigPair *addr_port;
} OMSStunServer;	
/*macro to map (addr_idx,port_idx) to socks_idx*/
#define GET_SOCKSPAIR_IDX(idx_addr, idx_port) \
			(2 *idx_addr + idx_port)
/*map:
 * addr	port socks_idx
 * 0	0	0
 * 0	1	1
 * 1	0	2
 * 1	1	3
 * */
#define SOCKSPAIR_IDX(s) \
	GET_SOCKSPAIR_IDX(s->addr_idx, s->port_idx)

/*from socks_idx to addr_idx or port_idx*/
#define IDX_IDXADDR(idx) ((idx<2)?0:1)
#define IDX_IDXPORT(idx) (idx%2)

/*API*/
OMSStunServer *
	OMSStunServerInit(uint8 *addr1,uint8 *port1,uint8 *addr2,uint8 *port2);

uint32 OMSstunserverActions(OMSStunServer *omsss, uint16 socks_pair_idx);


uint32 binding_response(OMS_STUN_PKT_DEV *pkt_dev, uint8 **buffer);

uint32 binding_error_response(OMS_STUN_PKT_DEV *pkt_dev, 
				uint32 error_code, uint8 **buffer);

#endif //_STUNSERVERH
