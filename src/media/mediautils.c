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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "mediautils.h"

// Ripped from ffmpeg, see sdp.c

static void digit_to_char(gchar *dst, guint8 src)
{
    if (src < 10) {
        *dst = '0' + src;
    } else {
        *dst = 'A' + src - 10;
    }
}

static gchar *data_to_hex(gchar *buff, const guint8 *src, gint s)
{
    gint i;

    for(i = 0; i < s; i++) {
        digit_to_char(buff + 2 * i, src[i] >> 4);
        digit_to_char(buff + 2 * i + 1, src[i] & 0xF);
    }

    return buff;
}

gchar *extradata2config(const guint8 *extradata, gint extradata_size)
{
    gchar *config = g_malloc(extradata_size * 2 + 1);

    if (config == NULL) {
        return NULL;
    }

    data_to_hex(config, extradata, extradata_size);

    config[extradata_size * 2] = '\0';

    return config;
}
