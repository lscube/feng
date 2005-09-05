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


#include <fenice/MediaParser.h>

int get_field( uint8 *d, uint32 bits, uint32 *offset )
{
        uint32 v = 0;
        uint32 i;

        for( i = 0; i < bits; )
        {
                if( bits - i >= 8 && *offset % 8 == 0 )
                {
                        v <<= 8;
                        v |= d[*offset/8];
                        i += 8;
                        *offset += 8;
                } else
                {
                        v <<= 1;
                        v |= ( d[*offset/8] >> ( 7 - *offset % 8 ) ) & 1;
                        ++i;
                        ++(*offset);
                }
        }
        return v;
}

int next_start_code(uint8 *buf, uint32 *buf_size,int fin) {
        char buf_aux[3];
        int i;
        unsigned int count=0;
        if ( read(fin,&buf_aux,3) <3) {                                                 /* If there aren't 3 more bytes we are at EOF */
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

