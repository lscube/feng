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
#include <unistd.h>

#include <fenice/intnet.h>
#include <fenice/mediainfo.h>
#include <fenice/utils.h>

int downgrade_MP3(RTP_session *changing_session)
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


        speed=changing_session->current_media->description.bitrate;
	p = NULL;
        switch(speed) {
                case 448000:
                        speed=416000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 416000:
                        speed=384000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 384000:
                        speed=352000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 352000:
                        speed=320000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 320000:
                        speed=288000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 288000:
                        speed=256000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 256000:
                        speed=224000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 224000:
                        speed=192000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 192000:
                        speed=176000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 176000:
                        speed=160000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 160000:
                        speed=144000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 144000:
                        speed=128000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 128000:
                        speed=112000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 112000:
                        speed=96000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 96000:
                        speed=80000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 80000:
                        speed=64000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 64000:
                        speed=56000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 56000:
                        speed=48000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 48000:
                        speed=40000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 40000:
                        speed=32000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 32000:
                        speed=24000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 24000:
                        speed=16000;
                        req.description.bitrate=speed;
                        p=search_media(&req,list);
                        if(p!=NULL) break;
                case 16000:
                        speed=8000;
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

