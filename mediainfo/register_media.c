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

#include <string.h>		/*strcmp */
#include <stdlib.h>		/*calloc */
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/mpeg4es.h>
#include <fenice/h26l.h>
#include <fenice/mp3.h>
#include <fenice/gsm.h>
#include <fenice/pcm.h>
#include <fenice/mpeg_system.h>
#include <fenice/mpeg_ts.h>
#include <fenice/mpeg.h>
#include <fenice/rtp_shm.h>
#include <fenice/fnc_log.h>

/*
 * \fn register_media
 * 	\brief register pointer functions in order to load, read, free media bitstream.
 *	\param media_entry *me .
 */
int register_media(media_entry * me)
{
	if (strcmp(me->description.encoding_name, "MP4V-ES") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_MP4ES;
		me->media_handler->load_media = load_MP4ES;
		me->media_handler->read_media = read_MPEG4ES_video;
	} else if (strcmp(me->description.encoding_name, "MPV") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_MPV;
		me->media_handler->load_media = load_MPV;
		me->media_handler->read_media = read_MPEG_video;
	} else if (strcmp(me->description.encoding_name, "MPA") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_MPA;
		me->media_handler->load_media = load_MPA;
		me->media_handler->read_media = read_MP3;
	} else if (strcmp(me->description.encoding_name, "GSM") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_GSM;
		me->media_handler->load_media = load_GSM;
		me->media_handler->read_media = read_GSM;
	} else if (strcmp(me->description.encoding_name, "L16") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_L16;
		me->media_handler->load_media = load_L16;
		me->media_handler->read_media = read_PCM;
	} else if (strcmp(me->description.encoding_name, "H26L") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_H26L;
		me->media_handler->load_media = load_H26L;
		me->media_handler->read_media = read_H26L;
	} else if (strcmp(me->description.encoding_name, "MP2T") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_MP2T;
		me->media_handler->load_media = load_MP2T;
		me->media_handler->read_media = read_MPEG_ts;
	} else if (strcmp(me->description.encoding_name, "RTP_SHM") == 0) {
		me->media_handler = (media_fn *) calloc(1, sizeof(media_fn));
		me->media_handler->free_media = free_RTP_SHM;
		me->media_handler->load_media = load_RTP_SHM;
		me->media_handler->read_media = NULL;	// read_RTP_SHM;            
	} else {
		fnc_log(FNC_LOG_ERR, "Encoding type not supported\n");
		return ERR_UNSUPPORTED_PT;
	}
	return ERR_NOERROR;
}
