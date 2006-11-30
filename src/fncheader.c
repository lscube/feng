/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
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

#include <stdio.h>
#include <string.h>

#include <config.h>

#ifdef FENICE_SVN
// #define SVNREV "$Rev$"
#include <svnrev.h>
#endif

inline void fncheader(void)
{
	char svnrev[32] = "";
#ifdef SVNREV
	char *tkn;

	snprintf(svnrev, sizeof(svnrev), "- SVN %s", SVNREV + 1);
	if ( (tkn=strchr(svnrev, '$')) )
		*tkn = '\0';
#endif
	printf("\n%s - Fenice Next Generation %s %s\n (LS)cube Project - Politecnico di Torino\n", PACKAGE, VERSION, svnrev);
	// nmsprintf(NMSML_ALWAYS, "\n"NMSCLR_BLUE_BOLD"%s - %s -- release %s %s(%s)\n\n"NMSCLR_DEFAULT, PROG_NAME, PROG_DESCR, VERSION, svnrev, VERSION_NAME);
}

