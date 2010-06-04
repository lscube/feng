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

#include "conf/buffer.h"
#include "conf/array.h"
#include "conf/configfile.h"

#include "feng.h"
#include "bufferqueue.h"
#include "fnc_log.h"
#include "network/rtp.h"
#include "network/rtsp.h"
#include <glib.h>

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
    unsigned int i;

    g_free(progname);

    g_free(feng_srv.srvconf.bindhost);
    g_free(feng_srv.srvconf.bindport);
    g_free(feng_srv.srvconf.errorlog_file);
    g_free(feng_srv.srvconf.username);
    g_free(feng_srv.srvconf.groupname);
    g_free(feng_srv.srvconf.twin);

    if ( feng_srv.config_storage != NULL ) {
        for(i = 0; i < feng_srv.config_context->used; i++) {
            g_free(feng_srv.config_storage[i].document_root);

            g_free(feng_srv.config_storage[i].access_log_file);
        }

        free(feng_srv.config_storage);
    }

    array_free(feng_srv.config_context);

    g_slist_free(feng_srv.clients);
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
    const char *wanted_group = feng_srv.srvconf.groupname;
    const char *wanted_user = feng_srv.srvconf.username;

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

static gboolean command_environment(int argc, char **argv)
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
        config_file = g_strdup(FENG_CONF_PATH_DEFAULT_STR);

    if (config_read(config_file)) {
        g_critical("unable to read configuration file '%s'\n", config_file);
        g_free(config_file);
        return false;
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

        fnc_log_init(feng_srv.srvconf.errorlog_file,
                     view_log,
                     feng_srv.srvconf.loglevel,
                     progname);
    }

    return true;
}

int main(int argc, char **argv)
{
    if (!g_thread_supported ()) g_thread_init (NULL);

    feng_srv.config_context = array_init();

    /* parses the command line and initializes the log*/
    if ( !command_environment(argc, argv) )
        return 1;

    /* This goes before feng_bind_ports */
    feng_loop = ev_default_loop(0);

    feng_handle_signals();

    if ( !feng_bind_ports() )
        return 1;

    if ( !accesslog_init() )
        return 1;

    feng_drop_privs();

    http_tunnel_initialise();

    ev_loop (feng_loop, 0);

    return 0;
}
