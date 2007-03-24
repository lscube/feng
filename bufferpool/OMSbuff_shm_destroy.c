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
#include <errno.h>

#include <fenice/bufferpool.h>
#include <fenice/fnc_log.h>

/*! \brief This function destroys the shared memory object.
 * Firstly it unlinks the shm objects referenced in buffer and
 * then unlink them thus removing them from system.
 * It doesn't stop on the first error, but completes all operations
 * and returns the code of first error.
 * \return 0 on succes or the number of the first error occurred.
 * */
int OMSbuff_shm_destroy(OMSBuffer * buffer)
{
    int unmap_err, shm_unlink_err = 0;
    char *shm_file_name;

    unmap_err = OMSbuff_shm_unmap(buffer);

    if (!
        (shm_file_name =
         fnc_ipc_name(buffer->filename, OMSBUFF_SHM_CTRLNAME)))
        return 1;
    if (shm_unlink(shm_file_name)) {
        switch (errno) {
        case ENOENT:
            fnc_log(FNC_LOG_ERR, "SHM Object %s doesn't exists\n");
            break;
        case EACCES:
            fnc_log(FNC_LOG_ERR,
                "Permission denied on SHM Object %s\n");
            break;
        default:
            fnc_log(FNC_LOG_ERR,
                "Could not unlink SHM Object %s\n");
            break;
        }
        shm_unlink_err = errno;
    }
    free(shm_file_name);

    if (!
        (shm_file_name =
         fnc_ipc_name(buffer->filename, OMSBUFF_SHM_SLOTSNAME)))
        return 1;
    if (shm_unlink(shm_file_name)) {
        switch (errno) {
        case ENOENT:
            fnc_log(FNC_LOG_ERR, "SHM Object %s doesn't exists\n");
            break;
        case EACCES:
            fnc_log(FNC_LOG_ERR,
                "Permission denied on SHM Object %s\n");
            break;
        default:
            fnc_log(FNC_LOG_ERR,
                "Could not unlink SHM Object %s\n");
            break;
        }
        shm_unlink_err = shm_unlink_err ? shm_unlink_err : errno;
    }
    free(shm_file_name);

    return unmap_err ? unmap_err : shm_unlink_err;
}
