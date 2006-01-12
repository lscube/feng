/* * 
 *  $Id$
 *  
 *  This file is part of NetEmbryo 
 *
 * NetEmbryo -- default network wrapper 
 *
 *  Copyright (C) 2005 by
 *  	
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 * 
 *  NetEmbryo is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NetEmbryo is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NetEmbryo; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <netembryo/wsocket.h>

int mcast_leave(int sockfd, const struct sockaddr *sa/*, socklen_t salen*/)
{
	switch (sa->sa_family) {
	case AF_INET: {
		struct ip_mreq		mreq;

		memcpy(&mreq.imr_multiaddr,&((struct sockaddr_in *) sa)->sin_addr,sizeof(struct in_addr));
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		return(setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,&mreq, sizeof(mreq)));
	}

#ifdef	IPV6
	case AF_INET6: {
		struct ipv6_mreq	mreq6;

		memcpy(&mreq6.ipv6mr_multiaddr,&((struct sockaddr_in6 *) sa)->sin6_addr,sizeof(struct in6_addr));
		mreq6.ipv6mr_interface = 0;
		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,&mreq6, sizeof(mreq6)));
	}
#endif

	default:
		return  WSOCK_ERRFAMILYUNKNOWN;
	}
}

