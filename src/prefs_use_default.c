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

#include <fenice/prefs.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

extern serv_prefs prefs;

void prefs_use_default(int index)
{
	switch (index) {
		case -1:
    			strcpy(prefs.serv_root,DEFAULT_ROOT);
	    		prefs.port=DEFAULT_PORT;
    			return;
    		case 0: 
    			strcpy(prefs.serv_root,DEFAULT_ROOT);
	    		return;
    		case 1: 
	    		prefs.port=DEFAULT_PORT;
    			return;
    		case 2: 
	    		prefs.max_session=DEFAULT_MAX_SESSION;
    			return;
		default:
			return;
	}
}

