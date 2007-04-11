/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
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

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>

#include <RTSP_utils.h>

/*
     ****************************************************************
     *            DESCRIBE METHOD HANDLING
     ****************************************************************
*/
int RTSP_describe(RTSP_buffer * rtsp)
{
    char url[255];
    ConnectionInfo cinfo;

    int error_id = 0;

    cinfo.descr_format = df_SDP_format; // shawill put to some default

    if ( (error_id = extract_url(rtsp, url)) ) // Extract the URL
	    goto error_management;
    else if ( (error_id = validate_url(url, &cinfo)) ) // Validate URL
    	goto error_management;
    else if ( (error_id = check_forbidden_path(&cinfo)) ) // Check for Forbidden Paths
    	goto error_management;
    else if ( (error_id = check_require_header(rtsp)) ) // Disallow Header REQUIRE
    	goto error_management;
    else if ( (error_id = get_description_format(rtsp, &cinfo)) ) // Get the description format. SDP is recomended
    	goto error_management;
    else if ( (error_id = get_cseq(rtsp)) ) // Get the CSeq 
        goto error_management;
    else if ( (error_id = get_session_description(&cinfo)) ) // Get Session Description
        goto error_management;

    if (max_connection() == ERR_GENERIC) {
        /*redirect */
        return send_redirect_3xx(rtsp, cinfo.object);
    }

    fnc_log(FNC_LOG_INFO, "DESCRIBE %s RTSP/1.0 ", url);
    send_describe_reply(rtsp, cinfo.object, cinfo.descr_format, cinfo.descr);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error_id, 0, rtsp);
    return ERR_NOERROR;
}
