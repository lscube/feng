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

#include <stdlib.h>
#include <string.h>
#include <fenice/mediainfo.h>

media_entry *search_media(media_entry *req,media_entry *list)
// return NULL if not found.
{
	media_entry *p;
	for (p=list; p!=NULL; p=p->next) {    	
		if (req->flags & ME_FILENAME) {
			if (strcmp(p->filename,req->filename)!=0) {
				continue;
			}
		}
		if (req->flags & ME_DESCR_FORMAT) {
			if (p->descr_format!=req->descr_format) {
				continue;
			}
		}
		if (req->flags & ME_AGGREGATE) {
			if (strcmp(p->aggregate,req->aggregate)!=0) {
				continue;
			}
		}
		if (req->description.flags & MED_MSOURCE) {						
    			if (p->description.msource!=req->description.msource) {
				continue;
    			}
    		}
		if (req->description.flags & MED_MTYPE) {						    	
    			if (p->description.mtype!=req->description.mtype) {
				continue;
    			}
    		}
		if (req->description.flags & MED_PAYLOAD_TYPE) {						    	
    			if (p->description.payload_type!=req->description.payload_type) {
				continue;
    			}		
    		}
		if (req->description.flags & MED_CLOCK_RATE) {						    	
    			if (p->description.clock_rate!=req->description.clock_rate) {
				continue;
    			}		
    		}		
		if (req->description.flags & MED_ENCODING_NAME) {						    	
			if (strcmp(p->description.encoding_name,req->description.encoding_name)!=0) {
				continue;
			}
    		}		
		if (req->description.flags & MED_AUDIO_CHANNELS) {						    	
    			if (p->description.audio_channels!=req->description.audio_channels) {
				continue;
    			}		
    		}				
		if (req->description.flags & MED_BIT_PER_SAMPLE) {
			if (p->description.bit_per_sample!=req->description.bit_per_sample) {
				continue;
			}
		}
		if (req->description.flags & MED_SAMPLE_RATE) {
			if (p->description.sample_rate!=req->description.sample_rate) {
				continue;
			}
		}    	
		if (req->description.flags & MED_BITRATE) {
			if (p->description.bitrate!=req->description.bitrate) {
				continue;
			}
		}    	
		if (req->description.flags & MED_PRIORITY) {
			if (p->description.priority!=req->description.priority) {
				continue;
			}
		}
		if (req->description.flags & MED_FRAME_RATE) {
			if (p->description.frame_rate!=req->description.frame_rate) {
				continue;
			}
		}
		if (req->description.flags & MED_BYTE_PER_PCKT) {
			if (p->description.byte_per_pckt!=req->description.byte_per_pckt) {
				continue;
			}
		}    		    	 	
		return p;
	}
	return NULL;
}


