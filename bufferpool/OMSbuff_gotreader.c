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

#include <stdio.h>

#include <sys/mman.h>

#include <fenice/bufferpool.h>

#define RETURN_ERR    do { \
                OMSbuff_unlock(cons->buffer); \
                return 1; \
            } while (0)

/* ! move current read position
 *
 * \return 1 on error, 0 otherwise
 * */
int OMSbuff_gotreader(OMSConsumer * cons)
{
    OMSSlot *last_read;
    OMSSlot *next;

    OMSbuff_lock(cons->buffer);

    if (OMSbuff_shm_refresh(cons->buffer))
        RETURN_ERR;

    last_read = OMStoSlot(cons->buffer, cons->last_read_pos);
    next = &cons->buffer->slots[cons->read_pos];

    if (!next->refs || (next->slot_seq < cons->last_seq)) {
        // added some slots?
        if (last_read && cons->buffer->slots[last_read->next].refs
            && (cons->buffer->slots[last_read->next].slot_seq >
            cons->last_seq))
            next = &cons->buffer->slots[last_read->next];
        else
            RETURN_ERR;
    } else if (last_read
           && (cons->buffer->slots[last_read->next].slot_seq <
               next->slot_seq))
        next = &cons->buffer->slots[last_read->next];

    next->refs--;

    cons->last_seq = next->slot_seq;

//      if ( msync(next, sizeof(OMSSlot), MS_ASYNC) )
//      if ( msync(cons->buffer->slots, cons->buffer->known_slots * sizeof(OMSSlot), MS_ASYNC) )
//              printf("*** slot msync error\n");

    cons->last_read_pos = OMStoSlotPtr(cons->buffer, next);
    cons->read_pos = next->next;

    // cons->read_pos->refs--;
    OMSbuff_dump(cons, NULL);
    OMSbuff_unlock(cons->buffer);

    // return cons->read_pos;
    return 0;        // next;
}

#undef RETURN_ERR
