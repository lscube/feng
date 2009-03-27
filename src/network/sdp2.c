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

/**
 * @file
 * @brief SDP session description
 *
 * This file contains the function used to generate the SDP session
 * description as needed by @ref RTSP_describe.
 */

#include <string.h>
#include <stdbool.h>

#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include "sdp2.h"
#include "mediathread/description.h"
#include <fenice/multicast.h>
#include <netembryo/wsocket.h>
#include <netembryo/url.h>

/**
 * @brief Append the description for a given media to an SDP
 *        description.
 *
 * @param m_descr_list List of MediaDescr instances to fetch the data
 *                     from.
 * @param descr GString instance to append the description to
 */
static void sdp_media_descr(MediaDescrList m_descr_list, GString *descr)
{
    MediaDescr *m_descr = m_descr_list ? MEDIA_DESCR(m_descr_list) : NULL;
    MediaDescrList tmp_mdl;
    sdp_field_list sdp_private;
    char *encoded_media_name;

    if (!m_descr)
        return;

    // m=
    /// @TODO Convert this to a string table
    switch (m_descr_type(m_descr)) {
    case MP_audio:
        g_string_append(descr, "m=audio ");
        break;
    case MP_video:
        g_string_append(descr, "m=video ");
        break;
    case MP_application:
        g_string_append(descr, "m=application ");
        break;
    case MP_data:
        g_string_append(descr, "m=data ");
        break;
    case MP_control:
        g_string_append(descr, "m=control ");
        break;
    }

    /// @TODO shawill: probably the transport should not be hard coded,
    /// but obtained in some way
    g_string_append_printf(descr, "%d RTP/AVP",
			   m_descr_rtp_port(m_descr));

    for (tmp_mdl = list_first(m_descr_list); tmp_mdl;
         tmp_mdl = list_next(tmp_mdl))
      g_string_append_printf(descr, " %u",
			     m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl)));

    g_string_append(descr, SDP2_EL);

    // i=*
    // c=*
    // b=*
    // k=*
    // a=*
    encoded_media_name = g_uri_escape_string(m_descr_name(m_descr), NULL, false);

    g_string_append_printf(descr, "a=control:"SDP2_TRACK_ID"=%s"SDP2_EL,
			   encoded_media_name);
    g_free(encoded_media_name);

    if (m_descr_frame_rate(m_descr) && m_descr_type(m_descr) == MP_video)
      g_string_append_printf(descr, "a=framerate:%f"SDP2_EL,
			     m_descr_frame_rate(m_descr));

    // other sdp private data
    for (tmp_mdl = list_first(m_descr_list); tmp_mdl;
         tmp_mdl = list_next(tmp_mdl))
        if ((sdp_private = m_descr_sdp_private(MEDIA_DESCR(tmp_mdl))))
            for (sdp_private = list_first(sdp_private); sdp_private;
                 sdp_private = list_next(sdp_private)) {
                switch (SDP_FIELD(sdp_private)->type) {
                    case empty_field:
		      g_string_append_printf(descr, "%s"SDP2_EL,
					     SDP_FIELD(sdp_private)->field);
                    break;
                    case fmtp:
		      g_string_append_printf(descr, "a=fmtp:%u %s"SDP2_EL,
					     m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl)),
					     SDP_FIELD(sdp_private)->field);
                    break;
                    case rtpmap:
		      g_string_append_printf(descr, "a=rtpmap:%u %s"SDP2_EL,
					     m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl)),
					     SDP_FIELD(sdp_private)->field);
                    break;
                // other supported private fields?
                default: // ignore private field
                    break;
            }
        }
    // CC licenses *
    if (m_descr_commons_deed(m_descr))
      g_string_append_printf(descr, "a=uriLicense:%s"SDP2_EL,
			     m_descr_commons_deed(m_descr));
    if (m_descr_rdf_page(m_descr))
      g_string_append_printf(descr, "a=uriMetadata:%s"SDP2_EL,
			     m_descr_rdf_page(m_descr));
    if (m_descr_title(m_descr))
      g_string_append_printf(descr, "a=title:%s"SDP2_EL,
			     m_descr_title(m_descr));
    if (m_descr_author(m_descr))
      g_string_append_printf(descr, "a=author:%s"SDP2_EL,
			     m_descr_author(m_descr));
}

/**
 * @brief Create description for an SDP session
 *
 * @param srv Pointer to the server-specific data instance.
 * @param server Hostname of the server used for the request.
 * @param name Name of the resource to describe.
 *
 * @return A new GString containing the complete description of the
 *         session or NULL if the resource was not found or no demuxer
 *         was found to handle it.
 *
 * @TODO Consider receiving a netembryo Url structure pointer instead
 *       of separated server and resource names.
 */
GString *sdp_session_descr(struct feng *srv, const char *server, const char *name)
{
    GString *descr = NULL;
    ResourceDescr *r_descr;
    MediaDescrListArray m_descrs;
    sdp_field_list sdp_private;
    guint i;
    double duration;

    const char *resname;
    time_t restime;
    float currtime_float, restime_float;

    fnc_log(FNC_LOG_DEBUG, "[SDP2] opening %s", name);
    if ( !(r_descr=r_descr_get(srv, name)) ) {
        fnc_log(FNC_LOG_ERR, "[SDP2] %s not found", name);
        return NULL;
    }

    descr = g_string_new("");

    /* Near enough approximation to run it now */
    currtime_float = NTP_time(time(NULL));
    restime = r_descr_last_change(r_descr);
    restime_float = restime ? NTP_time(restime) : currtime_float;

    if ( (resname = r_descr_name(r_descr)) == NULL )
      resname = "RTSP Session";

    g_string_append_printf(descr, "v=%d"SDP2_EL, SDP2_VERSION);

    /* Network type: Internet; Address type: IP4. */
    g_string_append_printf(descr, "o=- %.0f %.0f IN IP4 %s"SDP2_EL,
			   currtime_float, restime_float, server);

    g_string_append_printf(descr, "s=%s"SDP2_EL,
			   resname);
    // u=
    if (r_descr_descrURI(r_descr))
      g_string_append_printf(descr, "u=%s"SDP2_EL,
			     r_descr_descrURI(r_descr));

    // e=
    if (r_descr_email(r_descr))
      g_string_append_printf(descr, "e=%s"SDP2_EL,
			     r_descr_email(r_descr));
    // p=
    if (r_descr_phone(r_descr))
      g_string_append_printf(descr, "p=%s"SDP2_EL,
			     r_descr_phone(r_descr));

    // c=
    /* Network type: Internet. */
    /* Address type: IP4. */
    g_string_append(descr, "c=IN IP4 ");

    if(r_descr_multicast(r_descr)) {
        g_string_append_printf(descr, "%s/",
			       r_descr_multicast(r_descr));
        if (r_descr_ttl(r_descr))
	  g_string_append_printf(descr, "%s"SDP2_EL,
				 r_descr_ttl(r_descr));
        else
	  /* @TODO the possibility to change ttl.
	   * See multicast.h, RTSP_setup.c, send_setup_reply.c*/
	  g_string_append_printf(descr, "%d"SDP2_EL,
				 DEFAULT_TTL);
    } else
	g_string_append(descr, "0.0.0.0"SDP2_EL);

    // b=
    // t=
    g_string_append(descr, "t=0 0"SDP2_EL);
    // r=
    // z=
    // k=
    // a=
    // type attribute. We offer only broadcast
    g_string_append(descr, "a=type:broadcast"SDP2_EL);
    // tool attribute. Feng promo
    /// @TODO choose a better session description
    g_string_append_printf(descr, "a=tool:%s %s Streaming Server"SDP2_EL,
		    PACKAGE, VERSION);
    // control attribute. We should look if aggregate metod is supported?
    g_string_append(descr, "a=control:*"SDP2_EL);

    if ((duration = r_descr_time(r_descr)) > 0)
      g_string_append_printf(descr, "a=range:npt=0-%f"SDP2_EL, duration);

    // other private data
    if ( (sdp_private=r_descr_sdp_private(r_descr)) )
        for (sdp_private = list_first(sdp_private);
             sdp_private;
             sdp_private = list_next(sdp_private)) {
            switch (SDP_FIELD(sdp_private)->type) {
                case empty_field:
		  g_string_append_printf(descr, "%s"SDP2_EL,
					 SDP_FIELD(sdp_private)->field);
                    break;
                // other supported private fields?
                default: // ignore private field
                    break;
            }
        }

    // media
    m_descrs = r_descr_get_media(r_descr);

    for (i=0;i<m_descrs->len;i++) { /// @TODO wrap g_array functions
        sdp_media_descr(array_data(m_descrs)[i], descr);
    }

    fnc_log(FNC_LOG_INFO, "[SDP2] description:\n%s", descr->str);

    return descr;
}
