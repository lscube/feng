/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2007 by
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

/*
 * header file for MPEG1-2 video elementary stream
 */

#ifndef _MPEGH
#define _MPEGH

#include <stdint.h>

typedef struct {		/* MPEG video specific headers */
	unsigned int ffc:3;
	unsigned int ffv:1;
	unsigned int bfc:3;	/* |MBZ:5|T:1|TR:10|AN:1|N:1|S:1|B:1|E:1|P:3|FBV:1|BFC:3|FFV:1|FFC:3| */
	unsigned int fbv:1;
	unsigned int p:3;
	unsigned int e:1;
	unsigned int b:1;
	unsigned int s:1;
	unsigned int n:1;
	unsigned int an:1;
	unsigned int tr:10;
	unsigned int t:1;
	unsigned int mbz:5;
} video_spec_head1;

typedef struct {
	unsigned int d:1;	/* |X:1|E:1|F[0,0]:4|F[0,1]:4|F[1,0]:4|F[1,1]:4|DC:2|PS:2|T:1|P:1|C:1|Q:1|V:1|A:1|R:1|H:1|G:1|D:1| */
	unsigned int g:1;
	unsigned int h:1;
	unsigned int r:1;
	unsigned int a:1;
	unsigned int v:1;
	unsigned int q:1;
	unsigned int c:1;
	unsigned int p:1;
	unsigned int t:1;
	unsigned int ps:2;
	unsigned int dc:2;
	unsigned int f11:4;
	unsigned int f10:4;
	unsigned int f01:4;
	unsigned int f00:4;
	unsigned int e:1;
	unsigned int x:1;
} video_spec_head2;

typedef enum { MPEG_1, MPEG_2, TO_PROBE } standard;

/*--------------------------------------------------*/
/*	STARTING FROM HERE: NOT USED BY MT	    */
/*--------------------------------------------------*/
typedef struct {
	video_spec_head1 vsh1;
	video_spec_head2 vsh2;
	unsigned char final_byte;
	char temp_ref;
	char hours;
	char minutes;
	char seconds;
	char picture;
	unsigned long data_total;
	standard std;
	int fragmented;
	video_spec_head1 vsh1_aux;	/* without modifying other variables */
	video_spec_head2 vsh2_aux;
} static_MPEG_video;

	/* reads GOP header */
int read_gop_head(uint8_t *, uint32_t *, int fin, unsigned char *final_byte,
		  char *hours, char *minutes, char *seconds, char *picture,
		  standard std);
	/* reads picture head */
int read_picture_head(uint8_t *, uint32_t *, int fin, unsigned char *final_byte,
		      char *temp_ref, video_spec_head1 * vsh1, standard std);
	/* reads a slice */
int read_slice(uint8_t *, uint32_t *, int fin, char *final_byte);
	/* reads picture coding extension */
int read_picture_coding_ext(uint8_t *, uint32_t *, int fin,
			    unsigned char *final_byte, video_spec_head2 * vsh2);

#endif
