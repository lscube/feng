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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <fenice/socket.h>
#include <fenice/multicast.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>
#include <fenice/rtsp.h>

  extern serv_prefs prefs;
  
  //main function
int add_multicast_stream(char source[20]){
  char Ip[256];
  char port[6];
  char buffer[RTSP_BUFFERSIZE];
  RTSP_buffer *rtsp;
  tsocket fd;
  unsigned short fd_port=1554;

  	//create an fd socket (only for scheduling)
  	fd=tcp_connect(fd_port,get_address());
	rtsp->fd=fd;
	//describe
	strcpy(buffer,RTSP_METHOD_DESCRIBE);
	strcat (buffer," ");
	strcat(buffer,"rtsp://");
	strcat(buffer,get_address());
	strcat(buffer,":");
	sprintf(port,"%d",prefs_get_port());
	strcat(buffer,port);
	strcat(buffer,"/");
	strcat(buffer,source);
	strcat(buffer," ");
	strcat(buffer,RTSP_VER);
	strcat(buffer,"\nCSeq: 1\nAccept: application/sdp\r\n\r\n");
	
	strcpy(rtsp->in_buffer,buffer);
	rtsp->in_size=strlen(buffer);
	rtsp->rtsp_cseq=1;

	//RTSP_describe(&rtsp);

	//setup: rtp session (so port, address an so on)
          
  return ERR_NOERROR;
}
