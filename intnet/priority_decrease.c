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

int priority_decrease(RTP_session *changing_session)
{
        int priority;
        media_entry req,*list,*p;
	SD_descr *matching_descr;

        memset(&req,0,sizeof(req));

        req.description.flags|=MED_PRIORITY;
        enum_media(changing_session->sd_filename, &matching_descr);
	list=matching_descr->me_list;
        priority=changing_session->current_media->description.priority;
        priority += 1;
        req.description.priority=priority;
        p=search_media(&req,list);
        if (p!=NULL) {
       	       	close(changing_session->current_media->fd);
                changing_session->current_media->flags &= ~ME_FD;
                changing_session->current_media=p;
                return ERR_NOERROR;
        } else {
		changing_session->MinimumReached = 1;
                return ERR_NOERROR;
        }
}

