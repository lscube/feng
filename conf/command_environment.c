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
#include <getopt.h>
#include <fenice/command_environment.h>
#include <fenice/utils.h>
#include <netembryo/wsocket.h>
#include <bufferpool/bufferpool.h>
#include <glib.h>

void usage(char *name)
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

int command_environment(feng *srv, int argc, char **argv)
{

    static const char short_options[] = "f:vVsq";
    //"m:a:f:n:b:z:T:B:q:o:S:I:r:M:4:2:Q:X:D:g:G:v:V:F:N:tpdsZHOcCPK:E:R:";

    int n;
    int config_file = 0;
    int quiet = 0;
    int view_log = FNC_LOG_FILE;
    char *progname = g_path_get_basename(argv[0]);
    fnc_log_t fn;

    static struct option long_options[] = {
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
            prefs_init(srv, optarg);
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

    if (!config_file) prefs_init(srv, NULL);

    fn = fnc_log_init(prefs_get_log(), view_log, progname);

    Sock_init(fn);
    bp_log_init(fn);

    return 0;
}
