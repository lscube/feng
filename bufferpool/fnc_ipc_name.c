/* * 
 *  $Id: px_ipc_name.c 286 2006-02-07 19:16:47Z shawill $
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
 *  this piece of code was inspired from Richard Stevens source code of
 *  UNIX Network Programming, Volume 2, Second Edition: Interprocess Communications,
 *  Prentice Hall, 1999, ISBN 0-13-081081-9.
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

char *fnc_ipc_name(const char *firstname, const char *lastname)
{
    char *dir, *dst, *slash;

    if ((dst = malloc(PATH_MAX)) == NULL)
        return (NULL);

    /* 4can override default directory with environment variable */
    if ((dir = getenv("PX_IPC_NAME")) == NULL) {
#ifdef    POSIX_IPC_PREFIX
        dir = POSIX_IPC_PREFIX;    /* from "config.h" */
#else
        dir = "";    // "/tmp/";   /* default */
#endif
    }
    /* 4dir must end in a slash */
    slash = (strlen(dir) && (dir[strlen(dir) - 1] == '/')) ? "" : "/";
    snprintf(dst, PATH_MAX, "%s%s%s.%s", dir, slash, firstname, lastname);

    return (dst);        /* caller can free() this pointer */
}
