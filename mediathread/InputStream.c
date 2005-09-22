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
#include <sys/unistd.h>
#include <fcntl.h>
#include <errno.h>


#include <fenice/utils.h>
#include <fenice/InputStream.h>
#include <fenice/fnc_log.h>

static int open_mrl(char *, InputStream *);
static int open_mrl_st_file(InputStream *);
static int open_mrl_st_udp(InputStream *);
static int open_mrl_st_device(InputStream *);
static int open_socket(char *host, char *port, int *fd);
static void close_fd(InputStream *is);

static void is_setfl(InputStream *, istream_flags);
static void is_clrfl(InputStream *, istream_flags);

/*Interface*/
InputStream *istream_open(char *mrl)
{
	InputStream *is;

	if ( !(is=(InputStream *)malloc(sizeof(InputStream))) ) {
		fnc_log(FNC_LOG_FATAL,"Could not allocate memory for InputStream\n");
		return NULL;
	}
	// is->name[0] = '\0';
	is->flags=IS_FLAGS_INIT;
	is->cache=NULL;
	if(open_mrl(mrl, is)!=ERR_NOERROR) {
		fnc_log(FNC_LOG_ERR,"mrl not valid\n");
		free(is);
		return NULL;
	}

	return is;	
}

void istream_close(InputStream *is) 
{
	if(is!=NULL) {
		close_fd(is);
		free_cache(is->cache);
		is->cache=NULL;
		free(is);
		is=NULL;
	}
}

/*! InputStream read function
 * If buf pointer is NULL then the function act like a forward seek
 * */
inline int istream_read(uint32 nbytes, uint8 *buf, InputStream *is)
{
	return is ? read_c(nbytes, buf, is->cache, is->fd, is->type): ERR_ALLOC;
}

// static/private functions

static int open_mrl(char *mrl, InputStream *is)
{
	char *token;
	int res;
	char *colon;
	
        if ( !(colon = strstr(mrl, "://")) ) {
		is->type=st_file;	
		strcpy(is->name, mrl); // we store name w/o type
		return open_mrl_st_file(is);
	}
	*colon = '\0';
	token = colon + strlen("://");
	strcpy(is->name, token); // we store name w/o type
	if(!strcmp(mrl, FNC_UDP)) { 
		is->type=st_net;	
		res = open_mrl_st_udp(is);
	}
	/*TODO: tcp*/
	else if(!strcmp(token, FNC_FILE)) { 
		is->type=st_file;	
		res = open_mrl_st_file(is);
	}
	else if(!strcmp(token, FNC_DEV)) {
		is->type=st_device;	
		res = open_mrl_st_device(is);
	}
	else {
		fnc_log(FNC_LOG_ERR,"Invalid resource request \n");
		res = ERR_PARSE;
	}

	*colon = ':';

	return res;

}

static int open_mrl_st_file(InputStream *is)
{
	struct stat filestat;
	int oflag = O_RDONLY;

	if (!(*is->name))
		return ERR_INPUT_PARAM;
	
	fnc_log(FNC_LOG_DEBUG, "opening file %s...\n", is->name);

	// TODO: handle pipe
	if (stat(is->name, &filestat) == -1 ) {
		switch(errno) {
			case ENOENT:
				fnc_log(FNC_LOG_ERR,"%s: file not found\n", is->name);
				return ERR_NOT_FOUND;
				break;
			default:
				fnc_log(FNC_LOG_ERR,"Cannot stat file %s\n", is->name);
				return ERR_GENERIC;
				break;
		}
	}
	if ( S_ISFIFO(filestat.st_mode) ) {
		fnc_log(FNC_LOG_DEBUG, " IS_FIFO... ");
		oflag |= O_NONBLOCK;
		// is->flags|= IS_EXCLUSIVE;
		is_setfl( is, IS_EXCLUSIVE);
	}
	if( (is->fd=open(is->name, oflag))==-1 ) {
		switch (errno) {
			case EACCES:
				fnc_log(FNC_LOG_ERR,"%s: file not found\n", is->name);
				return ERR_NOT_FOUND;
				break;
			default:
				fnc_log(FNC_LOG_ERR,"It's impossible to open file %s\n", is->name);
				return ERR_GENERIC;
				break;
		}
	}
	return ERR_NOERROR;
}


static int open_mrl_st_udp(InputStream *is)
{
	char *host;
	char *port;
	int res;
	char *colon;

	if (!(*is->name))
		return ERR_INPUT_PARAM;

        // initialization
        if ( (colon = strchr(is->name, ':')) ) {
		*colon = '\0';
                host = is->name;
                port = colon + 1;
        } else
		return ERR_PARSE;

	res = open_socket(host, port, &is->fd);

	*colon = ':';

	return res;
}


static int open_mrl_st_device(InputStream *is)
{

	if (!(*is->name))
		return ERR_INPUT_PARAM;

	//...TODO
	//
	// return open_device();
	return ERR_GENERIC; // not yet inplemented
}

static int open_socket(char *host, char *port, int *fd)
{

	//...TODO
	return ERR_NOERROR;
}

static void close_fd(InputStream *is)
{
	close(is->fd);
	is->fd=-1;
}

static void is_setfl(InputStream *is, istream_flags flags)
{
	is->flags |= flags;
}

static void is_clrfl(InputStream *is, istream_flags flags)
{
	is->flags &= ~flags;
}

