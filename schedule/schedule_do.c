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
// #include <time.h>
#include <sys/time.h>

#include <pthread.h>
#include <fenice/schedule.h>
#include <fenice/intnet.h>
#include <fenice/rtcp.h>
#include <fenice/utils.h>

extern schedule_list sched[MAX_SESSION];
extern int stop_schedule;

#ifdef SELECTED	
void *schedule_do(void *nothing)
#endif
#ifdef THREADED
void *schedule_do(void *nothing)
#endif
#ifdef POLLED
void schedule_do(int sig)
#endif
#ifdef SIGNALED
void schedule_do(int sig)
#endif
{
	int i,res;
	struct timeval now;
	double mnow;
	// Fake timespec for fake nanosleep. See below.
	struct timespec ts = {0,0};
#ifdef SELECTED
	struct timeval t;
	fd_set rs;
#endif	
#ifdef THREADED
do {
#endif	
#ifdef SELECTED	
do{
	t.tv_sec=0;
	t.tv_usec=26122;
	FD_ZERO(&rs);
	if (select(0,&rs,&rs,&rs,&t)<0) {
		printf("failed!\n");
	}
#endif
	// Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
	// See also main.c
	nanosleep(&ts, NULL);
	
	for (i=0; i<MAX_SESSION; ++i) {		
		
		if (sched[i].valid) {
			sched[i].semaph=red; /* green = 0, red = 1 */
			
			if (!sched[i].rtp_session->pause) {
				
				gettimeofday(&now,NULL);				
				
				mnow=(double)now.tv_sec*1000+(double)now.tv_usec/1000;
				
				if (mnow >= sched[i].rtp_session->current_media->mstart) {
					
					if (mnow - sched[i].rtp_session->mprev_tx_time >= sched[i].rtp_session->current_media->description.pkt_len) {
    				//	if (mnow-sched[i].rtp_session->mtime>=sched[i].rtp_session->current_media->description.pkt_len) {   // old scheduler
						
						stream_change(sched[i].rtp_session, change_check(sched[i].rtp_session));
        					
						/*This operation is in RTP_send_packet function because it runs only if producer writes the slot*/
						//sched[i].rtp_session->mtime += sched[i].rtp_session->current_media->description.delta_mtime; //emma  
						//sched[i].rtp_session->mtime+=sched[i].rtp_session->current_media->description.pkt_len;     // old scheduler
        					
						RTCP_handler(sched[i].rtp_session);
						
	        				pthread_mutex_lock(&((sched[i].rtp_session)->cons->mutex));	
						// Send an RTP packet
						res = sched[i].play_action(sched[i].rtp_session);
	        				pthread_mutex_unlock(&((sched[i].rtp_session)->cons->mutex));	
						if (res!=ERR_NOERROR) {
    							if (res==ERR_EOF) {    						
    								printf("Stream Finished\n");
    								schedule_stop(i);
    							}
    							else {    					
								#ifdef DEBUG		
								//	printf("Packet Lost\n");
								#endif
    							}
        						continue;
						}
						sched[i].rtp_session->mprev_tx_time += sched[i].rtp_session->current_media->description.pkt_len;   				    				
        				}
        			}
    			}
    			
				
			sched[i].semaph=green;
		}
	}
#ifdef THREADED
} while (!stop_schedule);
stop_schedule=0;
return ERR_NOERROR;
#endif
#ifdef SELECTED
} while (1);
#endif
}

