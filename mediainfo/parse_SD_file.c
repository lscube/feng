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
#include <string.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <stdio.h>

int parse_SD_file(char *object,SD_descr *sd_descr)
{
        FILE *f;
        char keyword[80],line[80],trash[80],sparam[10];
        media_entry *p;
        char thefile[255];
        int res;
	struct stat data;
                
        // Save the .SD file name               
        strcpy(thefile,prefs_get_serv_root());

        strcat(thefile,object);
        fnc_log(FNC_LOG_INFO,"Requested file is: %s\n", thefile);

        f=fopen(thefile,"r");
        if (f==NULL) {
                /* The file doesn't exist */            
                return ERR_NOT_FOUND;
        }       
	stat(thefile,&data);	
        if (sd_descr->last_change==data.st_ctime) //date of last change
		return ERR_NOERROR;//.SD file is the same yet
	else
		sd_descr->last_change=data.st_ctime;
        
	// Start parsing
        p=NULL;
        do {
                memset(keyword,0,sizeof(keyword));
                while (strcasecmp(keyword,SD_STREAM)!=0 && !feof(f)) {
                        fgets(line,80,f);
                        sscanf(line,"%s",keyword);
			if (strcasecmp(keyword,SD_TWIN)==0){ 
                                sscanf(line,"%s%s",trash,sd_descr->twin);
				sd_descr->flags|=SD_FL_TWIN;
			}
			if (strcasecmp(keyword,SD_MULTICAST)==0){ 
                                sscanf(line,"%s%s",trash,sd_descr->multicast);
				sd_descr->flags|=SD_FL_MULTICAST;
			}
			
                }
                if (feof(f)) {
                        return ERR_NOERROR;
                }

                // Allocate an element in the list
                if (p==NULL) {
                        sd_descr->me_list=(media_entry*)calloc(1,sizeof(media_entry));
                        if (sd_descr->me_list==NULL) {
                                return ERR_ALLOC;
                        }
                        p=sd_descr->me_list;
                }
                else {
                        p->next=(media_entry*)calloc(1,sizeof(media_entry));
                        if (p->next==NULL) {
                                return ERR_ALLOC;
                        }
                        p=p->next;
                }       
                memset(keyword,0,sizeof(keyword));
                while (strcasecmp(keyword,SD_STREAM_END)!=0 && !feof(f)) {
                        fgets(line,80,f);
                        sscanf(line,"%s",keyword);
                        if (strcasecmp(keyword,SD_FILENAME)==0) {
                                sscanf(line,"%s%255s",trash,p->filename);
                                p->flags|=ME_FILENAME;
                        }
                        if (strcasecmp(keyword,SD_BITRATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.bitrate));
                                p->description.flags|=MED_BITRATE;
                        }
                        if (strcasecmp(keyword,SD_PRIORITY)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.priority));
                                p->description.flags|=MED_PRIORITY;
                        }
                        if (strcasecmp(keyword,SD_PAYLOAD_TYPE)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.payload_type));
                                p->description.flags|=MED_PAYLOAD_TYPE;
                        }
                        if (strcasecmp(keyword,SD_CLOCK_RATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.clock_rate));
                                p->description.flags|=MED_CLOCK_RATE;
                        }
                        if (strcasecmp(keyword,SD_ENCODING_NAME)==0) {
                                sscanf(line,"%s%10s",trash,p->description.encoding_name);
                                p->description.flags|=MED_ENCODING_NAME;
                                if ( (strstr(p->description.encoding_name,"H26L")!=0) || (strstr(p->description.encoding_name,"MPV")!=0) || (strstr(p->description.encoding_name,"MP2T")!=0) || (strstr(p->description.encoding_name,"MP4V-ES")!=0) ){
                                        p->description.mtype=video;
                                }
                        }       
                        if (strcasecmp(keyword,SD_AUDIO_CHANNELS)==0) {
                                sscanf(line,"%s %hd\n",trash,&(p->description.audio_channels));
                                p->description.flags|=MED_AUDIO_CHANNELS;
                        }       
                        if (strcasecmp(keyword,SD_AGGREGATE)==0) {
                                sscanf(line,"%s%50s",trash,p->aggregate);
                                p->flags|=ME_AGGREGATE;
                        }       
                        if (strcasecmp(keyword,SD_SAMPLE_RATE)==0) {
                                sscanf(line,"%s%d",trash,&(p->description.sample_rate));
                                p->description.flags|=MED_SAMPLE_RATE;
                        }       
                        if (strcasecmp(keyword,SD_BIT_PER_SAMPLE)==0) {
                                sscanf(line,"%s%d",trash,&(p->description.bit_per_sample));
                                p->description.flags|=MED_BIT_PER_SAMPLE;
                        }                                       
                        if (strcasecmp(keyword,SD_FRAME_LEN)==0) {
                                sscanf(line,"%s%d",trash,&(p->description.frame_len));
                                p->description.flags|=MED_FRAME_LEN;
                        }                                                       
                        if (strcasecmp(keyword,SD_CODING_TYPE)==0) {
                                sscanf(line,"%s%10s",trash,sparam);
                                p->description.flags|=MED_CODING_TYPE;
                                if (strcasecmp(sparam,"FRAME")==0)
                                        p->description.coding_type=frame;
                                if (strcasecmp(sparam,"SAMPLE")==0)
                                        p->description.coding_type=sample;
                        }
                        if (strcasecmp(keyword,SD_PKT_LEN)==0) {
                                sscanf(line,"%s%f",trash,&(p->description.pkt_len));
                                p->description.flags|=MED_PKT_LEN;
                        }
                        if (strcasecmp(keyword,SD_FRAME_RATE)==0) {
                                sscanf(line,"%s%d",trash,&(p->description.frame_rate));
                                p->description.flags|=MED_FRAME_RATE;
                        }
                        if (strcasecmp(keyword,SD_BYTE_PER_PCKT)==0) {
                                sscanf(line,"%s%d",trash,&(p->description.byte_per_pckt));
                                p->description.flags|=MED_BYTE_PER_PCKT;
                        }              
			                   
			if (strcasecmp(keyword,SD_MEDIA_SOURCE)==0) {
                                sscanf(line,"%s%10s",trash,sparam);
                                p->description.flags|=MED_CODING_TYPE;
                                if (strcasecmp(sparam,"STORED")==0) 
                                        p->description.msource=stored;
                                if (strcasecmp(sparam,"LIVE")==0) 
                                        p->description.msource=live;
                        }


			/*****START CC****/
			if (strcasecmp(keyword,SD_LICENSE)==0) {
			sscanf(line,"%s%s",trash,(p->description.commons_dead));
			
			p->description.flags|=MED_LICENSE;
		        }
			if (strcasecmp(keyword,SD_RDF)==0) {
			sscanf(line,"%s%s",trash,(p->description.rdf_page));
			
			p->description.flags|=MED_RDF_PAGE;
		        }                     
			
			if (strcasecmp(keyword,SD_TITLE)==0){
			  
			  int i=7;
			  int j=0;
			  while(line[i]!='\n')
			  {
			    p->description.title[j]=line[i];
			    i++;
			    j++;
			   }
			  p->description.title[j]='\0';  
			
			  
			  p->description.flags|=MED_TITLE;
			 }  
			 
			 if (strcasecmp(keyword,SD_CREATOR)==0){
			  int i=9;
			  int j=0;
			  while(line[i]!='\n')
			  {
			    p->description.author[j]=line[i];
			    i++;
			    j++;
			   }
			  p->description.author[j]='\0';  
			
			 
			  p->description.flags|=MED_CREATOR;
			 }                         

			/********END CC*********/

			
                }
                if ((res = validate_stream(p,&sd_descr)) != ERR_NOERROR) {
                        return res;
                }
        } while (!feof(f));     
        fclose(f);
        return ERR_NOERROR;
}

