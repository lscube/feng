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

#ifndef _SERV_PREFSH	
#define _SERV_PREFSH

#include <config.h>

	/* Please note (1): PROCESS_COUNT must be >=1
	*/
	/* Please note (2):
		MAX_CONNECTION must be an integral multiple of MAX_PROCESS
	*/
	#define MAX_PROCESS		10
	#define MAX_CONNECTION	100

	
	#define DEFAULT_CONF_FILE FENICE_CONF_DIR_DEFAULT "/fenice.conf"
	
	#define PREFS_ROOT "root"
	#define PREFS_PORT "port"

	#define DEFAULT_ROOT FENICE_AVROOT_DIR_DEFAULT
	#define DEFAULT_PORT 1554
	#define DEFAULT_MULTICAST_FILE ""
	
	typedef struct _serv_prefs {
		char hostname[256];
		char serv_root[256];
		unsigned int port;			
	} serv_prefs;

	void prefs_init(char *fileconf);
	char *prefs_get_serv_root();
	char *prefs_get_hostname();
	int prefs_get_port();		
	void prefs_use_default(int index);
		
#endif
