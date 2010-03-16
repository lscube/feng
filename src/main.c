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
 * @file main.c
 * server main loop
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
#include "bufferqueue.h"
#include "fnc_log.h"
#include "incoming.h"
#include "network/rtp.h"
#include "network/rtsp.h"
#include <glib.h>

#ifdef CLEANUP_DESTRUCTOR
/**
 * @brief Program name to clean up
 */
static char *progname;

static void CLEANUP_DESTRUCTOR main_cleanup()
{
    g_free(progname);
}
#endif

/**
 *  Handler to cleanly shut down feng
 */
static void sigint_cb (struct ev_loop *loop,
                       ATTR_UNUSED ev_signal * w,
                       ATTR_UNUSED int revents)
{
    ev_unloop (loop, EVUNLOOP_ALL);
}

/**
 * Drop privs to a specified user
 */
static void feng_drop_privs(feng *srv)
{
    char *id = srv->srvconf.groupname->ptr;

    if (id) {
        struct group *gr = getgrnam(id);
        if (gr) {
            if (setgid(gr->gr_gid) < 0)
                fnc_log(FNC_LOG_WARN,
                    "Cannot setgid to user %s, %s",
                    id, strerror(errno));
        } else {
            fnc_log(FNC_LOG_WARN,
                    "Cannot get group %s id, %s",
                    id, strerror(errno));
        }
    }

    id = srv->srvconf.username->ptr;
    if (id) {
        struct passwd *pw = getpwnam(id);
        if (pw) {
            if (setuid(pw->pw_uid) < 0)
                fnc_log(FNC_LOG_WARN,
                    "Cannot setuid to user %s, %s",
                    id, strerror(errno));
        } else {
            fnc_log(FNC_LOG_WARN,
                    "Cannot get user %s id, %s",
                    id, strerror(errno));
        }
    }
}


/**
 * catch TERM and INT signals
 * block PIPE signal
 */

static ev_signal signal_watcher_int;
static ev_signal signal_watcher_term;

static void feng_handle_signals(feng *srv)
{
    sigset_t block_set;
    ev_signal *sig = &signal_watcher_int;
    ev_signal_init (sig, sigint_cb, SIGINT);
    ev_signal_start (srv->loop, sig);
    sig = &signal_watcher_term;
    ev_signal_init (sig, sigint_cb, SIGTERM);
    ev_signal_start (srv->loop, sig);

    /* block PIPE signal */
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &block_set, NULL);
}


static void fncheader()
{
    printf("\n%s - Feng %s\n LScube Project - Politecnico di Torino\n",
            PACKAGE,
            VERSION);
}

static gboolean show_version(ATTR_UNUSED const gchar *option_name,
                             ATTR_UNUSED const gchar *value,
                             ATTR_UNUSED gpointer data,
                             ATTR_UNUSED GError **error)
{
  fncheader();
  exit(0);
}

static gboolean command_environment(feng *srv, int argc, char **argv)
{
    gchar *config_file = NULL;
    gboolean quiet = FALSE, verbose = FALSE, syslog = FALSE;

    GOptionEntry optionsTable[] = {
        { "config", 'f', 0, G_OPTION_ARG_STRING, &config_file,
            "specify configuration file", NULL },
        { "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet,
            "show as little output as possible", NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
            "output to standard error (debug)", NULL },
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version,
            "print version information and exit", NULL },
        { "syslog", 's', 0, G_OPTION_ARG_NONE, &syslog,
            "use syslog facility", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    GError *error = NULL;
    GOptionContext *context = g_option_context_new("");
    g_option_context_add_main_entries(context, optionsTable, PACKAGE_TARNAME);

    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if ( error != NULL ) {
        g_critical("%s\n", error->message);
        return false;
    }

    if (!quiet) fncheader();

    if ( config_file == NULL )
        config_file = g_strdup(FENICE_CONF_PATH_DEFAULT_STR);

    if (config_read(srv, config_file)) {
        g_critical("unable to read configuration file '%s'\n", config_file);
        g_free(config_file);
        return false;
    }
    g_free(config_file);

    {
#ifndef CLEANUP_DESTRUCTOR
        gchar *progname;
#endif
        fnc_log_t fn;
        int view_log;

        progname = g_path_get_basename(argv[0]);

        if ( verbose )
            view_log = FNC_LOG_OUT;
        else if ( syslog )
            view_log = FNC_LOG_SYS;
        else
            view_log = FNC_LOG_FILE;

        fn = fnc_log_init(srv->srvconf.errorlog_file->ptr,
                          view_log,
                          srv->srvconf.loglevel,
                          progname);

        Sock_init(fn);
    }

    return true;
}

/**
 * allocates a new instance variable
 * @return the new instance or NULL on failure
 */

static feng *feng_alloc(void)
{
    struct feng *srv = g_new0(server, 1);

    if (!srv) return NULL;

#define CLEAN(x) \
    srv->srvconf.x = buffer_init();
    CLEAN(bindhost);
    CLEAN(errorlog_file);
    CLEAN(username);
    CLEAN(groupname);
#undef CLEAN

#define CLEAN(x) \
    srv->x = array_init();
    CLEAN(config_context);
    CLEAN(config_touched);
#undef CLEAN

    return srv;
}

/**
 * @brief Free the feng server object
 *
 * @param srv The object to free
 *
 * This function frees the resources connected to the server object;
 * this function is empty when debug is disabled since it's unneeded
 * for actual production use, exiting the project will free them just
 * as fine.
 *
 * What this is useful for during debug is to avoid false positives in
 * tools like valgrind that expect a complete freeing of all
 * resources.
 */
static void feng_free(feng* srv)
{
#ifndef NDEBUG
    unsigned int i;

#define CLEAN(x) \
    buffer_free(srv->srvconf.x)
    CLEAN(bindhost);
    CLEAN(errorlog_file);
    CLEAN(username);
    CLEAN(groupname);
#undef CLEAN

    if ( srv->config_storage != NULL ) {
        for(i = 0; i < srv->config_context->used; i++) {
            buffer_free(srv->config_storage[i].document_root);
            buffer_free(srv->config_storage[i].server_name);
            buffer_free(srv->config_storage[i].ssl_pemfile);
            buffer_free(srv->config_storage[i].ssl_ca_file);
            buffer_free(srv->config_storage[i].ssl_cipher_list);

            buffer_free(srv->config_storage[i].access_log_file);
        }

        free(srv->config_storage);
    }

#define CLEAN(x) \
    array_free(srv->x)
    CLEAN(config_context);
    CLEAN(config_touched);
#undef CLEAN

    g_free(srv);

#endif /* NDEBUG */
}

int main(int argc, char **argv)
{
    feng *srv;
    int res = 0;

    if (!g_thread_supported ()) g_thread_init (NULL);

    if (! (srv = feng_alloc()) ) {
        res = 1;
        goto end;
    }

    /* parses the command line and initializes the log*/
    if ( !command_environment(srv, argc, argv) ) {
        res = 1;
        goto end;
    }

    config_set_defaults(srv);

    /* This goes before feng_bind_ports */
    srv->loop = ev_default_loop(0);

    feng_handle_signals(srv);

    if (!feng_bind_ports(srv)) {
        res = 1;
        goto end;
    }

    if ( !accesslog_init(srv) ) {
        res = 1;
        goto end;
    }

    feng_drop_privs(srv);

    /* puts in the global variable port_pool[MAX_SESSION] all the RTP usable
     * ports from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

    RTP_port_pool_init(srv, srv->srvconf.first_udp_port);
    http_tunnel_initialise();

    ev_loop (srv->loop, 0);

 end:
    accesslog_uninit(srv);
    feng_free(srv);

    return res;
}
