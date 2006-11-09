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

#include <stdio.h>
#include <stdlib.h>

#include <fenice/bufferpool.h>

#define OMSNEWBUFF_RET_ERR	do { \
					free(slots); \
					free(buffer); \
					free(control); \
					return NULL; \
				} while (0)

OMSBuffer *OMSbuff_new(uint32 buffer_size)
{
	OMSSlot *slots = NULL;
	OMSBuffer *buffer = NULL;
	OMSControl *control = NULL;
	uint32 index;
	pthread_mutexattr_t mutex_attr;

	if (!buffer_size)
		return NULL;

	if (!(slots = (OMSSlot *) calloc(buffer_size, sizeof(OMSSlot))))
		OMSNEWBUFF_RET_ERR;
	// *** slots initialization
	for (index = 0; index < buffer_size - 1; index++)
		(slots[index]).next = index + 1;
	(slots[index]).next = 0;	/*end of the list back to the head */

	// control struct allocation
	if (!(control = malloc(sizeof(OMSControl))))
		OMSNEWBUFF_RET_ERR;
	control->write_pos = buffer_size - 1;
	control->valid_read_pos = 0;	// buffer_size-1;

	control->refs = 0;
	control->nslots = buffer_size;

	if (pthread_mutexattr_init(&mutex_attr))
		OMSNEWBUFF_RET_ERR;
	if (pthread_mutex_init(&control->syn, &mutex_attr))
		OMSNEWBUFF_RET_ERR;
	if (!(buffer = (OMSBuffer *) malloc(sizeof(OMSBuffer))))
		OMSNEWBUFF_RET_ERR;
	buffer->type = buff_local;
	*buffer->filename = '\0';
	// buffer->fd = -1;
	// buffer->fd = NULL;
	buffer->known_slots = buffer_size;

	// link all allocated structs
	buffer->slots = slots;
	buffer->control = control;

	return buffer;
}

#undef OMSNEWBUFF_RET_ERR
