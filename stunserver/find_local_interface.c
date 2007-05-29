/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/socket.h>

#include <fenice/stunserver.h>

int find_local_interfaces(uint32 *addresses, int maxRet)
{
	struct ifconf ifc;
	int s = socket( AF_INET, SOCK_DGRAM, 0 );
	int len = 100 * sizeof(struct ifreq);
	char buf[ len ];
	ifc.ifc_len = len;
	ifc.ifc_buf = buf;
	int e = ioctl(s,SIOCGIFCONF,&ifc);
	char *ptr = buf;
	int tl = ifc.ifc_len;
	int count=0;
	uint32 ai;
	
	while ( (tl > 0) && ( count < maxRet) ) {
		struct ifreq* ifr = (struct ifreq *)ptr;
		int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
		struct ifreq ifr2;
		struct sockaddr a;
		struct sockaddr_in* addr;

		tl -= si;
		ptr += si;
		//char *name = ifr->ifr_ifrn.ifrn_name contains eth0, eth1 ecc...*/
  	
		ifr2 = *ifr;
  	
		e = ioctl(s,SIOCGIFADDR,&ifr2);
		if ( e == -1 ) 
			break;
  	
		a = ifr2.ifr_addr;
		addr = (struct sockaddr_in*) &a;
  	
		ai = ntohl( addr->sin_addr.s_addr );
		if ((int)((ai>>24)&0xFF) != 127) 
			addresses[count++] = ai;
		
#if 0
      cerr << "Detected interface "
           << int((ai>>24)&0xFF) << "." 
           << int((ai>>16)&0xFF) << "." 
           << int((ai>> 8)&0xFF) << "." 
           << int((ai    )&0xFF) << endl;
#endif
	}
	//closesocket(s);
	return count;
}

