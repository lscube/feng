/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by LScube team <team@streaming.polito.it>
 *      - see AUTHORS for the complete list
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

/**
 * @file RTP_port.c
 * Port handling utility functions
 */

#include <fenice/rtp.h>

int start_port = RTP_DEFAULT_PORT;       //!< Initial port, the RTP ports will start from here
int port_pool[ONE_FORK_MAX_CONNECTION];  //!< List of allocated ports

/**
 * Initializes the pool of ports and the initial port from the given one
 * @param port The first port from which to start allocating connections
 */
void RTP_port_pool_init(int port)
{
    int i;
    start_port = port;
    for (i = 0; i < ONE_FORK_MAX_CONNECTION; ++i) {
        port_pool[i] = i + start_port;
    }
}

/**
 * Gives a group of available ports for a new connection
 * @param pair where to set the group of available ports
 * @return ERR_NOERROR or ERR_GENERIC if there isn't an available port
 */
int RTP_get_port_pair(port_pair * pair)
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
 * @param pair the group of ports to release
 * @return ERR_NOERROR or ERR_GENERIC if the ports were not allocated
 */
int RTP_release_port_pair(port_pair * pair)
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
