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
#include <glib.h>
#include <glib/gprintf.h>
#include <sys/types.h>
#include <string.h>
#include <netembryo/wsocket.h>
#if HAVE_SSL
#include <openssl/ssl.h>
#endif

Sock * Sock_accept(Sock *s)
{
	int res;
	struct sockaddr_storage sock_stg;
	char remote_host[128]; /*Unix Domain is largest*/
	int32_t remote_port;
	int32_t local_port;
	Sock * new_s;
	socklen_t salen=sizeof(struct sockaddr_storage);
#if HAVE_SSL
	SSL *ssl_con;
	if(s->flags & USE_SSL)
		res = sock_SSL_accept(&ssl_con,s->fd);
	else
#endif
	res = sock_accept(s->fd);

	if(res < 0)
		return NULL;
	if(getpeername(res,(struct sockaddr *)&sock_stg,&salen)!=0)
	{
#if HAVE_SSL
		if(s->flags & USE_SSL) 
			sock_SSL_close(ssl_con);	
#endif
		printf("Close socket accepted after getpeername\n");
		return NULL;
	}
	if(!sock_ntop_host((struct sockaddr *)&sock_stg, /*sizeof(struct sockaddr_storage),*/ remote_host, sizeof(remote_host)))
		memset(remote_host,0,sizeof(remote_host));

	remote_port=sock_get_port((struct sockaddr *)&sock_stg);
	if(remote_port<0) {
#if HAVE_SSL
		if(s->flags & USE_SSL) 
			sock_SSL_close(ssl_con);
#endif
		printf("Close socket accepted because remote_port<0\n");
		sock_close(res);
		return NULL;
	}


	memset(&sock_stg,0,salen);
	if(getsockname(res,(struct sockaddr *)&sock_stg,&salen)!=0)
	{
#if HAVE_SSL
		if(s->flags & USE_SSL) 
			sock_SSL_close(ssl_con);	
#endif
		return NULL;
	}

	local_port=sock_get_port((struct sockaddr *)&sock_stg);
	if(local_port<0) {
#if HAVE_SSL
		if(s->flags & USE_SSL) 
			sock_SSL_close(ssl_con);
#endif
		printf("Close socket accepted because local_port<0\n");
		sock_close(res);
		return NULL;
	}

	new_s = calloc(1,sizeof(Sock));
	if(!new_s) {
#if HAVE_SSL
		if(s->flags & USE_SSL) 
			sock_SSL_close(ssl_con);
#endif
		sock_close(res);
		return NULL;
	}
#if HAVE_SSL
	if(s->flags & USE_SSL) 
		new_s->ssl=ssl_con;
#endif
	new_s->remote_host=g_strdup(remote_host);
	new_s->fd=res;
	new_s->socktype=s->socktype;
	new_s->flags |= s->flags;

	new_s->remote_port = g_strdup_printf("%d", remote_port);
	new_s->local_port = g_strdup_printf("%d", local_port);

	memcpy(&(new_s->sock_stg),&sock_stg,salen); /*copy the sockname*/
	
	return new_s;
}
