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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>

int enum_media(char *object,media_entry **list)
{
	static SD_descr *SD_global_list=NULL;
	SD_descr *matching_descr=NULL,*descr_curr,*last_descr=NULL;
	int res;
	char to_remove=0;
	
	if (!list)
		to_remove=1;
	
	//test about the loading of current SD (is it done?)
	for (descr_curr=SD_global_list; descr_curr && !matching_descr; descr_curr=descr_curr->next) {
		if (strcmp(descr_curr->filename,object)==0)
			matching_descr=descr_curr;
		else
			last_descr=descr_curr;
	}

	
	
	if (!matching_descr) {
	//.SD not found: update list
	//the first time SD_global_list must be initialized
		if (to_remove)
			return ERR_GENERIC; 
		if (!SD_global_list) {
			SD_global_list=(SD_descr*)calloc(1,sizeof(SD_descr));
			if (!SD_global_list) {
				return ERR_ALLOC;
			}
			matching_descr=SD_global_list;			
		}
		else {
			last_descr->next=(SD_descr*)calloc(1,sizeof(SD_descr));			
			if(!(matching_descr=last_descr->next))
				return ERR_ALLOC;
			
		}	
		strcpy(matching_descr->filename,object);
	}
	if (!to_remove)	
		res=parse_SD_file(object,matching_descr);
	if (res!=ERR_NOERROR || to_remove) {
		if (!last_descr) //matching is the first
			SD_global_list=SD_global_list->next;
		else {
			last_descr->next=matching_descr->next;
		}	
		free(matching_descr);
		return res;
	}
	*list=matching_descr->me_list;
	return 0;
}

