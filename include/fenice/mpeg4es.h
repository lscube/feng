/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#ifndef _MPEG4ESH
#define _MPEG4ESH

#include <fenice/types.h>
#include <fenice/mediainfo.h>

	/*Definitions of the start codes */
#define VIDEO_OBJECT_START_CODE	/*0x00 through 0x1F */
#define VIDEO_OBJECT_LAYER_START_CODE	/*0x20 through 0x2F */
#define RESERVED		/*0x30 through 0xAF */
#define VOS_START_CODE 0xB0	/*visual object sequence */
#define VOS_END_CODE 0xB1	/*visual object sequence */
#define USER_DATA_START_CODE 0xB2
#define GROUP_OF_VOP_START_CODE 0xB3
#define VIDEO_SESSION_ERROR_CODE 0xB4
#define VO_START_CODE 0xB5	/*visual object */
#define VOP_START_CODE 0xB6	/*visual object plane */
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
	/*System start codes aren't used in MPEG4 Visual ES */
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

typedef struct {
	uint32 vop_time_increment_resolution;
	uint32 vop_time_increment;
	uint32 var_time_increment;
	uint32 modulo_time_base;	/*cumulative number of whole modulo_time_base */
} mpeg4_time_ref;

typedef struct mpeg4 {
	int profile_id;
	char config[256];	/*for sdp. TODO. See load_MP4ES and get_SDP_descr */
	int vtir_bitlen;
	mpeg4_time_ref *ref1;
	mpeg4_time_ref *ref2;
	double last_non_b_timestamp;
	double last_b_timestamp;
	double timestamp;
	uint32 time_resolution;
	uint8 final_byte;
	int fragmented;
	uint32 data_read;
	uint32 remained_data_size;
	char *more_data;
	//char *header_data;
	uint32 header_data_size;
	int vop_coding_type;
} static_MPEG4_video_es;


int parse_visual_object_sequence(static_MPEG4_video_es *, uint8 *, uint32 *,
				 int fin);
int parse_visual_object(uint8 * data, uint32 * data_size, int fin);
int parse_video_object(uint8 * data, uint32 * data_size, int fin);
int parse_video_object_layer(static_MPEG4_video_es * out, uint8 * data,
			     uint32 * data_size, int fin);
int parse_group_video_object_plane(static_MPEG4_video_es * out, uint8 * data,
				   uint32 * data_size, int fin);
int parse_video_object_plane(static_MPEG4_video_es * out, uint8 * data,
			     uint32 * data_size, int fin);

int load_MP4ES(media_entry * me);
int read_MPEG4ES_video(media_entry * me, uint8 * data, uint32 * data_size,
		       double *mtime, int *recallme, uint8 * marker);
int free_MP4ES(void *);

#endif
