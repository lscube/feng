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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fenice/debug.h>

#if DEBUG
#include <stdio.h>
#endif

#include <fenice/mediainfo.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>

int mediaopen(media_entry *me)
{
	char thefile[256];
	struct stat fdstat;
	int oflag = O_RDONLY;

	if (!(me->flags & ME_FILENAME))
		return ERR_INPUT_PARAM;
	
	strcpy(thefile,prefs_get_serv_root());
	strcat(thefile,me->filename);

#if 1
	if ( me->description.msource == live ) {
		// fstat(me->fd, &fdstat);
		stat(thefile, &fdstat);
		if ( !S_ISFIFO(fdstat.st_mode) ) {
			// lseek(me->fd, 0, SEEK_END);
			// fcntl(me->fd, F_SETFD, O_NONBLOCK);
			oflag |= O_NONBLOCK;
		}
	}
#endif

	// me->fd=open(thefile, O_RDONLY /*| O_NONBLOCK , 0*/);
	me->fd=open(thefile, oflag);

#if DEBUG
	printf("%s - %d\n", thefile, me->fd);
#endif

	if (me->fd==-1)
		return ERR_NOT_FOUND;

	me->flags|=ME_FD;

	return me->fd;
}

