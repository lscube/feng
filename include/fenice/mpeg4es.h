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

#ifndef _MPEG4ESH
#define _MPEG4ESH
	#include <fenice/types.h>
	#include <fenice/mediainfo.h>
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
	//#define SYSTEM_START_CODES /*0xC6 through 0xFF*/

	/* MPEG4 Video ES format:
 	* 
 	* +--Included in SDP config line
 	* |
 	* | Visual Object Sequence
	* |   start code (0x000001B0), profile and level indication, stuff
	* |     Visual Object
	* |       start code (0x000001B5), visual object type, stuff
	* |     Video Object
	* |       start code (0x00000100-0x0000011F)
 	* |     Video Object Layer
 	* |       start code (0x00000120-0x0000012F), stuff to parse
 	*           Group Video Object Plane*
 	*             start code (0x000001B3), time codes, stuff
 	*           Video Object Plane*
 	*             start code (0x000001B6), vop coding type, modulo time base,
 	*             vop time increment
 	*     end code (0x000001B1)
 	*/
	typedef struct mpeg4 {
		int profile_id;
		uint8 config[256]; /*for sdp. Not used but TODO*/
		int vop_time_increment_resolution;
		int vop_time_increment;
		int modulo_time_base;/*cumulative number of whole modulo_time_base*/
		int vtir_bitlen;
		/*double rtp_timestamp;*/
		uint8 final_byte;
		uint32 init;
		int fragmented;
		uint32 data_read;
		uint32 remained_data_size;
		char *more_data;
	} static_MPEG4_video_es;

	int get_field( uint8 *d, uint32 bits, uint32 *offset);
	int parse_visual_object_sequence(static_MPEG4_video_es *,uint8 *, uint32 *,int fin);
	int parse_visual_object(uint8 *data, uint32 *data_size,int fin);
	int parse_video_object(uint8 *data, uint32 *data_size,int fin);
	int parse_video_object_layer(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin);
	int parse_video_object_plane(static_MPEG4_video_es *out, uint8 *data, uint32 *data_size,int fin);
	
#endif
