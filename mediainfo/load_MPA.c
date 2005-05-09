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
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fenice/utils.h>
#include <fenice/fnc_log.h>
#include <fenice/mediainfo.h>
#include <fenice/mp3.h>
#include <fenice/prefs.h>

int load_MPA(media_entry *p)
{

	int ret;
	long int tag_dim;
        int n,RowIndex, ColIndex;
        int BitrateMatrix[16][5] = {
                {0,     0,     0,     0,     0     },
                {32000, 32000, 32000, 32000, 8000  },
                {64000, 48000, 40000, 48000, 16000 },
                {96000, 56000, 48000, 56000, 24000 },
                {128000,64000, 56000, 64000, 32000 },
                {160000,80000, 64000, 80000, 40000 },
                {192000,96000, 80000, 96000, 48000 },
                {224000,112000,96000, 112000,56000 },
                {256000,128000,112000,128000,64000 },
                {288000,160000,128000,144000,80000 },
                {320000,192000,160000,160000,96000 },
                {352000,224000,192000,176000,112000},
                {384000,256000,224000,192000,128000},
                {416000,320000,256000,224000,144000},
                {448000,384000,320000,256000,160000},
                {0,     0,     0,     0,     0     }
        };

	fnc_log(FNC_LOG_INFO, "loading MPA...\n");

	if ( (ret=mediaopen(p)) < 0)
		return ret;
        
	
	p->buff_size=0;
	if ( (p->buff_size = read(p->fd, p->buff_data, 4)) != 4) return ERR_PARSE;

	// shawill: look if ID3 tag is present
	if (!memcmp(p->buff_data, "ID3", 3)) { // ID3 tag present
		n = read_dim(p->fd, &tag_dim); // shawill: one day we will look also at this function
		printf("%ld\n",tag_dim);
		p->description.flags|=MED_ID3;
		p->description.tag_dim=tag_dim;
		lseek(p->fd,tag_dim,SEEK_CUR);
		if ( (p->buff_size = read(p->fd, p->buff_data, 4)) != 4) return ERR_PARSE;
	}
        
	//fstat(p->fd, &fdstat);
	//if ( !S_ISFIFO(fdstat.st_mode) ) {
		mediaclose(p);
		// close(p->fd);
		p->buff_size = 4;
	//}
        
	if (! ((p->buff_data[0]==0xff) && ((p->buff_data[1] & 0xe0)==0xe0))) return ERR_PARSE;

        switch (p->buff_data[1] & 0x1e) {                /* Mpeg version and Level */
                case 18: ColIndex = 4; break;  /* Mpeg-2 L3 */
                case 20: ColIndex = 4; break;  /* Mpeg-2 L2 */
                case 22: ColIndex = 3; break;  /* Mpeg-2 L1 */
                case 26: ColIndex = 2; break;  /* Mpeg-1 L3 */
                case 28: ColIndex = 1; break;  /* Mpeg-1 L2 */
                case 30: ColIndex = 0; break;  /* Mpeg-1 L1 */
                default: {
				 return ERR_PARSE;
			 }
        }

        RowIndex = (p->buff_data[2] & 0xf0) / 16;
        p->description.bitrate = BitrateMatrix[RowIndex][ColIndex];
        p->description.flags|=MED_BITRATE;      

        if (p->buff_data[1] & 0x08) {     /* Mpeg-1 */
                switch (p->buff_data[2] & 0x0c) {
                        case 0x00: p->description.sample_rate=44100; break;
                        case 0x04: p->description.sample_rate=48000; break;
                        case 0x08: p->description.sample_rate=32000; break;
                	default: {
				 return ERR_PARSE;
			 }
                }
        } else {                /* Mpeg-2 */
                // switch (sync3 & 0x0c) {
                switch (p->buff_data[2] & 0x0c) {
                        case 0x00: p->description.sample_rate=22050; break;
                        case 0x04: p->description.sample_rate=24000; break;
                        case 0x08: p->description.sample_rate=16000; break;
                	default: {
				 return ERR_PARSE;
			 }
                }
        }
        p->description.flags|=MED_SAMPLE_RATE;          

        if ((p->buff_data[1] & 0x06) == 6)
		p->description.frame_len = 384;
        else
		p->description.frame_len = 1152;

        p->description.flags|=MED_FRAME_LEN;
        p->description.pkt_len=(double)p->description.frame_len/(double)p->description.sample_rate*1000;
        p->description.delta_mtime=p->description.pkt_len;
        p->description.flags|=MED_PKT_LEN;

        return ERR_NOERROR;
}

