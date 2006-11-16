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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/mp3.h>

int read_MP3(media_entry * me, uint8 * data, uint32 * data_size, double *mtime,
	     int *recallme, uint8 * marker)
{
	// char thefile[255];
	int ret;
	unsigned char *buff = me->buff_data;
	int N = 0, res;

	*marker = 0;
	*recallme = 0;
	if (!(me->flags & ME_FD)) {
		if ((ret = mediaopen(me)) < 0)
			return ret;
		//me->prev_mstart_offset=0;     
		/* TODO support Id3 TAG v2, this doesn't work */
		//if ((me->description.flags & MED_ID3) && (me->description).msource!=live)
		//      lseek(me->fd,(me->description.tag_dim)+10,SEEK_SET);
	}



	if (me->play_offset != -1 /*me->prev_mstart_offset */  && (me->description).msource != live) {	//random access 
		N = (float) me->description.bitrate / 8 * me->play_offset /
		    1000;
		/*me->prev_mstart_offset= */ me->play_offset = -1;
		// pkt_len is pkt lenght in milliseconds,
		lseek(me->fd, N, SEEK_SET);
	}

	if ((read(me->fd, &(buff[me->buff_size]), 4)) != 4) {
		return ERR_EOF;
	}
	me->buff_size = 0;
	if ((buff[0] == 0xff) && ((buff[1] & 0xe0) == 0xe0)) {
		N = (int) (me->description.frame_len *
			   (float) me->description.bitrate /
			   (float) me->description.sample_rate / 8);
		if (buff[2] & 0x02)
			N++;
	} else {
		// Sync not found, not Mpeg-1/2
		// id3 TAG v1 is suppressed, id3 TAG v2 is not supported and
		//fprintf(stderr,"ERROR: Sync not found, not Mpeg-1/2\n");
		N = (int) (me->description.frame_len *
			   (float) me->description.bitrate /
			   (float) me->description.sample_rate / 8);
		//return ERR_EOF;
	}
	*data_size = N + 4;	// 4 bytes are needed for mp3 in RTP encapsulation
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	// These first 4 bytes are for mp3 in RTP encapsulation

	data[4] = buff[0];
	data[5] = buff[1];
	data[6] = buff[2];
	data[7] = buff[3];

	if ((res = read(me->fd, &(data[8]), N - 4)) < (N - 4)) {
		if ((res <= 0)) {
			return ERR_EOF;
		} else
			*data_size = res + 8;
	}

	return ERR_NOERROR;
}
