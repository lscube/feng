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

#include <stun/stun.h>
#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

static int send_msg(OMSStunServer *omsss, uint16 socks_pair_idx, uint8 *msg, uint32 msgsize)
{
	int ret;
	ret = Sock_connect_by_sock(omsss->socks_pair[socks_pair_idx]->sock,
				get_remote_host(omsss->socks_pair[socks_pair_idx]->sock),
				get_remote_port(omsss->socks_pair[socks_pair_idx]->sock));	
	/*if ret== -1 not binded socket. Use Sock_connect instead*/
	if(ret == 0) 
		Sock_write(omsss->socks_pair[socks_pair_idx]->sock,msg,msgsize);
	else
		return ERR_GENERIC; 
	return 0;
}

int32 OMSstunserverActions(OMSStunServer *omsss, uint16 socks_pair_idx)
{
	OMS_STUN_PKT_DEV *pkt_dev = NULL;
	uint32 ret;
	uint8 *msg;
	uint32 msgsize;
	uint8 pkt[STUN_MAX_MESSAGE_SIZE];
	int32 pktsize;

	pktsize = Sock_read(omsss->socks_pair[socks_pair_idx]->sock,pkt,STUN_MAX_MESSAGE_SIZE);
	
	if(pktsize < 0) 
		return ERR_GENERIC; 

	ret = parse_stun_message(pkt, pktsize, &pkt_dev);
	if(ret != 0) {
		/*BAD_ERROR_CODE or GLOBAL_ERROR_CODE*/
		msgsize = binding_error_response(pkt_dev,ret,&msg);

		if(msgsize > 0) {
			int retsmsg;
			if((retsmsg=send_msg(omsss,socks_pair_idx,msg,msgsize)) != 0)
				ret = retsmsg; 
		}
		else
			ret = ERR_GENERIC; 
		
		free_pkt_dev(pkt_dev);
		return ret;
	}

	ret = parse_atrs(pkt_dev);
	if( ret!=0 ) {
		/*mmm malformed message*/
		/*BAD_ERROR_CODE or GLOBAL_ERROR_CODE*/
		msgsize = binding_error_response(pkt_dev,ret,&msg);

		if(msgsize > 0) {
			int retsmsg;
			if((retsmsg=send_msg(omsss,socks_pair_idx,msg,msgsize)) != 0)
				ret = retsmsg; 
		}
		else
			ret = ERR_GENERIC; 


		free_pkt_dev(pkt_dev);
		return ret;
	}

	if(pkt_dev->num_unknown_atrs > 0) {
		msgsize = binding_error_response(pkt_dev,ret,&msg);
		if(msgsize > 0) {
			int retsmsg;
			if((retsmsg=send_msg(omsss,socks_pair_idx,msg,msgsize)) != 0)
				ret = retsmsg; 
		}
		else
			ret = ERR_GENERIC; 
		
		free_pkt_dev(pkt_dev);
		return ret;
	}
	
	msgsize = binding_response(pkt_dev,&msg);
	
	 if(msgsize > 0) {
		int retsmsg;
		if((retsmsg=send_msg(omsss,socks_pair_idx,msg,msgsize)) != 0) 
			ret = retsmsg;
		/*MISTAKE. Select the right sock (change IP and/or PORT ??!!!)*/
	}
	else
		ret = ERR_GENERIC; 
	

	free_pkt_dev(pkt_dev);
	
	return 0;
}
