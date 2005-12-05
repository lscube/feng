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
 *  part of code is taken from NeMeSI source code
 * */

#ifndef __SOCKET_H
#define __SOCKET_H

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#if HAVE_SSL
#include <openssl/ssl.h>
#endif

#ifndef IN_IS_ADDR_MULTICAST
#define IN_IS_ADDR_MULTICAST(a)	((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((__const uint8_t *) (a))[0] == 0xff)
#endif

#ifdef WORDS_BIGENDIAN
#define ntohl24(x) (x)
#else
#define ntohl24(x) (((x&0xff) << 16) | (x&0xff00) | ((x&0xff0000)>>16)) 
#endif

#ifndef HAVE_STRUCT_SOCKADDR_STORAGE
/* Structure large enough to hold any socket address (with the historical exception of 
AF_UNIX). 128 bytes reserved.  */
#if ULONG_MAX > 0xffffffff
# define __ss_aligntype __uint64_t
#else
# define __ss_aligntype __uint32_t
#endif
#define _SS_SIZE        128
#define _SS_PADSIZE     (_SS_SIZE - (2 * sizeof (__ss_aligntype)))

struct sockaddr_storage
{
    sa_family_t ss_family;      /* Address family */
    __ss_aligntype __ss_align;  /* Force desired alignment.  */
    char __ss_padding[_SS_PADSIZE];
};
#endif // HAVE_STRUCT_SOCKADDR_STORAGE

/*definition for flags*/
/*ssl_flags*/
#define USE_SSL		01 
//#define USE_CRYPTO	03 /*set also USE_SSL*/
/*multicast and ipv6 flags*/
#define IS_MULTICAST	04

enum sock_types {
	TCP = 0,
	UDP = 1
};

/* NOTE:
 *	struct ip_mreq {
 *		struct in_addr imr_multiaddr;
 *		struct in_addr imr_interface;
 *	}
 *
 *	struct ipv6_mreq {
 *		struct in6_addr	ipv6mr_multiaddr;
 *		unsigned int ipv6mr_interface;
 *	}
 * */

struct ipv6_mreq_in6 {
	struct ipv6_mreq NETmreq6;
	struct in6_addr __imr_interface6;
};

struct ip_mreq_in {
	struct ip_mreq NETmreq;
	unsigned int __ipv4mr_interface;
};

#if 0
union ADDR {
	struct in_addr in;
	struct in6_addr in6;
};
#endif
union ADDR {
	struct ipv6_mreq_in6 mreq_in6; /*struct in6_addr ipv6mr_multiaddr; struct in6_addr imr_interface6 ; unsigned int ipv6mr_interface; */
	struct ip_mreq_in mreq_in; /*struct in_addr ipv4mr_multiaddr; struct in_addr imr_interface4; unsigned int ipv4mr_interface;*/
	#define imr_interface6 __imr_interface6
	#define ipv4_interface __ipv4mr_interface
	#define imr_interface4 NETmreq.imr_interface
	#define ipv4_multiaddr NETmreq.imr_multiaddr
	#define ipv6_interface NETmreq6.ipv6mr_interface
	#define ipv6_multiaddr NETmreq6.ipv6mr_multiaddr
};
/* Developper HowTo:
 *
 * union ADDR
 * 		struct ipv6_mreq_in6 mreq_in6
 * 					struct in6_addr ipv6_multiaddr // IPv6 class D multicast address. defined =  NETmreq6.ipv6mr_multiaddr
 * 					struct in6_addr imr_interface6   // IPv6 address of local interface.
 * 					unsigned int ipv6_interface	 // interface index, or 0              
 * 					struct ipv6_mreq NETmreq6
 * 		 struct ip_mreq_in mreq_in
 * 	 				struct in_addr ipv4_multiaddr  // IPv4 class D multicast address. defined = NETmreq.imr_multiaddr
 * 	 				struct in_addr imr_interface4    // IPv4 address of local interface. defined = NETmreq.imr_interface
 * 	 				unsigned int ipv4_interface    // interface index, or 0          
 * 	 				struct ip_mreq NETmreq
 */

typedef struct {
	int fd;
	struct sockaddr_storage sock_stg; /*sockname if bind or accept; peername if connect*/
	enum sock_types socktype; /*TCP, UDP*/ 
	sa_family_t family; 
	union ADDR addr;
	/*flags*/
	int flags; /*USE_SSL, USE_CRYPTO, IS_MULTICAST*/
	/*human readable datas*/
	char *remote_port;
	char *local_port;
	char *remote_host;
	/**/
#if HAVE_SSL
	SSL *ssl;
#endif
} Sock;


#define WSOCK_ERRORPROTONOSUPPORT -5	
#define WSOCK_ERRORIOCTL	-4	
#define WSOCK_ERRORINTERFACE	-3	
#define WSOCK_ERROR	-2	
#define WSOCK_ERRFAMILYUNKNOWN	-1
#define WSOCK_ERRSIZE	1
#define WSOCK_ERRFAMILY	2
#define WSOCK_ERRADDR	3
#define WSOCK_ERRPORT	4

/*low level wrapper*/
int sockfd_to_family(int sockfd);
int gethostinfo(struct addrinfo **res, char *host, char *serv, struct addrinfo *hints);
int sock_connect(char *host, char *port, int *sock, enum sock_types sock_type);
int sock_bind(char *host, char *port, int *sock, enum sock_types sock_type);
int sock_accept(int sock);
int sock_listen(int s, int backlog);
int sock_udp_read(int fd, void *buffer, int nbytes);
int sock_tcp_read(int fd, void *buffer, int nbytes);
int sock_write(int fd, void *buffer, int nbytes);
int sock_close(int s);
char *sock_ntop_host(const struct sockaddr *sa, /*socklen_t salen,*/ char *str, size_t len);
int32_t sock_get_port(const struct sockaddr *sa);
int16_t is_multicast(union ADDR *addr, sa_family_t family);
int16_t is_multicast_address(const struct sockaddr *sa, sa_family_t family);
int mcast_join (int sockfd, const struct sockaddr *sa/*, socklen_t salen*/, const char *ifname, unsigned int ifindex, union ADDR *addr);
int mcast_leave(int sockfd, const struct sockaddr *sa/*, socklen_t salen*/);

#if HAVE_SSL
/*ssl wrapper*/
SSL_CTX *create_ssl_ctx(void); /*SERVER VERSION:*/
SSL *get_ssl_connection(int sock); /*SERVER VERSION*/
int sock_SSL_connect(SSL **ssl_con,char *host, char *port, int *sock, enum sock_types sock_type);
int sock_SSL_bind(/*SSL **ssl_con, */char *host, char *port, int *sock, enum sock_types sock_type);
int sock_SSL_listen(int s, int backlog);
int sock_SSL_accept(SSL **ssl_con, int sock);
int sock_SSL_read(SSL *ssl_con, void *buffer, int nbytes);
int sock_SSL_write(SSL *ssl_con, void *buffer, int nbytes);
int sock_SSL_close(SSL *ssl_con);
#endif

/*medium level Interface*/
char *addr_ntop(const Sock  *addr, char *str, size_t len);

/*------------------------------------------------- INTERFACE --------------------------------------------------------------*/
/*TODO: doc them!!!!!*/
/*up level wrapper*/
Sock * Sock_connect(char *host, char *port, int *sock, enum sock_types sock_type, int ssl_flag);
Sock * Sock_bind(char *host, char *port, int *sock, enum sock_types sock_type, int ssl_flag);/*usually host is NULL for unicast. 
											       For multicast it is the multicast address.
											       Change it (ifi_xxx, see Stevens Chap.16)*/
Sock * Sock_accept(Sock *); /*returns the new Sock (like accept which returns the fd create for the new accepted connection*/
int Sock_create_ssl_connection(Sock *s);
int Sock_listen(Sock *n, int backlog);
int Sock_read(Sock *, void *buffer, int nbytes);
int Sock_write(Sock *, void *buffer, int nbytes);
int Sock_close(Sock *);
int get_fd(Sock *);
void Sock_init(void);

/*get_info.c*/
char * get_remote_host(Sock *);
char * get_local_host(Sock *);
char * get_remote_port(Sock *);
char * get_local_port(Sock *);
/*----------------------------------------------------------------------------------------------------------------------------*/
 
#endif
