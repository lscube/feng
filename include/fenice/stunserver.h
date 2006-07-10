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

#ifndef _STUNSERVER_H	
#define _STUNSERVER_H
/*RFC-3489*/

#include <fenice/types.h>
#include <netembryo/wsocket.h>
#include <stun/stun.h>

#define OMS_STUN_DEFAULT_PORT1 3478
#define OMS_STUN_DEFAULT_PORT2 3479 /*it shall be changeable*/

#define NUM_SOCKSPAIR 4
typedef struct STUN_SERVER_CONFIG {
	Sock *sock;	
	Sock *change_port;	
	Sock *change_addr;	
	Sock *change_port_addr;	
}OMSStunServerCfg;

typedef struct STUN_SERVER {
	OMSStunServerCfg sock[NUM_SOCKSPAIR];
} OMSStunServer;	
/*
	sock[0] => IP1:1
	sock[1] => IP2:2
	sock[2] => IP2:1
	sock[3] => IP1:2
*/

/*API*/
OMSStunServer *
	OMSStunServerInit(char *addr1, char *port1,char *addr2, char *port2);

void OMSstunserverActions(OMSStunServer *omsss);


/*idx_sock is the index of the received sock*/
void response(OMS_STUN_PKT_DEV *pkt_dev, OMSStunServer *omsss, uint32 idx_sock);
void binding_response(OMS_STUN_PKT_DEV *pkt_dev, OMSStunServer *omsss, uint32 idx_sock);
void binding_error_response(uint32 error_code, OMSStunServer *omsss, uint32 idx_sock);
uint32 get_local_s_addr( Sock * );
uint32 get_remote_s_addr( Sock * );
int find_local_interfaces(uint32 *addresses, int maxRet);

struct STUN_SERVER_IFCFG {
	char a1[16];
	char a2[16];
	char p1[6];
	char p2[6];
};

void *OMSstunserverStart(void *arg);

#endif //_STUNSERVER_H
