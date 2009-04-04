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

/**
 * @file
 * Port handling utility functions
 */

#include "feng.h"
#include "feng_utils.h"
#include "rtp.h"

static int start_port; //!< initial rtp port
static int *port_pool; //!< list of allocated ports

/**
 * Initializes the pool of ports and the initial port from the given one
 * @param srv The server instance in use (for ONE_FORK_MAX_CONNECTION)
 * @param port The first port from which to start allocating connections
 */
void RTP_port_pool_init(feng *srv, int port)
{
    int i;
    start_port = port;
    port_pool = g_new(int, ONE_FORK_MAX_CONNECTION);
    for (i = 0; i < ONE_FORK_MAX_CONNECTION; ++i) {
        port_pool[i] = i + start_port;
    }
}

/**
 * Gives a group of available ports for a new connection
 *
 * @param srv The server instance in use (for ONE_FORK_MAX_CONNECTION)
 * @param pair where to set the group of available ports
 *
 * @retval ERR_NOERROR No error
 * @retval ERR_GENERIC No port available
 */
int RTP_get_port_pair(feng *srv, port_pair * pair)
{
    int i;

    for (i = 0; i < ONE_FORK_MAX_CONNECTION; ++i) {
        if (port_pool[i] != 0) {
            pair->RTP =
                (port_pool[i] - start_port) * 2 + start_port;
            pair->RTCP = pair->RTP + 1;
            port_pool[i] = 0;
            return ERR_NOERROR;
        }
    }
    return ERR_GENERIC;
}

/**
 * Sets a group of ports as available for a new connection
 *
 * @param srv The server instance in use (for ONE_FORK_MAX_CONNECTION)
 * @param pair the group of ports to release
 *
 * @retval ERR_NOERROR No error
 * @retval ERR_GENERIC Ports not allocated
 */
int RTP_release_port_pair(feng *srv, port_pair * pair)
{
    int i;
    for (i = 0; i < ONE_FORK_MAX_CONNECTION; ++i) {
        if (port_pool[i] == 0) {
            port_pool[i] =
                (pair->RTP - start_port) / 2 + start_port;
            return ERR_NOERROR;
        }
    }
    return ERR_GENERIC;
}
