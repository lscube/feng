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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/gsm.h>

int read_GSM(media_entry *me,uint8 *data,uint32 *data_size,double *mtime, int *recallme, uint8 *marker)
{       
        unsigned char byte1;
        unsigned int N=0, res;
        // long frame_skip;
        unsigned int frame_skip;
        
	*marker=0;
        if (!(me->flags & ME_FD)) {             
		if ( (res=mediaopen(me)) < 0 )
			return res;
                me->data_chunk = 0;
        }               
        frame_skip=(long)lround(*mtime/(double)me->description.pkt_len);
        *mtime = (double)frame_skip * (double)(me->description.pkt_len);
        for (; me->data_chunk<frame_skip; ++me->data_chunk) {
                if ((read(me->fd,&byte1,1)) != 1) return ERR_EOF;
                switch (byte1 & 0x07) {
                        case 0: N=12; break;
                        case 1: N=13; break;
                        case 2: N=15; break;
                        case 3: N=17; break;
                        case 4: N=18; break;
                        case 5: N=20; break;
                        case 6: N=25; break;
                        case 7: N=30; break;
                }       
                if ((me->data_chunk + 1) < frame_skip) {
                        lseek(me->fd,N,SEEK_CUR);
                }
        }
        *data_size=N+1;
        //*data=(unsigned char *)calloc(1,*data_size);
        //if (*data==NULL) {
         //       return ERR_ALLOC;
        //}
        data[0]=byte1;
        if ((res = read(me->fd, &(data[1]), *data_size - 1)) <= *data_size - 1) {
                if (res <= 0) return ERR_EOF;
                else *data_size = res + 1;
        }
        return ERR_NOERROR;
}


