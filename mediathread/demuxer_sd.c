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

#include <fenice/demuxer.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

/*RESOURCE PRIVATE_DATA*/
#if !defined(_MEDIAINFOH) /*remove it when mediainfo will be removed*/ 
typedef enum {
	SD_FL_TWIN=1,
	SD_FL_MULTICAST=2,
	SD_FL_MULTICAST_PORT=4
} sd_descr_flags;

typedef struct __SD_descr {
		sd_descr_flags flags;
		char multicast[16];/*???*/
		char ttl[4];/*???*/
    		//char twin[255]; //see RESOURCE_INFO -> twin
} SD_descr;
/*----------------------*/


/*TRACK PRIVATE_DATA*/
typedef enum {
	ME_FILENAME=1,
	/*    disposable   ME=2,  */
	ME_DESCR_FORMAT=4,
	ME_AGGREGATE=8,
	ME_RESERVED=16,
	ME_FD=32
} me_flags;

typedef enum {
	MED_MSOURCE=2, /* live, stored => st_pipe/st_net/st_device, st_file*/
	MED_PAYLOAD_TYPE=4,
	MED_CLOCK_RATE=8,
	MED_ENCODING_NAME=16,
	MED_AUDIO_CHANNELS=32,
	MED_SAMPLE_RATE=64,    	
	MED_BIT_PER_SAMPLE=128,
	MED_CODING_TYPE=256,
	MED_FRAME_LEN=512,
	MED_BITRATE=1024,
	MED_PKT_LEN=2048,
	MED_PRIORITY=4096,
	MED_FRAME_RATE=16384,
	MED_BYTE_PER_PCKT=32768,
	/*start CC*/
	MED_LICENSE=65536,
	MED_RDF_PAGE=131072,
	MED_TITLE=262144,
	MED_CREATOR=524288,
	MED_ID3=1048576
	/*end CC*/
} me_descr_flags;
#endif //...MEDIAINFOH

typedef struct __FLAGS_DATA{
	me_flags general_flags; 
	me_descr_flags description_flags;	    	    	    		
   	struct __DATA {	
		int frame_len; // i need to move it in Parser
		int priority; //i need to move it. Where? Selector, Track 
		float pkt_len; //i need to move it in Parser
		int byte_per_pckt; //i need to move it in Parser
	} data;
} FlagsData;    	

/*----------------------*/

static int sd_init(Resource *r)
{

	char keyword[80],line[80],trash[80],sparam[10];
	Track *track;
	SD_descr *sd;
	FlagsData *me;
	
	/*Allocate Resource PRIVATE DATA and cast it*/
	if((sd=malloc(sizeof(SD_descr)))==NULL)
		return ERR_ALLOC;

	/**
	 * parse sd file.*/

	do{
		memset(keyword,0,sizeof(keyword));
		while (strcasecmp(keyword,SD_STREAM)!=0 && !feof(r->i_stream->fd)) {
		        fgets(line,80,r->i_stream->fd);
		        sscanf(line,"%s",keyword);
			if (strcasecmp(keyword,SD_TWIN)==0){ 
		                sscanf(line,"%s%s",trash,r->info->twin);
				sd->flags|=SD_FL_TWIN;
			}
			if (strcasecmp(keyword,SD_MULTICAST)==0){ 
		                sscanf(line,"%s%s",trash,sd->multicast);
				sd->flags|=SD_FL_MULTICAST;
			}
		}
		if (feof(r->i_stream->fd)) 
		        return RESOURCE_OK;

		/* Allocate and cast TRACK PRIVATE DATA foreach track.
		 * (in this case track = elementary stream media file)
		 * */

		if((track=add_track(r))==NULL) {
			fnc_log(FNC_LOG_ERR,"Memory allocation error during add_track\n");
			return ERR_ALLOC;
		}
		
		if((me=malloc(sizeof(FlagsData)))==NULL) {
			fnc_log(FNC_LOG_ERR,"Memory allocation error for track->private_data\n");
			free_track(track);
			r->num_tracks--;
			return ERR_ALLOC;
		}
		
                memset(keyword,0,sizeof(keyword));
                while (strcasecmp(keyword,SD_STREAM_END)!=0 && !feof(feof(r->i_stream->fd))) {
                        fgets(line,80,r->i_stream->fd);
                        sscanf(line,"%s",keyword);
                        if (strcasecmp(keyword,SD_FILENAME)==0) {
                                sscanf(line,"%s%255s",trash,track->track_name);
                                me->general_flags|=ME_FILENAME;
				if((track->i_stream=create_inputstream(track->track_name))==NULL) {
					free_track(track);
					return ERR_ALLOC;
				}
                        }
                        if (strcasecmp(keyword,SD_ENCODING_NAME)==0) {
                                sscanf(line,"%s%10s",trash,track->parser->parser_type->encoding_name);
                                me->description_flags|=MED_ENCODING_NAME;
                                set_media_entity(track->parser->parser_type,track->parser->parser_type->encoding_name);
                        }       
			/*
                        if (strcasecmp(keyword,SD_PRIORITY)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.priority));
                                p->description.flags|=MED_PRIORITY;
                        }
                        if (strcasecmp(keyword,SD_BITRATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(track->parser->parser_type->properties->bit_rate));
                                p->description.flags|=MED_BITRATE;
                        }
                        if (strcasecmp(keyword,SD_PAYLOAD_TYPE)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.payload_type));
                                p->description.flags|=MED_PAYLOAD_TYPE;
                        }
                        if (strcasecmp(keyword,SD_CLOCK_RATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(p->description.clock_rate));
                                p->description.flags|=MED_CLOCK_RATE;
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
	*/	

			/*****START CC****/
		/*	if (strcasecmp(keyword,SD_LICENSE)==0) {
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
	*/
			/********END CC*********/

			
                }
		/*
                if ((res = validate_stream(p,&sd_descr)) != ERR_NOERROR) {
                        return res;
                }
		*/
		track->private_data=me;	
		//
		//
	}while(!eof(r->i_stream->fd));

	r->private_data=sd;

	return RESOURCE_OK;
}

static int sd_probe(Resource *r)
{
	return RESOURCE_OK;
}

static int sd_read_header(Resource *r)
{
	return RESOURCE_OK;
}

static int sd_read_packet(Resource *r)
{
	return RESOURCE_OK;
}

static int sd_close(Resource *r)
{
	return RESOURCE_OK;
}

static int sd_seek(Resource *r, long int time_msec)
{
	return RESOURCE_NOT_SEEKABLE;
}


static InputFormat sd_iformat = {
    "sd",
    sd_init,/*ex parse_SD_file*/
    sd_probe,/*ex validate_stream*/
    sd_read_header,/*return RESOURCE_OK and nothing more*/ 
    sd_read_packet,/*switchs between the different parser reader according to the timestamp. \
		     I.e. 1 video frame mpeg1 (1pkt/40msec) and 1 or 2 mp3 audio pkts (1pkt/26.12msec*/
    sd_close, /*close all media described in sd*/
    sd_seek /*seek all media in sd*/
};


void register_format_sd(Resource *r)
{
	r->format=&sd_iformat;
}
