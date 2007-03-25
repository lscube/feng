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
#include <fenice/fnc_log.h>

int RTSP_valid_response_msg(unsigned short *status, char *msg,
                RTSP_buffer * rtsp)
// This routine is from OMS.
{
    char ver[32], trash[15];
    unsigned int stat;
    unsigned int seq;
    int pcnt;        /* parameter count */

    *ver = *msg = '\0';
    /* assuming "stat" may not be zero (probably faulty) */
    stat = 0;

    pcnt =
        sscanf(rtsp->in_buffer, " %31s %u %s %s %u\n%255s ", ver, &stat,
           trash, trash, &seq, msg);

    /* check for a valid response token. */
    if (strncasecmp(ver, "RTSP/", 5))
        return 0;    /* not a response message */

    /* confirm that at least the version, status code and sequence are there. */
    if (pcnt < 3 || stat == 0)
        return 0;    /* not a response message */

    /*
     * here is where code can be added to reject the message if the RTSP
     * version is not compatible.
     */

    /* check if the sequence number is valid in this response message. */
    if (rtsp->rtsp_cseq != seq + 1) {
        fnc_log(FNC_LOG_ERR,
            "Invalid sequence number returned in response.\n");
        return ERR_GENERIC;
    }

    *status = stat;
    return 1;
}
