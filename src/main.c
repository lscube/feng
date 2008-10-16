/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

/**
 * @file main.c
 * server main loop
 */

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h> /*SOMAXCONN*/
#include <signal.h>
#include <errno.h>
#include <buffer.h>
#include <array.h>
#include <configfile.h>

#include <fenice/server.h>
#include <fenice/eventloop.h>
#include <fenice/prefs.h>
#include <fenice/utils.h>
#include <fenice/mediathread.h>
#include <glib.h>
#include <getopt.h>

static int stopped = 0;

static void terminator_function (int num) {
    fnc_log(FNC_LOG_INFO, "Exiting...");
//    fprintf(stderr, "Exiting...\n");
    mt_shutdown();
    stopped = 1;
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
            srv->gid = gr->gr_gid;
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
            srv->uid = pw->pw_uid;
        } else {
            fnc_log(FNC_LOG_WARN,
                    "Cannot get user %s id, %s",
                    id, strerror(errno));
        }
    }
}
/**
 * Bind to the defined listening port
 */

static int feng_bind_port(feng *srv,
                          char *host, char *port, specific_config *s)
{
    int is_sctp = s->is_sctp;
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
        return 1;
    }

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    g_ptr_array_add(srv->listen_socks, sock);

    if(Sock_listen(sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }

    return 0;
}

static int feng_bind_ports(feng *srv)
{
    int i, err = 0;
    char *host = srv->srvconf.bindhost->ptr;
    char *port = g_strdup_printf("%d", srv->srvconf.port);

    if ((err = feng_bind_port(srv, host, port, srv->config_storage[0]))) {
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
            return 1;
        }
        /* check if we already know this socket,
         * if yes, don't init it */

        /* split the host:port line */
        host = dc->string->ptr;
        port = strrchr(host, ':');
        if (!port) {
            fnc_log(FNC_LOG_ERR,"Cannot parse \"%s\" as host:port",
                                dc->string->ptr);
            return 1;
        }

        port[0] = '\0';

        if (host[0] == '[' && port[-1] == ']') {
            port[-1] = '\0';
            host++;
            s->use_ipv6 = 1; //XXX
        }

        port++;

        if (feng_bind_port(srv, host, port, s)) return 1;
    }

    return 0;
}

/**
 * catch TERM and INT signals
 * block PIPE signal
 */

static void feng_handle_signals(feng *srv)
{
    struct sigaction term_action;
    sigset_t block_set;

    /* catch TERM and INT signals */
    memset(&term_action, 0, sizeof(term_action));
    term_action.sa_handler = terminator_function;
    sigaction(SIGINT, &term_action, NULL);
    sigaction(SIGTERM, &term_action, NULL);
    /* block PIPE signal */
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &block_set, NULL);
}

/**
 * Initialises the array of schedule_list sched and creates the thread
 * @see schedule_do
 */

static int feng_start_mt(feng *srv)
{
    if (schedule_init(srv) == ERR_FATAL) {
        fnc_log(FNC_LOG_FATAL,"Can't start scheduler. Server is aborting.");
        fprintf(stderr, "[fatal] Can't start scheduler. Server is aborting.\n");
        return 1;
    }

    srv->mth = g_thread_create(mediathread, NULL, FALSE, NULL);
    return 0;
}

static void feng_free(feng* srv)
{
#define CLEAN(x) \
    buffer_free(srv->srvconf.x)
    CLEAN(bindhost);
    CLEAN(errorlog_file);
    CLEAN(username);
    CLEAN(groupname);
#undef CLEAN

#define CLEAN(x) \
    array_free(srv->x)
    CLEAN(config_context);
    CLEAN(config_touched);
    CLEAN(srvconf.modules);
#undef CLEAN
    g_ptr_array_free(srv->listen_socks, FALSE); // XXX Socks aren't g_mallocated yet
}

static gboolean show_version(const gchar *option_name, const gchar *value,
			     gpointer data, GError **error)
{
  fncheader();
  exit(0);
}

static int command_environment(feng *srv, int argc, char **argv)
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

  if ( error != NULL ) {
    g_critical("%s\n", error->message);
    exit(-1);
  }

  if (!quiet) fncheader();

  if ( config_file == NULL )
    config_file = g_strdup(FENICE_CONF_PATH_DEFAULT_STR);

  if (config_read(srv, config_file)) {
    g_critical("unable to read configuration file '%s'\n", config_file);
    feng_free(srv);
    exit(-1);
  }
  
  {
    gchar *progname = g_path_get_basename(argv[0]);
    fnc_log_t fn;

    int view_log;
    if ( verbose )
      view_log = FNC_LOG_OUT;
    else if ( syslog )
      view_log = FNC_LOG_SYS;
    else
      view_log = FNC_LOG_FILE;
    
    fn = fnc_log_init(srv->srvconf.errorlog_file->ptr, view_log, progname);
    
    Sock_init(fn);
    bp_log_init(fn);

    g_free(progname);
  }

  return 0;
}

/**
 * allocates a new instance variable
 * @return the new instance or NULL on failure
 */

static feng *feng_alloc(void)
{
    server *srv = g_new0(server, 1);

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
    CLEAN(srvconf.modules);
#undef CLEAN

    srv->listen_socks = g_ptr_array_new();
    srv->clients = NULL;

    return srv;
}

static void free_sock(gpointer data, gpointer user_data)
{
    Sock_close(data);
}

int main(int argc, char **argv)
{
    feng *srv = feng_alloc();

    if (!srv) return 1;

    /* parses the command line and initializes the log*/
    if (command_environment(srv, argc, argv))
        return 1;

    config_set_defaults(srv);

    feng_handle_signals(srv);

    if (feng_bind_ports(srv))
        return 1;

    feng_drop_privs(srv);

    if (!g_thread_supported ()) g_thread_init (NULL);

    if (feng_start_mt(srv))
        return 1;

    /* puts in the global variable port_pool[MAX_SESSION] all the RTP usable
     * ports from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

    RTP_port_pool_init(srv, srv->srvconf.first_udp_port);

    while (!stopped) {

    /* eventloop looks for incoming RTSP connections and generates for each
       all the information in the structures RTSP_list, RTP_list, and so on */

        eventloop(srv);
    }

    g_ptr_array_foreach(srv->listen_socks, free_sock, NULL);

    return 0;
}
