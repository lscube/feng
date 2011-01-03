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

static inline char digit_to_char(uint8_t src)
{
    if (src < 10)
        return '0' + src;
    else
        return 'A' + src - 10;
}

void sdp_descr_append_config(Track *track)
{
    GString *descr = track->sdp_description;
    size_t len = track->properties.extradata_len, i, oldlen = descr->len;
    uint8_t *data = (uint8_t*)(track->properties.extradata);
    char *out;

    if ( len == 0 )
        return;

    /* enlarge the string to the needed size; the final NULL byte is
       taken care of by GString. */
    g_string_set_size(descr, descr->len + (len*2) + strlen("config=;"));

    /* First char to start converting the content to */
    out = &descr->str[oldlen];

    *out = '\0';

    g_strlcat(descr->str, "config=", descr->len);
    out += strlen("config=");

    for ( i = 0; i < len; i++, data++ ) {
        *out++ = digit_to_char(*data >> 4);
        *out++ = digit_to_char(*data & 0xF);
    }
    *out++ = '\0';

    g_strlcat(descr->str, ";", descr->len);
}
