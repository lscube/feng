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

#include <stdio.h>
#include <string.h>

#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <fenice/sdp2.h>
#include <fenice/description.h>
#include <fenice/multicast.h>
/*x-x*/
//#include <fenice/socket.h>
#include <netembryo/wsocket.h>
#include <netembryo/url.h>

/**
 * This function creates an array of MediaDescrList containing media
 * descriptions.
 * This array is returned to function caller.
 * Each element of the array is a MediaDescrList that contain all the
 * media of the same type with the same name.
 * All the elements of each list can be included together in the sdp
 * description in a single m= block.
 * @param r_descr Resource description that contains all the media
 * @param m_descrs this is a return parameter.
 *  It will contain the MediaDescrList array
 * @return the dimension of the array or an interger < 0 if an error occurred.
 * */
static MediaDescrListArray r_descr_get_media(ResourceDescr *r_descr)
{
    MediaDescrListArray new_m_descrs;
    MediaDescrList m_descr_list, m_descr;
    guint i;
    gboolean found;

    new_m_descrs = g_ptr_array_sized_new(
                        g_list_position(r_descr->media,
                                        g_list_last(r_descr->media))+1);

    for (m_descr = g_list_first(r_descr->media);
         m_descr;
         m_descr = g_list_next(m_descr)) {
        found = 0;
        for (i = 0; i < new_m_descrs->len; ++i) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            if ( (m_descr_type(MEDIA_DESCR(m_descr)) ==
                  m_descr_type(MEDIA_DESCR(m_descr_list))) &&
                 !strcmp(m_descr_name(MEDIA_DESCR(m_descr)),
                         m_descr_name(MEDIA_DESCR(m_descr_list))) ) {
                found = 1;
                break;
            }
        }
        if (found) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            m_descr_list = g_list_prepend(m_descr_list, MEDIA_DESCR(m_descr));
            new_m_descrs->pdata[i]=m_descr_list;
        } else {
            m_descr_list = g_list_prepend(NULL, MEDIA_DESCR(m_descr));
            g_ptr_array_add(new_m_descrs, m_descr_list);
        }
    }

    for (i = 0; i < new_m_descrs->len; ++i) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            m_descr_list = g_list_reverse(m_descr_list);
    }

    return new_m_descrs;
}

GString *sdp_session_descr(feng *srv, char *server, char *name)
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
    if ( !(r_descr=r_descr_get(srv, prefs_get_serv_root(), name)) ) {
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
	  /*TODO: the possibility to change ttl.
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
    /// @TODO: choose a better session description
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
		  g_string_append_printf("%s"SDP2_EL,
					 SDP_FIELD(sdp_private)->field);
                    break;
                // other supported private fields?
                default: // ignore private field
                    break;
            }
        }

    // media
    m_descrs = r_descr_get_media(r_descr);

    for (i=0;i<m_descrs->len;i++) { // TODO: wrap g_array functions
        sdp_media_descr(array_data(m_descrs)[i], descr);
    }

    fnc_log(FNC_LOG_INFO, "[SDP2] description:\n%s", descr);

    return descr;
}
