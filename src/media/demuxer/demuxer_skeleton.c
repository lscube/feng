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

#include "media/demuxer.h"

static int skel_probe(InputStream * i_stream)
{
    return RESOURCE_NOT_FOUND;
}

static int skel_init(Resource * r)
{
    return RESOURCE_DAMAGED;
}

static int skel_read_packet(Resource * r)
{
    return RESOURCE_NOT_PARSEABLE;
}

static int skel_seek(Resource * r, double time_sec)
{
    return RESOURCE_NOT_SEEKABLE;
}

static int skel_uninit(Resource * r)
{
    return RESOURCE_OK;
}

FENG_DEMUXER(skel,
             "Skeleton demuxer module"
             "skl",
             STORED_SOURCE);

