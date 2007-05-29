/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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
#include <stdlib.h>

#include <stun/stun.h>

/*based on stund implementation from Vovida.org*/
STUNint32 stun_rand()
{
	static STUNbool init=STUN_FALSE;
	if ( !init ) { 
		init = STUN_TRUE;
		STUNuint64 tick;
		
#if defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
		asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) || defined( __sparc__ )	
		tick = gethrtime();
#elif defined(__MACH__) 
		STUNint32 fd=open("/dev/random",O_RDONLY);
		read(fd,&tick,sizeof(tick));
		close(fd);
#else
#     error Need some way to seed the random number generator 
#endif 
		STUNint32 seed = (STUNint32)tick;
		srandom(seed);
	}
	
	return random(); 
}
