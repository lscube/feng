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

/*#include <string.h>*/
#include <stdio.h>
#include <fenice/utils.h>
#include <fenice/debug.h>
#include <fenice/mediainfo.h>
#include <fenice/types.h>

int get_frame(media_entry *me, double *mtime)
{
	int recallme=0;	
	OMSSlot *slot;
	int res = ERR_EOF;
	uint8 marker=0;

	do{
		recallme=0;
		res=ERR_EOF;
		slot=OMSbuff_getslot(me->pkt_buffer);
		res=me->media_handler->read_media(me,slot->data,&slot->data_size,mtime,&recallme,&marker);
		if (res==ERR_NOERROR) { // commit of buffer slot.
			if (OMSbuff_write(me->pkt_buffer, *mtime, marker, slot->data, slot->data_size))
				fprintf(stderr, "Error in bufferpool writing\n");
		}
		// slot->timestamp=*mtime;

#if DEBUG
//		dump_payload(slot->data,slot->data_size,"fenice_dump");
#endif
	} while(recallme && res==ERR_NOERROR);
	return res;
}

