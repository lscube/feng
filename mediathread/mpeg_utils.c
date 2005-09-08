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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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


#include <unistd.h>
#include <fenice/mpeg_utils.h>

int next_start_code2(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained) 
{	
        int i;
        int count;
	char buf[3];
	
	count=min(dst_remained,src_remained);

	for(i=0;i<count;i++) {
		buf[i%3]=src[i];
		if(i>=2) { 
			if(buf[(i-2)%3]==0x00 && buf[(i-1)%3]==0x00 && buf[i%3]==0x01)	
				return i - 3; /*start_code found, return the number of bytes write to dst/read from src until 000001 (not included)*/
			dst[i-3]=src[i-3];	
		}
	}
	dst[i-2]=src[i-2];
	dst[i-1]=src[i-1];
	dst[i]=src[i];

        return -1; /*start_code not found, dst is filled  or src is finished*/
}

