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
/*#include <stdlib.h>*//*free*/
#include <fenice/mediainfo.h>

#define CLOSE_PIPE

int mediaclose(media_entry *me)
{
	int ret=0;
#ifndef CLOSE_PIPE
	struct stat fdstat;
	// close file if it's not a pipe
	fstat(me->fd, &fdstat);
	if ( !S_ISFIFO(fdstat.st_mode) ){
#endif
		ret = close(me->fd);
		me->fd = -1;
		me->flags&=~ME_FD;
		me->buff_size=0;
		me->media_handler->free_media((void*) me->stat);	
		me->stat=NULL;
#ifndef CLOSE_PIPE
	}
#endif

	// me->media_handler->free_media((void*) me->stat);	
	/*do not release the media handler, because load_X is recalled only if .sd change*/
	/*free(me->media_handler);*/

	return ret;
}

