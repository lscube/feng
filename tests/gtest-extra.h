/* *
 * * This file is part of NetEmbryo and feng
 *
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 *
 * NetEmbryo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * NetEmbryo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NetEmbryo; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#ifndef GTEST_EXTRA_H
#define GTEST_EXTRA_H

#include <glib.h>

#define gte_fail(...)                           \
    do {                                        \
        g_printerr(__VA_ARGS__);               \
        g_assert_not_reached();                 \
    } while(0)

#define gte_fail_if(cond, ...)                  \
    do {                                        \
        if ( (cond) ) {                         \
            g_printerr(__VA_ARGS__);            \
            g_assert_not_reached();             \
        }                                       \
    } while(0)                                  \

#define gte_fail_unless(cond, ...)              \
    gte_fail_if(!(cond), __VA_ARGS__)

#endif
