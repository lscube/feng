/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
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

#ifndef FN_UTILS_H
#define FN_UTILS_H

#include "config.h"

#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>

//XXX should me moved somewhere else
#ifdef WIN32
#define open(a, b)   _open(a, b)
#define read(a,b,c)  _read(a,b,c)
#define lseek(a,b,c) _lseek(a,b,c)
#define close(a)     _close(a)
#endif

/*! autodescriptive error values */
#define ERR_NOERROR              0
#define ERR_GENERIC             -1
#define ERR_NOT_FOUND           -2
#define ERR_PARSE               -3
#define ERR_ALLOC               -4
#define ERR_INPUT_PARAM         -5
#define ERR_NOT_SD              -6  // XXX check it
#define ERR_UNSUPPORTED_PT      -7  /// Unsupported Payload Type
#define ERR_EOF                 -8
#define ERR_FATAL               -9
#define ERR_CONNECTION_CLOSE    -10

/*! message header keywords see rfc2326 and rfc2068 */

#define HDR_CONTENTLENGTH       "Content-Length"
#define HDR_ACCEPT              "Accept"
#define HDR_ALLOW               "Allow"
#define HDR_BLOCKSIZE           "Blocksize"
#define HDR_CONTENTTYPE         "Content-Type"
#define HDR_DATE                "Date"
#define HDR_REQUIRE             "Require"
#define HDR_TRANSPORTREQUIRE    "Transport-Require"
#define HDR_SEQUENCENO          "SequenceNo"
#define HDR_CSEQ                "CSeq"
#define HDR_STREAM              "Stream"
#define HDR_SESSION             "Session"
#define HDR_TRANSPORT           "Transport"
#define HDR_RANGE               "Range"
#define HDR_USER_AGENT          "User-Agent"

/*! rtsp methods */
#define RTSP_METHOD_MAXLEN 15
#define RTSP_METHOD_DESCRIBE        "DESCRIBE"
#define RTSP_METHOD_ANNOUNCE        "ANNOUNCE"
#define RTSP_METHOD_GET_PARAMETERS  "GET_PARAMETERS"
#define RTSP_METHOD_OPTIONS         "OPTIONS"
#define RTSP_METHOD_PAUSE           "PAUSE"
#define RTSP_METHOD_PLAY            "PLAY"
#define RTSP_METHOD_RECORD          "RECORD"
#define RTSP_METHOD_REDIRECT        "REDIRECT"
#define RTSP_METHOD_SETUP           "SETUP"
#define RTSP_METHOD_SET_PARAMETER   "SET_PARAMETER"
#define RTSP_METHOD_TEARDOWN        "TEARDOWN"
/*! rtsp method tokens */
#define RTSP_ID_DESCRIBE 0
#define RTSP_ID_ANNOUNCE 1
#define RTSP_ID_GET_PARAMETERS 2
#define RTSP_ID_OPTIONS 3
#define RTSP_ID_PAUSE 4
#define RTSP_ID_PLAY 5
#define RTSP_ID_RECORD 6
#define RTSP_ID_REDIRECT 7
#define RTSP_ID_SETUP 8
#define RTSP_ID_SET_PARAMETER 9
#define RTSP_ID_TEARDOWN 10


//XXX move them in a separate header?
/*! SD common tags */
#define SD_STREAM           "stream"
#define SD_STREAM_END       "stream_end"
#define SD_FILENAME         "file_name"
#define SD_CLOCK_RATE       "clock_rate"
#define SD_PAYLOAD_TYPE     "payload_type"
#define SD_AUDIO_CHANNELS   "audio_channels"
#define SD_ENCODING_NAME    "encoding_name"
#define SD_MEDIA_TYPE       "media_type"
#define SD_BIT_PER_SAMPLE   "bit_per_sample"
#define SD_SAMPLE_RATE      "sample_rate"
#define SD_CODING_TYPE      "coding_type"
#define SD_FRAME_LEN        "frame_len"
#define SD_PKT_LEN          "pkt_len"
#define SD_PRIORITY         "priority"
#define SD_BITRATE          "bitrate"
#define SD_FRAME_RATE       "frame_rate"
#define SD_FORCE_FRAME_RATE "force_frame_rate"
#define SD_BYTE_PER_PCKT    "byte_per_pckt"
#define SD_MEDIA_SOURCE     "media_source"
#define SD_TWIN             "twin"
#define SD_MULTICAST        "multicast"
#define SD_PORT             "port"
#define SD_FMTP             "fmtp"
/*! Creative commons specific tags */
#define SD_LICENSE          "license"
#define SD_RDF              "verify"
#define SD_TITLE            "title"
#define SD_CREATOR          "creator"

#define NTP_time(t) ((float)t + 2208988800U)

/**
 * Returns the current time in seconds
 */
#ifdef HAVE_CLOCK_GETTIME
static inline double gettimeinseconds(struct timespec *now) {
    struct timespec tmp;
    if (!now) {
        now = &tmp;
    }
    clock_gettime(CLOCK_REALTIME, now);
    return (double)now->tv_sec + (double)now->tv_nsec * .000000001;
}
#else
#warning Posix RealTime features not available
static inline double gettimeinseconds(struct timespec *now) {
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    if (now) {
        now->tv_sec = tmp.tv_sec;
        now->tv_nsec = tmp.tv_usec * 1000;
    }
    return (double)tmp.tv_sec + (double)tmp.tv_usec * .000001;
}
#endif

#endif // FN_UTILS_H
