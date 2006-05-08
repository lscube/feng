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

#include <stdlib.h> /*calloc*/
#include <arpa/inet.h> /*htons*/
#include <stun/stun.h>

stun_atr *create_address(STUNuint8 family, STUNuint16 port, STUNuint32 address, STUNuint16 type)
{
	stun_atr *atr = calloc(1,sizeof(stun_atr));
	if(atr == NULL)
		return NULL;
	atr->atr = (struct STUN_ATR_ADDRESS *) \
   		   calloc(1,sizeof(struct STUN_ATR_ADDRESS));
	if(atr->atr == NULL) {
		free(atr);
		return NULL;
	}
	
	((struct STUN_ATR_ADDRESS *)(atr->atr))->family = family;
	((struct STUN_ATR_ADDRESS *)(atr->atr))->port = htons(port);
	((struct STUN_ATR_ADDRESS *)(atr->atr))->address = address; 
	/*Question: htons(address) ?*/
	
	(atr->stun_atr_hdr).type = type;
	(atr->stun_atr_hdr).length = sizeof(struct STUN_ATR_ADDRESS);

	return atr;
}


