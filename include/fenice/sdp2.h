/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

#ifndef FN_SDP2_H
#define FN_SDP2_H

#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

#include <fenice/demuxer.h>

#include <config.h>

#define SDP2_EL "\r\n"
#define SDP2_VERSION 0
#define SDP2_TRACK_ID "TrackID"

#define MAX_DESCR_LENGTH 63000

gint sdp_session_id(char *, size_t);
gint sdp_get_version(ResourceDescr *, char *, size_t);
int sdp_session_descr(char *, char *, size_t);
int sdp_media_descr(MediaDescrList, char *, size_t);

#endif // FN_SDP2_H
