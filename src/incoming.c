/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#include <stdbool.h>
#include <ev.h>

#include "feng.h"
#include "fnc_log.h"
#include "incoming.h"
#include "network/rtsp.h"

/**
 * Bind to the defined listening port
 *
 * @param srv The server instance to bind ports for
 * @param host The hostname to bind ports on
 * @param port The port to bind
 * @param cfg_storage_idx The index in @ref feng::config_storage array
 *                        for the current configuration
 *
 * @retval true Binding complete
 * @retval false Error during binding
 */
static int feng_bind_port(feng *srv, const char *host, const char *port,
                          size_t cfg_storage_idx)
{
    specific_config *s = srv->config_storage[cfg_storage_idx];
    gboolean is_sctp = !!s->is_sctp;
    Sock *sock;

    if (is_sctp)
        sock = Sock_bind(host, port, NULL, SCTP, NULL);
    else
        sock = Sock_bind(host, port, NULL, TCP, NULL);
    if(!sock) {
        fnc_log(FNC_LOG_ERR,"Sock_bind() error for port %s.", port);
        fprintf(stderr,
                "[fatal] Sock_bind() error in main() for port %s.\n",
                port);
        return false;
    }

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    if(Sock_listen(sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return false;
    }
    sock->data = srv;
    srv->listeners[cfg_storage_idx].data = sock;
    ev_io_init(&srv->listeners[cfg_storage_idx],
               rtsp_client_incoming_cb,
               Sock_fd(sock), EV_READ);
    ev_io_start(srv->loop, &srv->listeners[cfg_storage_idx]);

    return true;
}

gboolean feng_bind_ports(feng *srv)
{
    size_t i;
    int err = 0;
    char *host = srv->srvconf.bindhost->ptr;
    char *port = g_strdup_printf("%d", srv->srvconf.port);

    srv->listeners = g_new0(ev_io, srv->config_context->used);

    if ((err = feng_bind_port(srv, host, port, 0))) {
        g_free(port);
        return err;
    }

    g_free(port);

   /* check for $SERVER["socket"] */
    for (i = 1; i < srv->config_context->used; i++) {
        data_config *dc = (data_config *)srv->config_context->data[i];
        specific_config *s = srv->config_storage[i];
//        size_t j;

        /* not our stage */
        if (COMP_SERVER_SOCKET != dc->comp) continue;

        if (dc->cond != CONFIG_COND_EQ) {
            fnc_log(FNC_LOG_ERR,"only == is allowed for $SERVER[\"socket\"].");
            return false;
        }
        /* check if we already know this socket,
         * if yes, don't init it */

        /* split the host:port line */
        host = dc->string->ptr;
        port = strrchr(host, ':');
        if (!port) {
            fnc_log(FNC_LOG_ERR,"Cannot parse \"%s\" as host:port",
                                dc->string->ptr);
            return false;
        }

        port[0] = '\0';

        if (host[0] == '[' && port[-1] == ']') {
            port[-1] = '\0';
            host++;
            s->use_ipv6 = 1; //XXX
        }

        port++;

        if (!feng_bind_port(srv, host, port, i)) return false;
    }

    return true;
}
