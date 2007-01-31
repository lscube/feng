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

#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <fenice/utils.h>
#include <fenice/bufferpool.h>

/*! \brief Write a new slot in the buffer using the input parameters.
 *  
 *  The function writes a new slot according with input parameters. If the
 *  <tt>data</tt> belongs to a slot that was prevoiusly requested with
 *  <tt>OMSbuff_getslot</tt> then there is not the copy of data, just the
 *  parameter setting like a commit of changes done.
 * \param seq impose explicitly the sequence number of pkt(slot). If this parameter is 0 the
 * it's set calculating 'prev_seq+1'.
 *  \return ERR_NOERROR or ERR_ALLOC
 *  */
int OMSbuff_write(OMSBuffer * buffer, uint64 seq, double timestamp,
           uint32 rtp_time, uint8 marker, uint8 * data, uint32 data_size)
{
    OMSSlot *slot; // = &buffer->slots[buffer->control->write_pos];
    uint64 curr_seq; // = slot->slot_seq;
#ifdef USE_VALID_READ_POS
    OMSSlot *valid_read_pos;
       // = &buffer->slots[buffer->control->valid_read_pos];
    double ts;
#endif // USE_VALID_READ_POS

    OMSbuff_lock(buffer);

    if (OMSbuff_shm_refresh(buffer))
        return ERR_ALLOC;

#ifdef USE_VALID_READ_POS
	valid_read_pos = &buffer->slots[buffer->control->valid_read_pos];
#endif // USE_VALID_READ_POS
	slot = &buffer->slots[buffer->control->write_pos];
	curr_seq = slot->slot_seq;

    if (buffer->slots[slot->next].data == data) {
        slot = &buffer->slots[slot->next];
    } else {
        if (buffer->slots[slot->next].refs > 0) {
            if ((slot = OMSbuff_addpage(buffer, slot)) == NULL) {
                OMSbuff_unlock(buffer);
                return ERR_ALLOC;
            }
        } else {
            slot = &buffer->slots[slot->next];
#ifdef USE_VALID_READ_POS
            // write_pos reaches valid_read_pos, we "push" it
	    if ((valid_read_pos->slot_seq)
                && (valid_read_pos == slot)) {
                for (ts = valid_read_pos->timestamp;
                     (buffer->slots[valid_read_pos->next].
                      slot_seq)
                     && (ts == valid_read_pos->timestamp);
                     ts =
                     valid_read_pos->timestamp, valid_read_pos =
                     &buffer->slots[valid_read_pos->next]);
                buffer->control->valid_read_pos =
                    OMStoSlotPtr(buffer, valid_read_pos);
            }
#endif // USE_VALID_READ_POS
        }

        memcpy(slot->data, data, data_size);
    }

    slot->timestamp = timestamp;
    slot->rtp_time = rtp_time;
    slot->marker = marker;
    slot->data_size = data_size;

    slot->refs = buffer->control->refs;
    slot->slot_seq = seq ? seq : curr_seq + 1;

    buffer->control->write_pos = OMStoSlotPtr(buffer, slot);

//      if ( msync(slot, sizeof(OMSSlot), MS_ASYNC) )
//      if ( msync(buffer->slots, buffer->known_slots*sizeof(OMSSlot), MS_ASYNC) )
//              printf("*** slot msync error\n");
//      if ( msync(buffer->control, sizeof(OMSControl), MS_ASYNC) )
//              printf("*** control msync error\n");
    OMSbuff_dump(NULL, buffer);

    OMSbuff_unlock(buffer);

    return ERR_NOERROR;
}
