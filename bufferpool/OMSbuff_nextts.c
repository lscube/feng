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

/*! return the next packet timestamp
 *
 * @return -1 on error, the packet timestamp otherwise
 **/

double OMSbuff_nextts(OMSConsumer * cons)
{
    OMSSlot *last_read;
    OMSSlot *next;
    int ret = -1;

    OMSbuff_lock(cons->buffer);

    if(OMSbuff_shm_refresh(cons->buffer))
        goto err_unlock;

    last_read = OMStoSlot(cons->buffer, cons->last_read_pos);
    next = &cons->buffer->slots[cons->read_pos];

    if (!next->refs || (next->slot_seq < cons->last_seq)) {
        // added some slots?
        if (last_read && cons->buffer->slots[last_read->next].refs
            && (cons->buffer->slots[last_read->next].slot_seq >
            cons->last_seq))
            next = &cons->buffer->slots[last_read->next];
        else {
            goto err_unlock;
        }
    } else if (last_read
           && (cons->buffer->slots[last_read->next].slot_seq <
               next->slot_seq))
        next = &cons->buffer->slots[last_read->next];
    
    ret = next->timestamp;

    err_unlock:
    OMSbuff_unlock(cons->buffer);

    return ret;
}
