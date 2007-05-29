/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 * 
 *  - (LS)� Team            <team@streaming.polito.it>    
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/sdp2.h>
#include <fenice/utils.h>

#define DESCRCAT(x) \
    do { \
        if ( (size_left -= x) < 0) \
        return ERR_INPUT_PARAM; \
        else cursor = descr + descr_size - size_left; \
    } while(0);

int sdp_media_descr(ResourceDescr *r_descr, MediaDescrList m_descr_list,
                    char *descr, uint32 descr_size)
{
    MediaDescr *m_descr = m_descr_list ? MEDIA_DESCR(m_descr_list) : NULL;
    MediaDescrList tmp_mdl;
    sdp_field_list sdp_private;
    gint64 size_left = descr_size;
    char *cursor = descr;
    
    if (!m_descr)
        return ERR_INPUT_PARAM;
    // m=
    switch (m_descr_type(m_descr)) {
        case MP_audio:
            DESCRCAT(g_strlcat(cursor, "m=audio ", size_left))
            break;
        case MP_video:
            DESCRCAT(g_strlcat(cursor, "m=video ", size_left))
            break;
        case MP_application:
            DESCRCAT(g_strlcat(cursor, "m=application ", size_left))
            break;
        case MP_data:
            DESCRCAT(g_strlcat(cursor, "m=data ", size_left))
            break;
        case MP_control:
            DESCRCAT(g_strlcat(cursor, "m=control ", size_left))
            break;
        default:
            return ERR_INPUT_PARAM;
            break;
    }

    // shawill: TODO: probably the transport should not be hard coded,
        // but obtained in some way
    DESCRCAT(g_snprintf(cursor, size_left, "%d RTP/AVP",
             m_descr_rtp_port(m_descr) ))

    for (tmp_mdl = list_first(m_descr_list); tmp_mdl;
         tmp_mdl = list_next(tmp_mdl))
            DESCRCAT(g_snprintf(cursor, size_left, " %u",
                     m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl))))

    DESCRCAT(g_strlcat(cursor, SDP2_EL, size_left))

    // i=*
    // c=*
    // b=*
    // k=*
    // a=*
    DESCRCAT(g_snprintf(cursor, size_left, "a=control:%s!%s"SDP2_EL,
            r_descr_name(r_descr), m_descr_name(m_descr)))

    // other sdp private data
    for (tmp_mdl = list_first(m_descr_list); tmp_mdl;
         tmp_mdl = list_next(tmp_mdl))
        if ((sdp_private = m_descr_sdp_private(MEDIA_DESCR(tmp_mdl))))
            for (sdp_private = list_first(sdp_private); sdp_private;
                 sdp_private = list_next(sdp_private)) {
                switch (SDP_FIELD(sdp_private)->type) {
                    case empty_field:
                        DESCRCAT(
                            g_snprintf(cursor, size_left,"%s"SDP2_EL,
                                        SDP_FIELD(sdp_private)->field))
                    break;
                    case fmtp:
                        DESCRCAT(
                            g_snprintf(cursor, size_left, "a=fmtp:%u %s"SDP2_EL,
                                        m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl)),
                                        SDP_FIELD(sdp_private)->field))
                    break;
                    case rtpmap:
                        DESCRCAT(
                            g_snprintf(cursor, size_left, "a=rtpmap:%u %s"SDP2_EL,
                                        m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl)),
                                        SDP_FIELD(sdp_private)->field))
                    break;
                // other supported private fields?
                default: // ignore private field
                    break;
            }
        }
    // CC licenses *
    if (m_descr_commons_deed(m_descr))
        DESCRCAT(g_snprintf(cursor, size_left, "a=uriLicense:%s"SDP2_EL,
                 m_descr_commons_deed(m_descr)))
    if (m_descr_rdf_page(m_descr))
        DESCRCAT(g_snprintf(cursor, size_left, "a=uriMetadata:%s"SDP2_EL,
                 m_descr_rdf_page(m_descr)))
    if (m_descr_title(m_descr))
        DESCRCAT(g_snprintf(cursor, size_left, "a=title:%s"SDP2_EL,
                 m_descr_title(m_descr)))
    if (m_descr_author(m_descr))
        DESCRCAT(g_snprintf(cursor, size_left, "a=author:%s"SDP2_EL,
                 m_descr_author(m_descr)))
        
    return ERR_NOERROR;
}
#undef DESCRCAT
