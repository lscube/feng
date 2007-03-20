/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include <fenice/stunserver.h>
#include <pthread.h>

#if ENABLE_STUN
static void enable_stun(void)
{
    struct STUN_SERVER_IFCFG *cfg = prefs_get_stuncfg();

    if( cfg!= NULL) {
        pthread_t thread;

        fnc_log(FNC_LOG_DEBUG, "Trying to start OMSstunserver thread\n");
        fnc_log(FNC_LOG_DEBUG, "stun parameters: %s,%s,%s,%s\n", 
                                cfg->a1, cfg->p1, cfg->a2, cfg->p2);

        pthread_create(&thread,NULL,OMSstunserverStart,(void *)(cfg));
    }
}
#endif

int main(int argc, char **argv)
{
    Sock *main_sock = NULL, *sctp_main_sock = NULL;
    pthread_t mth;
    char *port;
    char *id;

    /* Print version and other useful info */
    fncheader();

    /* parses the command line */
    if (command_environment(argc, argv))
        return 1;
    
    Sock_init(fnc_log);

#if ENABLE_STUN
    enable_stun();
#endif //ENABLE_STUN

    fnc_log(FNC_LOG_DEBUG, "Starting mediathread...");
    pthread_create(&mth, NULL, mediathread, NULL);
    
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

    fnc_log(FNC_LOG_INFO, "Waiting for RTSP connections on TCP port %s...\n",
            port);
    g_free(port);

    if(Sock_listen(main_sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR,"Sock_listen() error.");
        return 1;
    }

#ifdef HAVE_SCTP_FENICE
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
            fnc_log(FNC_LOG_ERR,"Sock_listen() error." );
            return 1;
        }
    }
#endif
    
    /*
     * Drop privs to a specified user
     * */
    id = get_pref(PREFS_USER);
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
    id = get_pref(PREFS_GROUP);
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

    /* Initialises the array of schedule_list sched and creates the thread
     * schedule_do() -> look at schedule.c */
    if (schedule_init() == ERR_FATAL) {
        fnc_log(FNC_LOG_FATAL,"Can't start scheduler. Server is aborting.");
        return 1;
    }

    /* puts in the global variable port_pool[MAX_SESSION] all the RTP usable
     * ports from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

    RTP_port_pool_init(RTP_DEFAULT_PORT);

    while (1) {

    /* eventloop looks for incoming RTSP connections and generates for each
       all the information in the structures RTSP_list, RTP_list, and so on */

        eventloop(main_sock, sctp_main_sock);
    }

    return 0;
}
