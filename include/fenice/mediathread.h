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

#ifndef __MEDIA_THREAD
#define __MEDIA_THREAD

#define ENABLE_MEDIATHREAD 0

#include <pthread.h>

#include <fenice/demuxer.h>
#include <fenice/InputStream.h>

/*
typedef struct __MT_RESOURCE_ITEM {
	Resource *resource;
	pthread_mutex_t *mutex;
	struct __MT_RESOURCE_ITEM *next;
} mt_resource_item;
*/

typedef struct __MT_EXCL_INS {
	InputStream *i_stream;
	struct __MT_EXCL_INS *next;
} mt_excl_ins;

int mt_add_track(Track *);
int mt_rem_track(Track *);

Resource *mt_resource_open(char *, char *);
// ... resource_info();
void mt_resource_close(Resource *);
// int resource_seek(Resource *, <abs time>);

#endif // __MEDIA_THREAD

