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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <config.h>
#ifdef WIN32
#include <winsock2.h>
#endif
#include <fenice/socket.h>
#include <fenice/eventloop.h>
#include <fenice/prefs.h>
#include <fenice/schedule.h>
#include <fenice/utils.h>
#include <fenice/command_environment.h>
#include <fenice/fnc_log.h>
//#include <sys/types.h> /*fork*/
//#include <unistd.h>    /*fork*/

inline void fncheader(void); // defined in src/fncheader.c

int main(int argc, char **argv)
{
	tsocket main_fd;
	unsigned int port;

	// Fake timespec for fake nanosleep. See below.
	struct timespec ts = { 0, 0 };

	// printf("\n%s %s - Open Media Streaming Project - Politecnico di Torino\n\n", PACKAGE, VERSION);
	fncheader();

	/*command_environment parses the command line and returns the number of error */
	if (command_environment(argc, argv))
		return 0;
	/* prefs_get_port() reads the static var prefs and returns the port number */
	port = prefs_get_port();

#ifdef WIN32
	{
		int err;
		unsigned short wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(1, 1);
		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) {
			fnc_log(FNC_LOG_ERR,"Could not detect Windows socket support.\n");
			fnc_log(FNC_LOG_ERR,"Make sure WINSOCK.DLL is installed.\n");
			return 1;
		}
	}
#endif

	printf("CTRL-C terminate the server.\n");
	fnc_log(FNC_LOG_INFO,"Waiting for RTSP connections on port %d...\n", port);
	main_fd = tcp_listen(port);

	/* next line: schedule_init() initialises the array of schedule_list sched 
	   and creates the thread schedule_do() -> look at schedule.c */
	if (schedule_init() == ERR_FATAL) {
		fnc_log(FNC_LOG_ERR_FATAL,"Fatal: Can't start scheduler. Server is aborting.\n");
		return 0;
	}
	RTP_port_pool_init(RTP_DEFAULT_PORT);
	/* puts in the global variable port_pool[MAX_SESSION] all the RTP usable ports
	   from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

	while (1) {
		// Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
		// See also schedule.c
		nanosleep(&ts, NULL);
		eventloop(main_fd);
	}
	/* eventloop looks for incoming RTSP connections and generates for each
	   all the information in the structures RTSP_list, RTP_list, and so on */

#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}
