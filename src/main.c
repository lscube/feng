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
#include <sys/ioctl.h>
#include <sys/socket.h> /*SOMAXCONN*/

#include <netembryo/wsocket.h>

#include <fenice/eventloop.h>
#include <fenice/prefs.h>
#include <fenice/schedule.h>
#include <fenice/utils.h>
#include <fenice/command_environment.h>
#include <glib.h>
#include <glib/gprintf.h>

#include <fenice/stunserver.h>
#include <pthread.h>

inline void fncheader(void); // defined in src/fncheader.c

int main(int argc, char **argv)
{
	Sock *main_sock = NULL, *sctp_main_sock = NULL;
	char *port;

	// Fake timespec for fake nanosleep. See below.
	//struct timespec ts = { 0, 0 };

	// printf("\n%s %s - Open Media Streaming Project - Politecnico di Torino\n\n", PACKAGE, VERSION);
	fncheader();

	/*command_environment parses the command line and returns the number of error */
	if (command_environment(argc, argv))
		return 0;
	

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

	Sock_init(fnc_log);

#if ENABLE_STUN
	struct STUN_SERVER_IFCFG *cfg = (struct STUN_SERVER_IFCFG *)prefs_get_stuncfg();
	
	if( cfg!= NULL) {
		pthread_t thread;

		fnc_log(FNC_LOG_DEBUG, "Trying to start OMSstunserver thread\n");
		fnc_log(FNC_LOG_DEBUG, "stun parameters: %s,%s,%s,%s\n",cfg->a1,cfg->p1,cfg->a2,cfg->p2);
	
		pthread_create(&thread,NULL,OMSstunserverStart,(void *)(cfg));
	}
#endif //ENABLE_STUN
	
	/* prefs_get_port() reads the static var prefs and returns the port number */
	port = g_strdup_printf("%d", prefs_get_port());
	main_sock = Sock_bind(NULL, port, TCP, 0);
	g_free(port);

	if(!main_sock) {
		fnc_log(FNC_LOG_ERR,"Sock_bind() error for TCP.\n" );
		fprintf(stderr, "[fatal] Sock_bind() error in main() for TCP.\n" );
		return 0;
	}

	fprintf(stderr, "CTRL-C terminate the server.\n");
	fnc_log(FNC_LOG_INFO, "Waiting for RTSP connections on port %s...\n", port);
	
/*	if (Sock_set_props(main_sock, FIONBIO, &on) < 0) { //set to non-blocking
		fnc_log(FNC_LOG_ERR,"Sock_set_props() error.\n" );
		return 0;
    	}*/

	if(Sock_listen(main_sock, SOMAXCONN)) {
		fnc_log(FNC_LOG_ERR,"Sock_listen() error.\n" );
		return 0;
	}

#ifdef HAVE_SCTP_FENICE
	if (prefs_get_sctp_port() >= 0) {
		port = g_strdup_printf("%d", prefs_get_sctp_port());
		sctp_main_sock = Sock_bind(NULL, port, SCTP, 0);
		g_free(port);
		if(!sctp_main_sock) {
			fnc_log(FNC_LOG_ERR,"Sock_bind() error for SCTP.\n" );
			fprintf(stderr, "[fatal] Sock_bind() error in main() for TCP.\n" );
			return 0;
		}
		if(Sock_listen(sctp_main_sock, SOMAXCONN)) {
			fnc_log(FNC_LOG_ERR,"Sock_listen() error.\n" );
			return 0;
		}
	}
#endif

	/* next line: schedule_init() initialises the array of schedule_list sched 
	   and creates the thread schedule_do() -> look at schedule.c */
	if (schedule_init() == ERR_FATAL) {
		fnc_log(FNC_LOG_FATAL,"Fatal: Can't start scheduler. Server is aborting.\n");
		return 0;
	}
	RTP_port_pool_init(RTP_DEFAULT_PORT);
	/* puts in the global variable port_pool[MAX_SESSION] all the RTP usable ports
	   from RTP_DEFAULT_PORT = 5004 to 5004 + MAX_SESSION */

	while (1) {
		// Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
		// See also schedule.c
		//nanosleep(&ts, NULL);
		eventloop(main_sock, sctp_main_sock);
	}
	/* eventloop looks for incoming RTSP connections and generates for each
	   all the information in the structures RTSP_list, RTP_list, and so on */

#ifdef WIN32
	WSACleanup();
#endif

	return 0;
}
