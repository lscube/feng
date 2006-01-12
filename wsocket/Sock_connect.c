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
#include <glib/gprintf.h>
#include <netembryo/wsocket.h>
#if HAVE_SSL
#include <openssl/ssl.h>
#endif


Sock * Sock_connect(char *host, char *port, int *sock, enum sock_types sock_type, int ssl_flag)
{
	int res;
	Sock *s;
	struct sockaddr_storage sock_stg;
	socklen_t salen =sizeof(struct sockaddr_storage);
	int32_t local_port;
#if HAVE_SSL
	SSL *ssl_con;
	if(ssl_flag & USE_SSL) {
		res = sock_SSL_connect(&ssl_con,host, port, sock, sock_type);	
	}
	else
#endif
		res = sock_connect(host, port, sock, sock_type);
		
	if(res!=0) { 
		fprintf(stderr,"Sock_connect res!=0\n");
		return NULL;
	}
	if(getsockname(*sock,(struct sockaddr *)&sock_stg,&salen)!=0)
	{
#if HAVE_SSL
		if(ssl_flag & USE_SSL) 
			sock_SSL_close(ssl_con);	
#endif
		fprintf(stderr,"Sock_connect getsockname error\n");
		return NULL;
	}
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

	s->remote_host=g_strdup(host);
	s->remote_port=g_strdup(port);
	s->family=sockfd_to_family(*sock);

	local_port=sock_get_port((struct sockaddr *)&sock_stg);
	s->local_port = g_strdup_printf("%d", local_port);

	memset(&sock_stg,0,salen);

	if(getpeername(*sock,(struct sockaddr *)&sock_stg,&salen) < 0) {
		Sock_close(s);
		return NULL;
	}

	if(is_multicast_address((struct sockaddr *)&sock_stg,s->family)) {
		//fprintf(stderr,"IS MULTICAST\n");
		if(sock_type==TCP) {
			Sock_close(s);
			/*TCP MULTICAST IS IMPOSSIBLE*/
			return NULL;
		}

		if(mcast_join(*sock,(struct sockaddr *)&sock_stg,NULL,0,&(s->addr))!=0) {
			Sock_close(s);
			return NULL;
		}
		s->flags |= IS_MULTICAST;
	}
	memcpy(&(s->sock_stg),&sock_stg,salen); /*copy the peername*/

	return s;
}
