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

stun_atr *create_unknown_attribute(OMS_STUN_PKT_DEV *received)
{
	STUNuint8 idx;

	stun_atr *atr = calloc(1,sizeof(stun_atr));

	if (atr == NULL)
		return NULL;

	if ((atr->atr = calloc(1,sizeof(struct STUN_ATR_UNKNOWN))) == NULL) {
		free(atr);
		return NULL;
	}

	((struct STUN_ATR_UNKNOWN *)(atr->atr))->attrType = 
		calloc(received->num_unknown_atrs,sizeof(STUNuint16));
	for(idx = 0; idx < received->num_unknown_atrs; idx++) {
		((struct STUN_ATR_UNKNOWN *)(atr->atr))->attrType[idx*sizeof(STUNuint16)] = 
			(received->list_unknown_attrType[idx]);
	}

	add_stun_atr_hdr(atr, UNKNOWN_ATTRIBUTES, sizeof(struct STUN_ATR_UNKNOWN));
	
		
	return atr;
}
