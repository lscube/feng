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


#include <fenice/utils.h>
#include <fenice/InputStream.h>

InputStream * create_inputstream(stream_type type, int fd)
{
	InputStream *is;
	is=(InputStream *)malloc(sizeof(InputStream));
	if(is==NULL){
		fnc_log(FNC_LOG_ERR,"FATAL!!! It is impossible to allocate InputStream\n");
		return is;/*NULL*/	
	}
	is->cache=NULL;
	is->type=type;
	is->fd=fd;
	return is;	
}

int read_stream(uint32 nbytes, uint8 * buf, InputStream *is)
{
	if(is==NULL){
		fnc_log(FNC_LOG_ERR,"FATAL!!! InputStream is NULL\n");
		return ERR_FATAL;
	}
	return read_c(nbytes, buf, is->cache, is->fd, is->type); 
}
