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

int sdp_get_descr(resource_name n, char *descr, uint32 descr_size)
{
	char thefile[255];
	struct passwd *pwitem=getpwuid(getuid());
	gint used_size=0;
	ResourceDescr *r_descr;

	strcpy(thefile, prefs_get_serv_root());
	strcat(thefile, n);
	
	fnc_log(FNC_LOG_DEBUG, "[SDP] opening %s\n", thefile);
	if ( !(r_descr=r_descr_get(thefile)) )
		return ERR_NOT_FOUND;
	
	g_snprintf(descr,descr_size , "v=%d"SDP2_EL, SDP2_VERSION);
	g_strlcat(descr, "o=", descr_size-strlen(descr));
//	g_strlcat(descr, PACKAGE, descr_size-strlen(descr));
   	if (pwitem && pwitem->pw_name && *pwitem->pw_name)
		strcat(descr, pwitem->pw_name);
	else
		strcat(descr, "-");
   	g_strlcat(descr," ", descr_size-strlen(descr));
//   	strcat(descr, get_SDP_session_id(s));
   	strcat(descr," ");
   	used_size = strlen(descr);
   	if (sdp_get_version(r_descr, descr+used_size, descr_size-used_size) < (gint)descr_size-used_size)
   		return ERR_INPUT_PARAM;
//   	strcat(descr, get_SDP_version(s));
   	strcat(descr, SDP2_EL);
   	strcat(descr, "c=");
   	strcat(descr, "IN ");		/* Network type: Internet. */
   	strcat(descr, "IP4 ");		/* Address type: IP4. */
   	
	return 0;
}
