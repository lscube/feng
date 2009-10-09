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
 * @file plugin.c
 * optional features loader
 */

#ifndef FN_PLUGIN_H
#define FN_PLUGIN_H

#include "config.h"

#include "feng.h"
#include "network/rtsp.h"
#include <gmodule.h>

typedef struct plugin {
    char *name;
    void* (*init) ();
    int (*set_defaults) (feng *srv, void *data);
    int (*uninit) (server *srv, void *data);
    GModule *module;
    void *data;
//XXX add the callbacks here once we have them set somewhere.
    int (*response_send) (RTSP_Client *client, RFC822_Response *resp, void *data);
} plugin;

int modules_load(feng *srv);

int module_response_send(RTSP_Client *client, RFC822_Response *resp);
int module_set_defaults(feng *srv);

#endif // FN_PLUGIN_H
