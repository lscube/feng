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
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <errno.h>

#include <netembryo/wsocket.h>

#include <fenice/eventloop.h>
#include <fenice/prefs.h>
#include <fenice/schedule.h>
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

int main(int argc, char **argv)
{
    Sock *main_sock = NULL, *sctp_main_sock = NULL;
    pthread_t mth;
    char *port;
    char *id;
    struct sigaction term_action;
    sigset_t block_set;

    /* parses the command line and initializes the log*/
    if (command_environment(argc, argv))
        return 1;

    /* catch TERM and INT signals */
    memset(&term_action, 0, sizeof(term_action));
    term_action.sa_handler = terminator_function;
    sigaction(SIGINT, &term_action, NULL);
    sigaction(SIGTERM, &term_action, NULL);
    /* block PIPE signal */
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &block_set, NULL);

    /* Bind to the defined listening port */
    port = g_strdup_printf("%d", prefs_get_port());
    main_sock = Sock_bind(NULL, port, TCP, 0);

    if(!main_sock) {
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

    if(Sock_listen(main_sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }

#ifdef HAVE_LIBSCTP
    if (prefs_get_sctp_port() >= 0) {
        port = g_strdup_printf("%d", prefs_get_sctp_port());
        sctp_main_sock = Sock_bind(NULL, port, SCTP, 0);

        if(!sctp_main_sock) {
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

        if(Sock_listen(sctp_main_sock, SOMAXCONN)) {
            fnc_log(FNC_LOG_ERR,"Sock_listen() error for SCTP socket." );
            fprintf(stderr, "[fatal] Sock_listen() error for SCTP socket.\n");
            return 1;
        }
    }
#endif

    /*
     * Drop privs to a specified user
     * */
    id = get_pref_str(PREFS_GROUP);
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

    id = get_pref_str(PREFS_USER);
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
    /* Initialises the array of schedule_list sched and creates the thread
     * schedule_do() -> look at schedule.c */
    if (schedule_init() == ERR_FATAL) {
        fnc_log(FNC_LOG_FATAL,"Can't start scheduler. Server is aborting.");
        fprintf(stderr, "[fatal] Can't start scheduler. Server is aborting.\n");
        return 1;
    }

    pthread_create(&mth, NULL, mediathread, NULL);

    /* puts in the global variable port_pool[MAX_SESSION] all the RTP usable
     * ports from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

    RTP_port_pool_init(get_pref_int(PREFS_FIRST_UDP_PORT));

    while (running) {

    /* eventloop looks for incoming RTSP connections and generates for each
       all the information in the structures RTSP_list, RTP_list, and so on */

        eventloop(main_sock, sctp_main_sock);
    }

    Sock_close(main_sock);
    Sock_close(sctp_main_sock);
    return 0;
}
