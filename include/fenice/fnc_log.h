/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2007 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
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


#ifndef FNC_LOGH
#define FNC_LOGH

#include <config.h>
#include <stdarg.h>

enum {  FNC_LOG_OUT,
        FNC_LOG_SYS,
        FNC_LOG_FILE };

	//level
#define FNC_LOG_FATAL 0
#define FNC_LOG_ERR 1
#define FNC_LOG_WARN 2
#define FNC_LOG_INFO 3
#define FNC_LOG_DEBUG 4
#define FNC_LOG_VERBOSE 5
#define FNC_LOG_CLIENT 6


extern void (*fnc_log) (int level, const char *fmt, ...);

void fnc_log_init(char *file, int out, char *name);

#endif
