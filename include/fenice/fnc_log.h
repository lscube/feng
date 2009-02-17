/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */


#ifndef FN_FNC_LOG_H
#define FN_FNC_LOG_H

#include <config.h>
#include <stdarg.h>

enum {  FNC_LOG_OUT,
        FNC_LOG_SYS,
        FNC_LOG_FILE };

	//level
#define FNC_LOG_FATAL 0
#define FNC_LOG_ERR 1
#define FNC_LOG_WARN 2
#define FNC_LOG_INFO 3
#define FNC_LOG_DEBUG 4
#define FNC_LOG_VERBOSE 5
#define FNC_LOG_CLIENT 6

typedef void (*fnc_log_t)(int, const char*, va_list);

void fnc_log(int level, const char *fmt, ...);

#ifdef TRACE
#define fnc_log(level, fmt, string...) \
    fnc_log(level, "[%s - %d]" fmt, __FILE__, __LINE__ , ## string)
#endif

fnc_log_t fnc_log_init(char *file, int out, char *name);

#endif // FN_FNC_LOG_H 
