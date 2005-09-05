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
#include <fenice/MediaParser.h>
#include <fenice/utils.h>
#include <fenice/types.h>

#define START_CODE 0x000001
#define PICTURE_START_CODE 0x00
#define SLICE_START_CODE /*0x01 to 0xAF*/
#define USER_DATA_START_CODE 0xB2
#define SEQ_START_CODE 0xB3
#define SEQ_ERROR_CODE 0xB4
#define EXT_START_CODE 0xB5 
#define SEQ_END_CODE 0xB7
#define GROUP_START_CODE 0xB8

/*
static int read_seq_head

static int read_gop_head

static int read_picture_head

static int read_slice

static int probe_standard

static int read_picture_coding_ext
*/


int read_mpeg12(uint8 *dest, uint32 dest_dim, uint8 *source, uint32 source_dim, uint32 nread, void *properties)
{
	video_spec_prop *prop;

	prop = (video_spec_prop *)properties;

	

	return ERR_NOERROR;
}



