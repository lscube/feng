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

#ifndef _RTPH
#define _RTPH

        #define NO_MULTICAST 0  //FEDERICO
        #define YES_MULTICAST 1 //FEDERICO
                	
		
	#include <time.h>
	#include <sys/timeb.h>
	#include <sys/socket.h>
	#include <fenice/socket.h>
	#include <fenice/mediainfo.h>
	#include <fenice/prefs.h>
	
	//#define MAX_SESSION ((int)(MAX_CONNECTION/MAX_PROCESS))
	#define MAX_SESSION 100

	#define RTP_DEFAULT_PORT 5004
	#define RTCP_BUFFERSIZE	1024
	
	typedef enum  {
		i_server=0,
		i_client=1
	} rtcp_index;

	typedef struct {
		int RTP;
		int RTCP;
	} port_pair;
	
	typedef struct _RTCP_stats {
		unsigned int RR_received;
		unsigned int SR_received;
		unsigned long dest_SSRC;
		unsigned int pkt_count;
		unsigned int octet_count;
		int pkt_lost;
		unsigned char fract_lost;
		unsigned int highest_seq_no;
		unsigned int jitter;
		unsigned int last_SR;
		unsigned int delay_since_last_SR;
	} RTCP_stats;

	typedef struct _RTP_session {
		tsocket rtp_fd;
		tsocket rtcp_fd_in,rtcp_fd_out;
		struct sockaddr rtcp_in_peer,rtcp_out_peer;
		unsigned char rtcp_inbuffer[RTCP_BUFFERSIZE];
		int rtcp_insize;
		unsigned char rtcp_outbuffer[RTCP_BUFFERSIZE];
		int rtcp_outsize;
		struct sockaddr rtp_peer;
	
		double mtime;
		double mstart;
		double mstart_offset;
		double mprev_tx_time;
	
		unsigned int PreviousCount;
		short MinimumReached;
		short MaximumReached;
		// Back references
		int sched_id;
		unsigned int start_seq;
		
		unsigned int start_rtptime;    	
		
		unsigned char pause;
		unsigned char started;
		unsigned int seq;
		unsigned int ssrc;
		port_pair ser_ports;
		port_pair cli_ports;
		char sd_filename[255];
		media_entry *current_media;
		RTCP_stats rtcp_stats[2]; //client e server
		struct _RTP_session *next;
		int isMulticast;  //FEDERICO
	} RTP_session;

	typedef struct _RTP_header {
		/* byte 0 */
    		#if (BYTE_ORDER == LITTLE_ENDIAN)
        		unsigned char csrc_len:4;		/* expect 0 */
        		unsigned char extension:1;		/* expect 1, see RTP_OP below */
            		unsigned char padding:1;		/* expect 0 */
            		unsigned char version:2;		/* expect 2 */
        	#elif (BYTE_ORDER == BIG_ENDIAN)
            		unsigned char version:2;
            		unsigned char padding:1;
            		unsigned char extension:1;
            		unsigned char csrc_len:4;
        	#else
    	    	#error Neither big nor little
        	#endif
        	/* byte 1 */
        	#if (BYTE_ORDER == LITTLE_ENDIAN)
    	    	unsigned char payload:7;		/* RTP_PAYLOAD_RTSP */
    	    	unsigned char marker:1;			/* expect 1 */
        	#elif (BYTE_ORDER == BIG_ENDIAN)
    	    	unsigned char marker:1;
    	    	unsigned char payload:7;
        	#endif
        	/* bytes 2, 3 */
        	unsigned short seq_no;
        	/* bytes 4-7 */
        	unsigned int timestamp;
        	/* bytes 8-11 */
        	unsigned int ssrc;					/* stream number is used here. */
	} RTP_header;
	
	typedef int (*RTP_play_action)(RTP_session *sess);
        			
	void RTP_port_pool_init(int port);
	int RTP_get_port_pair(port_pair *pair);
	int RTP_release_port_pair(port_pair *pair);

	int RTP_send_packet(RTP_session *session);

#endif
