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

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>

int read_MPEG_ts(media_entry *me, uint8 *data,uint32 *data_size, double *mtime, int *recallme)
{
	int ret;
	uint32 num_bytes;
 	
	if (!(me->flags & ME_FD)) {
		if ( (ret=mediaopen(me)) < 0 )
			return ret;
        }

        num_bytes = ((me->description).byte_per_pckt/188)*188;
	*data_size=0;
	*recallme=0;
	while(*data_size<num_bytes){
		if((ret=read(me->fd,&data[*data_size],188))==0)
			break;
		*data_size+=ret;
	}
	if(*data_size==0)
		return ERR_EOF;
	return ERR_NOERROR;
}

