/* * 
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


#include <stdio.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <config.h>

#include <fenice/fnc_log.h>
#include <fenice/debug.h>

#undef fnc_log

#define LOG_FORMAT "%d/%b/%Y:%H:%M:%S %z"
#define ERR_FORMAT "%a %b %d %H:%M:%S %Y"
#define MAX_LEN_DATE 30

//static char* fnc_name;
static FILE *fd = NULL;

/*Prints on standard error or file*/
static void fnc_errlog(int level, const char *fmt, va_list args)
{
    time_t now;
    char date[MAX_LEN_DATE];
    const struct tm *tm;

    time(&now);
    tm = localtime(&now);

    switch (level) {
        case FNC_LOG_FATAL:
            strftime(date, MAX_LEN_DATE, ERR_FORMAT, tm);
            fprintf(fd, "[%s] [fatal error] ", date);
            break;
        case FNC_LOG_ERR:
            strftime(date, MAX_LEN_DATE, ERR_FORMAT, tm);
            fprintf(fd, "[%s] [error] ", date);
            break;
        case FNC_LOG_WARN:
            strftime(date, MAX_LEN_DATE, ERR_FORMAT, tm);
            fprintf(fd, "[%s] [warning] ", date);
            break;
        case FNC_LOG_DEBUG:
#if DEBUG
            fprintf(fd, "[debug] ");
#else
            return;  
#endif
            break;
        case FNC_LOG_VERBOSE:
#ifdef VERBOSE
            fprintf(fd, "[verbose debug] ");
#else
            return;
#endif
            break;
        case FNC_LOG_CLIENT:
            break;
        default:
            /*FNC_LOG_INFO*/
            strftime(date, MAX_LEN_DATE, LOG_FORMAT ,tm);
            fprintf(fd, "[%s] ", date);
            break;
    }

    vfprintf(fd, fmt, args);
    fprintf(fd, "\n");
    fflush(fd);
}

#if HAVE_SYSLOG_H
static void fnc_syslog(int level, const char *fmt, va_list args)
{
    int l = LOG_ERR;

    switch (level) {
        case FNC_LOG_FATAL:
            l = LOG_CRIT;
            break;
        case FNC_LOG_ERR:
            l = LOG_ERR;
            break;
        case FNC_LOG_WARN:
            l = LOG_WARNING;
            break;
        case FNC_LOG_DEBUG:
#if DEBUG
            l = LOG_DEBUG;
#else
            return;
#endif
            break;
        case FNC_LOG_VERBOSE:
#ifdef VERBOSE
            l = LOG_DEBUG;
#else
            return;
#endif
            break;
        case FNC_LOG_CLIENT:
            l = LOG_INFO;
            return;
            break;
        default:
            l = LOG_INFO;
            break;
    }
    vsyslog(l, fmt, args);
}
#endif

static void (*fnc_vlog)(int, const char*, va_list) = fnc_errlog;

/**
 * Set the logger and returns the function pointer to be feed to the
 * Sock_init
 * @param file path to the logfile
 * @param out specifies the logger function
 * @param name specifies the application name
 * @return the logger currently in use
 * */
fnc_log_t fnc_log_init(char *file, int out, char *name)
{
    fd = stderr;
    switch (out) {
        case FNC_LOG_SYS:
#if HAVE_SYSLOG_H
            openlog(name, LOG_PID /*| LOG_PERROR*/, LOG_DAEMON);
            fnc_vlog = fnc_syslog;
#endif
            break;
        case FNC_LOG_FILE:
            if ((fd = fopen(file, "a+")) == NULL) fd = stderr;
            break;
        case FNC_LOG_OUT:
            break;
    }
    return fnc_vlog;
}

/**
 * External logger function
 * @param level log level
 * @param fmt as printf format string
 * @param ... as printf variable argument
 */
void fnc_log(int level, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    fnc_vlog(level, fmt, vl);
    va_end(vl);
}
