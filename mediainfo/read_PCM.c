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
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/pcm.h>
#include <fenice/prefs.h>
#include <fenice/types.h>
#include <fenice/bufferpool.h>

int read_PCM(media_entry *me, uint8 *buffer, uint32 *buf_size, double *mtime, int *recallme, uint8 *marker)
{
        char thefile[255];
        uint32 i;
        unsigned start_sample, samples;
       
	*marker=0;
        if (!(me->flags & ME_FD)) {
                strcpy(thefile,prefs_get_serv_root());
                strcat(thefile,me->filename);           
		// printf("Playing: %s - bitrate: %d\n", thefile, me->description.bitrate);
                me->fd=open(thefile,O_RDONLY);
                if (me->fd==-1) return ERR_NOT_FOUND;
                me->flags|=ME_FD;
                me->data_chunk=0;
        }
        samples=(unsigned int)(me->description.pkt_len*((float)me->description.clock_rate)/1000);
                // samples is the number of samples to be reproduced each pkt_len time slot
        start_sample=(unsigned int)(*mtime*me->description.clock_rate/1000);
                // Current sample to be reproduced
        
	*buf_size=samples*me->description.bit_per_sample/8*me->description.audio_channels;
        //buffer=(unsigned char*)calloc(1,buf_size);
        
	//if (buffer==NULL) {
        //        return ERR_ALLOC;
        //}
	
        for (; me->data_chunk<start_sample; me->data_chunk+=samples) {
                if ((me->data_chunk + samples) < start_sample) {    // Some chunks have been lost, so skip them
                        lseek(me->fd,samples*me->description.bit_per_sample/8*me->description.audio_channels,SEEK_CUR);
                }
        }
        *mtime = (double)me->data_chunk / (double)(me->description.clock_rate) * 1000;
        
	for (i = 0; i < *buf_size; i += 2) {
                if (read(me->fd, &(buffer[i + 1]), 1) <= 0 ) break;
                if (read(me->fd, &(buffer[i]), 1) <= 0 ) break;
        }
        *buf_size = i;
	
        if (i == 0) return ERR_EOF;
        return ERR_NOERROR;     
}

