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

#include <string.h>

#include "mediaparser.h"
#include "demuxer.h"
#include "bufferqueue.h"
#include <fenice/utils.h>
#include "fnc_log.h"

// global media parsers modules:
extern MediaParser fnc_mediaparser_mpv;
extern MediaParser fnc_mediaparser_mpa;
extern MediaParser fnc_mediaparser_h264;
extern MediaParser fnc_mediaparser_aac;
extern MediaParser fnc_mediaparser_mp4ves;
extern MediaParser fnc_mediaparser_vorbis;
extern MediaParser fnc_mediaparser_theora;
extern MediaParser fnc_mediaparser_speex;
extern MediaParser fnc_mediaparser_mp2t;
extern MediaParser fnc_mediaparser_h263;
extern MediaParser fnc_mediaparser_amr;

// static array containing all the available media parsers:
static MediaParser *media_parsers[] = {
    &fnc_mediaparser_mpv,
    &fnc_mediaparser_mpa,
    &fnc_mediaparser_h264,
    &fnc_mediaparser_aac,
    &fnc_mediaparser_mp4ves,
    &fnc_mediaparser_vorbis,
    &fnc_mediaparser_theora,
    &fnc_mediaparser_speex,
    &fnc_mediaparser_mp2t,
    &fnc_mediaparser_h263,
    &fnc_mediaparser_amr,
    NULL
};

MediaParser *mparser_find(const char *encoding_name)
{
    int i;

    for(i=0; media_parsers[i]; i++) {
        if ( !g_ascii_strcasecmp(encoding_name,
                                 media_parsers[i]->info->encoding_name) ) {
            fnc_log(FNC_LOG_DEBUG, "[MT] Found Media Parser for %s\n",
                    encoding_name);
            return media_parsers[i];
        }
    }

    fnc_log(FNC_LOG_DEBUG, "[MT] Media Parser for %s not found\n",
            encoding_name);
    return NULL;
}

void mparser_unreg(MediaParser *p, void *private_data)
{
    if (p) p->uninit(private_data);
}

void mparser_buffer_write(struct Track *tr, uint8_t marker,
                          uint8_t *data, size_t data_size) {
    MParserBuffer *buffer = g_malloc(sizeof(MParserBuffer) + data_size);

    buffer->timestamp = tr->properties->mtime;
    buffer->marker = marker;
    buffer->data_size = data_size;

    memcpy(buffer->data, data, data_size);

    bq_producer_put(tr->producer, buffer);
}
