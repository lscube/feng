/* * 
 *  $Id$
 *  
 *  This file is part of NetEmbryo 
 *
 * NetEmbryo -- default network wrapper 
 *
 *  Copyright (C) 2005 by
 *  	
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 * 
 *  NetEmbryo is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NetEmbryo is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NetEmbryo; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <netembryo/wsocket.h>

char *addr_ntop(const Sock  *addr, char *str, size_t len)
{
        switch (addr->family) {
                case AF_INET:
                        if (inet_ntop(AF_INET, &addr->addr.mreq_in.imr_interface4, str, len) == NULL)
                                return(NULL);
                        return(str);
                        break;
#ifdef  IPV6
                case AF_INET6:
                        if (inet_ntop(AF_INET6, &addr->addr.mreq_in6.imr_interface6, str, len) == NULL)
                                return(NULL);
                        return(str);
                        break;
#endif

                default:
                        //snprintf(str, len, "addr_ntop: unknown AF_xxx: %d", addr->family);
                        return(str);
        }
        return (NULL);
}

