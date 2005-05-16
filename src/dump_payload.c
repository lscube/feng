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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fenice/types.h>
#include <fenice/fnc_log.h>

#define MAX_FILE_DUMP 6

int dump_payload(uint8 *data_slot, uint32 data_size, uint8 fname[255]){
	static int fin[MAX_FILE_DUMP];	
	static char filename[MAX_FILE_DUMP][255];
	static int idx=0;
	int i=0, found=0;

	
	if(idx==0){
		for(i=0;i<MAX_FILE_DUMP;i++)
			memset(filename[i],0,sizeof(filename[i]));
	}

	for(i=0;i<idx;i++){
		if((strcmp(fname,filename[i]))==0){
			found=1;
			break;
		}
	}

	if(!found){
		if(idx>=MAX_FILE_DUMP)
			return -1
		strcpy(filename[idx],fname);
		i=idx;
		idx++;
	}
	
	if(fin[i]<=0){
		int oflag = O_RDWR;

		oflag|=O_CREAT;
		oflag|=O_TRUNC;
		fin[i]=open(fname,oflag,644);
	}
	if(fin[i]<0)
		fnc_log(FNC_LOG_VERBOSE,"Error to open file for dumping.\n");
	else
		write(fin[i],(void *)data_slot,data_size);

	return fin[i];
}
