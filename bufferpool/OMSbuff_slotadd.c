/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
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
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */



#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>

#include <fenice/bufferpool.h>

#ifndef OMSBUFF_REALLOC



/*\brief internal function used to add a slot or, in case of shared memory, a page of slots
 * (of OMSBUFF_SHM_PAGE size).
 * WARNING: the function assumes that the caller (OMSbuff_write or OMSbuff_getslot) locked the buffer mutex
 * \return the first OMSSlot of new added page of slots.
 * */
OMSSlot *OMSbuff_slotadd(OMSBuffer * buffer, OMSSlot * prev)
{
    OMSSlot *added;
    OMSSlotPtr prev_diff;

    switch (buffer->type) {
    case buff_shm:
        prev_diff = OMStoSlotPtr(buffer, prev);
        added = OMSbuff_shm_addpage(buffer);
        prev = &buffer->slots[prev_diff];

        buffer->slots[buffer->known_slots - 1].next = prev->next;    // last added slot in shm new page is linked to the prev->next in old queue

        fnc_log(FNC_LOG_DEBUG, "OMSSlots page added in SHM memory\n");
        break;
    case buff_local:
    default:
        if (!(added = calloc(1, sizeof(OMSSlotAdded))))
            return NULL;
        ((OMSSlotAdded *) added)->next_added = buffer->added_slots;
        buffer->added_slots = (OMSSlotAdded *) added;

        added->next = prev->next;
        fnc_log(FNC_LOG_DEBUG, "slot added\n");
        break;
    }

    prev->next = OMStoSlotPtr(buffer, added);

    return added;
}
#endif                // OMSBUFF_REALLOC
