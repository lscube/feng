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


/*! @brief This function remaps shared memory slots in case that the slots
 * queue size is changed.
 * The function only remap the shared memory according to the new size, but
 * it does not do anything on new shm slots page: if it must be initialized
 * you must do it by yourself.
 * WARNING: the function assumes that the caller locked the buffer mutex.
 * @return 0 on succes, 1 otherwis.
 * */

int OMSbuff_shm_remap(OMSBuffer * buffer)
{
    OMSSlot *slots;
    char *shm_file_name;
    int fd;
    struct stat fdstat;

    // *** slots mapping in shared memory ***
    if (!(shm_file_name = 
        fnc_ipc_name(buffer->filename, OMSBUFF_SHM_SLOTSNAME)))
        return 1;

    fd = shm_open(shm_file_name, O_RDWR, 0);
    free(shm_file_name);
    if ((fd < 0)) {
        fnc_log(FNC_LOG_ERR,
            "Could not open POSIX shared memory (OMSSlots):"
            " is Felix running?\n");
        return 1;
    }
    if ((fstat(fd, &fdstat) < 0)) {
        fnc_log(FNC_LOG_ERR, "Could not stat %s\n",
            OMSBUFF_SHM_SLOTSNAME);
        close(fd);
        return 1;
    }

    if (((size_t) fdstat.st_size !=
         buffer->control->nslots * sizeof(OMSSlot))) {
        fnc_log(FNC_LOG_ERR,
            "Strange size for shared memory!"
            " (not the number of slots reported in control struct)\n");
        close(fd);
        return 1;
    }

    if (munmap(buffer->slots, buffer->known_slots * sizeof(OMSSlot))) {
        fnc_log(FNC_LOG_ERR, "Could not unmap previous slots!!!\n");
        close(fd);
        return 1;
    }
    slots = mmap(NULL, fdstat.st_size,
                 PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (slots == MAP_FAILED) {
        fnc_log(FNC_LOG_FATAL, "SHM: error in mmap\n");
        return 1;
    }

    buffer->slots = slots;
    buffer->known_slots = buffer->control->nslots;

    fnc_log(FNC_LOG_DEBUG, "SHM memory remapped (known slots %d)\n",
        buffer->known_slots);

    return 0;
}
