/* * 
 *  $Id:$
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
#include <unistd.h>

#include <fenice/utils.h>
#include <fenice/mediainfo.h>

int read_dim(int file,long int *dim)
{
unsigned char byte1,byte2,byte3,byte4;
int n,res;
double skip=0.0;
if ((n=read(file,&byte1,1))!=1)
   {
   printf("Errore durante la lettura della dimensione del tag ID3  al byte1\n");
   return ERR_PARSE;
   }
     res=calculate_skip((int) byte1,&skip,24); 
   if ((n=read(file,&byte2,1))!=1)
   {
   fprintf(stderr,"Errore durante la lettura della dimensione del tag ID3 al byte2\n");
   return ERR_PARSE;
   } 
   res=calculate_skip((int) byte2,&skip,16);     
   if ((n=read(file,&byte3,1))!=1)
   {
   fprintf(stderr,"Errore durante la lettura della dimensione del tag ID3 al byte3\n");
   return ERR_PARSE;
   }   
   res=calculate_skip((int) byte3,&skip,8);	
     
   if ((n=read(file,&byte4,1))!=1)
   {
   fprintf(stderr,"Errore durante la lettura della dimensione del tag ID3 al byte4\n");
   return ERR_PARSE;
   }
   res=calculate_skip((int) byte4,&skip,0);
   *dim=(long int)skip;
   return 0;
}

