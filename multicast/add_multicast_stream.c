/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Wed Apr 28 10:53:08 CEST 2004
    copyright            : (C) 2004 by Federico Ridolfo
    email                : federico.ridolfo@poste.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
