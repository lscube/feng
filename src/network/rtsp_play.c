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

/** @file
 *  @brief Contains PLAY method and reply handlers
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <strings.h>

#include <bufferpool/bufferpool.h>
#include "rtsp.h"
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <math.h>
#include <netembryo/url.h>
#include <glib.h>

static int get_utc(struct tm *t, char *b)
{
    if ((sscanf(b,"%4d%2d%2dT%2d%2d%2d",
            &t->tm_year,&t->tm_mon,&t->tm_mday,
            &t->tm_hour,&t->tm_min,&t->tm_sec) == 6))
        return ERR_NOERROR;
    else
        return ERR_GENERIC;
}

/**
 * Parses the RANGE HEADER to get the required play range
 * @param rtsp the buffer from which to get the data
 * @param args where to save the play range informations
 * @return RTSP_Ok or RTSP_BadRequest on missing RANGE HEADER
 */
static gboolean parse_play_time_range(RTSP_buffer * rtsp, play_args * args)
{
    int time_taken = 0;
    char *p = NULL, *q = NULL;
    char tmp[5];

    /* Default values */
    args->playback_time_valid = 0;
    args->seek_time_valid = 0;
    args->start_time = gettimeinseconds(NULL);
    args->begin_time = 0.0;
    args->end_time = HUGE_VAL;

    if ((p = strstr(rtsp->in_buffer, HDR_RANGE)) != NULL) {
        if ((q = strstr(p, "npt")) != NULL) {      // FORMAT: npt
            if ((q = strchr(q, '=')) == NULL)
                return false;    /* Bad Request */

            if (sscanf(q + 1, "%lf", &(args->begin_time)) != 1) {
                if (sscanf(q + 1, "%4s", tmp) != 1 && !g_ascii_strcasecmp(tmp,"now-")) {
                    return false;
                }
            } else {
                args->seek_time_valid = 1;
            }

            if ((q = strchr(q, '-')) == NULL)
                return false;

            if (sscanf(q + 1, "%lf", &(args->end_time)) != 1)
                args->end_time = HUGE_VAL;
        } else if ((q = strstr(p, "smpte")) != NULL) { // FORMAT: smpte
            // Currently unsupported. Using default.
        } else if ((q = strstr(p, "clock"))!= NULL) { // FORMAT: clock
            // Currently unsupported. Using default.
        }
#if 0
        else if ((q = strstr(p, "time")) == NULL) { // No specific format. Assuming NeMeSI format.
            double t;
            if ((q = strstr(p, ":"))) {
                // Hours
                sscanf(q + 1, "%lf", &t);
                args->start_time = t * 60 * 60;
            }
            if ((q = strstr(q + 1, ":"))) {
                // Minutes
                sscanf(q + 1, "%lf", &t);
                args->start_time += (t * 60);
            }
            if ((q = strstr(q + 1, ":"))) {
                // Seconds
                sscanf(q + 1, "%lf", &t);
                args->start_time += t;
            }
            args->start_time_valid = 1;
        }
#endif
        else {
            // no range defined but start time expressed?
            time_taken = 1;
        }

        if ((q = strstr(p, "time")) == NULL) {
            // Start playing immediately
            memset(&(args->playback_time), 0, sizeof(args->playback_time));
        } else {
            // Start playing at desired time
            if (!time_taken) {
                if ((q = strchr(q, '=')) &&
                    get_utc(&(args->playback_time), q + 1) == ERR_NOERROR) {
                    args->playback_time_valid = 1;
                } else {
                    memset(&(args->playback_time), 0,
                           sizeof(args->playback_time));
                }
            }
        }
    } else {
        args->begin_time = 0.0;
        args->end_time = HUGE_VAL;
        memset(&(args->playback_time), 0, sizeof(args->playback_time));
    }

    return true;
}

static void rtp_session_seek(gpointer value, gpointer user_data)
{
  RTP_session *rtp_sess = (RTP_session *)value;
  play_args *args = (play_args *)user_data;

  if (rtp_sess->started && !rtp_sess->pause) {
    /* Pause scheduler while reiniting RTP session */
    rtp_sess->pause = 1;
  }
  rtp_sess->start_seq = 1 + rtp_sess->seq_no;
  rtp_sess->start_rtptime = g_random_int();
  rtp_sess->seek_time = args->begin_time;

  if (rtp_sess->cons) {
    while (bp_getreader(rtp_sess->cons)) {
      /* Drop spurious packets after seek */
      bp_gotreader(rtp_sess->cons);
    }
  }
}

/**
 * Actually seek through the media using mediathread
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InvalidRange
 */
static ProtocolReply do_seek(RTSP_session * rtsp_sess, play_args * args)
{
    Resource *r = rtsp_sess->resource;

    if (args->seek_time_valid &&
        ((rtsp_sess->started && args->begin_time == 0.0)
            || args->begin_time > 0.0)) {
        if(mt_resource_seek(r, args->begin_time)) {
            return RTSP_HeaderFieldNotValidforResource;
        }
	g_slist_foreach(rtsp_sess->rtp_sessions, rtp_session_seek, args);
    } else if (args->begin_time < 0.0) {
        fnc_log(FNC_LOG_DEBUG,"[RTSP] Negative seek to %f", args->begin_time);
        return RTSP_InvalidRange;
    }

    return RTSP_Ok;
}

static void rtp_session_play(gpointer value, gpointer user_data)
{
  RTP_session *rtp_sess = (RTP_session *)value;
  play_args *args = (play_args *)user_data;

  // Start playing all the presentation
  if (!rtp_sess->started) {
    // Start new -- assume no allocation error
    schedule_start (rtp_sess, args);
  } else {
    // Resume existing
    if (rtp_sess->pause) {
      schedule_resume (rtp_sess, args);
    }
  }
}

/**
 * Actually starts playing the media using mediathread
 * @param url the Url for which to start playing
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InternalServerError
 */
static ProtocolReply
do_play(Url *url, RTSP_session * rtsp_sess, play_args * args)
{
    ProtocolReply error = RTSP_Ok;
    char *q = NULL;

    if (!(q = strchr(url->path, '='))) {
        //if '=' is not present then a file has not been specified
        // aggregate content requested
        // Perform seek if needed
        if ((error = do_seek(rtsp_sess, args)).error) {
            return error;
        }
	if ( rtsp_sess->rtp_sessions &&
	     ((RTP_session*)(rtsp_sess->rtp_sessions->data))->is_multicast )
	  return RTSP_Ok;

	rtsp_sess->started = 1;

	g_slist_foreach(rtsp_sess->rtp_sessions, rtp_session_play, args);
    } else {
        // FIXME complete with the other stuff
        return RTSP_InternalServerError; /* Internal server error */
#if 0
        // resource!trackname
        strcpy (trackname, q + 1);
        // XXX Not really nice...
        while (url->path != q) if (*--q == '/') break;
        *q = '\0';
#endif
    }

    return RTSP_Ok;
}

typedef struct {
  GString *reply;
  char *server;
} rtp_session_send_play_reply_pair;

static void rtp_session_send_play_reply(gpointer element, gpointer user_data)
{
  rtp_session_send_play_reply_pair *pair = (rtp_session_send_play_reply_pair *)user_data;

  RTP_session *p = (RTP_session *)element;
  Track *t = r_selected_track(p->track_selector);

  char temp[256];
  Url_encode(temp, p->sd_filename, sizeof(temp));

  g_string_append_printf(pair->reply,
			 "url=rtsp://%s/%s/"SDP2_TRACK_ID"=", pair->server, temp);

  Url_encode(temp, t->info->name, sizeof(temp));

  g_string_append_printf(pair->reply,
			 "%s;seq=%u", temp, p->start_seq);

  if (t->properties->media_source != MS_live)
    g_string_append_printf(pair->reply,
			   ";rtptime=%u", p->start_rtptime);

  g_string_append(pair->reply, ",");
}

/**
 * Sends the reply for the play method
 * @param rtsp the buffer where to write the reply
 * @param url the Url related to the object that we wanted to play
 * @param rtsp_session the session for which to generate the reply
 * @return ERR_NOERROR
 */
static int send_play_reply(RTSP_buffer * rtsp, Url *url,
                           RTSP_session * rtsp_session, play_args * args)
{
    GString *reply = rtsp_generate_ok_response(rtsp->rtsp_cseq, rtsp_session->session_id);
    rtp_session_send_play_reply_pair pair = {
      .reply = reply,
      .server = url->hostname
    };

    g_string_append_printf(reply, "Range: npt=%f-", args->begin_time);

    if (args->end_time != HUGE_VAL)
      g_string_append_printf(reply, "%f", args->end_time);

    g_string_append(reply, RTSP_EL);

    g_string_append(reply, "RTP-info: ");

    g_slist_foreach(rtsp_session->rtp_sessions, rtp_session_send_play_reply, &pair);

    g_string_truncate(reply, reply->len-1);

    g_string_append(reply, RTSP_EL);

    // end of message
    g_string_append(reply, RTSP_EL);

    bwrite(reply, rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - %s ", url->path);

    return ERR_NOERROR;
}

/**
 * RTSP PLAY method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_play(RTSP_buffer * rtsp)
{
    Url url;
    RTSP_session *rtsp_sess;
    play_args args;

    ProtocolReply error;

    // Parse the input message

    // Get the range
    if ( !parse_play_time_range(rtsp, &args) ) {
        error = RTSP_BadRequest;
        goto error_management;
    }
    
    rtsp_sess = rtsp->session;
    
    if ( rtsp_sess->session_id == 0 ) {
        error = RTSP_BadRequest;
        goto error_management;
    }

    // Extract and validate the URL
    if ( (error = rtsp_extract_validate_url(rtsp, &url)).error )
        goto error_management;

    if ( (error = do_play(&url, rtsp_sess, &args)).error ) {
        goto error_management;
    }

    fnc_log(FNC_LOG_INFO, "PLAY %s://%s/%s RTSP/1.0 ",
	    url.protocol, url.hostname, url.path);
    send_play_reply(rtsp, &url, rtsp_sess, &args);
    log_user_agent(rtsp); // See User-Agent
    return ERR_NOERROR;

error_management:
    rtsp_send_reply(rtsp, error);
    return ERR_GENERIC;
}
