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

#include <fenice/sdp.h>
#include <fenice/mediainfo.h>
#include <fenice/utils.h>
#include <string.h>

/*
	If req is empty, return describe all the media_entry
	Else describe the requested one
*/
int get_media_descr(char *object,media_entry *req,media_entry *media,char *descr)
// Returns the media description according to the required format
{
    media_entry *list,*matching_me;
    SD_descr *matching_descr;
    int res;
    char *p;
    if (req==NULL || media==NULL) {
    	return ERR_INPUT_PARAM;
    }
    if (!(req->flags & ME_DESCR_FORMAT)) {
    	return ERR_INPUT_PARAM;
    }
    p=strchr(object,'.');
    if (strcasecmp(p,".SD")!=0) {
		return ERR_NOT_SD;
    }
    res=enum_media(object,&matching_descr);
    list=matching_descr->me_list;
    if (res!=ERR_NOERROR) {
    	return res;
    }
    if (list==NULL) {
    	return ERR_GENERIC;
    }	
	if (media_is_empty(req)) {
	    // If there is no specific request describe every media	
	    // in the SD (recursive=1)			
	    res=get_SDP_descr(list,descr,1,object);
		return res;
	}
	else {
		get_media_entry(req,list,&matching_me);	
		switch (req->descr_format) {
			// Add new description formats here
			case df_SDP_format: {
				if (get_SDP_descr(matching_me,descr,0,object)==-1) {
					return ERR_GENERIC;
				}			
				break;
			}
			default: {
				// Description format not supported
				return ERR_GENERIC;
			}
		}
	}
	return ERR_NOERROR;
}


