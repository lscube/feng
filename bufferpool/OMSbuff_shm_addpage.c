/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
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


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>

#include <fenice/bufferpool.h>


/*!\brief This function adds shared memory page of slots.
 * The function remaps the shared memory according to the new size, but
 * it does NOT link new page to previous existing queue:
 * This is done in OMSbuff_slotadd.
 * WARNING: the function assumes that the caller (OMSbuff_write or OMSbuff_getslot) locked the buffer mutex
 * \return the first OMSSlot of new added page of slots.
 * */
OMSSlot *OMSbuff_shm_addpage(OMSBuffer * buffer)
{
    OMSSlot *added;
    OMSSlot *slots;
    unsigned int i;
    char *shm_file_name;
    int fd;
    struct stat fdstat;

    // *** slots mapping in shared memory ***
    if (!
        (shm_file_name =
         fnc_ipc_name(buffer->filename, OMSBUFF_SHM_SLOTSNAME)))
        return NULL;

    fd = shm_open(shm_file_name, O_RDWR, 0);
    free(shm_file_name);
    if ((fd < 0)) {
        fnc_log(FNC_LOG_ERR,
            "Could not open POSIX shared memory (OMSSlots): is Felix running?\n");
        return NULL;
    }
    if ((fstat(fd, &fdstat) < 0)) {
        fnc_log(FNC_LOG_ERR, "Could not stat %s\n",
            OMSBUFF_SHM_SLOTSNAME);
        close(fd);
        return NULL;
    }

    if (((size_t) fdstat.st_size !=
         buffer->control->nslots * sizeof(OMSSlot))) {
        fnc_log(FNC_LOG_ERR,
            "Strange size for shared memory! (not the number of slots reported in control struct)\n");
        close(fd);
        return NULL;
    }
    if (ftruncate
        (fd,
         (buffer->control->nslots + OMSBUFF_SHM_PAGE) * sizeof(OMSSlot))) {
        fnc_log(FNC_LOG_ERR,
            "Could not set correct size for shared memory object (OMSControl)\n");
        close(fd);
        return NULL;
    }
    if (munmap(buffer->slots, buffer->known_slots * sizeof(OMSSlot))) {
        fnc_log(FNC_LOG_ERR, "Could not unmap previous slots!!!\n");
        close(fd);
        return NULL;
    }
    slots =
        mmap(NULL,
         (buffer->control->nslots + OMSBUFF_SHM_PAGE) * sizeof(OMSSlot),
         PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (slots == MAP_FAILED) {
        fnc_log(FNC_LOG_FATAL, "SHM: error in mmap\n");
        return NULL;
    }
    // inizialization of OMSSlots added
    for (i = buffer->control->nslots;
         i < buffer->control->nslots + OMSBUFF_SHM_PAGE - 1; i++) {
        slots[i].refs = 0;
        slots[i].slot_seq = 0;
        slots[i].next = i + 1;
    }
    slots[i].refs = 0;
    slots[i].slot_seq = 0;
    slots[i].next = -1;    // last added slot in shm new page has next slot to NULL: OMSbuff_slotadd will link it correctly.

    added = &slots[buffer->control->nslots];
    buffer->slots = slots;
    buffer->control->nslots += OMSBUFF_SHM_PAGE;
    buffer->known_slots = buffer->control->nslots;

    return added;
}
