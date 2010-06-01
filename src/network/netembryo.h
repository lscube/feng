/*
 * This file is part of feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * NetEmbryo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NetEmbryo; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef FN_NETEMBRYO_H__
#define FN_NETEMBRYO_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#ifndef WIN32
#   include <unistd.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#else
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <stdint.h>
#endif

#ifdef WIN32
typedef unsigned short sa_family_t;
typedef unsigned short in_port_t;
typedef unsigned int in_addr_t;
#endif

/** socket type definition */
typedef enum {
    /** socket fd not valid */
    SOCK_NONE,
    /** IP based protcols */
    TCP,
    UDP,
    SCTP,
    /** Local socket (Unix) */
    LOCAL
} sock_type;

/**
 * Socket abstraction structure
 */
typedef struct {
    sock_type socktype; ///< socket type
    int fd;    ///< low level socket file descriptor
    struct sockaddr_storage remote_stg;    ///< low level address storage from getpeername
    struct sockaddr_storage local_stg;    ///< low level address storage from getsockname
    /** human readable data */
    void *data; ///< user data
} Sock;

#define WSOCK_ERROR    -2
#define WSOCK_OK 0
#define WSOCK_ERRADDR    3

#define NETEMBRYO_MAX_SCTP_STREAMS 15

/** @defgroup NetEmbryo_Socket Sockets Access Interface
 *
 * @brief simple socket abstraction.
 *
 * @{
 */

/**
 * Create a new socket and binds it to an address/port.
 * @param host Local address to be used by this socket, if NULL the socket will
 *        be bound to all interfaces.
 * @param port Local port to be used by this socket, if NULL a random port will
 *        be used.
 * @param sock Pointer to a pre-created socket
 * @param socktype The type of socket to be created.
 */
Sock * neb_sock_bind(const char const *host,
                     const char const *port,
                     Sock *sock,
                     sock_type socktype);

int neb_sock_close(Sock *s);

/**
 * @}
 */

char *neb_sa_get_host(const struct sockaddr *sa);
in_port_t neb_sa_get_port(struct sockaddr *sa);
void neb_sa_set_port(struct sockaddr *sa, in_port_t port);

#endif
