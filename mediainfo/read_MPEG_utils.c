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
#include <fenice/mpeg.h>
/*#include <fenice/prefs.h>*/
/*#if HAVE_ALLOCA_H
#include <alloca.h>
#endif*/

int read_seq_head(media_entry *me, uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte, standard std) {  /* reads sequence header */
        uint8 tmp;
	uint32 x=0, bitcnt=0, n = 18,  picture_rate, bufptr;
	
	read(fin,&(buf[*buf_size]),4);                       			
        *buf_size+=4;
	/*try to calculate  frame rate*/
	tmp=buf[*buf_size-1];
	picture_rate=	tmp & 0x0F;
	/*end calculate frame rate*/
	
	/*try to calculate bit_rate*/
        read(fin,&(buf[*buf_size]),4);                       			
        *buf_size+=4;
	bufptr=*buf_size-4;
	n=18;
	bitcnt=0;
	x=0;
	while(n-->0){
		if(!bitcnt){
			tmp=buf[bufptr++];
			bitcnt=8;
		}
		x=(x<<1)|(tmp>>7);tmp<<=1;
		--bitcnt;
	}
	 /*0x3FFFFF means variable bit rate*/
 	me->description.bitrate = (x!=0x3FFFF)?(x * 400):0;
	//fprintf(stderr,"bit_rate b/s: %d frame rate: %d (see the reference table pg143)\n", me->description.bitrate, picture_rate);
	/*end calculate bit_rate*/
	
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

int read_gop_head(uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte, char *hours, char *minutes, char *seconds, char *picture, standard std) {  /* reads GOP header */
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

int read_picture_head(uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte, char *temp_ref, video_spec_head1* vsh1, standard std) { /* reads picture head */
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

int probe_standard(media_entry *me,uint8 *buf,uint32 *buf_size,int fin, standard *std)
{											/* If the sequence_extension occurs immediately */
        unsigned char final_byte;							/* after the sequence header, the sequence is an */
        next_start_code(buf,buf_size,fin);						/* MPEG-2 video sequence */
        read(fin,&(buf[*buf_size]),1);
        *buf_size+=1;
        final_byte=buf[*buf_size-1];
        if (final_byte == 0xb3) {
                read_seq_head(me, buf, buf_size, fin, &final_byte, *std);
        }
        if (final_byte  == 0xb5){
                *std=MPEG_2;
        } else {
                *std=MPEG_1;
        }
        return 1;
}

int read_picture_coding_ext(uint8 *buf,uint32 *buf_size, int fin, unsigned char *final_byte,video_spec_head2* vsh2) {  /* reads picture coding extension */
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

