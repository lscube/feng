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

static MediaParserInfo info = {
	"MPV",
	"V"
};

FNC_LIB_MEDIAPARSER(mpv);

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
static int seq_head()
{
}

static int seq_ext()
{
}

static int ext_and_user_data()
{
}

static int gop_head()
{
}

static int picture_head()
{
}

static int picture_coding_ext()
{
}

static int picture_data()
{
}

static int slice()
{
}

static int probe_standard()
{
}

*/

/*mediaparser_module interface implementation*/
static int init(MediaProperties *properties, void **private_data)
{
	return 0;
}

static int uninit(void *private_data)
{
	return 0;
}

static int get_frame2(uint8 *dst, uint32 dst_nbytes, int64 *timestamp, void *properties, InputStream *istream)
{
	return 0;
}


/*see RFC 2250: RTP Payload Format for MPEG1/MPEG2 Video*/
/*src contains a frame*/
static int packetize(uint8 *dst, uint32 dst_nbytes, uint8 *src, uint32 src_nbytes, void *properties)
{
	int ret=0;
	int dst_remained=dst_nbytes;
	int src_remained=src_nbytes;
	uint8 final_byte;
		
	do{
		ret=next_start_code2(dst+dst_nbytes-dst_remained,dst_remained,src+src_nbytes-src_remained,src_remained);
		if(ret==-1)
			return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
		dst_remained-=ret;
		src_remained-=ret;
		final_byte=src[src_nbytes-src_remained+3];

		if(final_byte==SEQ_START_CODE) {
		
		}
		else if(final_byte==EXT_START_CODE) {
			/*means MPEG2*/
		}
		/*
		else if(final_byte==) {
		
		}
		else if(final_byte==)
		else if(final_byte==)
		else if(final_byte==)
		**/
	}while(ret!=-1);
	
	return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
}



