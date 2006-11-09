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

#ifndef SDP2_H_
#define SDP2_H_

#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

#include <fenice/types.h>
#include <fenice/demuxer.h>
#include <fenice/wsocket.h>

#include <config.h>
#ifndef PACKAGE
#define PACKAGE "fenice"
#endif

#define SDP2_EL "\r\n"
#define SDP2_VERSION 0

gint sdp_session_id(char *, size_t);
gint sdp_get_version(ResourceDescr *, char *, size_t);
int sdp_session_descr(resource_name, char *, size_t);
int sdp_media_descr(ResourceDescr *, MediaDescrList, char *, uint32);

#endif /*SDP2_H_*/
