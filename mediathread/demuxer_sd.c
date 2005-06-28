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

int sd_init(Resource *r)
{
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

