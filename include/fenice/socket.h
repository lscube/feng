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

#ifndef _SOCKETH
#define _SOCKETH	

	typedef unsigned short	u_int16;

	#if defined(__alpha)
		typedef unsigned int	u_int32;
	#else
		typedef unsigned long	u_int32;
	#endif

	#ifndef BYTE_ORDER

		#define LITTLE_ENDIAN	1234
		#define BIG_ENDIAN	4321

		#if defined(sun) || defined(__BIG_ENDIAN) || defined(NET_ENDIAN)
			#define BYTE_ORDER	BIG_ENDIAN
		#else
			#define BYTE_ORDER	LITTLE_ENDIAN
		#endif

	#endif


	#ifdef WIN32
		#include <winsock2.h>
	#endif
	#include <sys/socket.h>

	#ifdef WIN32	
		typedef SOCKET tsocket;
	#else
		typedef int tsocket;
	#endif
		
	char *get_address();
	tsocket tcp_listen(unsigned short port);
	tsocket tcp_accept(tsocket fd);	
	int tcp_read(tsocket fd, void *buffer, int nbytes);
	int tcp_write(tsocket fd, void *buffer, int nbytes);
	void tcp_close(tsocket s);	
	tsocket tcp_connect(unsigned short port,char *addr);
	int udp_connect(unsigned short to_port,struct sockaddr *s_addr,int addr, tsocket *fd);	
	int udp_open(unsigned short port,struct sockaddr *s_addr, tsocket *fd);
	int udp_close(tsocket fd);
#endif
