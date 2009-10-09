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

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <string.h>
#include <stdbool.h>

#include "conf/buffer.h"
#include "conf/array.h"
#include "conf/configfile.h"

#include "feng.h"
#include "fnc_log.h"
#include "feng_utils.h"
#include "network/rtsp.h"
#include "plugin.h"
#include <glib.h>
#include <gmodule.h>

/**
 * load the plugin
 */

static int load_module(feng *srv, const char *module)
{
    int err = ERR_FATAL;
    int (*init) (plugin *p) = NULL;
    char * filename = g_strdup_printf("%s/%s.so",
                                      srv->srvconf.modules_dir,
                                      module);
    plugin *p = g_new0(plugin, 1);
    p->name = g_strdup(module);
    p->module = g_module_open(filename, G_MODULE_BIND_LAZY);
    if (!p->module) {
        fnc_log(FNC_LOG_FATAL, "Loading %s: %s", module, g_module_error());
        goto fail;
    }
    if (!g_module_symbol (p->module, "init", (gpointer *)&init)) {
        fnc_log(FNC_LOG_FATAL, "Loading init %s: %s", module, g_module_error());
        goto fail;
    }
    if (init && init(p)) {
        srv->modules = g_slist_append(srv->modules, p);
        p->data = p->init();
        err = ERR_NOERROR;
    }
    fail:
    g_free(filename);
    return err;
}

/**
 * load the plugins defined in srvconf.modules
 */

int modules_load(feng *srv)
{
    int i;
    for (i = 0; i < srv->srvconf.modules->used; i++) {
        data_string *d = (data_string *)srv->srvconf.modules->data[i];
        char *module = d->value->ptr;
        fprintf(stderr, "Loading %s\n", module);
        if (load_module(srv, module)) {
            fnc_log(FNC_LOG_FATAL, "Cannot load %s", module);
            return ERR_FATAL;
        };
    }
    return ERR_NOERROR;
}

int module_set_defaults(feng *srv)
{
    GSList *mod = srv->modules;
    int err = ERR_NOERROR;
    while (mod && !err)
    {
        plugin *p = mod->data;
        if (p->set_defaults)
        {
            err = p->set_defaults(srv, p->data);
            switch(err) {
                case ERR_NOERROR:
                    break;
                default:
                    fnc_log(FNC_LOG_ERR,
                            "%s:%s callback fails",
                            p->name, "setdefaults");
                    break;
            }
        }
        mod = g_slist_next(mod);
    }
    return err;
}

int module_response_send(RTSP_Client *client, RFC822_Response *resp)
{
    GSList *mod = client->srv->modules;
    int err = ERR_NOERROR;
    while (mod && !err)
    {
        plugin *p = mod->data;
        if (p->response_send)
        {
            err = p->response_send(client, resp, p->data);
            switch(err) {
                case ERR_NOERROR:
                    break;
                default:
                    fnc_log(FNC_LOG_ERR,
                            "%s:%s callback fails",
                            p->name, "response_send");
                    break;
            }
        }
        mod = g_slist_next(mod);
    }
    return err;
}
