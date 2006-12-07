/* * 
 *  $Id$
 *  
 *  This file is part of Fenice

Semplicemente RIDICOLO! (media: 0,09) Vedi sequenza del thread
Chiudi ramo(18) Non autenticato alle 00.14 - voto: 1,50
	morpheus_nora alle 00.36 - voto: 0
	Non autenticato alle 02.14 - voto: 0
	ugu
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

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <fenice/mediathread.h>
#include <fenice/fnc_log.h>
#include <fenice/utils.h>
#include <time.h>

static GList *tl_head = NULL;
static pthread_mutex_t tl_mutex = PTHREAD_MUTEX_INITIALIZER;

void *mediathread(void *arg) {
	GList *tl_ptr;
	mt_tracklist_item *tl_data;
	struct timespec ts = {0, 0};

	while(1) {
		pthread_mutex_lock(&tl_mutex);
		for (tl_ptr = g_list_first(tl_head); tl_ptr;
		     tl_ptr = g_list_next(tl_ptr)) {
			tl_data = (mt_tracklist_item *) tl_ptr->data;
			//unlock tracklist
			pthread_mutex_unlock(&tl_mutex);
			//lock working track
			pthread_mutex_lock(&(tl_data->mutex));

			//TODO: fill buffer
			
			//unlock working track
			pthread_mutex_unlock(&(tl_data->mutex));
			//lock tracklist
			pthread_mutex_lock(&tl_mutex);
		}
		pthread_mutex_unlock(&tl_mutex);
		//to avoid 100% cpu usage with empty tracklist
		nanosleep(&ts, NULL);
	}
	return NULL;
}

int mt_add_track(Track *t) {
	mt_tracklist_item *tl_data;

	if(!(tl_data = g_new0(mt_tracklist_item, 1))) {
		fnc_log(FNC_LOG_ERR, "Unable to allocate mt_tracklist_item.\n");
		return ERR_ALLOC;
	}

	if (pthread_mutex_init(&(tl_data->mutex), NULL)) {
		fnc_log(FNC_LOG_ERR, "Unable to initailize track mutex.\n");
		return ERR_GENERIC;
	}

	tl_data->track = t;

	pthread_mutex_lock(&tl_mutex);
	tl_head = g_list_prepend(tl_head, tl_data);
	pthread_mutex_unlock(&tl_mutex);

	return ERR_NOERROR;
}

static inline gint mt_comp_func(gconstpointer tl_data, gconstpointer track) {
	return (((const mt_tracklist_item *)tl_data)->track != (Track *)track);
}

int mt_rem_track(Track *t) {
	GList *tl_ptr;
	mt_tracklist_item *tl_data;

	pthread_mutex_lock(&tl_mutex);
	tl_ptr = g_list_find_custom(tl_head, t, mt_comp_func);
	if (!tl_ptr) {
		pthread_mutex_unlock(&tl_mutex);
		fnc_log(FNC_LOG_ERR, "Tried to remove from MT an unavailable track.\n");
		return ERR_GENERIC;
	}
	tl_data = tl_ptr->data;
	pthread_mutex_lock(&(tl_data->mutex));
	tl_head = g_list_remove_link (tl_head, tl_ptr);
	pthread_mutex_unlock(&(tl_data->mutex));
	pthread_mutex_unlock(&tl_mutex);

	pthread_mutex_destroy(&(tl_data->mutex));
	g_free(tl_data);

	return ERR_NOERROR;
}

