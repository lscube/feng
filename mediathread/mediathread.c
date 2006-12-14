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

static GList *tl_head = NULL;
static pthread_mutex_t tl_mutex = PTHREAD_MUTEX_INITIALIZER;

void *mediathread(void *arg) {
	GList *tl_ptr;
	struct timespec ts = {0, 0};

	while(1) {
		pthread_mutex_lock(&tl_mutex);
		for (tl_ptr = g_list_first(tl_head); tl_ptr;
		     tl_ptr = g_list_next(tl_ptr)) {

			//TODO: fill buffer
			
		}
		pthread_mutex_unlock(&tl_mutex);
		//to avoid 100% cpu usage with empty tracklist
		nanosleep(&ts, NULL);
	}
	return NULL;
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

	pthread_mutex_lock(&tl_mutex);
	tl_head = g_list_prepend(tl_head, t);
	pthread_mutex_unlock(&tl_mutex);

	return ERR_NOERROR;
}

static gint mt_comp_func(gconstpointer element, gconstpointer track) {
	return ((const Track *)element != (Track *)track);
}

int mt_rem_track(Track *t) {
	GList *tl_ptr;
	
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

	return ERR_NOERROR;
}

