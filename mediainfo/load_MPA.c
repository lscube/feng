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

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>

int load_MPA(media_entry *p) {

        char thefile[255];
	unsigned char *buffer;
	long int tag_dim;
	struct stat fdstat;
        int n,RowIndex, ColIndex;
	
        // unsigned char sync1,sync2,sync3,sync4;
        int BitrateMatrix[16][5] = {
                {0,     0,     0,     0,     0     },
                {32000, 32000, 32000, 32000, 8000  },
                {64000, 48000, 40000, 48000, 16000 },
                {96000, 56000, 48000, 56000, 24000 },
                {128000,64000, 56000, 64000, 32000 },
                {160000,80000, 64000, 80000, 40000 },
                {192000,96000, 80000, 96000, 48000 },
                {224000,112000,96000, 112000,56000 },
                {256000,128000,112000,128000,64000 },
                {288000,160000,128000,144000,80000 },
                {320000,192000,160000,160000,96000 },
                {352000,224000,192000,176000,112000},
                {384000,256000,224000,192000,128000},
                {416000,320000,256000,224000,144000},
                {448000,384000,320000,256000,160000},
                {0,     0,     0,     0,     0     }
        };
        strcpy(thefile,prefs_get_serv_root());
        strcat(thefile,p->filename);            
        p->fd=open(thefile,O_RDONLY);
        if (p->fd==-1) return ERR_NOT_FOUND;
	/*
	else
                p->flags|=ME_FD;
	*/
	
	
//	fstat(p->fd, &fdstat);
        
	/*
	if (read(p->fd,&sync1,1) != 1) return ERR_PARSE;
        if (read(p->fd,&sync2,1) != 1) return ERR_PARSE;
        if (read(p->fd,&sync3,1) != 1) return ERR_PARSE;
        if (read(p->fd,&sync4,1) != 1) return ERR_PARSE;
	*/
	
	/*
	In questa parte eseguo il controllo per verificare
	la presenza del tag ID3v2
	*/
	
  buffer=(unsigned char *)calloc(1,4);   /* I primi tre bytes devono assumemere il valore "ID3" */
  if (buffer==NULL) {
                printf("errore calloc in load_MPA\n");
                return ERR_ALLOC;
        }
  if ((n=read(p->fd,buffer,3))!=3)
  {
    printf("Errore durante la lettura del brano! in load_MPA\n");
    return ERR_PARSE;
  }
  buffer[3]='\0';  
  if (strcmp(buffer,"ID3")!=0)  
    lseek(p->fd,0,SEEK_SET);
  else{
       lseek(p->fd,3, SEEK_CUR);
       n=read_dim(p->fd,&tag_dim); 
       printf("%ld\n",tag_dim);
       p->description.flags|=MED_ID3;
       p->description.tag_dim=tag_dim;
       lseek(p->fd,tag_dim,SEEK_CUR);
   } 
  
	
	
	p->buff_size=0;
	for (;p->buff_size<4;p->buff_size++)
		if (read(p->fd,&(p->buff_data[p->buff_size]),1) != 1) return ERR_PARSE;
        
	fstat(p->fd, &fdstat);
	if ( !S_ISFIFO(fdstat.st_mode) ) {
		close(p->fd);
		p->buff_size = 0;
	} else
                p->flags|=ME_FD;
	/*
	p->buff_data[0] = sync1;
	p->buff_data[1] = sync2;
	p->buff_data[2] = sync3;
	p->buff_data[3] = sync4;
	p->buff_size = 4;
	*/
        
	// if (! ((sync1==0xff) && ((sync2 & 0xe0)==0xe0))) return ERR_PARSE;
	if (! ((p->buff_data[0]==0xff) && ((p->buff_data[1] & 0xe0)==0xe0))) return ERR_PARSE;
        // switch (sync2 & 0x1e) {                /* Mpeg version and Level */
        switch (p->buff_data[1] & 0x1e) {                /* Mpeg version and Level */
                case 18: ColIndex = 4; break;  /* Mpeg-2 L3 */
                case 20: ColIndex = 4; break;  /* Mpeg-2 L2 */
                case 22: ColIndex = 3; break;  /* Mpeg-2 L1 */
                case 26: ColIndex = 2; break;  /* Mpeg-1 L3 */
                case 28: ColIndex = 1; break;  /* Mpeg-1 L2 */
                case 30: ColIndex = 0; break;  /* Mpeg-1 L1 */
                default: return ERR_PARSE;
        }
        // RowIndex = (sync3 & 0xf0) / 16;
        RowIndex = (p->buff_data[2] & 0xf0) / 16;
        p->description.bitrate = BitrateMatrix[RowIndex][ColIndex];
        p->description.flags|=MED_BITRATE;      
        // if (sync2 & 0x08) {     /* Mpeg-1 */
        if (p->buff_data[1] & 0x08) {     /* Mpeg-1 */
                // switch (sync3 & 0x0c) {
                switch (p->buff_data[2] & 0x0c) {
                        case 0x00: p->description.sample_rate=44100; break;
                        case 0x04: p->description.sample_rate=48000; break;
                        case 0x08: p->description.sample_rate=32000; break;
                        default: return ERR_PARSE;
                }
        } else {                /* Mpeg-2 */
                // switch (sync3 & 0x0c) {
                switch (p->buff_data[2] & 0x0c) {
                        case 0x00: p->description.sample_rate=22050; break;
                        case 0x04: p->description.sample_rate=24000; break;
                        case 0x08: p->description.sample_rate=16000; break;
                        default: return ERR_PARSE;
                }
        }
        p->description.flags|=MED_SAMPLE_RATE;          
        // if ((sync2 & 0x06) == 6) p->description.frame_len = 384;
        if ((p->buff_data[1] & 0x06) == 6) p->description.frame_len = 384;
        else p->description.frame_len = 1152;
        p->description.flags|=MED_FRAME_LEN;
        p->description.pkt_len=(double)p->description.frame_len/(double)p->description.sample_rate*1000;
        p->description.delta_mtime=p->description.pkt_len;
        p->description.flags|=MED_PKT_LEN;

        return ERR_NOERROR;
}

