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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#ifndef __MEDIAPARSER_MODULE_H
#define __MEDIAPARSER_MODULE_H

#include <fenice/types.h>
#include <fenice/mediaparser.h>
#include <fenice/InputStream.h>

#define INIT_PROPS properties->media_type = info.media_type;

#define FNC_LIB_MEDIAPARSER(x) MediaParser fnc_mediaparser_##x =\
{\
	&info, \
	x##_init, \
	x##_get_frame2, \
	x##_packetize, \
        x##_parse, \
	x##_uninit \
}

#endif // __MEDIAPARSER_MODULE_H

