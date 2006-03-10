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
#include <sys/ioctl.h>

#include <glib.h>
#include <glib/gprintf.h>

#include <stun/stun.h>
#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

#define FREE_ALL	uint16 idxprev; \
			for(idxprev = 0;idxprev < idx; idxprev++) \
				free(omsss->socks_pair[idxprev]->sock); \
			for(idxprev = 0;idxprev < NUM_SOCKSPAIR; idxprev++) \
				free(omsss->socks_pair[idxprev]); \
			free(omsss->addr_port); \
			free(omsss);


OMSStunServer *
	OMSStunServerInit(uint8 *addr1,uint8 *port1,uint8 *addr2,uint8 *port2)
{
	uint16 idx;
	OMSStunServer *omsss = calloc(1,sizeof(OMSStunServer));

	if(omsss == NULL)
		return NULL;

	omsss->addr_port = calloc(1,sizeof(OMSStunServerConfigPair));

	if(omsss->addr_port == NULL) {
		free(omsss);
		return NULL;
	}

	for(idx = 0; idx < NUM_SOCKSPAIR; idx++) {
		omsss->socks_pair[idx] = (OMSStunSockPair *)calloc(1,sizeof(OMSStunSockPair));
		if(omsss->socks_pair[idx] == NULL) {
			uint16 idxprev;
			for(idxprev = 0;idxprev < idx; idxprev++)
				free(omsss->socks_pair[idxprev]);
			free(omsss->addr_port);
			free(omsss);
			return NULL;
		}
	}
	
	omsss->addr_port->addr[PRIMARY] = g_strdup_printf("%s", addr1);
	omsss->addr_port->addr[SECONDARY] = g_strdup_printf("%s", addr2);
	omsss->addr_port->port[PRIMARY] = g_strdup_printf("%s", port1);
	omsss->addr_port->port[SECONDARY] = g_strdup_printf("%s", port2);
	
	for(idx = 0; idx < NUM_SOCKSPAIR; idx++) {
		int *fd[NUM_SOCKSPAIR];
        	int on=1; /*setting non-blocking flag*/
			
		omsss->socks_pair[idx]->addr_idx = IDX_IDXADDR(idx);
		omsss->socks_pair[idx]->port_idx = IDX_IDXPORT(idx);
		
		/*mmm, i'm not sure to bind all ... chicco*/
		omsss->socks_pair[idx]->sock = Sock_bind(omsss->addr_port->addr[IDX_IDXADDR(idx)],omsss->addr_port->port[IDX_IDXPORT(idx)], fd[idx], UDP, 0);
	
		if (omsss->socks_pair[idx]->sock == NULL) {
			FREE_ALL
			fnc_log(FNC_LOG_ERR,"Error during bind the socket.\n" );
			
			return NULL;

		}
		
		if (Sock_set_props(get_fd(omsss->socks_pair[idx]->sock), FIONBIO, &on) < 0) { /*set to non-blocking*/
			FREE_ALL
			fnc_log(FNC_LOG_ERR,"ioctl() error.\n" );
			
			return NULL;
    		}

	}
	
	return omsss;
}
