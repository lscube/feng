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
#include <fenice/multicast.h>
#include <fenice/utils.h>
#include <fenice/rtp.h>
#include <fenice/schedule.h>

int add_multicast_stream(char source[20]){
  media_entry media, req;
  char descr[MAX_DESCR_LENGTH];
  int res;      

  //FEDERICO NOTES: 
  //parse source (is an .sd)
  //for all media set play_args set rtp_session schedule_add schedule_start
  //schedule_add return the id of the rtp session that i have to pass to schedule_start
  //in rtp_session i have to set is_multicast=yes
  //when a new request arrived i have to see if it is for a multicast session
  //and manage in a different manner unicast and multicast 
 	
	//describe: parse sd and etc.. etc.. 
	memset(&media, 0, sizeof(media));
        memset(&req, 0, sizeof(req));
        req.flags = ME_DESCR_FORMAT;
        req.descr_format = df_SDP_format;
        res = get_media_descr(source, &req, &media, descr);
        if (res == ERR_NOT_FOUND) {
                printf("\nMulticast request specified an object which can't be found.\n");
		return ERR_NOT_FOUND;
        }
	printf("\n%s\n",descr);
	//setup: rtp session (so port, address an so on)
          
  return ERR_NOERROR;
}
