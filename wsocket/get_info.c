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

#include <glib.h>
#include <netembryo/wsocket.h>
#include <netinet/in.h>
#include <netdb.h> // for getnameinfo()

inline char * get_remote_host(Sock *s)
{
	return s->remote_host;
}

char * get_local_host(Sock *s)
{
	char local_host[128]; /*Unix domain is largest*/

	return addr_ntop(s, local_host, sizeof(local_host));
}

int get_local_hostname(Sock *s, char *localhostname) //return 0 if ok
{
	char lhname[NI_MAXHOST];
	size_t/*socklen_t*/ localaddr_len = sizeof(s->sock_stg);
	int res;
	
	res=getnameinfo((struct sockaddr *)&(s->sock_stg), localaddr_len, lhname, sizeof(lhname), NULL, 0, 0);
	if(!res)
		localhostname = g_strdup(lhname);

	return res;
}

inline char * get_remote_port(Sock *s)
{
	return s->remote_port;
}

inline char * get_local_port(Sock *s)
{
	return s->local_port;
}

