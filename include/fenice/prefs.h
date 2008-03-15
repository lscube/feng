/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#ifndef FN_SERV_PREFS_H
#define FN_SERV_PREFS_H

#include <config.h>

#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    srv->srvconf.max_conns   /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

#define prefs_get_serv_root() srv->config_storage[0]->document_root->ptr
#define prefs_get_hostname() srv->srvconf.bindhost->ptr

#endif // FN_SERV_PREFS_H
