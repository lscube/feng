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

#include <openssl/ssl.h>
#include <netembryo/wsocket.h>

SSL *get_ssl_connection(int sock)
{
	SSL *ssl_con;
	SSL_CTX *ssl_ctx = NULL;
	
	ssl_ctx=create_ssl_ctx();
	if(!ssl_ctx)
		return NULL;
	/*SSL*/	
	ssl_con = SSL_new(ssl_ctx);
	if(!(ssl_con)) {
		SSL_CTX_free(ssl_ctx);
		return NULL;
	}
	SSL_set_fd (ssl_con, sock);
	
	SSL_get_cipher (ssl_con);

	return ssl_con; 

}
