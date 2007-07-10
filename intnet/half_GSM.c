/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

#include <string.h>
#include <unistd.h>

#include <fenice/intnet.h>
#include <fenice/mediainfo.h>
#include <fenice/utils.h>

int half_GSM(RTP_session *changing_session)
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

        speed=changing_session->current_media->description.bitrate / 2;
	p = NULL;
	if (changing_session->current_media->description.bitrate != 4750) {
		if (speed == 6100) speed = 6700;
		else if (speed == 5100) speed = 5150;
		else speed = 4750;
        	switch(speed) {
                	case 6700:
                        	req.description.bitrate=speed;
                        	p=search_media(&req,list);
                        	if(p!=NULL) break;
                        	speed=5900;
                	case 5900:
                        	req.description.bitrate=speed;
                        	p=search_media(&req,list);
                        	if(p!=NULL) break;
                        	speed=5150;
                	case 5150:
                        	req.description.bitrate=speed;
                        	p=search_media(&req,list);
                        	if(p!=NULL) break;
                        	speed=4750;
                	case 4750:
                        	req.description.bitrate=speed;
                        	p=search_media(&req,list);
                        	break;
        	}
	}
        if (p!=NULL) {
       	       	close(changing_session->current_media->fd);
                changing_session->current_media->flags &= ~ME_FD;
                changing_session->current_media=p;
                return ERR_NOERROR;
        } else {
                return priority_decrease(changing_session);
        }
}

