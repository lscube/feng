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
#include <config.h>
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
		/*return ERR_GENERIC;*/
	}
	i = out->vop_time_increment_resolution = get_field( data, 16, &off );
	if( ! get_field( data, 1, &off ) )
	{
		/*missing marker*/
		/*return ERR_GENERIC;*/
	}
	for( out->vtir_bitlen = 0; i != 0; i >>= 1 ) ++out->vtir_bitlen;
	if( get_field( data, 1, &off ) )
	{
		out->vop_time_increment = get_field( data, out->vtir_bitlen, &off );
		if( out->vop_time_increment == 0 )
		{
			/*fixed VOP time increment is 0!*/
			/*return ERR_GENERIC;*/
		}
	} else out->vop_time_increment = 0;

	return ERR_NOERROR;
}

int parse_video_object_plane(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin){
	uint32 off = *data_size*8;
	int vop_coding_type, modulo_time_base, var_time_increment;
	uint32 len;
	
        if(next_start_code(data,data_size,fin) < 0){
			return ERR_EOF;              
	}
        if(read(fin,&data[*data_size],1)<1){
		return ERR_EOF;
	}
        *data_size+=1;
	len=((*data_size) * 8)-off;/*right!??!*/
	/*
	 * vop_coding_type:
	 * 00 intra-coded (I)
	 * 01 predictive-coded (P)
	 * 10 bidirectionally-predictive-coded (B)
	 * 11 sprite (S)
	 *
	*/
	vop_coding_type = get_field( data, 2, &off );
	for( modulo_time_base = 0; off / 8 < len; ++modulo_time_base )
		if( ! get_field( data, 1, &off ) ) break;
	if( ! get_field( data, 1, &off ) )
	{
		/* missing marker*/
		/*return ERR_GENERIC;*/
	}
	out->modulo_time_base+=modulo_time_base;/*cumulative number of modulo_time_base*/
	var_time_increment = get_field( data, out->vtir_bitlen, &off );

#if ENABLE_DEBUG
//	fprintf(stderr,"frame type %d, vop time increment %d, modulo time base %d\n",vop_coding_type, var_time_increment, modulo_time_base );
#endif
	return ERR_NOERROR;
}


