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
#ifndef FN_MEDIAPARSER_H
#define FN_MEDIAPARSER_H

#include <glib.h>
#include <stdint.h>

#include "demuxer.h"

struct Track;

// return errors
#define MP_PKT_TOO_SMALL -101
#define MP_NOT_FULL_FRAME -102

typedef struct {
    const char *encoding_name; /*i.e. MPV, MPA ...*/
    const MediaType media_type;
} MediaParserInfo;

typedef struct MediaParser {
    const MediaParserInfo *info;
/*! init: inizialize the module
 *
 *  @param properties: pointer of allocated struct to fill with properties
 *  @param private_data: private data of parser will be, if needed, linked to this pointer (double)
 *  @return: 0 on success, non-zero otherwise.
 * */
    int (*init)(Track *track);

/*! parse: take a single elementary unit of the codec stream and prepare the rtp payload out of it.
 *
 *  @param track: track whose feng should be filled,
 *  @param data: packet from the demuxer layer,
 *  @param len: packet length,
 *  @return: 0 on success, non zero otherwise.
 * */
    int (*parse)(Track *track, uint8_t *data, size_t len);

    /** Uninit function to free the private data */
    void (*uninit)(Track *track);
} MediaParser;

const MediaParser *mparser_find(const char *);

char *extradata2config(MediaProperties *properties);

/**
 * @brief Buffer passed between parsers and RTP sessions
 *
 * This is what is being encapsulated by @ref BufferQueue_Element.
 */
typedef struct {
    double timestamp;   /*!< presentation time of packet */
    uint32_t rtp_timestamp; /*!< RTP version of the presenation time, used only by live */
    double delivery;    /*!< decoding time of packet */
    double duration;    /*!< packet duration */
    gboolean marker;    /*!< marker bit, set if we are sending the last frag */
    uint16_t seq_no;    /*!< Packet sequence number, used only by live */
    size_t data_size;   /*!< packet size */
    uint8_t data[];     /*!< actual packet data */
} MParserBuffer;

void mparser_buffer_write(struct Track *tr,
                          double presentation, 
                          double delivery,
                          double duration,
                          gboolean marker,
                          uint8_t *data, size_t data_size);

void mparser_live_buffer_write(struct Track *tr,
                          double presentation,
                          uint32_t rtp_timestamp,
                          double delivery,
                          double duration,
                          uint16_t seq_no,
                          gboolean marker,
                          uint8_t *data, size_t data_size); 


#define DEFAULT_MTU 1440

#endif // FN_MEDIAPARSER_H
