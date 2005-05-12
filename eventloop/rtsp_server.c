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
#include <string.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>

int rtsp_server(RTSP_buffer *rtsp)
{	
	fd_set rset,wset;  // read set and write set - set of i/o file descriptor
	struct timeval t;
	int size;
	char buffer[RTSP_BUFFERSIZE+1]; /* +1 to control the final '\0'*/
	int n;
	int res;
	RTSP_session *q;
	RTP_session *p;

	if (rtsp==NULL) {
		return ERR_NOERROR;
	}
	
	// RTSP SCHEDULING
	FD_ZERO(&rset);
 	FD_ZERO(&wset);
	t.tv_sec=0;
	t.tv_usec=100000;
	FD_SET(rtsp->fd,&rset);  // fd is tcp connection socket and FD_SET adds this one to rset
	if (rtsp->out_size>0) {
		FD_SET(rtsp->fd,&wset);  // idem for wset
	}
	if (select(MAX_FDS,&rset,&wset,0,&t)<0) {
		fnc_log(FNC_LOG_ERR,"select error\n");			
		send_reply(500, NULL, rtsp);
		return ERR_GENERIC; //errore interno al server
	}
	if (FD_ISSET(rtsp->fd,&rset)) {		
		// There are RTSP packets to read in
		memset(buffer,0,sizeof(buffer));
		size=sizeof(buffer)-1;
		n=tcp_read(rtsp->fd,buffer,size);			
		if (n==0) {
			return ERR_CONNECTION_CLOSE;
		}
		
		if (n<0) {
			fnc_log(FNC_LOG_DEBUG,"read() error in rtsp_server()\n");			
			send_reply(500, NULL, rtsp);
			return ERR_GENERIC;//errore interno al server    			
		}			
		
		if (rtsp->in_size+n>RTSP_BUFFERSIZE) {
			fnc_log(FNC_LOG_DEBUG,"RTSP buffer overflow (input RTSP message is most likely invalid).\n");
			send_reply(500, NULL, rtsp);
			return ERR_GENERIC;//errore da comunicare
		}
		
		fnc_log(FNC_LOG_VERBOSE,"INPUT_BUFFER was:\n");				
#ifdef VERBOSE
		dump_buffer(buffer);
#endif
		
		memcpy(&(rtsp->in_buffer[rtsp->in_size]),buffer,n);
		rtsp->in_size+=n;
		if ((res=RTSP_handler(rtsp))==ERR_GENERIC) {				
			fnc_log(FNC_LOG_ERR,"Invalid input message.\n");
			return ERR_NOERROR;
		}
	}	
	if (FD_ISSET(rtsp->fd,&wset)) {						
		// There are RTSP packets to send
		if (rtsp->out_size>0) {
			n=tcp_write(rtsp->fd,rtsp->out_buffer,rtsp->out_size);
			if (n<0) {
				fnc_log(FNC_LOG_ERR,"tcp_write() error in rtsp_server()\n");        			
				send_reply(500, NULL, rtsp);
       				return ERR_GENERIC; //errore interno al server
			}				
			rtsp->out_size-=n;
			fnc_log(FNC_LOG_VERBOSE,"OUTPUT_BUFFER was:\n");
#ifdef VERBOSE
			dump_buffer(rtsp->out_buffer);				
#endif
		}
	}
	#ifdef POLLED
	schedule_do(0);
	#endif
	// RTCP	scheduling
	if ( (q=rtsp->session_list) == NULL )
		return ERR_NOERROR;
	
	for (p=q->rtp_session; p!=NULL; p=p->next) {
		
		if (!p->started) {
			
			q->cur_state = READY_STATE; // é finita la riproduzione, passo in stato ready
			/*free della struttura rtp TODO*/	
		
		} else {
			
			FD_ZERO(&rset);
		 	FD_ZERO(&wset);
        		t.tv_sec=0;
        		t.tv_usec=100000;
			FD_SET(p->rtcp_fd_in,&rset);
    			if (p->rtcp_outsize>0) {
    				FD_SET(p->rtcp_fd_out,&wset);
	    		}
    			if (select(MAX_FDS,&rset,&wset,0,&t)<0) {		
    				fnc_log(FNC_LOG_ERR,"select error\n");
				send_reply(500, NULL, rtsp);
	    			return ERR_GENERIC; //errore interno al server
    			}
    			if (FD_ISSET(p->rtcp_fd_in,&rset)) {        	
	    			// There are RTCP packets to read in
    				int peer_len=sizeof(p->rtcp_in_peer);
        			if ((p->rtcp_insize=recvfrom(p->rtcp_fd_in,p->rtcp_inbuffer,sizeof(p->rtcp_inbuffer),0,&(p->rtcp_in_peer),&peer_len))<0) {            	
        				fnc_log(FNC_LOG_VERBOSE,"Input RTCP packet Lost\n");
        			}
        			else {
            				RTCP_recv_packet(p);
	            		}
        			fnc_log(FNC_LOG_VERBOSE,"IN RTCP\n");
			}
	    		if (FD_ISSET(p->rtcp_fd_out,&wset)) {
    				// There are RTCP packets to send
        			if (sendto(p->rtcp_fd_out,p->rtcp_outbuffer,p->rtcp_outsize,0,&(p->rtcp_out_peer),sizeof(p->rtcp_out_peer))<0) {
        				fnc_log(FNC_LOG_VERBOSE,"RTCP Packet Lost\n");
        			}    		
        			p->rtcp_outsize=0;
	        		fnc_log(FNC_LOG_VERBOSE,"OUT RTCP\n");         	
			}
		}	
	}
	return ERR_NOERROR;
}
