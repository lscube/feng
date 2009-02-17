/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

/**
 * @file server.h
 * Server instance structure
 */

#ifndef FN_SERVER_H
#define FN_SERVER_H

// #include <fenice/schedule.h>
#include <fenice/prefs.h>
#include "conf/array.h"
#include "conf/conf.h"
#include <netembryo/wsocket.h>
#include <glib.h>
#include <pwd.h>
#include <ev.h>

typedef struct feng {

    // Metadata begin
    //pthread_t cpd;                  //!< CPD Metadata thread
#ifdef HAVE_METADATA
    void *metadata_clients;	    //!< CPD Clients
#endif
    // Metadata end
/**
 * @name lighttpd-alike preferences
 * lemon based, lighttpd alike preferences
 */
//@{
    array *config;
    array *config_touched;

    array *config_context;
    specific_config **config_storage;

    server_config srvconf;
//@}
/** 
 * @name eventloop state
 * Includes the 
 */
//@{
    /**
     * Once it reaches the maximum the server redirects
     * to a twin if available
     */
    struct ev_loop *loop;       //!< event loop
    int num_conn;               //!< number of active connections
    int conn_count;             //!< number of active connections (FIXME)
    int stop_schedule;          //!< to be refactored away
//@}
} feng;

typedef feng server;

#endif // FN_SERVER_H
