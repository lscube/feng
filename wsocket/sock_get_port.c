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
 * this piece of code is taken from NeMeSI
 * */


#include <sys/types.h>
#include <netembryo/wsocket.h>

int32_t sock_get_port(const struct sockaddr *sa)
{
	switch (sa->sa_family) {
		case AF_INET: {
			struct sockaddr_in	*sin = (struct sockaddr_in *) sa;
	
			return(sin->sin_port);
		}

#ifdef	IPV6
		case AF_INET6: {
			struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;
	
			return(sin6->sin6_port);
		}
#endif
	}

	return -1;
}
