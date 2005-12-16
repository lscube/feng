/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <fenice/sdp2.h>
#include <fenice/utils.h>

#define DESCRCAT(x) { if ( (size_left -= x) < 0) return ERR_INPUT_PARAM; else cursor=descr+descr_size-size_left; }
int sdp_media_descr(ResourceDescr *r_descr, MediaDescrList m_descr_list, char *descr, uint32 descr_size)
{
	MediaDescr *m_descr = m_descr_list ? MEDIA_DESCR(m_descr_list) : NULL;
	MediaDescrList tmp_mdl;
	gint64 size_left=descr_size;
	char *cursor=descr;
	
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

	// shawill: TODO: probably the transport should not be hard coded, but obtained in some way
	DESCRCAT(g_snprintf(cursor, size_left, "%d RTP/AVP", m_descr_rtp_port(m_descr) ))
	for (tmp_mdl = mdl_first(m_descr_list); tmp_mdl; tmp_mdl=mdl_next(tmp_mdl))
		DESCRCAT(g_snprintf(cursor, size_left, " %u", m_descr_rtp_pt(MEDIA_DESCR(tmp_mdl))))
	DESCRCAT(g_strlcat(cursor, SDP2_EL, size_left))
	
	// i=*
	// c=*
	// b=*
	// k=*
	// a=*
	DESCRCAT(g_snprintf(cursor, size_left, "a=control:%s!%s"SDP2_EL, r_descr_mrl(r_descr), m_descr_name(m_descr)))
	// CC licenses *
		
	return ERR_NOERROR;
}
#undef DESCRCAT
