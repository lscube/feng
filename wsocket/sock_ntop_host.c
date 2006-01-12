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


#include <string.h>
#include <netembryo/wsocket.h>

#ifdef	HAVE_SOCKADDR_DL_STRUCT
# include	<net/if_dl.h>
#endif
#ifdef	AF_UNIX
#include <sys/un.h>
#endif

char *sock_ntop_host(const struct sockaddr *sa,/* socklen_t salen,*/ char *str, size_t len)
{
    // static char str[128];		/* Unix domain is largest */
    // size_t len = sizeof(str);

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, len) == NULL)
			return(NULL);
		return(str);
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, len) == NULL)
			return(NULL);
		return(str);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		/*else
			snprintf(str, len, "%s", unp->sun_path);*/
		return(str);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;

		/*if (sdl->sdl_nlen > 0)
			snprintf(str, len, "%*s",
					 sdl->sdl_nlen, &sdl->sdl_data[0]);
		else
			snprintf(str, len, "AF_LINK, index=%d", sdl->sdl_index);*/
		return(str);
	}
#endif
	default:
		/*snprintf(str, len, "sock_ntop_host: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);*/
		return(str);
	}
    return (NULL);
}


