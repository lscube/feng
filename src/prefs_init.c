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

#include <fenice/prefs.h>
#include <string.h>
#include <stdio.h>

serv_prefs prefs;

void prefs_init(char *fileconf)
{
	FILE *f;
	char opened = 0;
	char line[256];
	char *p,*cont;
	int l;
			
	if ( (f = fopen(fileconf,"rt")) == NULL ) {
		printf("Error opening file %s, trying default (%s):\n", fileconf, DEFAULT_CONF_FILE);
		if ( (f = fopen(DEFAULT_CONF_FILE, "rt")) == NULL ) {
			printf("Error opening default file, using internal defaults:\n");
			prefs_use_default(-1);
		} else opened = 1;
	} else opened = 1;
	if (opened) {		
		// SERVER ROOT
		do {
			cont=fgets(line,80,f);
			p=strstr(line,PREFS_ROOT);
		} while (p==NULL && cont);
		if (p!=NULL) {
			p=strstr(p,"=");
			if (p!=NULL) {
				strcpy(prefs.serv_root,p+1);
				p=strstr(prefs.serv_root,"\n");
				if (p!=NULL) {
					*p='\0';
				}
				else {
					prefs_use_default(0);
				}
			}
			else {
				prefs_use_default(0);
			}
		}
		else {
			prefs_use_default(0);
		}
		// PORT
		fseek(f,0,SEEK_SET);
		do {
			cont=fgets(line,80,f);
			p=strstr(line,PREFS_PORT);
		} while (p==NULL && cont);
		if (p!=NULL) {
			p=strstr(p,"=")+1;
			if (p!=NULL) {
				if (sscanf(p,"%i",&(prefs.port))!=1) {
					prefs_use_default(1);
				}
			}
			else {
				prefs_use_default(1);
			}
		}
		else {
			prefs_use_default(1);
		}
	       //MULTICAST
		fseek(f,0,SEEK_SET);

		do {
			cont=fgets(line,80,f);
			p=strstr(line,PREFS_MULTICAST_FILE);
		} while (p==NULL && cont);
		if (p!=NULL) {
			p=strstr(p,"=");
			if (p!=NULL) {
				strcpy(prefs.multicast_file,p+1);
				p=strstr(prefs.multicast_file,"\n");
				if (p!=NULL) {
					*p='\0';
				//	printf("Multicast file is %s\n", prefs.multicast_file);
				}
				else {
					prefs_use_default(2);
				}
			}
			else {
				prefs_use_default(2);
			}
		}
		else {
			prefs_use_default(2);
		}

		
	}
	gethostname(prefs.hostname,sizeof(prefs.hostname));
	l=strlen(prefs.hostname);
	if (getdomainname(prefs.hostname+l+1,sizeof(prefs.hostname)-l)!=0) {
		prefs.hostname[l]='.';
	}
	printf("\n");
	printf("\tavroot directory is: %s\n", prefs.serv_root);
	printf("\thostname is: %s\n", prefs.hostname);
	printf("\trtsp listening port is: %d\n", prefs.port);
	printf("\n");
}
