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
#include <fcntl.h>

#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/mpeg_system.h>

int read_MPEG_system(media_entry *me, uint8 *data,uint32 *data_size, double *mtime, int *recallme, uint8 *marker)
{
	int ret;
	uint32 num_bytes;
	int count,count1,flag,packet_done=0,not_remove=0,time_set=0,pts_diff,clock,dts_present=0,audio_flag=0;
	float pkt_len;
	static_MPEG_system *s=NULL;
 	
	if (!(me->flags & ME_FD)) {
		if ( (ret=mediaopen(me)) < 0 )
			return ret;
		s = (static_MPEG_system *) calloc (1, sizeof(static_MPEG_system));
		me->stat = (void *) s;
		s->final_byte=0x00; 
		s->offset_flag=0;
		s->offset=0;
		s->new_dts=0;
        } else 
		s = (static_MPEG_system *) me->stat;

        num_bytes = (me->description).byte_per_pckt;

	*data_size=0;
	count=count1=0;
	flag = 1;


	lseek(me->fd,s->data_total,SEEK_SET);                             	/* At this point it should be right to find the nearest lower frame */
 									/* computing it from the value of mtime */
	//*data=(unsigned char *)calloc(1,65000);                        	/* and starting the reading from this */
        //if (*data==NULL) {                                             	/* feature not yet implemented */
	//	return ERR_ALLOC;
        //}

	if ( next_start_code(data,data_size,me->fd) == -1) {
		close(me->fd);
		return ERR_EOF;
	}

 	read(me->fd,&s->final_byte,1);
	data[*data_size]=s->final_byte;
	*data_size+=1;

  	if (s->final_byte == 0xba) {
		read_pack_head(data,data_size,me->fd,&s->final_byte,&s->scr);
	}

	if (s->final_byte >= 0xbc) {
		if (s->final_byte == 0xc0) {
			audio_flag = 1;
		}
		read_packet_head(data,data_size,me->fd,&s->final_byte,&time_set,&s->pts,&s->dts,&dts_present,&s->pts_audio);
		if (s->new_dts != 1){
			s->new_dts = dts_present;
		}
		if ( (!s->offset_flag) && (s->pts.pts!=0) ) {
			s->offset=s->pts.pts;
			s->offset_flag=1;
		}
	}

	if (num_bytes != 0) {
		time_set=0;
		while ( ((*data_size <= num_bytes) && (*recallme == 1)) || (!packet_done) ) {
  			count = read_packet(data,data_size,me->fd,&s->final_byte);
     			if (!packet_done) {
				not_remove = 1;
			} else {
				not_remove = 0;
			}
     			packet_done = 1;
     			next_start_code(data,data_size,me->fd);
			read(me->fd,&s->final_byte,1);
			data[*data_size]=s->final_byte;
			*data_size+=1;
			if ( (s->final_byte < 0xbc) && (*data_size <= num_bytes) ) {
				*recallme = 0;
				flag = 0;              	// next packet coudn't stay in the same rtp packet
			}
		}
		if (flag && !not_remove) {
			count+=4;
   			lseek(me->fd,-count,SEEK_CUR);
			*data_size-=count;
			not_remove = 0;
		} else {
			lseek(me->fd,-4,SEEK_CUR);
			*data_size-=4;
		}
		s->data_total+=*data_size;
		clock = (me->description).clock_rate;
		if (!audio_flag){
			*mtime = ((float)(s->pts.pts-s->offset)*1000)/(float)clock;
		} else {
			*mtime = ((float)(s->pts_audio.pts-s->offset)*1000)/(float)clock;
		}
		count=0;
		do	{                   	                                 /* finds next packet */
			count1=next_start_code(data,data_size,me->fd);
			count+=count1;
			count+=3;
			read(me->fd,&s->final_byte,1);
			data[*data_size]=s->final_byte;
			*data_size+=1;
			count+=1;
		} while ((s->final_byte < 0xbc) && (s->final_byte != 0xb9) );
		if (s->final_byte == 0xb9) {
			s->data_total+=4;
		} else {
			count1=read_packet_head(data,data_size,me->fd,&s->final_byte,&time_set,&s->next_pts,&s->next_dts,&dts_present,&s->pts_audio); 	 /* reads next packet head */
			count += count1;
			*data_size-=count;
			if ( (s->pts.pts == s->next_pts.pts) || (s->final_byte == 0xbe) || (s->final_byte == 0xc0) || (s->offset==0))	{
				*recallme=1;
			} else {
				if (!dts_present){ 	    /* dts_present now is referring to the next packet */
					if (!s->new_dts){
						pts_diff = s->next_pts.pts - s->pts.pts;
					} else {
						pts_diff = s->next_pts.pts - s->dts.pts;
					}
				} else {
					pts_diff = s->next_dts.pts - s->pts.pts;
				}
				pkt_len = (((float)pts_diff*1000)/(float)clock);
				changePacketLength(pkt_len,me);
				*recallme=0;
				s->new_dts = 0;
			}
                                                                   /* compute the delta_mtime */

			me->description.delta_mtime =  (s->next_pts.pts - s->pts.pts)*1000/(float)clock;
			*mtime=(s->scr.scr*1000)/(double)me->description.clock_rate;	/* adjust SRC value to be passed as argument to the msec2tick and do not */
		}
		*marker=!(*recallme);
		return ERR_NOERROR;
	} else {
		read_packet(data,data_size,me->fd,&s->final_byte);
		s->data_total+=*data_size;
		clock = (me->description).clock_rate;
		if (!audio_flag){
			*mtime = ((float)(s->pts.pts-s->offset)*1000)/(float)clock;
		} else {
			*mtime = ((float)(s->pts_audio.pts-s->offset)*1000)/(float)clock;
		}
		count=0;
		time_set=0;
		do	{
			count1=next_start_code(data,data_size,me->fd);
			count+=count1;
			count+=3;
			read(me->fd,&s->final_byte,1);
			data[*data_size]=s->final_byte;
			*data_size+=1;
			count+=1;
		} while ((s->final_byte < 0xbc) && (s->final_byte != 0xb9) );

		if (s->final_byte == 0xb9) {
			s->data_total+=4;
		} else {
			count1=read_packet_head(data,data_size,me->fd,&s->final_byte,&time_set,&s->next_pts,&s->next_dts,&dts_present,&s->pts_audio);
			count += count1;
			*data_size-=count;

			if ( (s->pts.pts == s->next_pts.pts) || (s->final_byte == 0xbe) || (s->final_byte == 0xc0) || (s->offset==0))	{
				*recallme=1;
			} else {
				if (!dts_present){   /* dts_present now is referring to the next packet */
					if (!s->new_dts){
						pts_diff = s->next_pts.pts - s->pts.pts;
					} else {
						pts_diff = s->next_pts.pts - s->dts.pts;
					}
				} else {
					pts_diff = s->next_dts.pts - s->pts.pts;
				}
				pkt_len = (((float)pts_diff*1000)/(float)clock);
				changePacketLength(pkt_len,me);
				*recallme=0;
				s->new_dts = 0;
			}
                                                                    /* compute the delta_mtime */

                        me->description.delta_mtime =  (s->next_pts.pts - s->pts.pts)*1000/(float)clock;
                        *mtime=(s->scr.scr*1000)/(double)me->description.clock_rate;	/* adjust SRC value to be passed as argument to the msec2tick and do not */
		}
		*marker=!(*recallme);
		return ERR_NOERROR;
	}
}

