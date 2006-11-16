/* * 
 *  $Id:$
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

#include <stdio.h>
#include <string.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/bufferpool.h>
#include <fenice/prefs.h>

uint32 mediacpy(media_entry ** media_out, media_entry ** media_in)
{

	uint32 dim_buff;

	switch ((*media_in)->description.mtype) {
	case audio:
		dim_buff = DIM_AUDIO_BUFFER;
		break;
	case video:
		dim_buff = DIM_VIDEO_BUFFER;
		break;
	default:
		return ERR_GENERIC;
	}

	if ((*media_in)->description.msource == live)
		*media_out = *media_in;
	else
		memcpy((*media_out), (*media_in), sizeof(media_entry));

	if ((*media_out)->pkt_buffer == NULL) {
		if (!strcasecmp
		    ((*media_out)->description.encoding_name, "RTP_SHM")) {
			if (!
			    ((*media_out)->pkt_buffer =
			     OMSbuff_shm_map((*media_out)->filename)))
				return ERR_ALLOC;
		} else {
			if (!((*media_out)->pkt_buffer = OMSbuff_new(dim_buff)))
				return ERR_ALLOC;
		}
	}

	/*if (((*media_out)->cons = OMSbuff_ref((*media_out)->pkt_buffer)) == NULL)
	   return ERR_ALLOC; */

	return ERR_NOERROR;
}
