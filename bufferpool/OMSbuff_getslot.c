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
#include <fenice/bufferpool.h>


/*! @brief get a new empty slot from bufferpool.
 *
 * It do NOT mark the slot as busy, this action will be performed by
 * OMSbuff_write that MUST be called at the end of operations with the obtained
 * slot.
 *
 * @return empty slot.
 */
OMSSlot *OMSbuff_getslot(OMSBuffer * buffer)
{
    OMSSlot *slot;

    OMSbuff_lock(buffer);

    if (OMSbuff_shm_refresh(buffer))
        return NULL;
    
    slot = &buffer->slots[buffer->control->write_pos];

    if (buffer->slots[slot->next].refs > 0) {
        if (!(slot = OMSbuff_addpage(buffer, slot))) {
            OMSbuff_unlock(buffer);
            return NULL;
        }
    } else {
        slot = &buffer->slots[slot->next];
    }

    OMSbuff_unlock(buffer);

    return slot;
}
