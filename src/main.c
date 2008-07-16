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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <config.h>
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
#include <glib/gprintf.h>
#include <getopt.h>
#include <metadata/cpd.h>

#include <pthread.h>

int stopped = 0;

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

    pthread_create(&srv->mth, NULL, mediathread, NULL);

    pthread_create(&srv->cpd, NULL, cpd_server, (void *) srv);

    return 0;
}

static void usage(char *name)
{
    fprintf(stdout,
        "%s [options] \n"
        "--help\t\t| -h | -? \tshow this message\n"
        "--quiet\t\t| -q \tshow as little output as possible\n"
        "--config\t| -f <config> \tspecify configuration file\n"
        "--verbose\t| -v \t\toutput to standar error (debug)\n"
        "--version\t| -V \t\tprint version and exit\n"
        "--syslog\t| -s \t\tuse syslog facility\n", name);
    return;
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

static int command_environment(feng *srv, int argc, char **argv)
{
    static const char short_options[] = "f:vVsq";
    //"m:a:f:n:b:z:T:B:q:o:S:I:r:M:4:2:Q:X:D:g:G:v:V:F:N:tpdsZHOcCPK:E:R:";

    int n;
    int config_file = 0;
    int quiet = 0;
    int view_log = FNC_LOG_FILE;
    char *progname = g_path_get_basename(argv[0]);
    fnc_log_t fn;

    static const struct option long_options[] = {
        {"config",   1, 0, 'f'},
        {"quiet",    0, 0, 'q'},
        {"verbose",  0, 0, 'v'},
        {"version",  0, 0, 'V'},
        {"syslog",   0, 0, 's'},
        {"help",     0, 0, '?'},
        {0,          0, 0,  0 }
    };

    while ((n = getopt_long(argc, argv, short_options, long_options, NULL))
                != -1)
    {
        switch (n) {
        case 0:    /* Flag setting handled by getopt-long */
            break;
        case 'f':
            if (config_read(srv, optarg)) {
                feng_free(srv);
                return -1;
            }
            config_file = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'v':
            view_log = FNC_LOG_OUT;
            break;
        case 's':
            view_log = FNC_LOG_SYS;
            break;
        case '?':
            fncheader();
            usage(progname);
            g_free(progname);
            return 1;
            break;
case 'V':
            fncheader();
            g_free(progname);
            return 1;
            break;
        default:
            break;
        }
    }

    if (!quiet) fncheader();

    if (!config_file)
        if (config_read(srv, FENICE_CONF_PATH_DEFAULT_STR)) {
            feng_free(srv);
            return -1;
        }

    fn = fnc_log_init(srv->srvconf.errorlog_file->ptr, view_log, progname);

    Sock_init(fn);
    bp_log_init(fn);

    return 0;
}

/**
 * allocates a new instance variable
 * @return the new instance or NULL on failure
 */

static feng *feng_alloc(void)
{
    server *srv = calloc(1, sizeof(*srv));

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

    return srv;
}

static void free_sock(gpointer data, gpointer user_data)
{
    Sock_close(data);
}

int main(int argc, char **argv)
{
    feng *srv = feng_alloc(); //calloc(1,sizeof(feng));

    if (!srv) return 1;

    /* parses the command line and initializes the log*/
    if (command_environment(srv, argc, argv))
        return 1;

    config_set_defaults(srv);

    feng_handle_signals(srv);

    if (feng_bind_ports(srv))
        return 1;

    feng_drop_privs(srv);

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
