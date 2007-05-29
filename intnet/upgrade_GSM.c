/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
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

#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#include <fenice/intnet.h>
#include <fenice/mediainfo.h>
#include <fenice/rtp.h>
#include <fenice/utils.h>

int upgrade_GSM(RTP_session *changing_session)
{
        int speed;
        media_entry req,*list,*p;
	SD_descr *matching_descr;
	
        memset(&req,0,sizeof(req));

        req.description.flags|=MED_BITRATE;
	req.description.flags|=MED_ENCODING_NAME;
	strcpy(req.description.encoding_name, "GSM");

        enum_media(changing_session->sd_filename, &matching_descr);
	list=matching_descr->me_list;

        speed=changing_session->current_media->description.bitrate;
	p = NULL;
        switch(speed) {
                case 4750:
                        speed=5150;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 5150:
                        speed=5900;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 5900:
                        speed=6700;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 6700:
                        speed=7400;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 7400:
                        speed=7950;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 7950:
                        speed=10200;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 10200:
                        speed=12200;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        break;
        }
        if (p!=NULL) {
       	       	close(changing_session->current_media->fd);
                changing_session->current_media->flags &= ~ME_FD;
                changing_session->current_media=p;
                return ERR_NOERROR;
        } else {
                return priority_increase(changing_session);
        }
}

