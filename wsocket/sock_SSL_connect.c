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

#include <stdio.h>
#include <openssl/ssl.h>
#include <netembryo/wsocket.h>

int sock_SSL_connect(SSL **ssl_con,char *host, char *port, int *sock, enum sock_types sock_type)
{
	int ssl_err;
	SSL_CTX *ssl_ctx = NULL;
	
	if(sock_connect(host,port,sock,sock_type)!=0) {
		fprintf(stderr,"sock_SSL_connect: !sock_connect\n");
		return WSOCK_ERROR;
	}

	ssl_ctx = SSL_CTX_new(SSLv3_client_method());
	if(!ssl_ctx) {
		fprintf(stderr,"sock_SSL_connect: !ssl_ctx\n");
		return WSOCK_ERROR;
	}

	*ssl_con = SSL_new(ssl_ctx);
	if(!(*ssl_con)) {
		fprintf(stderr,"sock_SSL_connect: !ssl_con\n");
		SSL_CTX_free(ssl_ctx);
		return WSOCK_ERROR;
	}
	
	SSL_set_fd (*ssl_con, *sock);
	SSL_set_connect_state(*ssl_con);
	ssl_err = SSL_connect(*ssl_con);

	if(ssl_err<0) 
		SSL_set_shutdown(*ssl_con,SSL_SENT_SHUTDOWN);
	if(ssl_err<=0) {
		fprintf(stderr,"sock_SSL_connect: ssl_err<=0\n");
		SSL_free(*ssl_con);
		SSL_CTX_free(ssl_ctx);
		return WSOCK_ERROR;
	}

	return 0;
}
