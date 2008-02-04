/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <config.h>
#include <sys/ioctl.h>
#include <sys/socket.h> /*SOMAXCONN*/
#include <signal.h>
#include <errno.h>


#include <fenice/server.h>
#include <fenice/eventloop.h>
#include <fenice/prefs.h>
#include <fenice/utils.h>
#include <fenice/command_environment.h>
#include <fenice/mediathread.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <pthread.h>

int running = 1;

static void terminator_function (int num) {
    fnc_log(FNC_LOG_INFO, "Exiting...");
//    fprintf(stderr, "Exiting...\n");
    mt_shutdown();
    running = 0;
}

/**
 * Drop privs to a specified user
 */
static void feng_drop_privs(feng *srv)
{
    char *id = get_pref_str(PREFS_GROUP);

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

    id = get_pref_str(PREFS_USER);
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

static int feng_bind_ports(feng *srv)
{
    char *port;
    port = g_strdup_printf("%d", prefs_get_port());
    srv->main_sock = Sock_bind(NULL, port, TCP, 0);

    if(!srv->main_sock) {
        fnc_log(FNC_LOG_ERR,"Sock_bind() error for TCP port %s.", port);
        fprintf(stderr,
                "[fatal] Sock_bind() error in main() for TCP port %s.\n",
                port);
        g_free(port);
        return 1;
    }

    fnc_log(FNC_LOG_INFO, "Waiting for RTSP connections on TCP port %s...",
            port);
    g_free(port);

    if(Sock_listen(srv->main_sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }

#ifdef HAVE_LIBSCTP
    if (prefs_get_sctp_port() >= 0) {
        port = g_strdup_printf("%d", prefs_get_sctp_port());
        srv->sctp_main_sock = Sock_bind(NULL, port, SCTP, 0);

        if(!srv->sctp_main_sock) {
            fnc_log(FNC_LOG_ERR,"Sock_bind() error for SCTP port %s.", port);
            fprintf(stderr,
                    "[fatal] Sock_bind() error in main() for SCTP port %s.\n",
                    port);
            g_free(port);
            return 1;
        }

        fnc_log(FNC_LOG_INFO,
                "Waiting for RTSP connections on SCTP port %s...", port);
        g_free(port);

        if(Sock_listen(srv->sctp_main_sock, SOMAXCONN)) {
            fnc_log(FNC_LOG_ERR,"Sock_listen() error for SCTP socket." );
            fprintf(stderr, "[fatal] Sock_listen() error for SCTP socket.\n");
            return 1;
        }
    }
#endif
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
    return 0;
}

int main(int argc, char **argv)
{
    feng *srv = calloc(1,sizeof(feng));

    /* parses the command line and initializes the log*/
    if (command_environment(srv, argc, argv))
        return 1;

    feng_handle_signals(srv);

    if (feng_bind_ports(srv))
        return 1;

    feng_drop_privs(srv);

    if (feng_start_mt(srv))
        return 1;

    /* puts in the global variable port_pool[MAX_SESSION] all the RTP usable
     * ports from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

    RTP_port_pool_init(get_pref_int(PREFS_FIRST_UDP_PORT));

    while (running) {

    /* eventloop looks for incoming RTSP connections and generates for each
       all the information in the structures RTSP_list, RTP_list, and so on */

        eventloop(srv);
    }

    Sock_close(srv->main_sock);
    Sock_close(srv->sctp_main_sock);
    return 0;
}
