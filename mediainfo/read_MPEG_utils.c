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
#include <string.h>
#include <fcntl.h>

#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>

int next_start_code(uint8 *buf, uint32 *buf_size,int fin) {  			
	char buf_aux[3];                                               	
	int i;
	unsigned int count=0;
	if ( read(fin,&buf_aux,3) <3) {                                			/* If there aren't 3 more bytes we are at EOF */
		return -1;
	}
	while ( !((buf_aux[0] == 0x00) && (buf_aux[1]==0x00) && (buf_aux[2]==0x01))) {
    		buf[*buf_size]=buf_aux[0];
    		*buf_size+=1;
		buf_aux[0]=buf_aux[1];
		buf_aux[1]=buf_aux[2];
      		if ( read(fin,&buf_aux[2],1) < 1) {
			return -1;
        	}
      		count++;
    	}
    	for (i=0;i<3;i++) {
    		buf[*buf_size]=buf_aux[i];
    		*buf_size+=1;
    	}
     	return count;
}

int read_seq_head(uint8 *buf,uint32 *buf_size, int fin, char *final_byte, standard std) {  /* reads sequence header */
        read(fin,&(buf[*buf_size]),8);                       			//reads first 95 bits +1
        *buf_size+=8;
        if (buf[*buf_size-1] & 0x02) {                       			// intra quantizer matrix present
                read(fin,&(buf[*buf_size]),64);              			// reads intra quantizer matrix +1 bit
                *buf_size+=64;
        }
        if (buf[*buf_size-1] & 0x01) {                       			// non intra quantizer matrix present
                read(fin,&(buf[*buf_size]),64);              			// reads non intra quantizer matrix
                *buf_size+=64;
        }
        next_start_code(buf,buf_size,fin);
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        if (std == MPEG_1) {
                if (buf[*buf_size-1] == 0xb5) {              			// reads extension data if present (MPEG-1 only)
                        next_start_code(buf,buf_size,fin);
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                }
                if (buf[*buf_size-1] == 0xb2) {              			// reads user data if present (MPEG-1 only)
                        next_start_code(buf,buf_size,fin);
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                }
        }
        *final_byte=buf[*buf_size-1];
        return 1;
}

int read_gop_head(uint8 *buf,uint32 *buf_size, int fin, char *final_byte, char *hours, char *minutes, char *seconds, char *picture, standard std) {  /* reads GOP header */
	int count=0;
        unsigned char byte1, byte2, byte3, byte4, byte5, byte6, byte7;
        read(fin,&(buf[*buf_size]),4);                       			/* reads GOP hours, minutes, seconds and picture number */
        *buf_size+=4;
        count+=4;
        byte1 = buf[*buf_size-4];
        byte2 = buf[*buf_size-3];
        byte3 = buf[*buf_size-2];
        byte4 = buf[*buf_size-1];
        *hours = ((byte1>>2) & 0x1f);
        byte5 = ((byte2>>4) & 0x0f);
        *minutes = ((byte1*16) & 0x30) + byte5;
        byte6 = ((byte3>>5) & 0x07);
        *seconds = ((byte2*8) & 0x38) + byte6;
        byte7 = ((byte4>>7) & 0x01);
        *picture = ((byte3*2) & 0x3e) + byte7;

        count+=next_start_code(buf,buf_size,fin);
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        count+=4;
        if (std == MPEG_1)      {
                if (buf[*buf_size-1] == 0xb5) {                      		// reads extension data if present (MPEG-1 only)
                        count+=next_start_code(buf,buf_size,fin);
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                        count+=4;
                }
                if (buf[*buf_size-1] == 0xb2) {                      		// reads user data if present (MPEG-1 only)
                        count+=next_start_code(buf,buf_size,fin);
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                        count+=4;
                }
        }
        *final_byte=buf[*buf_size-1];
        return count;
}

int read_picture_head(uint8 *buf,uint32 *buf_size, int fin, char *final_byte, char *temp_ref, video_spec_head1* vsh1, standard std) { /* reads picture head */
	int count=0;
        unsigned char byte1,byte2,byte3,byte4,byte5,byte6;
        read(fin,&(buf[*buf_size]),2);            					/* reads picture temporal reference */
        *buf_size+=2;
        count+=2;
        byte1 = buf[*buf_size-2];
        byte2 = byte5 = buf[*buf_size-1];
        byte3 = ((byte2>>6) & 0x03);
        byte4 = byte1*4 + byte3;
        byte6 = (byte5>>3) & 0x07;
        *temp_ref = byte4;
        vsh1->p = byte6;
        vsh1->tr = *temp_ref;
        if ((vsh1->p == 2) || (vsh1->p == 3)) {       					/* If P or B picture */
                read(fin,&(buf[*buf_size]),2);
                *buf_size+=2;
                count+=2;
                byte1 = byte3= buf[*buf_size-1];
                byte2 = (byte1>>2) & 0x01;
                vsh1->ffv = byte2;
                read(fin,&(buf[*buf_size]),1);
                *buf_size+=1;
                count+=1;
                byte4 = byte1 = byte2 = buf[*buf_size-1];
                byte5 = (((byte3 & 0x03) *2) + ((byte4 >> 7) & 0x01));
                vsh1->ffc = byte5;
                if (vsh1->p == 3) {                            				/* If B picture */
			//fprintf(stderr,"B Frame occurred\n");
                        byte3 = (byte1>>6) & 0x01;
                        vsh1->fbv = byte3;
                        byte4 = (byte2>>3) & 0x07;
                        vsh1->bfc = byte4;
                } else {
                     vsh1->fbv = vsh1->bfc =0;
             }
        } else {
                vsh1->ffv = vsh1->ffc = vsh1->fbv = vsh1->bfc = 0;
        }
        count+=next_start_code(buf,buf_size,fin);
        count+=4;
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        if (std == MPEG_1){
                if (buf[*buf_size-1] == 0xb5) {                      		// reads extension data if present (MPEG-1 only )
                        count+=next_start_code(buf,buf_size,fin);
                        count+=4;
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                }
                if (buf[*buf_size-1] == 0xb2) {                      		// reads user data if present (MPEG-1 only )
                        count+=next_start_code(buf,buf_size,fin);
                        count+=4;
                        read(fin,&(buf[*buf_size]),1);
                        *buf_size+=1;
                }
        }
        *final_byte=buf[*buf_size-1];
        return count;
}

int read_slice(uint8 *buf,uint32 *buf_size, int fin, char *final_byte) {     /* reads a slice */
        int count;                                                                      /* at the end count is the slice size */
        count=next_start_code(buf,buf_size,fin);
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        *final_byte=buf[*buf_size-1];
        //*buf_size-=4;
        //lseek(fin,-4,SEEK_CUR);
        return count;
}

int probe_standard(uint8 *buf,uint32 *buf_size,int fin, standard *std) {    /* If the sequence_extension occurs immediately */
        unsigned char final_byte;                                                      /* after the sequence header, the sequence is an */
        next_start_code(buf,buf_size,fin);                                             /* MPEG-2 video sequence */
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        final_byte=buf[*buf_size-1];
        if (final_byte == 0xb3) {
                read_seq_head(buf,buf_size,fin,&final_byte,*std);
        }
        if (final_byte  == 0xb5){
                *std=MPEG_2;
        } else {
                *std=MPEG_1;
        }
        return 1;
}

int read_picture_coding_ext(uint8 *buf,uint32 *buf_size, int fin, char *final_byte,video_spec_head2* vsh2) {  /* reads picture coding extension */
	int count=0;
        unsigned char byte1,byte2,byte3,byte4,byte5,byte6;
        vsh2->x=0;
        vsh2->e=0;
        read(fin,&(buf[*buf_size]),3);
        *buf_size+=3;
        count+=3;
        byte1 = buf[*buf_size-3];
        byte2 = byte4 = buf[*buf_size-2];
        byte3 = byte5 = byte6 = buf[*buf_size-1];
        vsh2->f00 = byte1 & 0x0f;
        vsh2->f01 = (byte2>>4) & 0x0f;
        vsh2->f10 = byte4 & 0x0f;
        vsh2->f11 = (byte3>>4) & 0x0f;
        vsh2->dc = (byte5>>2) & 0x03;
        vsh2->ps = byte6 & 0x03;
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        count+=1;
        byte1 = buf[*buf_size-1];
        if (byte1 & 0x80) {
             vsh2->t=1;
        } else vsh2->t=0;
        if (byte1 & 0x40) vsh2->p=1;
        if (byte1 & 0x20) vsh2->c=1;
        if (byte1 & 0x10) {
                vsh2->q=1;
        } else  vsh2->q=0;
        if (byte1 & 0x08) vsh2->v=1;
        if (byte1 & 0x04) vsh2->a=1;
        if (byte1 & 0x02) vsh2->r=1;
        if (byte1 & 0x01) vsh2->h=1;
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        count+=1;
        byte1 = buf[*buf_size-1];
        if (byte1 & 0x80) vsh2->g=1;
        vsh2->d=0;
        count+=next_start_code(buf,buf_size,fin);
        count+=4;
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        //count+=1;
        *final_byte=buf[*buf_size-1];
        return count;
}

int read_pack_head(uint8 *buf, uint32 *buf_size, int fin, unsigned char *final_byte, SCR *scr) {  /* reads pack header */
	unsigned char byte1;
        unsigned char vett_scr[32];
	int i,count=0;
	read(fin,&(buf[*buf_size]),1);
	count+=1;
	*buf_size+=1;
	byte1 = buf[*buf_size-1];
	scr->msb = ((byte1 & 0x08)!=0);
	vett_scr[0] = ((byte1 & 0x04)!=0);
	vett_scr[1] = ((byte1 & 0x02)!=0);
	read(fin,&(buf[*buf_size]),1);
	*buf_size+=1;
	count+=1;
	byte1 = buf[*buf_size-1];
	vett_scr[2] = ((byte1 & 0x80)!=0);
	vett_scr[3] = ((byte1 & 0x40)!=0);
	vett_scr[4] = ((byte1 & 0x20)!=0);
	vett_scr[5] = ((byte1 & 0x10)!=0);
	vett_scr[6] = ((byte1 & 0x08)!=0);
	vett_scr[7] = ((byte1 & 0x04)!=0);
	vett_scr[8] = ((byte1 & 0x02)!=0);
	vett_scr[9] = ((byte1 & 0x01)!=0);
	read(fin,&(buf[*buf_size]),1);
	*buf_size+=1;
	count+=1;
	byte1 = buf[*buf_size-1];
	vett_scr[10] = ((byte1 & 0x80)!=0);
	vett_scr[11] = ((byte1 & 0x40)!=0);
	vett_scr[12] = ((byte1 & 0x20)!=0);
	vett_scr[13] = ((byte1 & 0x10)!=0);
	vett_scr[14] = ((byte1 & 0x08)!=0);
	vett_scr[15] = ((byte1 & 0x04)!=0);
	vett_scr[16] = ((byte1 & 0x02)!=0);
	read(fin,&(buf[*buf_size]),1);
	*buf_size+=1;
	count+=1;
	byte1 = buf[*buf_size-1];
	vett_scr[17] = ((byte1 & 0x80)!=0);
	vett_scr[18] = ((byte1 & 0x40)!=0);
	vett_scr[19] = ((byte1 & 0x20)!=0);
	vett_scr[20] = ((byte1 & 0x10)!=0);
	vett_scr[21] = ((byte1 & 0x08)!=0);
	vett_scr[22] = ((byte1 & 0x04)!=0);
	vett_scr[23] = ((byte1 & 0x02)!=0);
	vett_scr[24] = ((byte1 & 0x01)!=0);
	read(fin,&(buf[*buf_size]),1);
	*buf_size+=1;
	count+=1;
	byte1 = buf[*buf_size-1];
	vett_scr[25] = ((byte1 & 0x80)!=0);
	vett_scr[26] = ((byte1 & 0x40)!=0);
	vett_scr[27] = ((byte1 & 0x20)!=0);
	vett_scr[28] = ((byte1 & 0x10)!=0);
	vett_scr[29] = ((byte1 & 0x08)!=0);
	vett_scr[30] = ((byte1 & 0x04)!=0);
	vett_scr[31] = ((byte1 & 0x02)!=0);
	count+=next_start_code(buf,buf_size,fin);
	read(fin,&(buf[*buf_size]),1);
	*buf_size+=1;
	count+=1;
	*final_byte=buf[*buf_size-1];
	if (*final_byte == 0xbb) {                             				// read system_header if present
		count+=next_start_code(buf,buf_size,fin);
		read(fin,&(buf[*buf_size]),1);
		*buf_size+=1;
		*final_byte=buf[*buf_size-1];
	}
	for (i=0;i<32;i++) {
		scr->scr = (scr->scr)*2 + vett_scr[i];
	}

	return count;
}

int read_packet_head(uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte, int *time_set, PTS *pts, PTS *dts, int *dts_present, PTS *pts_audio) { /* reads packet header */
        unsigned char byte1;
        unsigned char vett_pts[32];
        unsigned char vett_dts[32];
        int i, pts_present=0, count=0;
	read(fin,&(buf[*buf_size]),2);						// read packet_length
	*buf_size+=2;
	count+=2;
	*dts_present = 0;
	if (*final_byte != 0xbf) {
		do {                                            			// read stuffing bytes
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
		} while (buf[*buf_size-1] == 0xff);
  		byte1 = buf[*buf_size-1];
    		if ((byte1 & 0xc0) == 0x40) {                   			// read buffer scale and buffer size
	     		read(fin,&(buf[*buf_size]),2);
			*buf_size+=2;
			count+=2;
			byte1 = buf[*buf_size-1];
      		}
        	if ((byte1 & 0xf0) == 0x20) {                   			// read presentation time stamp
		 	pts_present = 1;
		  	*dts_present = 0;
		   	if (*final_byte != 0xc0){
				pts->msb = ((byte1 & 0x08)!=0);
			} else {
				pts_audio->msb = ((byte1 & 0x08)!=0);
			}
			vett_pts[0] = ((byte1 & 0x04)!=0);
			vett_pts[1] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[2] = ((byte1 & 0x80)!=0);
			vett_pts[3] = ((byte1 & 0x40)!=0);
			vett_pts[4] = ((byte1 & 0x20)!=0);
			vett_pts[5] = ((byte1 & 0x10)!=0);
			vett_pts[6] = ((byte1 & 0x08)!=0);
			vett_pts[7] = ((byte1 & 0x04)!=0);
			vett_pts[8] = ((byte1 & 0x02)!=0);
			vett_pts[9] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[10] = ((byte1 & 0x80)!=0);
			vett_pts[11] = ((byte1 & 0x40)!=0);
			vett_pts[12] = ((byte1 & 0x20)!=0);
			vett_pts[13] = ((byte1 & 0x10)!=0);
			vett_pts[14] = ((byte1 & 0x08)!=0);
			vett_pts[15] = ((byte1 & 0x04)!=0);
			vett_pts[16] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[17] = ((byte1 & 0x80)!=0);
			vett_pts[18] = ((byte1 & 0x40)!=0);
			vett_pts[19] = ((byte1 & 0x20)!=0);
			vett_pts[20] = ((byte1 & 0x10)!=0);
			vett_pts[21] = ((byte1 & 0x08)!=0);
			vett_pts[22] = ((byte1 & 0x04)!=0);
			vett_pts[23] = ((byte1 & 0x02)!=0);
			vett_pts[24] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[25] = ((byte1 & 0x80)!=0);
			vett_pts[26] = ((byte1 & 0x40)!=0);
			vett_pts[27] = ((byte1 & 0x20)!=0);
			vett_pts[28] = ((byte1 & 0x10)!=0);
			vett_pts[29] = ((byte1 & 0x08)!=0);
			vett_pts[30] = ((byte1 & 0x04)!=0);
			vett_pts[31] = ((byte1 & 0x02)!=0);
   		} else if ((byte1 & 0xf0) == 0x30) {
	       		pts_present = 1;
	         	*dts_present = 1;                                  		// read presentation time stamp
	        	if (*final_byte != 0xc0){
				pts->msb = ((byte1 & 0x08)!=0);
			} else {
				pts_audio->msb = ((byte1 & 0x08)!=0);
			}
			vett_pts[0] = ((byte1 & 0x04)!=0);
			vett_pts[1] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[2] = ((byte1 & 0x80)!=0);
			vett_pts[3] = ((byte1 & 0x40)!=0);
			vett_pts[4] = ((byte1 & 0x20)!=0);
			vett_pts[5] = ((byte1 & 0x10)!=0);
			vett_pts[6] = ((byte1 & 0x08)!=0);
			vett_pts[7] = ((byte1 & 0x04)!=0);
			vett_pts[8] = ((byte1 & 0x02)!=0);
			vett_pts[9] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[10] = ((byte1 & 0x80)!=0);
			vett_pts[11] = ((byte1 & 0x40)!=0);
			vett_pts[12] = ((byte1 & 0x20)!=0);
			vett_pts[13] = ((byte1 & 0x10)!=0);
			vett_pts[14] = ((byte1 & 0x08)!=0);
			vett_pts[15] = ((byte1 & 0x04)!=0);
			vett_pts[16] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[17] = ((byte1 & 0x80)!=0);
			vett_pts[18] = ((byte1 & 0x40)!=0);
			vett_pts[19] = ((byte1 & 0x20)!=0);
			vett_pts[20] = ((byte1 & 0x10)!=0);
			vett_pts[21] = ((byte1 & 0x08)!=0);
			vett_pts[22] = ((byte1 & 0x04)!=0);
			vett_pts[23] = ((byte1 & 0x02)!=0);
			vett_pts[24] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_pts[25] = ((byte1 & 0x80)!=0);
			vett_pts[26] = ((byte1 & 0x40)!=0);
			vett_pts[27] = ((byte1 & 0x20)!=0);
			vett_pts[28] = ((byte1 & 0x10)!=0);
			vett_pts[29] = ((byte1 & 0x08)!=0);
			vett_pts[30] = ((byte1 & 0x04)!=0);
			vett_pts[31] = ((byte1 & 0x02)!=0);

			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 =buf[*buf_size-1];
                        dts->msb = ((byte1 & 0x08)!=0); 				// read decoding time stamp
			vett_dts[0] = ((byte1 & 0x04)!=0);
			vett_dts[1] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_dts[2] = ((byte1 & 0x80)!=0);
			vett_dts[3] = ((byte1 & 0x40)!=0);
			vett_dts[4] = ((byte1 & 0x20)!=0);
			vett_dts[5] = ((byte1 & 0x10)!=0);
			vett_dts[6] = ((byte1 & 0x08)!=0);
			vett_dts[7] = ((byte1 & 0x04)!=0);
			vett_dts[8] = ((byte1 & 0x02)!=0);
			vett_dts[9] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_dts[10] = ((byte1 & 0x80)!=0);
			vett_dts[11] = ((byte1 & 0x40)!=0);
			vett_dts[12] = ((byte1 & 0x20)!=0);
			vett_dts[13] = ((byte1 & 0x10)!=0);
			vett_dts[14] = ((byte1 & 0x08)!=0);
			vett_dts[15] = ((byte1 & 0x04)!=0);
			vett_dts[16] = ((byte1 & 0x02)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_dts[17] = ((byte1 & 0x80)!=0);
			vett_dts[18] = ((byte1 & 0x40)!=0);
			vett_dts[19] = ((byte1 & 0x20)!=0);
			vett_dts[20] = ((byte1 & 0x10)!=0);
			vett_dts[21] = ((byte1 & 0x08)!=0);
			vett_dts[22] = ((byte1 & 0x04)!=0);
			vett_dts[23] = ((byte1 & 0x02)!=0);
			vett_dts[24] = ((byte1 & 0x01)!=0);
			read(fin,&(buf[*buf_size]),1);
			*buf_size+=1;
			count+=1;
			byte1 = buf[*buf_size-1];
			vett_dts[25] = ((byte1 & 0x80)!=0);
			vett_dts[26] = ((byte1 & 0x40)!=0);
			vett_dts[27] = ((byte1 & 0x20)!=0);
			vett_dts[28] = ((byte1 & 0x10)!=0);
			vett_dts[29] = ((byte1 & 0x08)!=0);
			vett_dts[30] = ((byte1 & 0x04)!=0);
			vett_dts[31] = ((byte1 & 0x02)!=0);
       		}
     	}
	if (!*time_set && pts_present && (*final_byte != 0xc0)) {
		for (i=0;i<32;i++) {
			pts->pts = (pts->pts)*2 + vett_pts[i];
		}
		*time_set = 1;
	}

	if (!*time_set && (*final_byte == 0xc0)) {
		for (i=0;i<32;i++) {
			pts_audio->pts = (pts_audio->pts)*2 + vett_pts[i];
		}
		*time_set = 1;
	}

	if (*dts_present) {
		for (i=0;i<32;i++) {
			dts->pts = (dts->pts)*2 + vett_dts[i];
		}
	}
	return count;
}

int read_packet(uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte) {     /* reads a packet */
	int count=0;                                                                              /* at the end count is the size of the packet */
	unsigned char byte1,byte2;
	do {
		count+=next_start_code(buf,buf_size,fin);
		count+=4;
		read(fin,&(buf[*buf_size]),1);
		*buf_size+=1;
		*final_byte=buf[*buf_size-1];
		if (*final_byte == 0x00)	{
			read(fin,&(buf[*buf_size]),2);
			*buf_size+=2;
			count+=2;
			byte1 = buf[*buf_size-1];
			byte2 = (byte1 >> 3) & 0x07;
			if (byte2 == 0x01) {
				printf ("Pictute type: I\n");
			} else if (byte2 == 0x02) {
				printf ("Pictute type: P\n");
			} else {
				printf ("Pictute type: B\n");
			}
		}
		printf("Final Byte: %x\n", *final_byte);
	} while ( *final_byte < 0xb9);
	*buf_size-=4;
	lseek(fin,-4,SEEK_CUR);
	return count;
}

int changePacketLength(float offset, media_entry *me){
	(me->description).pkt_len = offset;
	return 1;
}


