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

#include <fenice/socket.h>
#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/rtsp.h>
#include <fenice/schedule.h>
#include <fenice/command_environment.h>

void eventloop(tsocket main_fd, tsocket command_fd)
{
	static int child_count=0;
	static int conn_count=0;	
	tsocket fd;
	tsocket fd_command; /*socket for the command environment*/
	static RTSP_buffer *rtsp_list=NULL;
	RTSP_buffer *p;
	int fd_found;
	if (conn_count!=-1)
	{
		fd=tcp_accept(main_fd);
		//fd_command=tcp_accept(command_fd);
	}

	//Handle command environment
	//if(fd_command>=0)
	//	init_command(fd_command);
	
	
	// Handle a new connection
	if (fd>=0)
	{
		for (fd_found=0,p=rtsp_list; p!=NULL; p=p->next)
		{
			
			if (p->fd==fd)
			{				
				fd_found=1;
				break;
			}
		}
		if (!fd_found)
		{
        		if (conn_count<MAX_SESSION)
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
        				RTP_port_pool_init(MAX_SESSION*child_count*2+RTP_DEFAULT_PORT);
					if (schedule_init()==ERR_FATAL)
					{
                  				printf("Fatal: Can't start scheduler. Server is aborting.\n");
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
        	}
    	}
	schedule_connections(&rtsp_list,&conn_count);
}
