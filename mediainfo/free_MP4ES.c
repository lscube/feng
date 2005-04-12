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

#include <stdlib.h>/*free*/

#include <fenice/mpeg4es.h>
#include <fenice/utils.h>

int free_MP4ES (void *stat){
	static_MPEG4_video_es *s;	

	if(stat==NULL)
		return ERR_ALLOC;
	s=(static_MPEG4_video_es *) stat;

	free(s->more_data);
	s->more_data=NULL;
	free(s->ref1);
	s->ref1=NULL;
	free(s->ref2);
	s->ref2=NULL;
	s->fragmented=0;
	s->final_byte=0x00;
	s->data_read=0;
	s->remained_data_size=0;
	s->vop_coding_type=0;
	s->vtir_bitlen=0;
	
	return ERR_NOERROR;
}
