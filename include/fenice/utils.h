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

#ifndef FN_UTILS_H
#define FN_UTILS_H

#include "config.h"

#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>

/*! autodescriptive error values */
#define ERR_NOERROR              0
#define ERR_GENERIC             -1
#define ERR_NOT_FOUND           -2
#define ERR_PARSE               -3
#define ERR_ALLOC               -4
#define ERR_INPUT_PARAM         -5
#define ERR_NOT_SD              -6  // XXX check it
#define ERR_UNSUPPORTED_PT      -7  /// Unsupported Payload Type
#define ERR_EOF                 -8
#define ERR_FATAL               -9
#define ERR_CONNECTION_CLOSE    -10

/**
 * Returns the current time in seconds
 */
#ifdef HAVE_CLOCK_GETTIME
static inline double gettimeinseconds(struct timespec *now) {
    struct timespec tmp;
    if (!now) {
        now = &tmp;
    }
    clock_gettime(CLOCK_REALTIME, now);
    return (double)now->tv_sec + (double)now->tv_nsec * .000000001;
}
#else
#warning Posix RealTime features not available
static inline double gettimeinseconds(struct timespec *now) {
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    if (now) {
        now->tv_sec = tmp.tv_sec;
        now->tv_nsec = tmp.tv_usec * 1000;
    }
    return (double)tmp.tv_sec + (double)tmp.tv_usec * .000001;
}
#endif

#endif // FN_UTILS_H
