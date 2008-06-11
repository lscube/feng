/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

/* 
 * mmap wrappers mostly ripped from xine's patches
 * from Diego Pettenò <flameeyes@gentoo.org>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef ENABLE_DUMA
#include <duma.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <errno.h>


#include <fenice/utils.h>
#include <fenice/InputStream.h>
#include <fenice/fnc_log.h>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

static int open_mrl(char *, InputStream *);
static int open_mrl_st_file(InputStream *);
static int open_mrl_st_udp(InputStream *);
static int open_mrl_st_device(InputStream *);
static int open_socket(char *host, char *port, int *fd);
static void close_fd(InputStream *is);

static void is_setfl(InputStream *, istream_flags);
// static void is_clrfl(InputStream *, istream_flags);

/*Interface*/
InputStream *istream_open(char *mrl)
{
	InputStream *is;

	if ( !(is=(InputStream *)calloc(1, sizeof(InputStream))) ) {
		fnc_log(FNC_LOG_FATAL,"Could not allocate memory for InputStream\n");
		return NULL;
	}

	is->fd = -1;
	if(open_mrl(mrl, is)!=ERR_NOERROR) {
		fnc_log(FNC_LOG_ERR," %s is not a valid mrl\n", mrl);
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

#ifdef HAVE_MMAP
static int read_mmap(uint32_t nbytes, uint8_t *buf, InputStream *is)
{
    off_t len = nbytes;

    if (!nbytes) //FIXME: move higher...
        return 0;
 
    if ( (is->mmap_curr + len) > (is->mmap_base + is->mmap_len) )
        len = (is->mmap_base + is->mmap_len) - is->mmap_curr;
    /* FIXME: don't memcpy! */
    if (buf)
        memcpy(buf, is->mmap_curr, len);

    is->mmap_curr += len;

    return len;
}
#endif


/*! InputStream read function
 * If buf pointer is NULL then the function act like a forward seek
 * */
inline int istream_read(InputStream *is, uint8_t *buf, uint32_t nbytes)
{
    if (is) {
#ifdef HAVE_MMAP
        if (is->mmap_on)
            return read_mmap(nbytes, buf, is);
        else
#endif
	    return read_c(nbytes, buf, &is->cache, is->fd, is->type);
    } else
        return ERR_ALLOC;
}

/*! InputStream reset function
 * Move the pointer to begin of file
 * */
inline int istream_reset(InputStream *is)
{
    if (is) {
#ifdef HAVE_MMAP
        if (is->mmap_on)
            return (int) (is->mmap_curr = 0);
        else
#endif
            return (int) lseek(is->fd, 0, SEEK_SET);
    } else
        return ERR_ALLOC;
}

stream_type parse_mrl(char *mrl, char **resource_name)
{
	char *colon;
	stream_type res;

        // if ( !(colon = strstr(mrl, "://")) ) {
        if ( !(colon = strstr(mrl, FNC_PROTO_SEPARATOR)) ) {
		*resource_name=mrl;
		return DEFAULT_ST_TYPE;
	}
	*colon = '\0';
	// *resource_name = colon + strlen("://");
	*resource_name = colon + strlen(FNC_PROTO_SEPARATOR);

	if(!strcmp(mrl, FNC_UDP))
		res = st_net;	
	/*TODO: tcp*/
	else if(!strcmp(mrl, FNC_FILE))
		res = st_file;
	else if(!strcmp(mrl, FNC_DEV))
		res = st_device;	
	else
		res = st_unknown;

	// *colon = ':';
	// *colon = *FNC_PROTO_SEPARATOR;
	*colon = FNC_PROTO_SEPARATOR[0];

	return res;
}



/*! @brief The function just returns the last change time of given mrl.
 * @return modification time of mrl or 0 if not guessable.
 * */
time_t mrl_mtime(char *mrl)
{
	char *resource_name;
	struct stat filestat;

	if (!mrl)
		return 0;
	
	switch (parse_mrl(mrl, &resource_name)) {
		case st_file:
			if (stat(resource_name, &filestat) == -1 ) {
				switch(errno) {
					case ENOENT:
						fnc_log(FNC_LOG_ERR,"%s: file not found\n", resource_name);
						return ERR_NOT_FOUND;
						break;
					default:
						fnc_log(FNC_LOG_ERR,"Cannot stat file %s\n", resource_name);
						return ERR_GENERIC;
						break;
				}
			}
			return filestat.st_mtime;
			break;
		case st_pipe:
			return 0; // we cannot know if pipe content media-type is changed, we must trust in it!
			break;
		case st_net:
		case st_device:
			return 0; // TODO other stream types case
			break;
		default:
			return ERR_INPUT_PARAM;
			break;
	}
	
	return 0;
}

/*! @brief Checks if the given mrl has been modified
 * The function parses the given mrl and, depending on the type, checks if that mrl has been modified. The second parameter of the function is both normal and return parameter.
 * @param mrl mrl to check for changes
 * @param last_change uptodate time for mrl and return parameter for new time if mrl is changed.
 * @return 1 is mrl is changed since last_change.
 * */
int mrl_changed(char *mrl, time_t *last_change)
{
	char *resource_name;
	struct stat filestat;
	
	switch (parse_mrl(mrl, &resource_name)) {
		case st_file:
			if (stat(resource_name, &filestat) == -1 ) {
				switch(errno) {
					case ENOENT:
						fnc_log(FNC_LOG_ERR,"%s: file not found\n", resource_name);
						return ERR_NOT_FOUND;
						break;
					default:
						fnc_log(FNC_LOG_ERR,"Cannot stat file %s\n", resource_name);
						return ERR_GENERIC;
						break;
				}
			}
			if (filestat.st_mtime > *last_change) {
				*last_change = filestat.st_mtime;
				return 1; // file changed
			} else
				return 0; // file NOT changed
			break;
		case st_pipe:
			return 0; // we cannot know if pipe content media-type is changed, we must trust in it!
			break;
		case st_net:
		case st_device:
			return 0; // TODO other stream types case
			break;
		default:
			return ERR_INPUT_PARAM;
			break;
	}
	
	return 0;
}

// static/private functions

static int open_mrl(char *mrl, InputStream *is)
{
	char *token;
	
	is->type=parse_mrl(mrl, &token);
	strcpy(is->name, token); // we store name w/o type
	switch ( is->type ) {
		case st_file:
		case st_pipe:
			return open_mrl_st_file(is);
			break;
		case st_net:
			return open_mrl_st_udp(is);
			break;
		case st_device:
			return open_mrl_st_device(is);
			break;
		default:
			fnc_log(FNC_LOG_ERR,"Invalid resource request \n");
			return ERR_PARSE;
			break;
	}

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
#ifdef HAVE_MMAP
	} else {
            if ( (is->mmap_base =
                    mmap(NULL, filestat.st_size, PROT_READ, MAP_SHARED,
                         is->fd, 0)) != MAP_FAILED ) {
                    is->mmap_on = 1;
                    is->mmap_curr = is->mmap_base;
                    is->mmap_len = filestat.st_size;
                    return ERR_NOERROR;
            }
#endif
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
#ifdef HAVE_MMAP
        if ( is->mmap_base ) {
            munmap(is->mmap_base, is->mmap_len);
            is->mmap_on = 0;
            is->mmap_base = NULL;
            is->mmap_curr = NULL;
            is->mmap_len = 0;
        }
#endif
}

static void is_setfl(InputStream *is, istream_flags flags)
{
	is->flags |= flags;
}
#if 0
static void is_clrfl(InputStream *is, istream_flags flags)
{
	is->flags &= ~flags;
}
#endif
