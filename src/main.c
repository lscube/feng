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
#include <unistd.h>

#include "feng.h"
#include "bufferqueue.h"
#include "fnc_log.h"
#include "network/rtp.h"
#include "network/rtsp.h"
#include "media/demuxer.h"
#include <glib.h>
#include <time.h>

/**
 * @brief Version string to use to report feng's signature
 */
const char feng_signature[] = PACKAGE "/" VERSION;

/**
 * @brief Feng server eventloop
 */
struct ev_loop *feng_loop;

GList *configured_sockets;
GList *configured_vhosts;

cfg_options_t feng_srv = {
    /* set default here for pre-initialisation logs */
    .log_level = FNC_LOG_WARN
};

#ifdef CLEANUP_DESTRUCTOR
/**
 * @brief Program name to clean up
 */
static char *progname;

/**
 * @brief Wrapper for g_free to call from g_list_foreach
 */
static void feng_glist_free(gpointer data, ATTR_UNUSED gpointer user_data)
{
    g_free(data);
}

/**
 * @brief Cleanp and free a configured vhost structure
 *
 * @param vhost_p A generic pointer to a @ref cfg_vhost_t structure
 * @param user_data Unused
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void vhost_cleanup(gpointer vhost_p, ATTR_UNUSED gpointer user_data)
{
    cfg_vhost_t *vhost = vhost_p;

    fclose(vhost->access_log_file);
    g_free((char*)vhost->access_log);
    g_free((char*)vhost->twin);
    g_free((char*)vhost->document_root);

    g_list_foreach(vhost->aliases, feng_glist_free, NULL);
    g_list_free(vhost->aliases);

    g_slice_free(cfg_vhost_t, vhost);
}

/**
 * @brief Cleanup and free a configured socket structure
 *
 * @param socket_p A generic pointer to a @ref cfg_socket_t structure
 * @param user_data Unused
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void socket_cleanup(gpointer socket_p, ATTR_UNUSED gpointer user_data)
{
    cfg_socket_t *socket = socket_p;

    g_free((char*)socket->listen_on);
    g_free((char*)socket->port);

    g_slice_free(cfg_socket_t, socket);
}

/**
 * @brief Cleanup the data structures used by main()
 *
 * This function frees the resources that are allocated during the
 * initialisation of the server, and used by the main() function
 * directly.
 *
 * @note This si a cleanup destructure, which means that in non-debug
 *       builds it will not be compiled, while in debug builds will
 *       free the resources before exiting. This trick is useful to
 *       avoid false positives in tools like valgrind that expect
 *       resources to be completely freed at the end of the process.
 */
static void CLEANUP_DESTRUCTOR main_cleanup()
{
    g_free(progname);

    g_list_foreach(configured_vhosts, vhost_cleanup, NULL);
    g_list_free(configured_vhosts);
    g_list_foreach(configured_sockets, socket_cleanup, NULL);
    g_list_free(configured_sockets);
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
 * Drop privileges to the configured user
 *
 * This function takes care of changing the running group and user to
 * those defined in @ref feng_srv::groupname and @ref
 * feng_srv::username, defaulting to feng:feng.
 *
 * We do not drop privileges if we're running as non-root, as in that
 * case we assume we have been given already only the limited set of
 * privileges we need.
 */
static void feng_drop_privs()
{
    const char *const wanted_group = feng_srv.groupname;
    const char *const wanted_user = feng_srv.username;

    if ( getuid() != 0 ) /* only if root */
        return;

    errno = 0;
    if ( wanted_group != NULL ) {
        struct group *gr = getgrnam(wanted_group);
        if ( errno != 0 )
            fnc_perror("getgrnam");
        else if ( gr == NULL )
            fnc_log(FNC_LOG_ERR, "%s: group %s not found", __func__, wanted_group);
        else if ( setgid(gr->gr_gid) < 0 )
            fnc_perror("setgid");
    }

    errno = 0;
    if ( wanted_user != NULL ) {
        struct passwd *pw = getpwnam(wanted_user);
        if ( errno != 0 )
            fnc_perror("getpwnam");
        else if ( pw == NULL )
            fnc_log(FNC_LOG_ERR, "%s: user %s not found", __func__, wanted_user);
        else if ( setuid(pw->pw_uid) < 0 )
            fnc_perror("setuid");
    }
}


/**
 * catch TERM and INT signals
 * block PIPE signal
 */

static ev_signal signal_watcher_int;
static ev_signal signal_watcher_term;

static void feng_handle_signals()
{
    sigset_t block_set;
    ev_signal *sig = &signal_watcher_int;
    ev_signal_init (sig, sigint_cb, SIGINT);
    ev_signal_start (feng_loop, sig);
    sig = &signal_watcher_term;
    ev_signal_init (sig, sigint_cb, SIGTERM);
    ev_signal_start (feng_loop, sig);

    /* block PIPE signal */
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &block_set, NULL);
}


static void fncheader()
{
    printf("\n"PACKAGE" - Feng "VERSION"\n LScube Project - Politecnico di Torino\n");
}

static gboolean show_version(ATTR_UNUSED const gchar *option_name,
                             ATTR_UNUSED const gchar *value,
                             ATTR_UNUSED gpointer data,
                             ATTR_UNUSED GError **error)
{
  fncheader();
  exit(0);
}

static void command_environment(int argc, char **argv)
{
#ifndef CLEANUP_DESTRUCTOR
    gchar *progname;
#endif
    gchar *config_file = NULL;
    gboolean quiet = FALSE, verbose = FALSE, lint = FALSE;

    GOptionEntry optionsTable[] = {
        { "config", 'f', 0, G_OPTION_ARG_STRING, &config_file,
            "specify configuration file", NULL },
        { "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet,
            "show as little output as possible", NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
            "output to standard error (debug)", NULL },
        { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version,
            "print version information and exit", NULL },
        { "lint", 'l', 0, G_OPTION_ARG_NONE, &lint,
          "check the configuration file for errors, then exit", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    GError *error = NULL;
    GOptionContext *context = g_option_context_new("");
    g_option_context_add_main_entries(context, optionsTable, PACKAGE_TARNAME);

    g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);

    if ( error != NULL ) {
        fnc_log(FNC_LOG_FATAL, "%s", error->message);
        exit(1);
    }

    if (!quiet && !lint) fncheader();

    config_file_parse(config_file, lint);

    g_free(config_file);

    /* if we're doing a verbose run, make sure to output
       everything directly on standard error.
    */
    if ( verbose ) {
        feng_srv.log_level = FNC_LOG_INFO;
        feng_srv.error_log = "stderr";
    }

    progname = g_path_get_basename(argv[0]);

    fnc_log_init(progname);
}

int main(int argc, char **argv)
{
    if (!g_thread_supported ()) g_thread_init (NULL);

#ifdef HAVE_AVFORMAT
    ffmpeg_init();
#endif

    /* parses the command line and initializes the log*/
    command_environment(argc, argv);

    /* This goes before feng_bind_ports */
    feng_loop = ev_default_loop(0);

    feng_handle_signals();

    g_list_foreach(configured_sockets, feng_bind_socket, NULL);
    accesslog_init(feng_default_vhost, NULL);

    stats_init();

    feng_drop_privs();

    http_tunnel_initialise();

    clients_init();

    ev_loop (feng_loop, 0);

    /* This is explicit to send disconnections! */
    clients_cleanup();

#ifdef CLEANUP_DESTRUCTOR
    ev_loop_destroy(feng_loop);
#endif

    return 0;
}
