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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <fenice/mpeg.h>
#include <fenice/mpeg_utils.h>
#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>

/*see ISO/IEC 11172-2:1993 and ISO/IEC 13818-2:1995 (E)*/
/*prefix*/
#define START_CODE 0x000001

/*value*/
#define PICTURE_START_CODE 0x00
#define SLICE_START_CODE /*0x01 to 0xAF*/
#define USER_DATA_START_CODE 0xB2
#define SEQ_START_CODE 0xB3
#define SEQ_ERROR_CODE 0xB4
#define EXT_START_CODE 0xB5 
#define SEQ_END_CODE 0xB7
#define GROUP_START_CODE 0xB8

/*
static int seq_head

static int seq_ext

static int ext_and_user_data

static int gop_head

static int picture_head

static int picture_coding_ext

static int picture_data

static int slice

static int probe_standard

*/

/*mediaparser_module interface implementation*/
static int init(void)
{
	return 0;
}

static int uninit(void)
{
	return 0;
}

static int get_frame2(uint8 *dst, uint32 dst_nbytes, int64 *timestamp, void *properties, InputStream *istream)
{
	return 0;
}


/*see RFC 2250: RTP Payload Format for MPEG1/MPEG2 Video*/
static int packetize(uint8 *dst, uint32 dst_nbytes, uint8 *src, uint32 src_nbytes, void *properties)
{
	video_spec_prop *prop;

	prop = (video_spec_prop *)properties;

	

	return ERR_NOERROR;
}



