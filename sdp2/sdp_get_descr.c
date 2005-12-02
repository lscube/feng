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
#include <netdb.h> // for getnameinfo()

#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <fenice/sdp2.h>
#include <fenice/socket.h>


#define CK_OVERFLOW(x) if ( (size_left -= x) < 0) return ERR_INPUT_PARAM;
int sdp_get_descr(resource_name n, int net_fd, char *descr, size_t descr_size)
{
	struct sockaddr_storage localaddr;
	socklen_t localaddr_len = sizeof(localaddr);
	char thefile[255], localhostname[NI_MAXHOST];
	struct passwd *pwitem=getpwuid(getuid());
	gint64 size_left=descr_size;
	ResourceDescr *r_descr;

	strcpy(thefile, prefs_get_serv_root());
	strcat(thefile, n);
	
	fnc_log(FNC_LOG_DEBUG, "[SDP2] opening %s\n", thefile);
	if ( !(r_descr=r_descr_get(thefile)) )
		return ERR_NOT_FOUND;
	// get name of localhost
	if (getsockname(net_fd, (struct sockaddr *)&localaddr, &localaddr_len) < 0)
		return ERR_INPUT_PARAM; // given socket is not valid
	if (getnameinfo((struct sockaddr *)&localaddr, localaddr_len, localhostname, sizeof(localhostname), NULL, 0, NI_NUMERICHOST))
		return ERR_INPUT_PARAM; // could not get address name or IP
	// v=
	CK_OVERFLOW(g_snprintf(descr, size_left, "v=%d"SDP2_EL, SDP2_VERSION))
	// o=
	CK_OVERFLOW(g_strlcat(descr, "o=", size_left ))
	// g_strlcat(descr, PACKAGE, descr_size-strlen(descr));
	if (pwitem && pwitem->pw_name && *pwitem->pw_name) {
		CK_OVERFLOW(g_strlcat(descr, pwitem->pw_name, size_left))
	} else {
		CK_OVERFLOW(g_strlcat(descr, "-", size_left))
	}
	CK_OVERFLOW(g_strlcat(descr, " ", size_left))
	CK_OVERFLOW(sdp_session_id(descr+strlen(descr), size_left))
	CK_OVERFLOW(g_strlcat(descr," ", size_left))
	CK_OVERFLOW(sdp_get_version(r_descr, descr+strlen(descr), size_left))
	CK_OVERFLOW(g_strlcat(descr, " IN IP4 ", size_left))		/* Network type: Internet; Address type: IP4. */
//	CK_OVERFLOW(g_strlcat(descr, localhostname, size_left))
	CK_OVERFLOW(g_strlcat(descr, get_address(), size_left))
   	CK_OVERFLOW(g_strlcat(descr, SDP2_EL, size_left))
   	// c=
	CK_OVERFLOW(g_strlcat(descr, "c=", size_left))
	CK_OVERFLOW(g_strlcat(descr, "IN ", size_left))		/* Network type: Internet. */
	CK_OVERFLOW(g_strlcat(descr, "IP4 ", size_left))		/* Address type: IP4. */
	
	// s=
	// i=
	// u=
	// e=
	// p=
	// c=
	// b=
	// z=
	// k=
	// a=
	
	// t=
	// r=
	
	// media
   	
	fnc_log(FNC_LOG_INFO, "\n[SDP2] description:\n%s\n", descr);
	
	return 0;
}
#undef CK_OVERFLOW
