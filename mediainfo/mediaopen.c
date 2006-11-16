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
#include <stdio.h>

#include <fenice/mediainfo.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>

int mediaopen(media_entry * me)
{
	char thefile[256];
	struct stat filestat;
	int oflag = O_RDONLY;

	if (!(me->flags & ME_FILENAME))
		return ERR_INPUT_PARAM;

	snprintf(thefile, sizeof(thefile) - 1, "%s%s%s", prefs_get_serv_root(),
		 (prefs_get_serv_root()[strlen(prefs_get_serv_root()) - 1] ==
		  '/') ? "" : "/", me->filename);

	fnc_log(FNC_LOG_DEBUG, "opening file %s...\n", thefile);

	if (me->description.msource == live) {
		fnc_log(FNC_LOG_DEBUG, " Live stream... ");
		stat(thefile, &filestat);
		if (S_ISFIFO(filestat.st_mode)) {
			fnc_log(FNC_LOG_DEBUG, " IS_FIFO... ");
			oflag |= O_NONBLOCK;
		}
	}

	me->fd = open(thefile, oflag);

#if 0
	if ((me->description.msource == live) && (!S_ISFIFO(filestat.st_mode))) {
		fprintf(stderr, "illusion of live... ");
		lseek(me->fd, 4, SEEK_END);
	}
#endif

	if (me->fd == -1) {
		fnc_log(FNC_LOG_ERR, "Could not open %s", thefile);
		return ERR_NOT_FOUND;
	}

	me->flags |= ME_FD;

	return me->fd;
}
