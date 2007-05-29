/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 * 
 *  - (LS) Team			<team@streaming.polito.it>	
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
#include <sys/ioctl.h>
#include <glib.h>
#include <stun/stun.h>

#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>


OMSStunServer *
	OMSStunServerInit(char *addr1, char *port1,char *addr2,char *port2)
{
	OMSStunServer *omsss;
	int i = 0;
	int on = 1;
	int nif = 0;
	uint32 ai[10];
	char *tmp;
	uint16 found_1 = 0, found_2 = 0;

	fnc_log(FNC_LOG_DEBUG,"OMSStunServerInit args: %s,%s,%s,%s\n",addr1,port1,addr2,port2);

	nif=find_local_interfaces(ai,10);

	while(i<nif) {
		tmp = g_strdup_printf("%d.%d.%d.%d", ((ai[i]>>24)&0xFF),((ai[i]>>16)&0xFF),((ai[i]>> 8)&0xFF),((ai[i])&0xFF));			
		i++;
		if ( strcmp(tmp, g_strdup( addr1)) == 0 ) { 
			fnc_log(FNC_LOG_DEBUG,"Found Stun Addr1\n" );
			found_1 = 1;
		}
		else if ( strcmp(tmp,  g_strdup(addr2)) == 0 ) {
			fnc_log(FNC_LOG_DEBUG,"Found Stun Addr2\n" );
			found_2 = 1;
		}
	}

	if ( found_1 == 0 || found_2 == 0 ) {
		fnc_log(FNC_LOG_ERR,"Let control ethernet interface configurations\n" );
		
		return NULL;
	}
		

	omsss = calloc(1,sizeof(OMSStunServer));
	if(omsss == NULL)
		return NULL;

	((omsss->sock)[0]).sock = Sock_bind (g_strdup(addr1),g_strdup(port1),UDP,0);
	((omsss->sock)[1]).sock = Sock_bind (g_strdup(addr2),g_strdup(port2),UDP,0);
	((omsss->sock)[2]).sock = Sock_bind (g_strdup(addr2),g_strdup(port1),UDP,0);
	((omsss->sock)[3]).sock = Sock_bind (g_strdup(addr1),g_strdup(port2),UDP,0);
	
	for(i=0;i<4;i++) {
		if ( ((omsss->sock)[i]).sock == NULL) {
			fnc_log(FNC_LOG_ERR,"Error during binding. Let control ethernet interface configurations\n" );
			free(omsss);
			return NULL;
		}
		if (Sock_set_props(((omsss->sock)[i]).sock, FIONBIO, &on) < 0) { /*set to non-blocking*/
			fnc_log(FNC_LOG_ERR,"ioctl() error.\n" );
			free(omsss);
			return NULL;
    		}
	
	}


	((omsss->sock)[0]).change_port = ((omsss->sock)[3]).sock;
	((omsss->sock)[0]).change_addr = ((omsss->sock)[2]).sock;
	((omsss->sock)[0]).change_port_addr = ((omsss->sock)[1]).sock;

	
	((omsss->sock)[1]).change_port = ((omsss->sock)[2]).sock;
	((omsss->sock)[1]).change_addr = ((omsss->sock)[3]).sock;
	((omsss->sock)[1]).change_port_addr = ((omsss->sock)[0]).sock;

	((omsss->sock)[2]).change_port = ((omsss->sock)[1]).sock;
	((omsss->sock)[2]).change_addr = ((omsss->sock)[0]).sock;
	((omsss->sock)[2]).change_port_addr = ((omsss->sock)[3]).sock;

	((omsss->sock)[3]).change_port = ((omsss->sock)[0]).sock;
	((omsss->sock)[3]).change_addr = ((omsss->sock)[1]).sock;
	((omsss->sock)[3]).change_port_addr = ((omsss->sock)[2]).sock;

	return omsss;
}


