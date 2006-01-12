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
#include <net/if.h>
#include <sys/ioctl.h>

/* On success, zero is returned*/
int mcast_join (int sockfd, const struct sockaddr *sa/*, socklen_t salen*/,const char *ifname, unsigned int ifindex, union ADDR *addr)
{
	switch (sa->sa_family) {
	case AF_INET: {
		/*struct ip_mreq		mreq;*/
		struct ifreq		ifreq;
		
		memcpy(&(*addr).mreq_in.ipv4_multiaddr,&((struct sockaddr_in *) sa)->sin_addr,sizeof(struct in_addr));

		if (ifindex > 0) {
			if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
				return WSOCK_ERRORINTERFACE;
			}
			(*addr).mreq_in.ipv4_interface=ifindex;
			goto doioctl;
		} else if (ifname != NULL) {
			strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
			(*addr).mreq_in.ipv4_interface=if_nametoindex(ifname);
doioctl:
			if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
				return WSOCK_ERRORIOCTL;
			memcpy(&(*addr).mreq_in.imr_interface4,&((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,sizeof(struct in_addr));
		} else {
			(*addr).mreq_in.imr_interface4.s_addr = htonl(INADDR_ANY);
			(*addr).mreq_in.ipv4_interface=0;
		}
		return(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&(*addr).mreq_in.NETmreq, sizeof((*addr).mreq_in.NETmreq)));
	}
#ifdef	IPV6
	case AF_INET6: {
		/*struct ipv6_mreq	mreq6;*/
		struct ifreq		ifreq;

		memcpy(&(*addr).mreq_in6.ipv6_multiaddr,&((struct sockaddr_in6 *) sa)->sin6_addr,sizeof(struct in6_addr));

		if (ifindex > 0)
			(*addr).mreq_in6.ipv6_interface=ifindex;
		else if (ifname != NULL) {
			if ( ((*addr).mreq_in6.ipv6_interface = if_nametoindex(ifname)) == 0) {
				return WSOCK_ERRORINTERFACE;
			}
			
			/*this is right? Use getsockname instead?*/
			if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
				return WSOCK_ERRORIOCTL;
			memcpy(&(*addr).mreq_in6.imr_interface6,&((struct sockaddr_in6 *) &ifreq.ifr_addr)->sin6_addr,sizeof(struct in6_addr));
			/*???*/
		}
		else {
			//(*addr).mreq_in6.imr_interface6.s6_addr32 = htonl(INADDR_ANY);
			(*addr).mreq_in6.ipv6_interface = 0;
		}

		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,&(*addr).mreq_in6.NETmreq6, sizeof((*addr).mreq_in6.NETmreq6)));
	}
#endif
	default:
		return WSOCK_ERRFAMILYUNKNOWN;
	}

}
