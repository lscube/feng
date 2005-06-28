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
#include <sys/types.h>
#include <unistd.h>

#include <fenice/socket.h>
#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/rtsp.h>
#include <fenice/schedule.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

int num_conn = 0;

void eventloop(tsocket main_fd)
{
	static uint32 child_count=0;
	static int conn_count = 0;
	tsocket fd = -1;
	static RTSP_buffer *rtsp_list=NULL;
	RTSP_buffer *p=NULL;
	uint32 fd_found;

	if (conn_count!=-1)
	{
		fd=tcp_accept(main_fd);
	} // shawill: and... if not?

	// Handle a new connection
	if (fd>=0)
	{
		for (fd_found=0,p=rtsp_list; p!=NULL; p=p->next)
			if (p->fd==fd)
			{				
				fd_found=1;
				break;
			}
		if (!fd_found)
		{
        		if (conn_count<ONE_FORK_MAX_CONNECTION)
			{
        			++conn_count;
        			// ADD A CLIENT
        			add_client(&rtsp_list,fd);
        		}
        		else
			{
				if (fork()==0)
				{
        				// I'm the child
        				++child_count;
        				RTP_port_pool_init(ONE_FORK_MAX_CONNECTION*child_count*2+RTP_DEFAULT_PORT);
					if (schedule_init()==ERR_FATAL)
					{
                  				fnc_log(FNC_LOG_FATAL,"Can't start scheduler. Server is aborting.\n");
                  				return;
                  			}        			
        				conn_count=1;
        				rtsp_list=NULL;
        				add_client(&rtsp_list,fd);
        			}
        			else
				{
        				// I'm the father
        				fd=-1;
        				conn_count=-1;
        				tcp_close(main_fd);				
        			}					
        		}
			num_conn++;	
			fnc_log(FNC_LOG_INFO,"Connection arriched: %d\n",num_conn);
        	}
    	}
	schedule_connections(&rtsp_list,&conn_count);
}
