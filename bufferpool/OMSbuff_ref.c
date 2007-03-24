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
#include <stdlib.h>

#include <sys/mman.h>

#include <fenice/bufferpool.h>


/* ! Add and return a new consumer reference to the buffer,
 * \return NULL if an error occurs*/
OMSConsumer *OMSbuff_ref(OMSBuffer * buffer)
{
    OMSConsumer *cons;
    ptrdiff_t i;

    if (!buffer)
        return NULL;

    if ((cons = (OMSConsumer *) malloc(sizeof(OMSConsumer))) == NULL)
        return NULL;

    cons->last_read_pos = -1;    // OMStoSlotPtr(buffer, NULL);
    cons->buffer = buffer;
    cons->frames = 0;
    cons->first_rtpseq = -1;
    cons->first_rtptime = -1;

    OMSbuff_lock(buffer);
#ifndef USE_VALID_READ_POS
    cons->read_pos = buffer->slots[buffer->control->write_pos].next;
    cons->last_seq = buffer->slots[buffer->control->write_pos].slot_seq;
#else // USE_VALID_READ_POS
    cons->read_pos = buffer->control->valid_read_pos;    // buffer->slots[buffer->control->valid_read_pos].next;
    cons->last_seq = 0;    // buffer->slots[buffer->control->valid_read_pos].slot_seq;

    if (buffer->slots[cons->read_pos].slot_seq) {
        for (i = cons->read_pos; i != buffer->control->write_pos;
             i = buffer->slots[i].next)
            buffer->slots[i].refs++;
        buffer->slots[i].refs++;
    }
#endif // USE_VALID_READ_POS

    // printf("ref at position %d (write_pos @ %d)\n", cons->read_pos, buffer->control->write_pos);
    buffer->control->refs++;

//      if ( msync(buffer->control, sizeof(OMSControl), MS_SYNC) )
//              printf("*** control msync error\n");
    OMSbuff_unlock(buffer);

    fnc_log(FNC_LOG_DEBUG, "Buffer ref (%d)\n", buffer->control->refs);

    return cons;
}
