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

#include <config.h>

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#if HAVE_SYSLOG_H
# include <syslog.h>
#endif

#include "feng.h"
#include "fnc_log.h"

#undef fnc_log

#define LOG_FORMAT "%d/%b/%Y:%H:%M:%S %z"
#define ERR_FORMAT "%a %b %d %H:%M:%S %Y"
#define MAX_LEN_DATE 30

static FILE *error_log;

/**
 * Log to file descriptor
 * @brief print on standard error or file
 */
static void fnc_filelog(unsigned int level, const char *fmt, va_list args)
{
    static const char *const log_prefix[] = {
        [FNC_LOG_FATAL] = "[fatal error] ",
        [FNC_LOG_ERR] = "[error] ",
        [FNC_LOG_WARN] = "[warning] ",
        [FNC_LOG_INFO] = "",
        [FNC_LOG_CLIENT] = "[client] ",
        [FNC_LOG_DEBUG] = "[debug] ",
        [FNC_LOG_VERBOSE] = "[verbose debug] "
    };

    time_t now;
    char date[MAX_LEN_DATE];
    const struct tm *tm;

    if (level > feng_srv.log_level) return;

    time(&now);
    tm = localtime(&now);
    strftime(date, MAX_LEN_DATE, ERR_FORMAT, tm);

    /* Add this here so that we have a known fallback for logs
       happening before initialisation is complete */
    if ( error_log == NULL )
        error_log = stderr;

    fprintf(error_log, "[%s] %s", date, log_prefix[level]);
    vfprintf(error_log, fmt, args);
    fprintf(error_log, "\n");
    fflush(error_log);
}

#if HAVE_SYSLOG_H
static void fnc_syslog(unsigned int level, const char *fmt, va_list args)
{
    static const int fnc_to_syslog_level[] = {
        [FNC_LOG_FATAL] = LOG_CRIT,
        [FNC_LOG_ERR] = LOG_ERR,
        [FNC_LOG_WARN] = LOG_WARNING,
        [FNC_LOG_INFO] = LOG_INFO,
        [FNC_LOG_CLIENT] = LOG_INFO,
        [FNC_LOG_DEBUG] = LOG_DEBUG,
        [FNC_LOG_VERBOSE] = LOG_DEBUG
    };

    if (level > feng_srv.log_level) return;

    vsyslog(fnc_to_syslog_level[level], fmt, args);
}
#endif

static void (*fnc_vlog)(unsigned int, const char*, va_list) = fnc_filelog;

/**
 * Initialize the logger.
 **/
void fnc_log_init(const char *progname)
{
    /* If the error_log is set to the constant "syslog" use syslog as
       output. */
    if ( strcmp(feng_srv.error_log, "syslog") == 0 ) {
#if HAVE_SYSLOG_H
        openlog(progname, LOG_PID /*| LOG_PERROR*/, LOG_DAEMON);
        fnc_vlog = fnc_syslog;
#endif
        /* fallback: if no syslog support is present, but syslog is
           requested, keep using stderr. */
    } else if ( strcmp(feng_srv.error_log, "stderr") != 0 ) {
        /* We're not asking for syslog, nor stderr, so the value is the
           file name to use instead. */
        FILE *new_error_log = fopen(feng_srv.error_log, "a+");
        if ( new_error_log == NULL ) {
            fnc_perror("unable to open error log");
            return;
        }

        error_log = new_error_log;
    }
}

/**
 * External logger function
 * @param level log level
 * @param fmt as printf format string
 * @param ... as printf variable argument
 */
void fnc_log(unsigned int level, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    fnc_vlog(level, fmt, vl);
    va_end(vl);
}

/**
 * @brief Wrapper function to simulate perror() on the fnc_log
 *
 * @param errno_val The value of errno to call this for
 * @param function The function this error occurred on
 * @param comment An optional comment on where this happened (use "" for none)
 */
void _fnc_perror(int errno_val, const char *function, const char *comment)
{
    char errbuffer[512];
#if STRERROR_R_CHAR_P
    char *strerr = strerror_r(errno_val, errbuffer, sizeof(errbuffer)-1);
    static const int res = 0;
#else
    int res = strerror_r(errno_val, errbuffer, sizeof(errbuffer)-1);
    char *const strerr = errbuffer;
#endif

    if ( res == 0 )
        fnc_log(FNC_LOG_ERR, "%s%s%s: %s", function,
                *comment ? " " : "", comment,
                strerr);
    else
        fnc_log(FNC_LOG_ERR, "%s%s%s: Unknown error %d", function,
                *comment ?  " " : "", comment,
                errno_val);
}
