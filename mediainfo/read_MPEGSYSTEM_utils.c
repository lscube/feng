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
/*#include <string.h>*/
#include <fcntl.h>

#include <fenice/types.h>
#include <fenice/utils.h>	/*TODO: return ERR_X */
#include <fenice/mediainfo.h>
#include <fenice/mpeg_system.h>
//#include <fenice/prefs.h>
/*#if HAVE_ALLOCA_H
#include <alloca.h>
#endif*/

int read_pack_head(uint8 * buf, uint32 * buf_size, int fin,
		   unsigned char *final_byte, SCR * scr)
{				/* reads pack header */
	unsigned char byte1;
	unsigned char vett_scr[32];
	int i, count = 0;
	read(fin, &(buf[*buf_size]), 1);
	count += 1;
	*buf_size += 1;
	byte1 = buf[*buf_size - 1];
	scr->msb = ((byte1 & 0x08) != 0);
	vett_scr[0] = ((byte1 & 0x04) != 0);
	vett_scr[1] = ((byte1 & 0x02) != 0);
	read(fin, &(buf[*buf_size]), 1);
	*buf_size += 1;
	count += 1;
	byte1 = buf[*buf_size - 1];
	vett_scr[2] = ((byte1 & 0x80) != 0);
	vett_scr[3] = ((byte1 & 0x40) != 0);
	vett_scr[4] = ((byte1 & 0x20) != 0);
	vett_scr[5] = ((byte1 & 0x10) != 0);
	vett_scr[6] = ((byte1 & 0x08) != 0);
	vett_scr[7] = ((byte1 & 0x04) != 0);
	vett_scr[8] = ((byte1 & 0x02) != 0);
	vett_scr[9] = ((byte1 & 0x01) != 0);
	read(fin, &(buf[*buf_size]), 1);
	*buf_size += 1;
	count += 1;
	byte1 = buf[*buf_size - 1];
	vett_scr[10] = ((byte1 & 0x80) != 0);
	vett_scr[11] = ((byte1 & 0x40) != 0);
	vett_scr[12] = ((byte1 & 0x20) != 0);
	vett_scr[13] = ((byte1 & 0x10) != 0);
	vett_scr[14] = ((byte1 & 0x08) != 0);
	vett_scr[15] = ((byte1 & 0x04) != 0);
	vett_scr[16] = ((byte1 & 0x02) != 0);
	read(fin, &(buf[*buf_size]), 1);
	*buf_size += 1;
	count += 1;
	byte1 = buf[*buf_size - 1];
	vett_scr[17] = ((byte1 & 0x80) != 0);
	vett_scr[18] = ((byte1 & 0x40) != 0);
	vett_scr[19] = ((byte1 & 0x20) != 0);
	vett_scr[20] = ((byte1 & 0x10) != 0);
	vett_scr[21] = ((byte1 & 0x08) != 0);
	vett_scr[22] = ((byte1 & 0x04) != 0);
	vett_scr[23] = ((byte1 & 0x02) != 0);
	vett_scr[24] = ((byte1 & 0x01) != 0);
	read(fin, &(buf[*buf_size]), 1);
	*buf_size += 1;
	count += 1;
	byte1 = buf[*buf_size - 1];
	vett_scr[25] = ((byte1 & 0x80) != 0);
	vett_scr[26] = ((byte1 & 0x40) != 0);
	vett_scr[27] = ((byte1 & 0x20) != 0);
	vett_scr[28] = ((byte1 & 0x10) != 0);
	vett_scr[29] = ((byte1 & 0x08) != 0);
	vett_scr[30] = ((byte1 & 0x04) != 0);
	vett_scr[31] = ((byte1 & 0x02) != 0);
	count += next_start_code(buf, buf_size, fin);
	read(fin, &(buf[*buf_size]), 1);
	*buf_size += 1;
	count += 1;
	*final_byte = buf[*buf_size - 1];
	if (*final_byte == 0xbb) {	// read system_header if present
		count += next_start_code(buf, buf_size, fin);
		read(fin, &(buf[*buf_size]), 1);
		*buf_size += 1;
		*final_byte = buf[*buf_size - 1];
	}
	for (i = 0; i < 32; i++) {
		scr->scr = (scr->scr) * 2 + vett_scr[i];
	}

	return count;
}

int read_packet_head(uint8 * buf, uint32 * buf_size, int fin,
		     unsigned char *final_byte, int *time_set, PTS * pts,
		     PTS * dts, int *dts_present, PTS * pts_audio)
{				/* reads packet header */
	unsigned char byte1;
	unsigned char vett_pts[32];
	unsigned char vett_dts[32];
	int i, pts_present = 0, count = 0;
	read(fin, &(buf[*buf_size]), 2);	// read packet_length
	*buf_size += 2;
	count += 2;
	*dts_present = 0;
	if (*final_byte != 0xbf) {
		do {		// read stuffing bytes
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
		} while (buf[*buf_size - 1] == 0xff);
		byte1 = buf[*buf_size - 1];
		if ((byte1 & 0xc0) == 0x40) {	// read buffer scale and buffer size
			read(fin, &(buf[*buf_size]), 2);
			*buf_size += 2;
			count += 2;
			byte1 = buf[*buf_size - 1];
		}
		if ((byte1 & 0xf0) == 0x20) {	// read presentation time stamp
			pts_present = 1;
			*dts_present = 0;
			if (*final_byte != 0xc0) {
				pts->msb = ((byte1 & 0x08) != 0);
			} else {
				pts_audio->msb = ((byte1 & 0x08) != 0);
			}
			vett_pts[0] = ((byte1 & 0x04) != 0);
			vett_pts[1] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[2] = ((byte1 & 0x80) != 0);
			vett_pts[3] = ((byte1 & 0x40) != 0);
			vett_pts[4] = ((byte1 & 0x20) != 0);
			vett_pts[5] = ((byte1 & 0x10) != 0);
			vett_pts[6] = ((byte1 & 0x08) != 0);
			vett_pts[7] = ((byte1 & 0x04) != 0);
			vett_pts[8] = ((byte1 & 0x02) != 0);
			vett_pts[9] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[10] = ((byte1 & 0x80) != 0);
			vett_pts[11] = ((byte1 & 0x40) != 0);
			vett_pts[12] = ((byte1 & 0x20) != 0);
			vett_pts[13] = ((byte1 & 0x10) != 0);
			vett_pts[14] = ((byte1 & 0x08) != 0);
			vett_pts[15] = ((byte1 & 0x04) != 0);
			vett_pts[16] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[17] = ((byte1 & 0x80) != 0);
			vett_pts[18] = ((byte1 & 0x40) != 0);
			vett_pts[19] = ((byte1 & 0x20) != 0);
			vett_pts[20] = ((byte1 & 0x10) != 0);
			vett_pts[21] = ((byte1 & 0x08) != 0);
			vett_pts[22] = ((byte1 & 0x04) != 0);
			vett_pts[23] = ((byte1 & 0x02) != 0);
			vett_pts[24] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[25] = ((byte1 & 0x80) != 0);
			vett_pts[26] = ((byte1 & 0x40) != 0);
			vett_pts[27] = ((byte1 & 0x20) != 0);
			vett_pts[28] = ((byte1 & 0x10) != 0);
			vett_pts[29] = ((byte1 & 0x08) != 0);
			vett_pts[30] = ((byte1 & 0x04) != 0);
			vett_pts[31] = ((byte1 & 0x02) != 0);
		} else if ((byte1 & 0xf0) == 0x30) {
			pts_present = 1;
			*dts_present = 1;	// read presentation time stamp
			if (*final_byte != 0xc0) {
				pts->msb = ((byte1 & 0x08) != 0);
			} else {
				pts_audio->msb = ((byte1 & 0x08) != 0);
			}
			vett_pts[0] = ((byte1 & 0x04) != 0);
			vett_pts[1] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[2] = ((byte1 & 0x80) != 0);
			vett_pts[3] = ((byte1 & 0x40) != 0);
			vett_pts[4] = ((byte1 & 0x20) != 0);
			vett_pts[5] = ((byte1 & 0x10) != 0);
			vett_pts[6] = ((byte1 & 0x08) != 0);
			vett_pts[7] = ((byte1 & 0x04) != 0);
			vett_pts[8] = ((byte1 & 0x02) != 0);
			vett_pts[9] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[10] = ((byte1 & 0x80) != 0);
			vett_pts[11] = ((byte1 & 0x40) != 0);
			vett_pts[12] = ((byte1 & 0x20) != 0);
			vett_pts[13] = ((byte1 & 0x10) != 0);
			vett_pts[14] = ((byte1 & 0x08) != 0);
			vett_pts[15] = ((byte1 & 0x04) != 0);
			vett_pts[16] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[17] = ((byte1 & 0x80) != 0);
			vett_pts[18] = ((byte1 & 0x40) != 0);
			vett_pts[19] = ((byte1 & 0x20) != 0);
			vett_pts[20] = ((byte1 & 0x10) != 0);
			vett_pts[21] = ((byte1 & 0x08) != 0);
			vett_pts[22] = ((byte1 & 0x04) != 0);
			vett_pts[23] = ((byte1 & 0x02) != 0);
			vett_pts[24] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_pts[25] = ((byte1 & 0x80) != 0);
			vett_pts[26] = ((byte1 & 0x40) != 0);
			vett_pts[27] = ((byte1 & 0x20) != 0);
			vett_pts[28] = ((byte1 & 0x10) != 0);
			vett_pts[29] = ((byte1 & 0x08) != 0);
			vett_pts[30] = ((byte1 & 0x04) != 0);
			vett_pts[31] = ((byte1 & 0x02) != 0);

			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			dts->msb = ((byte1 & 0x08) != 0);	// read decoding time stamp
			vett_dts[0] = ((byte1 & 0x04) != 0);
			vett_dts[1] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_dts[2] = ((byte1 & 0x80) != 0);
			vett_dts[3] = ((byte1 & 0x40) != 0);
			vett_dts[4] = ((byte1 & 0x20) != 0);
			vett_dts[5] = ((byte1 & 0x10) != 0);
			vett_dts[6] = ((byte1 & 0x08) != 0);
			vett_dts[7] = ((byte1 & 0x04) != 0);
			vett_dts[8] = ((byte1 & 0x02) != 0);
			vett_dts[9] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_dts[10] = ((byte1 & 0x80) != 0);
			vett_dts[11] = ((byte1 & 0x40) != 0);
			vett_dts[12] = ((byte1 & 0x20) != 0);
			vett_dts[13] = ((byte1 & 0x10) != 0);
			vett_dts[14] = ((byte1 & 0x08) != 0);
			vett_dts[15] = ((byte1 & 0x04) != 0);
			vett_dts[16] = ((byte1 & 0x02) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_dts[17] = ((byte1 & 0x80) != 0);
			vett_dts[18] = ((byte1 & 0x40) != 0);
			vett_dts[19] = ((byte1 & 0x20) != 0);
			vett_dts[20] = ((byte1 & 0x10) != 0);
			vett_dts[21] = ((byte1 & 0x08) != 0);
			vett_dts[22] = ((byte1 & 0x04) != 0);
			vett_dts[23] = ((byte1 & 0x02) != 0);
			vett_dts[24] = ((byte1 & 0x01) != 0);
			read(fin, &(buf[*buf_size]), 1);
			*buf_size += 1;
			count += 1;
			byte1 = buf[*buf_size - 1];
			vett_dts[25] = ((byte1 & 0x80) != 0);
			vett_dts[26] = ((byte1 & 0x40) != 0);
			vett_dts[27] = ((byte1 & 0x20) != 0);
			vett_dts[28] = ((byte1 & 0x10) != 0);
			vett_dts[29] = ((byte1 & 0x08) != 0);
			vett_dts[30] = ((byte1 & 0x04) != 0);
			vett_dts[31] = ((byte1 & 0x02) != 0);
		}
	}
	if (!*time_set && pts_present && (*final_byte != 0xc0)) {
		for (i = 0; i < 32; i++) {
			pts->pts = (pts->pts) * 2 + vett_pts[i];
		}
		*time_set = 1;
	}

	if (!*time_set && (*final_byte == 0xc0)) {
		for (i = 0; i < 32; i++) {
			pts_audio->pts = (pts_audio->pts) * 2 + vett_pts[i];
		}
		*time_set = 1;
	}

	if (*dts_present) {
		for (i = 0; i < 32; i++) {
			dts->pts = (dts->pts) * 2 + vett_dts[i];
		}
	}
	return count;
}

int read_packet(uint8 * buf, uint32 * buf_size, int fin,
		unsigned char *final_byte)
{				/* reads a packet */
	int count = 0;		/* at the end count is the size of the packet */
	unsigned char byte1, byte2;
	do {
		count += next_start_code(buf, buf_size, fin);
		//count+=4;
		read(fin, &(buf[*buf_size]), 1);
		*buf_size += 1;
		*final_byte = buf[*buf_size - 1];
		if (*final_byte == 0x00) {
			read(fin, &(buf[*buf_size]), 2);
			*buf_size += 2;
			count += 2;
			byte1 = buf[*buf_size - 1];
			byte2 = (byte1 >> 3) & 0x07;
			/*if (byte2 == 0x01) {
			   printf ("Pictute type: I\n");
			   } else if (byte2 == 0x02) {
			   printf ("Pictute type: P\n");
			   } else {
			   printf ("Pictute type: B\n");
			   } */
		}
		//printf("Final Byte: %x\n", *final_byte);
	} while (*final_byte < 0xb9);
	//*buf_size-=4;
	//lseek(fin,-4,SEEK_CUR);
	return count;
}
