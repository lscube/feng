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

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <fenice/mediathread.h>
#include <fenice/fnc_log.h>
#include <fenice/utils.h>
#include <time.h>

static GList *el_head = NULL;
static pthread_mutex_t el_mutex = PTHREAD_MUTEX_INITIALIZER;

void *mediathread(void *arg) {
	GList *el_cur, *el_prev;
	struct timespec ts = {0, 0};

	while(1) {
		pthread_mutex_lock(&el_mutex);
		el_cur = g_list_last(el_head);
		pthread_mutex_unlock(&el_mutex);

		while (el_cur) {
			pthread_mutex_lock(&el_mutex);
			el_prev = g_list_previous(el_cur);
			pthread_mutex_unlock(&el_mutex);

			mt_process_event(el_cur->data);
			mt_dispose_event(el_cur->data);

			pthread_mutex_lock(&el_mutex);
			el_head = g_list_delete_link(el_head, el_cur);
			pthread_mutex_unlock(&el_mutex);

			el_cur = el_prev;
		}
		//to avoid 100% cpu usage with empty eventlist
		nanosleep(&ts, NULL);
	}
	return NULL;
}

int mt_add_event(mt_event_id id, void **args) {
	mt_event_item *item;

	if (!(item = g_new0(mt_event_item, 1))) {
		fnc_log(FNC_LOG_FATAL, "Allocation failure in mt_create_event()\n");
		return ERR_GENERIC;
	}

	item->id = id;
//	item->sender = sender;
	item->args = args;

	pthread_mutex_lock(&el_mutex);
	el_head = g_list_prepend(el_head, item);
	pthread_mutex_unlock(&el_mutex);

	return ERR_NOERROR;
}

inline int mt_process_event(mt_event_item *ev) {
	if (!ev)
		return ERR_GENERIC;
	switch (ev->id) {
		default:
			break;
	}
	return ERR_NOERROR;
}

inline void mt_disable_event(mt_event_item *ev) {
	if (!ev)
		return;
	mt_dispose_event_args(ev->id, ev->args);
	ev->id = MT_EV_NOP;
}

inline void mt_dispose_event(mt_event_item *ev) {
	if (!ev)
		return;
	mt_dispose_event_args(ev->id, ev->args);
	g_free(ev);
}

inline void mt_dispose_event_args(mt_event_id id, void ** args) {
	switch (id) {
		default:
			break;
	}
	g_free(args);
}

Resource *mt_resource_open(char * path, char *filename) {

	// TODO: is enough ?

	return r_open(path, filename);

}

void mt_resource_close(Resource *resource) {

	if (!resource)
		return;

	// TODO: remove tracks from list if still there!

	r_close(resource);
}

int mt_add_track(Track *t) {

	if(!t)
		return ERR_GENERIC;
/*
	pthread_mutex_lock(&tl_mutex);
	tl_head = g_list_prepend(tl_head, t);
	pthread_mutex_unlock(&tl_mutex);
*/
	return ERR_NOERROR;
}

static gint mt_comp_func(gconstpointer element, gconstpointer track) {
	return ((const Track *)element != (Track *)track);
}

int mt_rem_track(Track *t) {
/*	GList *tl_ptr;
	
	pthread_mutex_lock(&tl_mutex);
	tl_ptr = g_list_find_custom(tl_head, t, mt_comp_func);
	if (!tl_ptr) {
		pthread_mutex_unlock(&tl_mutex);
		fnc_log(FNC_LOG_ERR, "Tried to remove from MT an unavailable track.\n");
		return ERR_GENERIC;
	}

	tl_head = g_list_remove_link (tl_head, tl_ptr);
	pthread_mutex_unlock(&tl_mutex);

	g_list_free_1(tl_ptr);
*/
	return ERR_NOERROR;
}
