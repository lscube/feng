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
//#include <string.h>
#include <sys/ioctl.h>
#include <glib.h>
//#include <glib/gprintf.h>
#include <stun/stun.h>

#include <fenice/stunserver.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>


OMSStunServer *
	OMSStunServerInit(char *addr1, char *port1,char *addr2,char *port2)
{
	OMSStunServer *omsss = calloc(1,sizeof(OMSStunServer));
	int *fd;
	int i;
	int on = 1;

	if(omsss == NULL)
		return NULL;
	//NOTE: g_strdup_printf
	((omsss->sock)[0]).sock = Sock_bind (g_strdup(addr1),g_strdup(port1),fd,UDP,0);
	((omsss->sock)[1]).sock = Sock_bind (g_strdup(addr2),g_strdup(port2),fd,UDP,0);
	((omsss->sock)[2]).sock = Sock_bind (g_strdup(addr2),g_strdup(port1),fd,UDP,0);
	((omsss->sock)[3]).sock = Sock_bind (g_strdup(addr1),g_strdup(port2),fd,UDP,0);
	
	for(i=0;i<4;i++) {
		//printf("local port: %d\n", atoi(get_local_port( ((omsss->sock)[i]).sock )) );

		if ( ((omsss->sock)[i]).sock == NULL) {
			fnc_log(FNC_LOG_ERR,"Error during binding. Control ethernet interface configurations\n" );
			free(omsss);
			return NULL;
		}
		if (Sock_set_props( get_fd(((omsss->sock)[i]).sock), FIONBIO, &on) < 0) { /*set to non-blocking*/
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


