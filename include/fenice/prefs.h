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
	#define MAX_PROCESS	1/*number of fork*/	
	#define MAX_CONNECTION	100/*rtsp connection*/	
	#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS))/*rtsp connection for one fork*/

/* Moved to config.h. Now in FENICE_MAX_SESSION_DEFAULT
 * Was:
 * 
 * default max session after which i need to send a redirect
 * #define DEFAULT_MAX_SESSION 100
 *
*/

	#define PREFS_ROOT "root"
	#define PREFS_PORT "port"
	#define PREFS_MAX_SESSION "max_session"
	#define PREFS_LOG "log_file"

	typedef struct _serv_prefs {
		char hostname[256];
		char serv_root[256];
		char log[256];
		unsigned int port;			
		unsigned int max_session;			
	} serv_prefs;

	void prefs_init(char *fileconf);
	char *prefs_get_serv_root();
	char *prefs_get_hostname();
	int prefs_get_port();		
	int prefs_get_max_session();
	char *prefs_get_log();
	void prefs_use_default(int index);
		
#endif
