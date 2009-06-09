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

#include "mediaparser.h"

// Ripped from ffmpeg, see sdp.c

static void digit_to_char(char *dst, uint8_t src)
{
    if (src < 10) {
        *dst = '0' + src;
    } else {
        *dst = 'A' + src - 10;
    }
}

char *extradata2config(MediaProperties *properties)
{
    const size_t config_len = properties->extradata_len * 2 + 1;
    char *config = g_malloc(config_len);
    size_t i;

    if (config == NULL)
        return NULL;

    for(i = 0; i < properties->extradata_len; i++) {
        digit_to_char(config + 2 * i, properties->extradata[i] >> 4);
        digit_to_char(config + 2 * i + 1, properties->extradata[i] & 0xF);
    }

    config[config_len-1] = '\0';

    return config;
}
