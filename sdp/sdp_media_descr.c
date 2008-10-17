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

#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>
#include <fenice/description.h>
#include <fenice/utils.h>
#include <netembryo/url.h>

int sdp_media_descr(MediaDescrList m_descr_list, GString *descr)
{
    MediaDescr *m_descr = m_descr_list ? MEDIA_DESCR(m_descr_list) : NULL;
    MediaDescrList tmp_mdl;
    sdp_field_list sdp_private;
    char encoded_media_name[256];

    if (!m_descr)
        return ERR_INPUT_PARAM;
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
        default:
	  return ERR_INPUT_PARAM;
	  break;
    }

    // shawill: TODO: probably the transport should not be hard coded,
        // but obtained in some way
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
    Url_encode (encoded_media_name, m_descr_name(m_descr),
                sizeof(encoded_media_name));

    g_string_append_printf(descr, "a=control:"SDP2_TRACK_ID"=%s"SDP2_EL,
			   encoded_media_name);
			   
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

    return ERR_NOERROR;
}
