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
#include <string.h> /*memcpy*/
#include <fenice/debug.h>
#include <fenice/mpeg4es.h>
#include <fenice/mediainfo.h>
#include <fenice/types.h>
#include <fenice/utils.h>

int get_field( uint8 *d, uint32 bits, uint32 *offset )
{
	uint32 v = 0;
	uint32 i;

	for( i = 0; i < bits; )
	{
		if( bits - i >= 8 && *offset % 8 == 0 )
		{
			v <<= 8;
			v |= d[*offset/8];
			i += 8;
			*offset += 8;
		} else
		{
			v <<= 1;
			v |= ( d[*offset/8] >> ( 7 - *offset % 8 ) ) & 1;
			++i;
			++(*offset);
		}
	}
	return v;
}

int parse_visual_object_sequence(static_MPEG4_video_es *mpeg4_struct,uint8 *data, uint32 *data_size,int fin){
	if(read(fin,&data[*data_size],1)<1)
		return ERR_EOF;
	mpeg4_struct->profile_id=data[*data_size];
      	*data_size+=1;
	return ERR_NOERROR;
}

int parse_visual_object(uint8 *data, uint32 *data_size,int fin){
	int visual_object_type;/*now not used yet*/
	int off = *data_size*8;
	/*
	 *32 bits visual object start code 
	 *if (is_visual_object_identifier 1 bit){
	 *	visual_object_verid 4 bits
	 *	visual_object_priority 3 bits
	 *}
	 *visual_object_type 4 bits	
	 * */
        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;

	if( get_field( data, 1, &off ) ) off += 7; /*if is_visual_oject_identifier => skip visual_object_verid (4 bits) and visual_object_priority (3 bits)*/
	visual_object_type = get_field( data, 4, &off );
	return ERR_NOERROR;
}

int parse_video_object(uint8 *data, uint32 *data_size,int fin){
	/*should be empty except for the start code*/
        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;
	return ERR_NOERROR;
}

int parse_video_object_layer(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin){
	int off = *data_size*8+9; /*skip 1 bit random_accessible_vol and 8 bits visual_object_type_indication*/
	int i;
	int flag=0;
	int vop_time_increment=0;

        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;

	if( get_field( data, 1, &off ))  off += 7; // object layer identifier
	if( get_field( data, 4, &off ) == 15 ) // aspect ratio info
		off += 16; // extended par
	if( get_field( data, 1, &off ) ) // vol control parameters
	{
		off += 3; // chroma format, low delay
		if( get_field( data, 1, &off ) ) off += 79; // vbw parameters
	}
	off += 2; // video object layer shape
	if( ! get_field( data, 1, &off ) )
	{
		/*missing marker*/
		return ERR_PARSE;
	}
	if(out->ref2==NULL){
		out->ref2=(mpeg4_time_ref *)calloc(1,sizeof(mpeg4_time_ref));
	}
	if(out->ref1==NULL){
		i = out->ref2->vop_time_increment_resolution = get_field( data, 16, &off );
		out->ref1=(mpeg4_time_ref *)calloc(1,sizeof(mpeg4_time_ref));
		memcpy(out->ref1,out->ref2,sizeof(mpeg4_time_ref));	
		flag=1;
	}
	else{
		mpeg4_time_ref *tmp;
		tmp=out->ref2;
		out->ref2=out->ref1;
		i = out->ref2->vop_time_increment_resolution = get_field( data, 16, &off );
		out->ref1=tmp;
		flag=0;
	}
	if( ! get_field( data, 1, &off ) )
	{
		/*missing marker*/
		return ERR_PARSE;
	}
	for( out->vtir_bitlen = 0; i != 0; i >>= 1 ) ++out->vtir_bitlen;
	if( get_field( data, 1, &off ) )/*fixed VOP time increment*/
	{
		vop_time_increment = get_field( data, out->vtir_bitlen, &off );
		if( vop_time_increment == 0 )
		{
			/*fixed VOP time increment is 0!*/
			return ERR_PARSE;
		}
	} else 
		vop_time_increment = 0;/*variable vop rate*/
	if(flag)
		out->ref2->vop_time_increment=out->ref1->vop_time_increment=vop_time_increment;
	else{
		out->ref1->vop_time_increment=out->ref2->vop_time_increment;
		out->ref2->vop_time_increment=vop_time_increment;
	}
	
	return ERR_NOERROR;
}

int parse_group_video_object_plane(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin){
	/*startcode 000001B3*/
	/*uint32 off = *data_size*8;*/

        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;
	/*TODO*/
	/*				range		bits
		time_code_hours 	0 - 23		5
		time_code_minutes	0 - 59		6
		marker_bit		1		1
		time_code_seconds	0 - 59		6

		The parameters correspond to those defined in the IEC standard publication 461 for ???time and control codes for video tape recorders???. 
		The time code specifies the modulo part (i.e. the full second units) of the time base for the first object plane (in display order) 
		after the GOV header.
	*/
	return ERR_NOERROR;
}

int parse_video_object_plane(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin){
	uint32 off = *data_size*8;
	int modulo_time_base;
	uint32 len;
	
        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;
	len=(*data_size)-4;/*right!??! -4 = - next start code read*/
	/*
	 * vop_coding_type:
	 * 00 intra-coded (I)
	 * 01 predictive-coded (P)
	 * 10 bidirectionally-predictive-coded (B)
	 * 11 sprite (S)
	 *
	*/
	out->vop_coding_type = get_field( data, 2, &off );
	for( modulo_time_base = 0; off / 8 < len; ++modulo_time_base )
		if( ! get_field( data, 1, &off ) ) break;

	if(out->ref1==NULL)
		out->ref1=(mpeg4_time_ref *)calloc(1,sizeof(mpeg4_time_ref));
	
	if(out->ref2==NULL)
		out->ref2=(mpeg4_time_ref *)calloc(1,sizeof(mpeg4_time_ref));

	if(out->vop_coding_type != 2)/*B-FRAME*/
		out->ref1->modulo_time_base=out->ref2->modulo_time_base;
	out->ref2->modulo_time_base+=modulo_time_base;/*cumulative number of modulo_time_base*/

	if( ! get_field( data, 1, &off ) )
	{
		/* missing marker*/
		return ERR_PARSE;
	}

	/*TODO: variable frame rate*/
	out->ref1->var_time_increment=out->ref2->var_time_increment = get_field( data, out->vtir_bitlen, &off );

#if DEBUG
/*	
	fprintf(stderr," - FRAME: %c \n","IPBS"[out->vop_coding_type]);
	
	fprintf(stderr,"\t - REF2: var_time_increment = %d - vop_time_increment= %d - vop_time_increment_resolution = %d - modulo_time_base = %d -\n",out->ref2->var_time_increment, out->ref2->vop_time_increment, out->ref2->vop_time_increment_resolution, out->ref2->modulo_time_base);

	fprintf(stderr,"\t - REF1: var_time_increment = %d - vop_time_increment= %d - vop_time_increment_resolution = %d - modulo_time_base = %d -\n",out->ref1->var_time_increment, out->ref1->vop_time_increment, out->ref1->vop_time_increment_resolution,out->ref1->modulo_time_base);
*/	
#endif	

	return ERR_NOERROR;
}


