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

void OMSbuff_free(OMSBuffer * buffer)
{
    switch (buffer->type) {
    case buff_shm:
        OMSbuff_shm_unmap(buffer);
        fnc_log(FNC_LOG_DEBUG, "Buffer in SHM unmapped \n");
        break;
    case buff_local:{
            pthread_mutex_destroy(&buffer->control->syn);
            free(buffer->control);
            free(buffer->slots);
            free(buffer);
            fnc_log(FNC_LOG_DEBUG, "Buffer is freed \n");
            break;
        }
    default:
        break;
    }

}
