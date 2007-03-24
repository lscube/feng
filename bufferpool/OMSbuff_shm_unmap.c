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

#include <fenice/bufferpool.h>


int OMSbuff_shm_unmap(OMSBuffer * buffer)
{
    int slots_err;
    int control_err;

    if (buffer->type != buff_shm) {
        fnc_log(FNC_LOG_ERR,
            "Bufferpool desn't seems to be a Shared Memory object");
        return 1;
    }

    if ((slots_err =
         munmap(buffer->slots, buffer->control->nslots * sizeof(OMSSlot))))
        fnc_log(FNC_LOG_ERR, "Error unmapping OMSSlots SHM object\n");
    if ((control_err = munmap(buffer->control, sizeof(OMSControl))))
        fnc_log(FNC_LOG_ERR, "Error unmapping OMSControl SHM object\n");

    return slots_err ? slots_err : control_err;
}
