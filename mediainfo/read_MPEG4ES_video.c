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

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
#define DEFAULT_BYTE_X_PKT 1400

/*Definitions of the start codes*/
#define VIDEO_OBJECT_START_CODE 	/*0x00 through 0x1F*/
#define VIDEO_OBJECT_LAYER_START_CODE 	/*0x20 through 0x2F*/
#define RESERVED	/*0x30 through 0xAF*/
#define VOS_START_CODE 0xB0	/*visual object sequence*/
#define VOS_END_CODE 0xB1	/*visual object sequence*/
#define USER_DATA_START_CODE 0xB2
#define GROUP_OF_VOP_START_CODE 0xB3
#define VIDEO_SESSION_ERROR_CODE 0xB4
#define VO_START_CODE 0xB5	/*visual object*/
#define VOP_START_CODE 0xB6	/*visual object plane*/
#define RESERVED_1 0xB7
#define RESERVED_2 0xB9
#define FBA_OBJECT_START_CODE 0xBA
#define FBA_OBJECT_PLANE_START_CODE 0xBB
#define MESH_OBJECT_START_CODE 0xBC
#define MESH_OBJECT_PLANE_START_CODE 0xBD
#define STILL_TEXTURE_OBJECT_START_CODE 0xBE
#define TEXTURE_SPATIAL_LAYER_START_CODE 0xBF
#define TEXTURE_SNR_LAYER_START_CODE 0xC0
#define TEXTURE_TILE_START_CODE 0xC1
#define TEXTURE_SHAPE_LAYER_START_CODE 0xC2
#define RESERVED_3 0xC2
/*System start codes aren't used in MPEG4 Visual ES*/
//#define SYSTEM_START_CODES /*01xC6 through 01xFF*/

int read_MPEG4ES_video (media_entry *me, uint8 *data, uint32 *data_size, double *mtime, int *recallme)   /* reads MPEG4 Elementary stream Video */
{
	int ret;
	uint32 num_bytes;
	static_MPEG_video *s=NULL;
	uint32 init=0;
	uint32 out_of_while=0;
		
	*data_size=0;
	num_bytes = ((me->description).byte_per_pckt>0)?(me->description).byte_per_pckt:DEFAULT_BYTE_X_PKT;
	if(!(me->flags & ME_FD)){
		if ( (ret=mediaopen(me)) < 0 )
			return ret;
                s = (static_MPEG_video *) calloc (1, sizeof(static_MPEG_video));
                me->stat = (void *) s;
                s->final_byte=0x00;
                s->fragmented=0;
		*recallme=0;
		init=1;
	}
	else
		s = (static_MPEG_video *) me->stat;
		
	if(s->fragmented==0){
		char buf_aux[3];	
		int i;
		if(init==0 && s->final_byte!=VOS_END_CODE){
                	data[*data_size]=0x00;
                	*data_size+=1;
                	data[*data_size]=0x00;
                	*data_size+=1;
                	data[*data_size]=0x01;
                	*data_size+=1;
                	data[*data_size]=s->final_byte;
                	*data_size+=1;
		}
		
		while(s->final_byte != VOP_START_CODE){
                        if(next_start_code(data,data_size,me->fd) < 0)
				return ERR_EOF;              
                       	if(read(me->fd,&s->final_byte,1)<1)
				return ERR_EOF;
                       	data[*data_size]=s->final_byte;
                       	*data_size+=1;
			*recallme=0;
		}
		while(num_bytes > *data_size && out_of_while!=1){
			if ( read(me->fd,&buf_aux,3) <3){  /* If there aren't 3 more bytes we are at EOF */
				return ERR_EOF;
			}
			while ( !((buf_aux[0] == 0x00) && (buf_aux[1]==0x00) && (buf_aux[2]==0x01)) && *data_size < num_bytes) {
    				data[*data_size]=buf_aux[0];
    				*data_size+=1;
				buf_aux[0]=buf_aux[1];
				buf_aux[1]=buf_aux[2];
      				if ( read(me->fd,&buf_aux[2],1) <1){ 
					return ERR_EOF;
				}
 		   	}
			for (i=0;i<3;i++) {
    				data[*data_size]=buf_aux[i];
    				*data_size+=1;
    			}
                       	if(read(me->fd,&s->final_byte,1)<1)
				return ERR_EOF;
                	data[*data_size]=s->final_byte;
       	        	*data_size+=1;
                        if (buf_aux[0] == 0x00 && buf_aux[1]==0x00 && buf_aux[2]==0x01) {
				/*multiple VOP in a RTP pkt*/
				/*if(s->final_byte == VOP_START_CODE) 
					out_of_while=0;
				else*/
					out_of_while=1;
				*recallme=0;
				if(s->final_byte!=VOS_END_CODE) *data_size-=4;
				s->fragmented=0;
			}
			else{
				out_of_while=1;
				*recallme=1;
				s->fragmented=1;
			}
		}/*end while *data_size < num_bytes*/
	}

	else{/*fragmented*/
		char buf_aux[3];	
		int i;
	
		if ( read(me->fd,&buf_aux,3) <3){  /* If there aren't 3 more bytes we are at EOF */
			return ERR_EOF;
		}
		while ( !((buf_aux[0] == 0x00) && (buf_aux[1]==0x00) && (buf_aux[2]==0x01)) && *data_size <num_bytes) {
    			data[*data_size]=buf_aux[0];
    			*data_size+=1;
			buf_aux[0]=buf_aux[1];
			buf_aux[1]=buf_aux[2];
      			if ( read(me->fd,&buf_aux[2],1) <1){ 
				return ERR_EOF;
			}
	   	}
    		for (i=0;i<3;i++) {
    			data[*data_size]=buf_aux[i];
    			*data_size+=1;
    		}
                if(read(me->fd,&s->final_byte,1)<1)
			return ERR_EOF;
               	data[*data_size]=s->final_byte;
        	*data_size+=1;
		
                if (buf_aux[0] == 0x00 && buf_aux[1]==0x00 && buf_aux[2]==0x01) {
			*recallme=s->fragmented=0;
			if(s->final_byte!=VOS_END_CODE) *data_size-=4;
			}
		else{
			*recallme=s->fragmented=1;
		}
	}
	if(*data_size>=num_bytes)
		fprintf(stderr,"%d\n",*data_size);
	
	return ERR_NOERROR;
}


