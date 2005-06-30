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

#include <fenice/utils.h>
#include <fenice/InputStream.h>
#include <fenice/fnc_log.h>


static int parse_mrl_st_file(uint8 *mrl, int *fd)
{
	
	return ERR_NOERROR;
}


static int parse_mrl_st_net(uint8 *mrl, int *fd)
{
	return ERR_NOERROR;
}


static int parse_mrl_st_device(uint8 *mrl, int *fd)
{
	return ERR_NOERROR;
}
	

InputStream *create_inputstream(uint8 *mrl)
{
	InputStream *is;

	if ( !(is=(InputStream *)malloc(sizeof(InputStream))) ) {
		fnc_log(FNC_LOG_FATAL,"Could not allocate memory for InputStream\n");
		return NULL;
	}
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

int parse_mrl(uint8 *mrl, stream_type *type, int *fd)
{
	char *token;
	uint32 token_size;
	
	if ((token = (char *) malloc(sizeof(char) * (strlen(mrl) + 1))) == NULL)
                return ERR_ALLOC;
        strcpy(token, mrl);
        if ((token = strstr(token, "://")) == NULL) {
		fnc_log(FNC_LOG_ERR,"Invalid resource request \n");
		free(token);
		return ERR_PARSE;
	}
	token = strtok(token, "://");
	token_size=strlen(token);
	if(strcmp(token,"udp")==0) { 
		*type=st_net;	
		free(token);
		return parse_mrl_st_net(mrl+token_size+3, fd);
	}
	else if(strcmp(token,"file")==0) { 
		*type=st_file;	
		free(token);
		return parse_mrl_st_file(mrl+token_size+3, fd);
	}
	else if(strcmp(token,"dev")==0) {
		*type=st_device;	
		free(token);
		return parse_mrl_st_device(mrl+token_size+3, fd);
	}
	else {
		fnc_log(FNC_LOG_ERR,"Invalid resource request \n");
		free(token);
		return ERR_PARSE;
	}

}

		
