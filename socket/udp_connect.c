/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
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

#include <fenice/socket.h>
#include <fenice/utils.h>
#ifndef WIN32
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <sys/ioctl.h>
#else
	#include <io.h>	
#endif
              
int udp_connect(unsigned short to_port, struct sockaddr *s_addr, int addr, tsocket *fd)
{
	struct sockaddr_in s;
	int on = 1; //,v=1;
	if (!*fd) {
		
		if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf( "socket() error in udp_connect.\n" );
		    	return ERR_GENERIC;
    		}
    		// set to non-blocking
    		if (ioctl(*fd, FIONBIO, &on) < 0) {
			printf( "ioctl() error in udp_connect.\n" );
    	  		return ERR_GENERIC;
    		}
	}
    	s.sin_family = AF_INET;
    	s.sin_addr.s_addr = addr;
    	s.sin_port = htons(to_port);
    	if (connect(*fd,(struct sockaddr*)&s,sizeof(s))<0) {
		printf( "connect() error in udp_connect.\n" );
    		return ERR_GENERIC;
    	}
    	*s_addr=*((struct sockaddr*)&s);

	return ERR_NOERROR;
}

