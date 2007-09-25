/* * 
 *  $Id: $
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *  h263 parser based on rfc 2190
 *
 *  Copyright (C) 2007 by Luca Barbato <lu_zero@gentoo.org>
 *
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/mediaparser.h>
#include <fenice/mediaparser_module.h>

static MediaParserInfo info = {
    "H263",
    MP_video
};

/* H263 MODE A RTP Header (RFC 2190)
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |F|P|SBIT |EBIT | SRC |I|U|S|A|   R   |DBQ| TRB |      TR       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct {
    /* byte 0 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned char ebit:3;
    unsigned char sbit:3;
    unsigned char p:1;
    unsigned char f:1;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned char f:1; /* Indicates the mode of the payload header */
    unsigned char p:1; /* Optional PB-frames mode */
    unsigned char sbit:3; /* Start bit position */
    unsigned char ebit:3; /* End bit position */
#else
#error Neither big nor little
#endif
    /* byte 1 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned char r1:1;
    unsigned char a:1;
    unsigned char s:1;
    unsigned char u:1;
    unsigned char i:1;
    unsigned char src:3;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned char src:3; /* Source format, bit 6,7 and 8 in PTYPE */
    unsigned char i:1; /* Picture coding type, bit 9 in PTYPE */
    unsigned char u:1; /* Unrestricted Motion Vector option, bit 10 in PTYPE */
    unsigned char s:1; /* Syntax-based Arithmetic Coding option, bit 11 in PTYPE */
    unsigned char a:1; /* Advanced Prediction option, bit 12 in PTYPE */
    unsigned char r1:1; /* Reserved: must be zero */
#endif
    /* byte 2 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned char trb:3;
    unsigned char dbq:2;
    unsigned char r2:3;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned char r2:3; /* Reserved: must be zero */
    unsigned char dbq:2; /* same as DBQUANT defined by H.263 */
    unsigned char trb:3; /* Temporal Reference for the B frame */
#endif
    /* byte 3 */
    unsigned char tr; /* Temporal Reference for the P frame */
} h263a_rtpheader;

/* H263 Picture Layer Header
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    PSC                    |      TR       |...|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      PTYPE          |  CHAFF  |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
// Taken from gstreamer gstrtph263pay.c
typedef struct {
#if BYTE_ORDER == LITTLE_ENDIAN
  unsigned int psc1:16;

  unsigned int tr1:2;
  unsigned int psc2:6;

  unsigned int ptype_263:1;
  unsigned int ptype_start:1;
  unsigned int tr2:6;

  unsigned int ptype_umvmode:1;
  unsigned int ptype_pictype:1;
  unsigned int ptype_srcformat:3;
  unsigned int ptype_freeze:1;
  unsigned int ptype_camera:1;
  unsigned int ptype_split:1;

  unsigned int chaff:5;
  unsigned int ptype_pbmode:1;
  unsigned int ptype_apmode:1;
  unsigned int ptype_sacmode:1;
#elif BYTE_ORDER == BIG_ENDIAN
  unsigned int psc1:16;

  unsigned int psc2:6;
  unsigned int tr1:2;

  unsigned int tr2:6;
  unsigned int ptype_start:1;
  unsigned int ptype_263:1;

  unsigned int ptype_split:1;
  unsigned int ptype_camera:1;
  unsigned int ptype_freeze:1;
  unsigned int ptype_srcformat:3;
  unsigned int ptype_pictype:1;
  unsigned int ptype_umvmode:1;

  unsigned int ptype_sacmode:1;
  unsigned int ptype_apmode:1;
  unsigned int ptype_pbmode:1;
  unsigned int chaff:5;
#endif
} h263_piclayer;

typedef struct {
    h263_piclayer header; //holds the latest header
} h263_priv;

static int h263_init(MediaProperties *properties, void **private_data)
{
    h263_priv *priv = calloc(1, sizeof(h263_priv));

    if (!priv) return ERR_ALLOC;

    INIT_PROPS

    *private_data = priv;

    return ERR_NOERROR;
}

static int h263_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{
    return ERR_PARSE;
}

static int h263_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

static int h263_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{

    return ERR_NOERROR;
}

static int h263_uninit(void *private_data)
{
    //that's all?
    if (private_data) free(private_data);
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(h263);

