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
#include <netembryo/wsocket.h>
#include <openssl/ssl.h>
#ifndef CERT_N_KEY_NAME 
# define CERT_N_KEY_NAME NETEMBRYO_CERT_FILE_DEFAULT_STR
#endif

#if 0
#if SSLEAY_VERSION_NUMBER > 0x0A000 
static RSA *__RSA_generate_temp_key_CB (SSL *ssl, int export, int KEY_512_LEN) 
#else 
static RSA *__RSA_generate_temp_key_CB (SSL *ssl, int export) 
#define KEY_512_LEN 512 
#endif
{ 
	RSA *rsa; 
	if (!(rsa = RSA_generate_key (KEY_512_LEN,RSA_F4,NULL,NULL))) 
		return NULL;
	return rsa; 
}
#endif

SSL_CTX *create_ssl_ctx(void)
{
	SSL_CTX *ssl_ctx = NULL;
	char *env,*cert_n_key = CERT_N_KEY_NAME;

		/*SSL_CTX*/
	ssl_ctx = SSL_CTX_new(SSLv3_server_method());
	if(!ssl_ctx)
		return NULL;
	SSL_CTX_set_options (ssl_ctx,SSL_OP_ALL);
	
	/*SSL CIPHERS*/
	/*
	if ((env = getenv ("SSL_CIPHERS")))  
		SSL_CTX_set_cipher_list(ssl_ctx,env); 
	//SSL_CTX_set_cipher_list(ssl_ctx,"ALL"); 
	*/

	if ((env = getenv ("SSL_CERT_N_KEY_FILE"))) 
		cert_n_key = env; 
	if (access (cert_n_key, R_OK) < 0) { 
		fprintf(stderr,"can't access certificate file %s.",cert_n_key); 
		SSL_CTX_free(ssl_ctx);
		return NULL; 
	} 
	if (!SSL_CTX_use_certificate_file (ssl_ctx, cert_n_key,SSL_FILETYPE_PEM)) {
		fprintf(stderr,"unable to load certificate from %s.",cert_n_key); 
		SSL_CTX_free(ssl_ctx);
		return NULL; 
	} 
	if (!SSL_CTX_use_RSAPrivateKey_file (ssl_ctx, cert_n_key,SSL_FILETYPE_PEM)) { 
		printf("unable to load private key from %s.",cert_n_key); 
		SSL_CTX_free(ssl_ctx);
		return NULL; 
	}
	
	if (!SSL_CTX_check_private_key(ssl_ctx)) {
		fprintf(stderr,"Private key does not match the certificate public key\n");
		return NULL;
	}
	/*
	if (SSL_CTX_need_tmp_RSA (ssl_ctx)) 
		SSL_CTX_set_tmp_rsa_callback (ssl_ctx,__RSA_generate_temp_key_CB); */
	return ssl_ctx;

}
