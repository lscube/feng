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
              
/* Return the IP address of this machine. */
char *get_address()
{
  static char     Ip[256];
  char server[256];
  u_char          addr1, addr2, addr3, addr4, temp;
  u_long          InAddr;
  struct hostent *host;
  
  gethostname(server,256);
  host = gethostbyname(server);

  temp = 0;
  InAddr = *(u_int32 *) host->h_addr;
  addr4 = (unsigned char) ((InAddr & 0xFF000000) >> 0x18);
  addr3 = (unsigned char) ((InAddr & 0x00FF0000) >> 0x10);
  addr2 = (unsigned char) ((InAddr & 0x0000FF00) >> 0x8);
  addr1 = (unsigned char) (InAddr & 0x000000FF);

#if (BYTE_ORDER == BIG_ENDIAN)
  temp = addr1;
  addr1 = addr4;
  addr4 = temp;
  temp = addr2;
  addr2 = addr3;
  addr3 = temp;
#endif

  sprintf(Ip, "%d.%d.%d.%d", addr1, addr2, addr3, addr4);

  return Ip;
}


