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
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
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

#include <string.h>
#include <unistd.h>

#include <fenice/intnet.h>
#include <fenice/mediainfo.h>
#include <fenice/utils.h>

int half_MP3(RTP_session *changing_session)
{
        int speed;
        media_entry req,*list,*p;
	SD_descr *matching_descr;
	
        memset(&req,0,sizeof(req));

        req.description.flags|=MED_BITRATE;
	req.description.flags|=MED_ENCODING_NAME;
	strcpy(req.description.encoding_name, "MPA");

        enum_media(changing_session->sd_filename, &matching_descr);
	list=matching_descr->me_list;

        speed=changing_session->current_media->description.bitrate / 2;
	p = NULL;
	if (speed == 208000) speed = 224000;
	else if (speed == 88000) speed = 96000;
	else if (speed == 72000) speed = 80000;
	else if (speed == 28000) speed = 32000;
	else if (speed == 20000) speed = 24000;
	else if (speed == 12000) speed = 16000;
        switch(speed) {
                case 224000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=192000;
                case 192000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=176000;
                case 176000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=160000;
                case 160000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=144000;
                case 144000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=128000;
                case 128000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=112000;
                case 112000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=96000;
                case 96000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=80000;
                case 80000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=64000;
                case 64000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=56000;
                case 56000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=48000;
                case 48000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=40000;
                case 40000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=32000;
                case 32000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=24000;
                case 24000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=16000;
                case 16000:
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                        speed=8000;
                case 8000:
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
                return priority_decrease(changing_session);
        }
}

