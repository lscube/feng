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

#include <fenice/prefs.h>
#include <fenice/stunserver.h>

extern serv_prefs prefs;

struct PREFS_STUN_CFG {
		char a1[16];
		char a2[16];
		char p1[6];
		char p2[6];
};

void *prefs_get_stuncfg()
{
	struct PREFS_STUN_CFG *cfg;

	if(!prefs.use_stun)
		return  NULL;
	
	cfg = calloc(1,sizeof(struct STUN_SERVER_IFCFG));
	strcpy((cfg)->a1,prefs.a1);
	strcpy((cfg)->p1,prefs.p1);
	strcpy((cfg)->a2,prefs.a2);
	strcpy((cfg)->p2,prefs.p2);

//	printf("par: %s,%s,%s,%s\n",(cfg)->a1,(cfg)->p1,(cfg)->a2,(cfg)->p2);
	
	return (void *)cfg;
}

