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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#ifndef __MEDIAPARSER_MODULE_H
#define __MEDIAPARSER_MODULE_H

#include <fenice/types.h>
#include <fenice/MediaParser.h>
#include <fenice/InputStream.h>

/* init: inizialize the module
 *    arg:
 *       properties; pointer of allocated struct to fill with properties
 *       private_data: private data of parser will be, if needed, linked to this pointer (double)
 *    return: 0 on success, non-zero otherwise.
 * */

static int init(MediaProperties *properties, void **private_data);
/* get_frame: parse one frame from media bitstream.
 *    args:
 *       dst: destination memory slot,
 *       dst_nbytes: number of bytes of *dest memory area,
 *       timestamp; return value for timestap of read frame
 *       void *properties: private data specific for each media parser.
 *       istream: InputStream of source Elementary Stream,
 *    return: ...
 * */
static int get_frame2(uint8 *dst, uint32 dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data);

/* packetize: ...
 *    args:
 *       dst: destination memory slot,
 *       dst_nbytes: number of bytes of *dest memory area,
 *       src: source memory slot,
 *       src_nbytes: number of bytes of *source memory area,
 *       void *properties: private data specific for each media parser.
 *    return: ...
 * */
static int packetize(uint8 *dst, uint32 *dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data);

/* uninit: free the media parser structures.
 *    args:
 *       private_data: pointer to parser specific private data.
 *    return: void.
 * */
static int uninit(void *private_data); /*before call free_parser */

#define FNC_LIB_MEDIAPARSER(x) MediaParser fnc_mediaparser_##x =\
{\
	&info, \
	init, \
	get_frame2, \
	packetize, \
	uninit \
}

#endif // __MEDIAPARSER_MODULE_H

