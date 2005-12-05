/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2005 by
 *  	
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- (LS)³			<team@streaming.polito.it>
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

#include <glib.h>
#include <fenice/wsocket.h>
#include <netinet/in.h>

inline char * get_remote_host(Sock *s)
{
	return s->remote_host;
}

char * get_local_host(Sock *s)
{
	char local_host[128]; /*Unix domain is largest*/

	return addr_ntop(s, local_host, sizeof(local_host));
}

inline char * get_remote_port(Sock *s)
{
	return s->remote_port;
}

inline char * get_local_port(Sock *s)
{
	return s->local_port;
}

