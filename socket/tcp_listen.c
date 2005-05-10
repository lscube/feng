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

#include <fenice/socket.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

tsocket tcp_listen(unsigned short port)
{
	tsocket f;
	int on=1;
	
    struct sockaddr_in s;
    int v = 1;

    if ((f = socket(AF_INET, SOCK_STREAM, 0))<0) {
		fnc_log(FNC_LOG_ERR,"socket() error in tcp_listen.\n" );
		return ERR_GENERIC;
    }

    setsockopt(f, SOL_SOCKET, SO_REUSEADDR, (char *) &v, sizeof(int));

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    if (bind (f, (struct sockaddr *)&s, sizeof (s))) {
		fnc_log(FNC_LOG_ERR,"bind() error in tcp_listen" );
		return ERR_GENERIC;
    }

    // set to non-blocking
    if (ioctl(f, FIONBIO, &on) < 0) {
		fnc_log(FNC_LOG_ERR,"ioctl() error in tcp_listen.\n" );
      	return ERR_GENERIC;
    }	

    if (listen(f, SOMAXCONN) < 0) {
		fnc_log(FNC_LOG_ERR,"listen() error in tcp_listen.\n" );
		return ERR_GENERIC;
    }

    return f;
}

