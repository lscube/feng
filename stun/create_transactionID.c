/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
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

#include <stun/stun.h>

/*based on stund from Vovida.org*/
STUNuint128 create_transactionID(STUNuint32 testNum)
{
	STUNint32 i;
	STUNuint128 id;

	for ( i=0; i<16; i=i+4 ) {
		STUNint32 r = stun_rand();
		id.octet[i+0]= r>>0;
		id.octet[i+1]= r>>8;
		id.octet[i+2]= r>>16;
		id.octet[i+3]= r>>24;
	}
	
	if ( testNum != 0 ) {
		id.octet[0] = testNum; 
	}

	return id;
}
