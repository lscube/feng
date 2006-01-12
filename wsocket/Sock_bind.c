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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <netembryo/wsocket.h>

Sock * Sock_bind(char *host, char *port, int *sock, enum sock_types sock_type, int ssl_flag)
{
	int res;
	Sock *s;
	struct sockaddr_storage stg;
	socklen_t len=sizeof(struct sockaddr_storage);
#if HAVE_SSL
	SSL *ssl_con=NULL;
	if((ssl_flag & USE_SSL) && sock_type==UDP) {
		/*SSL - UDP NOT ENABLED*/
		fprintf(stderr,"SSL over UDP NOT ENABLED\n");
		return NULL;
	}
	if(ssl_flag & USE_SSL) {
		res = sock_SSL_bind(host, port, sock, sock_type);	
	}
	else
#endif
		res = sock_bind(host, port, sock, sock_type);
		
	if(res!=0)
		return NULL;
	s = calloc(1,sizeof(Sock));
	if (s == NULL)
		return s;
	s->fd = *sock;
	s->socktype = sock_type;
#if HAVE_SSL
	if(ssl_flag & USE_SSL) 
		s->ssl = ssl_con;
#endif
	s->flags |= ssl_flag;
	s->local_port=g_strdup(port);
	
	s->family=sockfd_to_family(*sock);

	if(getsockname(*sock,(struct sockaddr *)&stg,&len) < 0) {
		Sock_close(s);
		return NULL;
	}

	if(is_multicast_address((struct sockaddr *)&stg,s->family)) {
		//fprintf(stderr,"IS MULTICAST\n");
		if(sock_type==TCP) {
			Sock_close(s);
			/*TCP MULTICAST IS IMPOSSIBLE*/
			fprintf(stderr,"TCP MULTICAST IS IMPOSSIBLE\n");
			return NULL;
		}

		if(mcast_join(*sock,(struct sockaddr *)&stg,NULL,0,&(s->addr))!=0) {
			Sock_close(s);
			return NULL;
		}
		s->flags |= IS_MULTICAST;
	}

	memcpy(&(s->sock_stg),&stg,len); /*copy the sockname*/
	
	return s;
}
