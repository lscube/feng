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
#include <fenice/prefs.h>

int read_MP3(media_entry *me,unsigned char **data,unsigned int *data_size,double *mtime)
{
        char thefile[255];
        unsigned char sync1,sync2,sync3,sync4;
        int N, res;
        unsigned int frame_skip;

        if (!(me->flags & ME_FD)) {
                strcpy(thefile,prefs_get_serv_root());
                strcat(thefile,me->filename);    
// printf("Playing: %s - bitrate: %d\n", thefile, me->description.bitrate);
                me->fd=open(thefile,O_RDONLY);
                if (me->fd==-1) return ERR_NOT_FOUND;
                me->flags|=ME_FD;
                me->data_chunk = 0;
        }
        frame_skip=round((*mtime)/(double)(me->description.pkt_len));   // mtime is play time in milliseconds, starting
                                                                        // from zero and incremented by one each time
                                                                        // this function is called (see schedule_do);
                                                                        // pkt_len is pkt lenght in milliseconds,
                                                                        // frame_skip is the number of current frame
        *mtime = (double)frame_skip * (double)(me->description.pkt_len);

        for (; me->data_chunk<=frame_skip; ++me->data_chunk) {
                if ((read(me->fd,&sync1,1)) != 1) return ERR_EOF;
                if ((read(me->fd,&sync2,1)) != 1) return ERR_EOF;
                if ((read(me->fd,&sync3,1)) != 1) return ERR_EOF;
                if ((read(me->fd,&sync4,1)) != 1) return ERR_EOF;
                if ((sync1==0xff) && ((sync2 & 0xe0)==0xe0)) {
                        N = (int)(me->description.frame_len * (float)me->description.bitrate / (float)me->description.sample_rate / 8);
                        if (sync3 & 0x02) N++;
                } else {
                        // Sync not found, not Mpeg-1/2
			// id3 TAG v1 is suppressed, id3 TAG v2 is not supported and
			// causes ERR_EOF immediately: id3 TAG v2 is prepended to the
			// audio content of the file and has variable lenght
			// To support it, id3 TAG v2 header must be read in order to
			// get its lenght and skip it
                        return ERR_EOF;
                }
                if ((me->data_chunk) < frame_skip) {
                        lseek(me->fd,N-4,SEEK_CUR);
                }
        }
        *data_size = N + 4; // 4 bytes are needed for mp3 in RTP encapsulation
        *data=(unsigned char *)calloc(1,*data_size);
        if (*data==NULL) {
                return ERR_ALLOC;
        }
        (*data)[0]=0;
        (*data)[1]=0;
        (*data)[2]=0;
        (*data)[3]=0;
        // These first 4 bytes are for mp3 in RTP encapsulation
        (*data)[4]=sync1;
        (*data)[5]=sync2;
        (*data)[6]=sync3;
        (*data)[7]=sync4;
        if ((res = read(me->fd,&((*data)[8]),N-4))<(N-4)) {
                if (res <= 0) return ERR_EOF;
                else *data_size = res + 8;
        }
        return ERR_NOERROR;
}

