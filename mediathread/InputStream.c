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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <fenice/utils.h>
#include <fenice/InputStream.h>
#include <fenice/fnc_log.h>

static int open_socket(char *host, char *port, int *fd)
{

	//...TODO
	return ERR_NOERROR;
}

static int open_file(char *filename, int *fd)
{
	int oflag = O_RDONLY;
	
	// TODO: handle pipe
	if(((*fd)=open(filename,oflag))==-1) {
		fnc_log(FNC_LOG_ERR,"It's impossible to open file\n");
		return ERR_FATAL;
	}
	return ERR_NOERROR;
}

static int open_device()
{

	//...TODO
	return ERR_NOERROR;
}

static int parse_mrl_st_file(char *mrl, int *fd)
{
	return open_file(mrl,fd);
}


static int parse_mrl_st_udp(char *urlname, int *fd)
{
	char *host;
	char *port;
	int res;
	char *colon;

        // initialization
        if ( (colon = strchr(urlname, ':')) ) {
		*colon = '\0';
                host = urlname;
                port = colon + 1;
        } else
		return ERR_PARSE;

	res = open_socket(host,port,fd);

	*colon = ':';

	return res;
}


static int parse_mrl_st_device(char *mrl, int *fd)
{
	//...TODO
	//
	return open_device();
}
	

/*Interface*/
InputStream *create_inputstream(char *mrl)
{
	InputStream *is;

	if ( !(is=(InputStream *)malloc(sizeof(InputStream))) ) {
		fnc_log(FNC_LOG_FATAL,"Could not allocate memory for InputStream\n");
		return NULL;
	}
	strcpy(is->name,mrl); /* ie. file://path/to/file */
	if(parse_mrl(mrl, &(is->type), &(is->fd))!=ERR_NOERROR) {
		fnc_log(FNC_LOG_ERR,"mrl not valid\n");
		free(is);
		return NULL;
	}
	is->cache=NULL;

	return is;	
}

inline int read_stream(uint32 nbytes, uint8 *buf, InputStream *is)
{
	return is ? read_c(nbytes, buf, is->cache, is->fd, is->type): ERR_ALLOC;
}

int parse_mrl(char *mrl, stream_type *type, int *fd)
{
	char *token;
	int res;
	char *colon;
	
        if ( !(colon = strstr(mrl, "://")) ) {
		*type=st_file;	
		return parse_mrl_st_file(mrl, fd);
	}
	*colon = '\0';
	token = colon + strlen("://");
	if(!strcmp(mrl, FNC_UDP)) { 
		*type=st_net;	
		res = parse_mrl_st_udp(token, fd);
	}
	/*TODO: tcp*/
	else if(!strcmp(token, FNC_FILE)) { 
		*type=st_file;	
		res = parse_mrl_st_file(token, fd);
	}
	else if(!strcmp(token, FNC_DEV)) {
		*type=st_device;	
		res = parse_mrl_st_device(token, fd);
	}
	else {
		fnc_log(FNC_LOG_ERR,"Invalid resource request \n");
		res = ERR_PARSE;
	}

	*colon = ':';

	return res;

}

