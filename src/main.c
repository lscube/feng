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

#include "conf/array.h"
#include "conf/configfile.h"

#include "feng.h"
#include "bufferqueue.h"
#include "fnc_log.h"
#include "network/rtp.h"
#include "network/rtsp.h"
#include <glib.h>
#include <time.h>

/**
 * @brief Version string to use to report feng's signature
 */
const char feng_signature[] = PACKAGE "/" VERSION;

/**
 * @brief Global structure for feng configuration
 *
 * This strucutre holds (part of) the global settings for the feng
 * process.
 *
 * @todo Break this up so that each piece of code takes care of
 * handling its own global state.
 *
 * @note Using a structure to aggregate the state might seem smart,
 *       but forces the same memory area for all the information to be
 *       in the same cacheline, as well as disallowing the linker from
 *       reordering the variables for best performance.
 */
struct feng feng_srv;

/**
 * @brief Feng server eventloop
 */
struct ev_loop *feng_loop;

#ifdef CLEANUP_DESTRUCTOR
/**
 * @brief Program name to clean up
 */
static char *progname;

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

    if ( feng_srv.vhost.access_log_fp )
        fclose(feng_srv.vhost.access_log_fp);

    g_free(feng_srv.errorlog_file);
    g_free(feng_srv.username);
    g_free(feng_srv.groupname);
    g_free(feng_srv.vhost.twin);
    g_free(feng_srv.vhost.document_root);
    g_free(feng_srv.vhost.access_log_file);

    array_free(feng_srv.config_context);
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
static void feng_drop_privs()
{
    const char *wanted_group = feng_srv.groupname;
    const char *wanted_user = feng_srv.username;

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
        exit(1);
    }

    if (!quiet) fncheader();

    if ( config_file == NULL )
        config_file = g_strdup(FENG_CONF_PATH_DEFAULT_STR);

    if (config_read(config_file)) {
        g_critical("unable to read configuration file '%s'\n", config_file);
        g_free(config_file);
        exit(1);
    }
    g_free(config_file);

    {
#ifndef CLEANUP_DESTRUCTOR
        gchar *progname;
#endif
        int view_log;

        progname = g_path_get_basename(argv[0]);

        if ( verbose )
            view_log = FNC_LOG_OUT;
        else if ( syslog )
            view_log = FNC_LOG_SYS;
        else
            view_log = FNC_LOG_FILE;

        fnc_log_init(feng_srv.errorlog_file,
                     view_log,
                     feng_srv.loglevel,
                     progname);
    }
}

int main(int argc, char **argv)
{
    if (!g_thread_supported ()) g_thread_init (NULL);

    feng_srv.config_context = array_init();

    /* parses the command line and initializes the log*/
    command_environment(argc, argv);

    /* This goes before feng_bind_ports */
    feng_loop = ev_default_loop(0);

    feng_handle_signals();

    g_slist_foreach(feng_srv.sockets, feng_bind_socket, NULL);
    accesslog_init(&feng_srv.vhost, NULL);

    stats_init();

    feng_drop_privs();

    http_tunnel_initialise();

    clients_init();

    ev_loop (feng_loop, 0);

    /* This is explicit to send disconnections! */
    clients_cleanup();

    return 0;
}
