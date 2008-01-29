/* *
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fenice/prefs.h>
#include <fenice/server.h>
#include <fenice/rtp.h> //RTP_DEFAULT_PORT

static pref_record prefs[PREFS_LAST] = {
        { STRING, "root", NULL },
        { INTEGER, "tcp_port", NULL },
        { INTEGER, "sctp_port", NULL },
        { INTEGER, "ssl_port", NULL },
        { INTEGER, "first_udp_port", NULL },
        { INTEGER, "max_session", NULL },
        { INTEGER, "buffered_frames", NULL },
        { STRING, "log_file", NULL },
        { STRING, "host.name", NULL },
        { STRING, "user", NULL },
        { STRING, "group", NULL }
};

void prefs_init(feng *srv, char *fileconf)
{
    FILE *f = NULL;
    char line[256];
    char *p, *q, *cont;
    int l;
    pref_id i;

    srv->prefs = prefs; //XXX
    //malloc(sizeof(pref_record)*PREFS_LAST);

    prefs_use_default(srv, PREFS_ALL);

    if (fileconf) {
        if ((f = fopen(fileconf, "rt")) == NULL) {
            fprintf(stderr,
                    "Error opening file %s, trying default (%s):",
                    fileconf, FENICE_CONF_PATH_DEFAULT_STR);
        }
    }

    if (!f) {
        if ((f = fopen(FENICE_CONF_PATH_DEFAULT_STR, "rt")) == NULL) {
            fprintf(stderr,
                "Error opening default file, using internal defaults.");
        }
    }

    if (f) {
        do {
            cont = fgets(line, 80, f);
            if (cont && line[0] != '#') {
                p = NULL;
                for (i = PREFS_FIRST; i < PREFS_LAST && p == NULL; i++) {
                    p = strstr(line, srv->prefs[i].tag);
                    if (p != NULL) {
                        p = strstr(p, "=");
                        if (p != NULL) {
                            q = p + 1;
                            if (srv->prefs[i].type == STRING) {
                                p = strstr(q,"\n");
                                if (p != NULL) {
                                    *p = '\0';
                                    SET_STRING_DATA(i, q);
                                }
                            } else if (srv->prefs[i].type == INTEGER) {
                                if (sscanf(q, "%i", &l) == 1) {
                                    SET_INTEGER_DATA(i, l);
                                }
                            }
                        }
                    }
                }
            }
        } while (cont);
        fclose(f);
    }
    // PREFS_HOSTNAME
    gethostname(line, sizeof(line));
    l = strlen(line);
    if (getdomainname(line + l + 1, sizeof(line) - l) != 0) {
        line[l] = '.';
    }
    SET_STRING_DATA(PREFS_HOSTNAME, line);
#if ENABLE_DEBUG
    printf("\n");
    printf("\tavroot directory is: %s\n", prefs_get_serv_root());
    printf("\thostname is: %s\n", prefs_get_hostname());
    printf("\trtsp listening port for TCP is: %d\n", prefs_get_port());
    printf("\trunning as: %s:%s\n",
           get_pref_str(PREFS_USER), get_pref_str(PREFS_GROUP));
#ifdef HAVE_LIBSCTP
    if (prefs_get_sctp_port() > 0)
        printf("\trtsp listening port for SCTP is: %d\n",
               prefs_get_sctp_port());
#endif
    printf("\tlog file is: %s\n", prefs_get_log());
    printf("\n");
#endif
}

void prefs_use_default(feng *srv, pref_id index)
{
    char buffer[256];

    switch (index) {
    case PREFS_ROOT:
        strcpy(buffer, FENICE_AVROOT_DIR_DEFAULT_STR);
        strcat(buffer, "/");
        SET_STRING_DATA(PREFS_ROOT, buffer);
        break;
    case PREFS_TCP_PORT:
        SET_INTEGER_DATA(PREFS_TCP_PORT, FENICE_RTSP_PORT_DEFAULT);
        break;
    case PREFS_SCTP_PORT:
        SET_INTEGER_DATA(PREFS_SCTP_PORT, -1);
        break;
    case PREFS_SSL_PORT:
        SET_INTEGER_DATA(PREFS_SSL_PORT, -1);
        break;
    case PREFS_FIRST_UDP_PORT:
        SET_INTEGER_DATA(PREFS_FIRST_UDP_PORT, RTP_DEFAULT_PORT);
        break;
    case PREFS_BUFFERED_FRAMES:
        SET_INTEGER_DATA(PREFS_BUFFERED_FRAMES, BUFFERED_FRAMES_DEFAULT);
        break;
    case PREFS_MAX_SESSION:
        SET_INTEGER_DATA(PREFS_MAX_SESSION, FENICE_MAX_SESSION_DEFAULT);
        break;
    case PREFS_LOG:
        SET_STRING_DATA(PREFS_LOG, FENICE_LOG_FILE_DEFAULT_STR);
        break;
    case PREFS_ALL:
        strcpy(buffer, FENICE_AVROOT_DIR_DEFAULT_STR);
        strcat(buffer, "/");
        SET_STRING_DATA(PREFS_ROOT, buffer);
        SET_INTEGER_DATA(PREFS_TCP_PORT, FENICE_RTSP_PORT_DEFAULT);
        SET_INTEGER_DATA(PREFS_SCTP_PORT, -1);
        SET_INTEGER_DATA(PREFS_SSL_PORT, -1);
        SET_INTEGER_DATA(PREFS_FIRST_UDP_PORT, RTP_DEFAULT_PORT);
        SET_INTEGER_DATA(PREFS_MAX_SESSION, FENICE_MAX_SESSION_DEFAULT);
        SET_INTEGER_DATA(PREFS_BUFFERED_FRAMES, BUFFERED_FRAMES_DEFAULT);
        SET_STRING_DATA(PREFS_LOG, FENICE_LOG_FILE_DEFAULT_STR);
        //USER and GROUP are unset by default
        break;
    default:
        break;
    }
}

void *get_pref(feng *srv, pref_id id)
{
    if (id >= PREFS_FIRST && id < PREFS_LAST)
        return srv->prefs[id].data;
    else
        return NULL;
}
