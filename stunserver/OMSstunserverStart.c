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

#include <fenice/stunserver.h>
#include <fenice/fnc_log.h>

void *OMSstunserverStart(void *arg)
{

	OMSStunServer *omsss;
	struct STUN_SERVER_IFCFG *cfg = (struct STUN_SERVER_IFCFG *)arg;

	fnc_log(FNC_LOG_DEBUG,"OMSStunServerStart args: %s,%s,%s,%s\n",cfg->a1,cfg->p1,cfg->a2,cfg->p2);

	omsss=OMSStunServerInit(cfg->a1,cfg->p1,cfg->a2,cfg->p2);
	if (omsss == NULL)
		return NULL;

	OMSstunserverActions(omsss);
	
	return NULL;
}
