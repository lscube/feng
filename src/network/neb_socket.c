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

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <glib.h>

#ifdef ENABLE_SCTP
# include <netinet/sctp.h>
/* FreeBSD and Mac OS X don't have SOL_SCTP and re-use IPPROTO_SCTP
   for setsockopt() */
# if !defined(SOL_SCTP)
#  define SOL_SCTP IPPROTO_SCTP
# endif
#endif

#ifndef WIN32
# include <sys/ioctl.h>
#endif

#include "netembryo.h"
#include "fnc_log.h"

static struct addrinfo *_neb_sock_getaddrinfo(const char const *host,
                                              const char *const port,
                                              sock_type socktype)
{
    struct addrinfo *res;
    struct addrinfo hints;
    int n;

    memset(&hints, 0, sizeof(hints));

    if (host == NULL)
        hints.ai_flags = AI_PASSIVE;
    else
        hints.ai_flags = AI_CANONNAME;

#ifdef IPV6
    hints.ai_family = AF_UNSPEC;
#else
    hints.ai_family = AF_INET;
#endif

    switch (socktype) {
#ifdef ENABLE_SCTP
    case SCTP:
        hints.ai_socktype = SOCK_SEQPACKET;
        break;
#endif
    case TCP:
        hints.ai_socktype = SOCK_STREAM;
        break;
    default:
        g_assert_not_reached();
        return NULL;
    }

    if ((n = getaddrinfo(host, port, &hints, &res)) != 0) {
        fnc_log(FNC_LOG_ERR, "Unable to resolve %s:%s (%s)\n",
                host, port, gai_strerror(n));
        return NULL;
    }

    return res;
}

static int _neb_sock_sctp_setparams(int sd)
{
#ifdef ENABLE_SCTP
    struct sctp_initmsg initparams;
    struct sctp_event_subscribe subscribe;

    // Enable the propagation of packets headers
    memset(&subscribe, 0, sizeof(subscribe));
    subscribe.sctp_data_io_event = 1;
    if (setsockopt(sd, SOL_SCTP, SCTP_EVENTS, &subscribe,
                   sizeof(subscribe)) < 0) {
        fnc_perror("setsockopts(SCTP_EVENTS)");
        return WSOCK_ERROR;
    }

    // Setup number of streams to be used for SCTP connection
    memset(&initparams, 0, sizeof(initparams));
    initparams.sinit_max_instreams = NETEMBRYO_MAX_SCTP_STREAMS;
    initparams.sinit_num_ostreams = NETEMBRYO_MAX_SCTP_STREAMS;
    if (setsockopt(sd, SOL_SCTP, SCTP_INITMSG, &initparams,
                   sizeof(initparams)) < 0) {
        fnc_perror("setsockopts(SCTP_INITMSG)");
        return WSOCK_ERROR;
    }

    return WSOCK_OK;
#else
    g_assert_not_reached();
    return WSOCK_ERROR;
#endif
}

/**
 * bind wrapper
 */

static int _neb_sock_bind(const char const *host, const char const *port, int *sock, sock_type socktype)
{
    int bind_new;
    struct addrinfo *res, *ressave;

    if ( (ressave = res = _neb_sock_getaddrinfo(host, port, socktype)) == NULL ) {
        return WSOCK_ERRADDR;
    }

    bind_new = (*sock < 0);

    do {
        if (bind_new && (*sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
            continue;

        if ( socktype == SCTP &&
             _neb_sock_sctp_setparams(*sock) < 0 )
            continue;

        if (bind(*sock, res->ai_addr, res->ai_addrlen) == 0)
            break;

        if (bind_new) {
            if (close(*sock) < 0)
                return WSOCK_ERROR;
            else
                *sock = -1;
        }

    } while ((res = res->ai_next) != NULL);

    freeaddrinfo(ressave);

    if ( !res )
        return WSOCK_ERROR;

    return 0;
}

Sock * neb_sock_bind(const char const *host, const char const *port, Sock *sock,
                     sock_type socktype)
{

    Sock *s = NULL;
    int sockfd = -1;
    struct sockaddr *sa_p = NULL;
    socklen_t sa_len = sizeof(struct sockaddr_storage);

    if(sock) {
        sockfd = sock->fd;
    }

    if (_neb_sock_bind(host, port, &sockfd, socktype)) {
        fnc_perror("_neb_sock_bind");
        return NULL;
    }

    if ( (s = calloc(1, sizeof(Sock))) == NULL )
        goto error;

    s->fd = sockfd;
    s->socktype = socktype;

    sa_p = (struct sockaddr *) &(s->local_stg);

    if ( getsockname(s->fd, sa_p, &sa_len) )
        goto error;

    return s;

 error:
    if ( s != NULL )
        neb_sock_close(s);
    else
        close(sockfd);
    return NULL;
}

/**
 * Close an existing socket.
 * @param s Existing socket.
 */

int neb_sock_close(Sock *s)
{
    int res;

    if (!s)
        return -1;

    res = close(s->fd);

    free(s);

    return res;
}
