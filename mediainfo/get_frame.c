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

#include <string.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>

int get_frame(media_entry *me,unsigned char **data,unsigned int *data_size,double *mtime, int *recallme)
{
        if (strcmp(me->description.encoding_name,"MPA")==0) {
                *recallme = 0;
                me->description.delta_mtime=me->description.pkt_len;                 /* For MPA, L16 and GSM delta_mtime is fixed to pkt_len */
                return read_MP3(me,data,data_size,mtime);
        }
        if (strcmp(me->description.encoding_name,"L16")==0) {
                *recallme = 0;
                me->description.delta_mtime=me->description.pkt_len;
                return read_PCM(me,data,data_size,mtime);
        }
        if (strcmp(me->description.encoding_name,"GSM")==0) {
                *recallme = 0;
                me->description.delta_mtime=me->description.pkt_len;
                return read_GSM(me,data,data_size,mtime);
        }
        if (strcmp(me->description.encoding_name,"H26L")==0) {
                return read_H26L(me,data,data_size,mtime,recallme);
        }
        if (strcmp(me->description.encoding_name,"MPV")==0) {
                return read_MPEG_video(me,data,data_size,mtime,recallme);
        }
        if (strcmp(me->description.encoding_name,"MP2T")==0) {
                return read_MPEG_system(me,data,data_size,mtime,recallme);
        }
        return ERR_UNSUPPORTED_PT;
}

