/* * 
 *  $Id: OMSbuff_new_shm.c 286 2006-02-07 19:16:47Z shawill $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/mman.h>

#include <fenice/bufferpool.h>
#include <fenice/fnc_log.h>
#include <fenice/utils.h>

OMSBuffer *OMSbuff_shm_map(char *shm_name)
{
	OMSBuffer *buffer;
	OMSSlot *slots;
	OMSControl *control;
//      uint32 index;
	char *shm_file_name;
	int fd;
	struct stat fdstat;

	// *** control struct mapping in shared memory ***
	if (!(shm_file_name = fnc_ipc_name(shm_name, OMSBUFF_SHM_CTRLNAME)))
		return NULL;

	fd = shm_open(shm_file_name, O_RDWR, 0);
	free(shm_file_name);
	if ((fd < 0)) {
		fnc_log(FNC_LOG_ERR,
			"Could not open POSIX shared memory (OMSControl): is Felix running?\n");
		return NULL;
	}
	if ((fstat(fd, &fdstat) < 0)) {
		fnc_log(FNC_LOG_ERR, "Could not stat %s\n",
			OMSBUFF_SHM_CTRLNAME);
		close(fd);
		return NULL;
	}

	if (((size_t) fdstat.st_size != sizeof(OMSControl))) {
		fnc_log(FNC_LOG_ERR,
			"Strange size for OMSControl shared memory! (not an integer number of slots)\n");
		close(fd);
		return NULL;
	}
	control =
	    mmap(NULL, fdstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 0);
	close(fd);
	if (control == MAP_FAILED) {
		fnc_log(FNC_LOG_FATAL, "SHM: error in mmap\n");
		return NULL;
	}
	// *** XXX inizialization of control struct made by creator!
	// control->refs = 0;
	// control->nslots = buffer_size-1;

	// if ( pthread_mutexattr_init(&mutex_attr) )
	//      return NULL;
	// if ( pthread_mutex_init(&control->syn, &mutex_attr) )
	//      return NULL;
	// *** wait for completion af all operations, especially for initializzation
	pthread_mutex_lock(&control->syn);
	pthread_mutex_unlock(&control->syn);

	// *** slots mapping in shared memory ***
	if (!(shm_file_name = fnc_ipc_name(shm_name, OMSBUFF_SHM_SLOTSNAME)))
		return NULL;

	fd = shm_open(shm_file_name, O_RDWR, 0);
	free(shm_file_name);
	if ((fd < 0)) {
		fnc_log(FNC_LOG_ERR,
			"Could not open POSIX shared memory (OMSSlots): is Felix running?\n");
		munmap(control, sizeof(OMSControl));
		return NULL;
	}
	if ((fstat(fd, &fdstat) < 0)) {
		fnc_log(FNC_LOG_ERR, "Could not stat %s\n",
			OMSBUFF_SHM_SLOTSNAME);
		close(fd);
		munmap(control, sizeof(OMSControl));
		return NULL;
	}

	if (((size_t) fdstat.st_size != control->nslots * sizeof(OMSSlot))) {
		fnc_log(FNC_LOG_ERR,
			"Strange size for shared memory! (not an integer number of slots)\n");
		close(fd);
		munmap(control, sizeof(OMSControl));
		return NULL;
	}
	slots =
	    mmap(NULL, fdstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		 0);
	close(fd);
	if (slots == MAP_FAILED) {
		fnc_log(FNC_LOG_FATAL, "SHM: error in mmap\n");
		munmap(control, sizeof(OMSControl));
		return NULL;
	}
	// initialized by creator!
	// for(index=0; index<buffer->nslots-1; index++)
	//      (slots[index]).next = OMSSLOT_SETADDR(buffer, &(slots[index+1]));
	// (slots[index]).next=OMSSLOT_SETADDR(buffer, slots); /*end of the list back to the head*/

	if (!(buffer = (OMSBuffer *) malloc(sizeof(OMSBuffer)))) {
		munmap(slots, control->nslots * sizeof(OMSSlot));
		munmap(control, sizeof(OMSControl));
		return NULL;
	}

	buffer->type = buff_shm;
	buffer->slots = slots;
	buffer->known_slots = control->nslots;
	strncpy(buffer->filename, shm_name, sizeof(buffer->filename) - 1);
	// buffer->fd = -1;
	// buffer->fd = NULL;
	buffer->slots = slots;
	buffer->control = control;

	return buffer;
}
