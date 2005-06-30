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

#include <fenice/demuxer.h>
#include <fenice/utils.h>

/*RESOURCE PRIVATE_DATA*/
#if !defined(_MEDIAINFOH) /*remove it when mediainfo will be removed*/ 
typedef enum {
	SD_FL_TWIN=1,
	SD_FL_MULTICAST=2,
	SD_FL_MULTICAST_PORT=4
} sd_descr_flags;

typedef struct __SD_descr {
		sd_descr_flags flags;
		char multicast[16];/*???*/
		char ttl[4];/*???*/
    		//char twin[255]; //see RESOURCE_INFO -> twin
} SD_descr;
/*----------------------*/


/*TRACK PRIVATE_DATA*/
typedef enum {
	ME_FILENAME=1,
	/*    disposable   ME=2,  */
	ME_DESCR_FORMAT=4,
	ME_AGGREGATE=8,
	ME_RESERVED=16,
	ME_FD=32
} me_flags;

typedef enum {
	MED_MSOURCE=2, /* live, stored => st_pipe/st_net/st_device, st_file*/
	MED_PAYLOAD_TYPE=4,
	MED_CLOCK_RATE=8,
	MED_ENCODING_NAME=16,
	MED_AUDIO_CHANNELS=32,
	MED_SAMPLE_RATE=64,    	
	MED_BIT_PER_SAMPLE=128,
	MED_CODING_TYPE=256,
	MED_FRAME_LEN=512,
	MED_BITRATE=1024,
	MED_PKT_LEN=2048,
	MED_PRIORITY=4096,
	MED_FRAME_RATE=16384,
	MED_BYTE_PER_PCKT=32768,
	/*start CC*/
	MED_LICENSE=65536,
	MED_RDF_PAGE=131072,
	MED_TITLE=262144,
	MED_CREATOR=524288,
	MED_ID3=1048576
	/*end CC*/
} me_descr_flags;
#endif //...MEDIAINFOH

typedef struct __FLAGS_DATA{
	me_flags general_flags; 
	me_descr_flags description_flags;	    	    	    		
   	struct __DATA {	
		int frame_len; // i need to move it in Parser
		int priority; //i need to move it. Where? Selector, Track 
		float pkt_len; //i need to move it in Parser
		int byte_per_pckt; //i need to move it in Parser
	} data;
} FlagsData;    	

/*----------------------*/

int sd_init(Resource *r)
{

	/*Allocate Resource PRIVATE DATA and cast it*/
	r->private_data=(SD_descr *)malloc(sizeof(SD_descr));

	/**
	 * parse sd file. Allocate and cast TRACK PRIVATE DATA foreach track.
	 * (in this case track = elementary stream media file)
	 * */
	do{
		//...	
	}while(!eof(r->i_stream->fd));

	return RESOURCE_OK;
}

int sd_probe(Resource *r)
{
	return RESOURCE_OK;
}

int sd_read_header(Resource *r)
{
	return RESOURCE_OK;
}

int sd_read_packet(Resource *r)
{
	return RESOURCE_OK;
}

int sd_close(Resource *r)
{
	return RESOURCE_OK;
}

int sd_seek(Resource *r, long int time_msec)
{
	return RESOURCE_NOT_SEEKABLE;
}

static InputFormat sd_iformat = {
    "sd",
    sd_init,/*ex parse_SD_file*/
    sd_probe,/*ex validate_stream*/
    sd_read_header,/*return RESOURCE_OK and nothing more*/ 
    sd_read_packet,/*switchs between the different parser reader according to the timestamp. \
		     I.e. 1 video frame mpeg1 (1pkt/40msec) and 1 or 2 mp3 audio pkts (1pkt/26.12msec*/
    sd_close, /*close all media described in sd*/
    sd_seek /*seek all media in sd*/
};

