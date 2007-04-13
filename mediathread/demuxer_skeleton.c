/* * 
 *  $Id$
 *
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2007 by
 *    - Dario Gallucci      <dario.gallucci@polito.it>
 *
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * */

#include <fenice/demuxer.h>
#include <fenice/demuxer_module.h>

static DemuxerInfo info = {
    "Skeleton demuxer_module",
    "skel",
    "LScube Team",
    "",
    "skl"
};

FNC_LIB_DEMUXER(skel);

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
