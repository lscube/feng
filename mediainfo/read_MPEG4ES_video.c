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

#include <fenice/debug.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/mpeg4es.h>

#if HAVE_ALLOCA_H
#include <alloca.h>
#define FREE_DATA
#else
#define FREE_DATA free(data)
#endif

#define DEFAULT_BYTE_X_PKT 1400


int read_MPEG4ES_video (media_entry *me, uint8 *data_slot, uint32 *data_size, double *mtime, int *recallme)   /* reads MPEG4 Elementary stream Video */
{
	int ret;
	uint32 num_bytes;
	static_MPEG4_video_es *s=NULL;
	uint8 *data;	

	*data_size=0;
	*recallme=0;
	num_bytes = ((me->description).byte_per_pckt>0)?(me->description).byte_per_pckt:DEFAULT_BYTE_X_PKT;

#if HAVE_ALLOCA
        data=(unsigned char *)alloca(65000); 
#else
        data=(unsigned char *)calloc(1,65000);
#endif
        if (data==NULL)
               return ERR_ALLOC;

	s = (static_MPEG4_video_es *) me->stat;
	if(s->more_data==NULL)
		s->more_data=(char *)calloc(1,65000);
	
	/*clubbing visual object sequence - visual object - video object - video object layer*/
	if((me->description).msource==live && !s->fragmented && (int)(*mtime)==0){ 
		memcpy(data,s->header_data,s->header_data_size);
		*data_size=s->header_data_size;
                *recallme=0;
                s->fragmented=0;
	}
	
	if (!(me->flags & ME_FD)) {
		fprintf(stderr,"Open File in read*\n");
		if ( (ret=mediaopen(me)) < 0 )
			return ret;
		if(next_start_code(data,data_size,me->fd) < 0){
                        FREE_DATA;
                        return ERR_EOF;
                }
                if(read(me->fd,&s->final_byte,1)<1){
                        FREE_DATA;
                        return ERR_EOF;
                }
                data[*data_size]=s->final_byte;
                *data_size+=1;
                *recallme=0;
                s->fragmented=0;
        }

	else if(s->final_byte!=VOS_END_CODE && !s->fragmented){
               	data[*data_size]=0x00;
               	*data_size+=1;
               	data[*data_size]=0x00;
               	*data_size+=1;
               	data[*data_size]=0x01;
               	*data_size+=1;
               	data[*data_size]=s->final_byte;
               	*data_size+=1;
	}

	if(s->fragmented){
		if(s->remained_data_size>num_bytes){
			memcpy(data_slot,s->more_data + s->data_read,num_bytes);
			s->remained_data_size-=num_bytes;
			s->data_read+=num_bytes;
			s->fragmented=1;
			*recallme=1;
			*data_size=num_bytes;
		}
		else{
		
			memcpy(data_slot,s->more_data + s->data_read,s->remained_data_size);
			*data_size=s->remained_data_size;
			s->remained_data_size=0;
			s->data_read+=s->remained_data_size;
			s->fragmented=0;
			*recallme=0;
		}
		if(!s->use_clock_system){
			if(s->vop_coding_type==2)/*B FRAME*/
	 			*mtime=((double)s->ref1->var_time_increment + (double)s->ref1->modulo_time_base *s->ref1->vop_time_increment_resolution) * ( 1000 / (double)s->ref1->vop_time_increment_resolution);
			else
	 			*mtime=((double)s->ref2->var_time_increment + (double)s->ref2->modulo_time_base *s->ref2->vop_time_increment_resolution) * ( 1000 / (double)s->ref2->vop_time_increment_resolution);
		}
#if DEBUG	
		fprintf(stderr,"*mtime=%f | pkt_len=%f | delta_mtime=%f | vtir =%d\n",*mtime,me->description.pkt_len, me->description.delta_mtime,s->vtir_bitlen);
#endif
		FREE_DATA;
		return ERR_NOERROR;
	}

	while(s->final_byte != VOP_START_CODE){
		if( s->final_byte == VOS_START_CODE){
			ret=parse_visual_object_sequence(s,data,data_size,me->fd);
			if(ret!=ERR_NOERROR){
				FREE_DATA;
				return ret;
			}
		        if(next_start_code(data,data_size,me->fd) < 0){
				FREE_DATA;
				return ERR_EOF;              
			}
                	if(read(me->fd,&s->final_byte,1)<1){
				FREE_DATA;
				return ERR_EOF;
			}
              		data[*data_size]=s->final_byte;
               		*data_size+=1;
		}
		else if(s->final_byte == VO_START_CODE){
			ret=parse_visual_object(data,data_size,me->fd);
			if(ret!=ERR_NOERROR){
				FREE_DATA;
				return ret;
			}
			s->final_byte=data[*data_size - 1];
		}
		else if(/*(s->final_byte >= 0x00) &&*/ (s->final_byte <= 0x1F)) {
			ret=parse_video_object(data,data_size,me->fd);
			if(ret!=ERR_NOERROR){
				FREE_DATA;
				return ret;
			}
			s->final_byte=data[*data_size - 1];
		}
		else if((s->final_byte >= 0x20) && (s->final_byte <= 0x2F)){
			ret=parse_video_object_layer(s,data,data_size,me->fd);
			if(ret!=ERR_NOERROR){
				FREE_DATA;
				return ret;
			}
			s->final_byte=data[*data_size - 1];
		}
		else{
                	if(next_start_code(data,data_size,me->fd) < 0){
				FREE_DATA;
				return ERR_EOF;              
			}
                	if(read(me->fd,&s->final_byte,1)<1){
				FREE_DATA;
				return ERR_EOF;
			}
              		data[*data_size]=s->final_byte;
               		*data_size+=1;
		
		}
	}

	if(s->final_byte == VOP_START_CODE){
		ret=parse_video_object_plane(s,data,data_size,me->fd);
			if(ret!=ERR_NOERROR){
				FREE_DATA;
				return ret;
			}
			s->final_byte=data[*data_size - 1];
	}

	if(*data_size>num_bytes){
		if(s->final_byte!=VOS_END_CODE) *data_size-=4;
		memcpy(data_slot,data,num_bytes);
		memcpy(s->more_data,data,*data_size);
		s->remained_data_size=*data_size-num_bytes;
		s->data_read=num_bytes;
		*data_size=num_bytes;
		s->fragmented=1;
		*recallme=1;
	}
	else {
		if(s->final_byte!=VOS_END_CODE) *data_size-=4;
		memcpy(data_slot, data, *data_size); 
		*recallme=0;
		s->fragmented=0;
	}
	if(s->ref2->var_time_increment == 0 && s->ref2->modulo_time_base==0 && (int)(*mtime) !=0) 
		s->use_clock_system=1;
	if(s->vop_coding_type==2)/*B FRAME*/
		s->use_clock_system=0;
	if(!s->use_clock_system){
		if(s->vop_coding_type==2)/*B FRAME*/
 			*mtime=((double)s->ref1->var_time_increment + (double)s->ref1->modulo_time_base *s->ref1->vop_time_increment_resolution) * ( 1000 / (double)s->ref1->vop_time_increment_resolution);
		else
 			*mtime=((double)s->ref2->var_time_increment + (double)s->ref2->modulo_time_base *s->ref2->vop_time_increment_resolution) * ( 1000 / (double)s->ref2->vop_time_increment_resolution);
	}
	
	
#if DEBUG	
	fprintf(stderr,"*mtime=%f | pkt_len=%f | delta_mtime=%f | vtir =%d\n",*mtime,me->description.pkt_len, me->description.delta_mtime,s->vtir_bitlen);
#endif
	FREE_DATA;
	return ERR_NOERROR;
}


