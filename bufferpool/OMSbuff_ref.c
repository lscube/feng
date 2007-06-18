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




#include <sys/mman.h>

#include <fenice/bufferpool.h>


/* ! Add and return a new consumer reference to the buffer,
 * @return NULL if an error occurs*/
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
    cons->read_pos = buffer->slots[buffer->control->write_pos].next;
    cons->last_seq = buffer->slots[buffer->control->write_pos].slot_seq;

    buffer->control->refs++;

    OMSbuff_unlock(buffer);

    fnc_log(FNC_LOG_DEBUG, "Buffer ref (%d)\n", buffer->control->refs);

    return cons;
}
