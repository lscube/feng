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

#include <stdio.h>
#include <string.h>
#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "feng_utils.h"

static const MediaParserInfo info = {
    "speex",
    MP_audio
};

static int speex_init(ATTR_UNUSED Track *track)
{
    return ERR_NOERROR;
}

static int speex_parse(Track *tr, uint8_t *data, size_t len)
{
    if (len > DEFAULT_MTU)
        return ERR_ALLOC;

    mparser_buffer_write(tr,
                         tr->properties.pts,
                         tr->properties.dts,
                         tr->properties.frame_duration,
                         1,
                         data, len);

    return ERR_NOERROR;
}

#define speex_uninit NULL

FNC_LIB_MEDIAPARSER(speex);

