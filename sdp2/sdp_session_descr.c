/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
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

#include <stdio.h>
#include <string.h>
#include <pwd.h>

#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <fenice/sdp2.h>
#include <fenice/multicast.h>
/*x-x*/
//#include <fenice/socket.h>
#include <netembryo/wsocket.h>

#include "sdp_get_version.c"


#define DESCRCAT(x) do { \
					 if ( (size_left -= x) < 0) \
					 	return ERR_INPUT_PARAM; \
					 	else cursor=descr+descr_size-size_left; \
					 } while(0);
int sdp_session_descr(char *n, char *descr, size_t descr_size)
{
	/*x-x*/
	//struct sockaddr_storage localaddr;
	//socklen_t localaddr_len = sizeof(localaddr);
	char localhostname[NI_MAXHOST];
	struct passwd *pwitem=getpwuid(getuid());
	gint64 size_left=descr_size;
	char *cursor=descr;
	ResourceDescr *r_descr;
	MediaDescrListArray m_descrs;
	sdp_field_list sdp_private;
//	GList *m = NULL;
//	MediaDescrList m_list;
	guint i;
	
	fnc_log(FNC_LOG_DEBUG, "[SDP2] opening %s\n", n);
	if ( !(r_descr=r_descr_get(prefs_get_serv_root(), n)) )
		return ERR_NOT_FOUND;
		
	// get name of localhost
	//if (getsockname(net_fd, (struct sockaddr *)&localaddr, &localaddr_len) < 0)
	//	return ERR_INPUT_PARAM; // given socket is not valid
	//if (getnameinfo((struct sockaddr *)&localaddr, localaddr_len, localhostname, sizeof(localhostname), NULL, 0, 0))
	//	return ERR_INPUT_PARAM; // could not get address name or IP
	if(get_local_hostname(localhostname,sizeof(localhostname))) {
		fnc_log(FNC_LOG_ERR, "[SDP2] get_local_hostname\n");
		return ERR_INPUT_PARAM; // could not get address name or IP
	}
	// v=
	DESCRCAT(g_snprintf(cursor, size_left, "v=%d"SDP2_EL, SDP2_VERSION))
	// o=
	DESCRCAT(g_strlcat(cursor, "o=", size_left ))
	if (pwitem && pwitem->pw_name && *pwitem->pw_name)
		DESCRCAT(g_strlcat(cursor, pwitem->pw_name, size_left))
	else
		DESCRCAT(g_strlcat(cursor, "-", size_left))
	DESCRCAT(g_strlcat(cursor, " ", size_left))
	DESCRCAT(sdp_session_id(cursor, size_left))
	DESCRCAT(g_strlcat(cursor," ", size_left))
	DESCRCAT(sdp_get_version(r_descr, cursor, size_left))
	DESCRCAT(g_strlcat(cursor, " IN IP4 ", size_left))		/* Network type: Internet; Address type: IP4. */
	DESCRCAT(g_strlcat(cursor, localhostname, size_left))
//	DESCRCAT(g_strlcat(cursor, get_address(), size_left))
   	DESCRCAT(g_strlcat(cursor, SDP2_EL, size_left))
	
	// s=
	if (r_descr_name(r_descr))
		DESCRCAT(g_snprintf(cursor, size_left, "s=%s"SDP2_EL, r_descr_name(r_descr)))
	else
		DESCRCAT(g_strlcat(cursor, "s=RTSP Session"SDP2_EL, size_left)) // TODO: choose a better session name
	// i=
	DESCRCAT(g_snprintf(cursor, size_left, "i=%s %s Streaming Server"SDP2_EL, PACKAGE, VERSION)) // TODO: choose a better session description
	// u=
	if (r_descr_descrURI(r_descr))
		DESCRCAT(g_snprintf(cursor, size_left, "u=%s"SDP2_EL, r_descr_descrURI(r_descr)))
	// e=
	if (r_descr_email(r_descr))
		DESCRCAT(g_snprintf(cursor, size_left, "e=%s"SDP2_EL, r_descr_email(r_descr)))
	// p=
	if (r_descr_phone(r_descr))
		DESCRCAT(g_snprintf(cursor, size_left, "p=%s"SDP2_EL, r_descr_phone(r_descr)))
	// c=
	DESCRCAT(g_strlcat(cursor, "c=", size_left))
	DESCRCAT(g_strlcat(cursor, "IN ", size_left))		/* Network type: Internet. */
	DESCRCAT(g_strlcat(cursor, "IP4 ", size_left))		/* Address type: IP4. */
	if(r_descr_multicast(r_descr)) {
		DESCRCAT(g_strlcat(cursor, r_descr_multicast(r_descr), size_left))
		DESCRCAT(g_strlcat(cursor, "/", size_left))
		if (r_descr_ttl(r_descr))
			DESCRCAT(g_snprintf(cursor, size_left, "%s"SDP2_EL, r_descr_ttl(r_descr)))
		else
			DESCRCAT(g_snprintf(cursor, size_left, "%d"SDP2_EL, DEFAULT_TTL))
//		sprintf(ttl, "%d", (int)DEFAULT_TTL);
//		strcat(cursor, ttl); /*TODO: the possibility to change ttl. See multicast.h, RTSP_setup.c, send_setup_reply.c*/
	} else
		DESCRCAT(g_strlcat(cursor, "0.0.0.0"SDP2_EL, size_left))
	// b=
	// t=
	DESCRCAT(g_strlcat(cursor, "t=0 0"SDP2_EL, size_left)) // TODO: should use better this parameter?
	// r=
	// z=
	// k=
	// a=
	// control attribute. We should look if aggregate metod is supported?
	DESCRCAT(g_snprintf(cursor, size_left, "a=control:%s"SDP2_EL, n))
	// other private data
	if ( (sdp_private=r_descr_sdp_private(r_descr)) )
		for (sdp_private = list_first(sdp_private); sdp_private; sdp_private = list_next(sdp_private)) {
			switch (SDP_FIELD(sdp_private)->type) {
				case empty_field:
					DESCRCAT(g_snprintf(cursor, size_left, "%s"SDP2_EL, SDP_FIELD(sdp_private)->field))
					break;
				// other supported private fields?
				default: // ignore private field
					break;
			}
		}
		
	// media
	m_descrs = r_descr_get_media(r_descr);

	for (i=0;i<m_descrs->len;i++) { // TODO: wrap g_array functions
//		printf("*** %d\n", i);
		sdp_media_descr(r_descr, array_data(m_descrs)[i], cursor, size_left);
	}
   	
	fnc_log(FNC_LOG_INFO, "\n[SDP2] description:\n%s\n", descr);
	
	return ERR_NOERROR;
}
#undef DESCRCAT
