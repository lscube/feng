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

#include <string.h>
#include <stdio.h>

#include <fenice/demuxer.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <fenice/multicast.h>	/*is_valid_multicast_address*/
#include <fenice/rtsp.h>	/*parse_url*/
#include <fenice/rtpptdefs.h>	/*payload type definitions*/

#include <fenice/demuxer_module.h>

static DemuxerInfo info = {
	"Source Description",
	"sd",
	"OMSP Team",
	"",
	"sd"
};

FNC_LIB_DEMUXER(sd);

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
		char aggregate[80];
	} data;
} FlagsData;    	

/*----------------------*/
static int validate_audio_track(Track *t)
{
        RTP_static_payload pt_info;
	FlagsData *me;
	// shawill: audio_spec_prop *prop;
	MediaProperties *prop;

	me=(FlagsData *)t->private_data;

	// shawill: prop=(audio_spec_prop *)t->parser->parser_type->properties;
	prop=t->properties;

        if (prop->payload_type>=96) {
                // Basic information needed for a dynamic payload type (96-127)
                if (!(me->description_flags & MED_ENCODING_NAME)) {
                        return ERR_PARSE;
                }
                if (!(me->description_flags & MED_CLOCK_RATE)) {
                        return ERR_PARSE;
                }
        }
        else {
                // Set payload type for well-kwnown encodings and some default configurations if i know them
                // see include/fenice/rtpptdefs.h
                pt_info=RTP_payload[prop->payload_type];
                // shawill: strcpy(t->parser->parser_type->encoding_name,pt_info.EncName);
                strcpy(prop->encoding_name, pt_info.EncName);
               	prop->clock_rate=pt_info.ClockRate;
                prop->audio_channels=pt_info.Channels;
                prop->bit_per_sample=pt_info.BitPerSample;
                prop->coding_type=pt_info.Type;
                //prop->pkt_len=pt_info.PktLen;
                me->description_flags|=MED_ENCODING_NAME;
                me->description_flags|=MED_CLOCK_RATE;
                me->description_flags|=MED_AUDIO_CHANNELS;
                me->description_flags|=MED_BIT_PER_SAMPLE;
                me->description_flags|=MED_CODING_TYPE;
                //p->description.flags|=MED_PKT_LEN;
        }
	return ERR_NOERROR;
}

static int validate_video_track(Track *t)
{
        RTP_static_payload pt_info;
	FlagsData *me;
	// shawill: video_spec_prop *prop;
	MediaProperties *prop;

	me=(FlagsData *)t->private_data;

	// shawill: prop=(video_spec_prop *)t->parser->parser_type->properties;
	prop=t->properties;

        if (prop->payload_type>=96) {
                // Basic information needed for a dynamic payload type (96-127)
                if (!(me->description_flags & MED_ENCODING_NAME)) {
                        return ERR_PARSE;
                }
                if (!(me->description_flags & MED_CLOCK_RATE)) {
                        return ERR_PARSE;
                }
        }
        else {
                // Set payload type for well-kwnown encodings and some default configurations if i know them
                // see include/fenice/rtpptdefs.h
                pt_info=RTP_payload[prop->payload_type];
                // shawill: strcpy(t->parser->parser_type->encoding_name,pt_info.EncName);
                strcpy(prop->encoding_name, pt_info.EncName);
               	prop->clock_rate=pt_info.ClockRate;
                prop->coding_type=pt_info.Type;
                //prop->pkt_len=pt_info.PktLen;
                me->description_flags|=MED_ENCODING_NAME;
                me->description_flags|=MED_CLOCK_RATE;
                me->description_flags|=MED_CODING_TYPE;
                //p->description.flags|=MED_PKT_LEN;
        }
	return ERR_NOERROR;
}

static int validate_track(Resource *r)
{
	
	FlagsData *me;
	int i=r->num_tracks-1;

	me=(FlagsData *)r->tracks[i]->private_data;

        if (!(me->general_flags & ME_FILENAME)) {
                return ERR_PARSE;
        }
        if (!(me->description_flags & MED_PAYLOAD_TYPE)) {
                return ERR_PARSE;
        }
        if (!(me->description_flags & MED_PRIORITY)) {
                return ERR_PARSE;
        }

	if(!strcmp(r->tracks[i]->properties->media_type, "audio"))
		return validate_audio_track(r->tracks[i]);
	else if(!strcmp(r->tracks[i]->properties->media_type,"audio"))
		return validate_video_track(r->tracks[i]);
	else
		return ERR_NOERROR;
/*

*/
	/*
        res=register_media(p);
        if(res==ERR_NOERROR)
                return p->media_handler->load_media(p);
        else
                return res;
	*/
}

static int probe(char *filename)
{
	char *ext;
	
	if ( (ext=strrchr(filename, '.')) && (!strcmp(ext, ".sd"))) {
		return RESOURCE_OK;
	}
	return RESOURCE_DAMAGED;
}

static int init(Resource *r)
{

	char keyword[80],line[80],trash[80],sparam[10];
	Track *track;
	SD_descr *sd;
	FlagsData *me;
        char object[255], server[255];
        unsigned short port;
	int res;

	/*--*/
	int32 bit_rate; /*average if VBR or -1 is not usefull*/ 
	MediaCoding coding_type = mc_undefined;
	uint32 payload_type; 
	uint32 clock_rate;
	float sample_rate;/*SamplingFrequency*/
	short audio_channels;
	uint32 bit_per_sample;/*BitDepth*/
	uint32 frame_rate;
	FILE *fd;
	/*--*/
	fd=fdopen(r->i_stream->fd,"r");
	/*Allocate Resource PRIVATE DATA and cast it*/
	if((sd=malloc(sizeof(SD_descr)))==NULL)
		return ERR_ALLOC;

	/**
	 * parse sd file.*/

	do{
		memset(keyword,0,sizeof(keyword));
		while (strcasecmp(keyword,SD_STREAM)!=0 && !feof(fd)) {
		        fgets(line,80,fd);
		        sscanf(line,"%s",keyword);
			/*
			*- validate multicast
			*- validate twin
			* */
			if (strcasecmp(keyword,SD_TWIN)==0){ 
		                sscanf(line,"%s%s",trash,r->info->twin);
        			if(parse_url(r->info->twin, server, &port, object))
					sd->flags|=SD_FL_TWIN;
			}
			if (strcasecmp(keyword,SD_MULTICAST)==0){ 
		                sscanf(line,"%s%s",trash,sd->multicast);
				sd->flags|=SD_FL_MULTICAST;
			        if(!is_valid_multicast_address(sd->multicast))
       				         strcpy(sd->multicast,DEFAULT_MULTICAST_ADDRESS);
			}
		}
		if (feof(fd)) 
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
                while (strcasecmp(keyword,SD_STREAM_END)!=0 && !feof(fd)) {
                        fgets(line,80,fd);
                        sscanf(line,"%s",keyword);
                        if (strcasecmp(keyword,SD_FILENAME)==0) {
                                sscanf(line,"%s%255s",trash,track->track_name);
                                me->general_flags|=ME_FILENAME;
				if((track->i_stream=istream_open(track->track_name))==NULL) {
					free_track(track);
					return ERR_ALLOC;
				}
                        }
                        if (strcasecmp(keyword,SD_ENCODING_NAME)==0) {
                                sscanf(line,"%s%10s",trash,track->properties->encoding_name);
                                me->description_flags|=MED_ENCODING_NAME;
			}       
                        if (strcasecmp(keyword,SD_PRIORITY)==0) {
                                sscanf(line,"%s %d\n",trash,&(me->data.priority));
                                me->description_flags|=MED_PRIORITY;
                        }
                        if (strcasecmp(keyword,SD_BITRATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(/*track->parser->parser_type->properties->*/bit_rate));
                                me->description_flags|=MED_BITRATE;
                        }
                        if (strcasecmp(keyword,SD_PAYLOAD_TYPE)==0) {
                                sscanf(line,"%s %d\n",trash,&(payload_type));
                                me->description_flags|=MED_PAYLOAD_TYPE;
                        }
                        if (strcasecmp(keyword,SD_CLOCK_RATE)==0) {
                                sscanf(line,"%s %d\n",trash,&(clock_rate));
                                me->description_flags|=MED_CLOCK_RATE;
                        }
                        if (strcasecmp(keyword,SD_AUDIO_CHANNELS)==0) {
                                sscanf(line,"%s %hd\n",trash,&(audio_channels));
                                me->description_flags|=MED_AUDIO_CHANNELS;
                        }       
                        if (strcasecmp(keyword,SD_AGGREGATE)==0) {
                                sscanf(line,"%s%50s",trash,me->data.aggregate);
                                me->general_flags|=ME_AGGREGATE;
                        }       
                        if (strcasecmp(keyword,SD_SAMPLE_RATE)==0) {
                                sscanf(line,"%s%f",trash,&(sample_rate));
                                me->description_flags|=MED_SAMPLE_RATE;
                        }       
                        if (strcasecmp(keyword,SD_BIT_PER_SAMPLE)==0) {
                                sscanf(line,"%s%d",trash,&(bit_per_sample));
                                me->description_flags|=MED_BIT_PER_SAMPLE;
                        }                                       
                        if (strcasecmp(keyword,SD_FRAME_LEN)==0) {
                                sscanf(line,"%s%d",trash,&(me->data.frame_len));
                                me->description_flags|=MED_FRAME_LEN;
                        }                                                       
                        if (strcasecmp(keyword,SD_CODING_TYPE)==0) {
                                sscanf(line,"%s%10s",trash,sparam);
                                me->description_flags|=MED_CODING_TYPE;
                                if (strcasecmp(sparam,"FRAME")==0)
                                        coding_type=mc_frame;
				else if (strcasecmp(sparam,"SAMPLE")==0)
                                        coding_type=mc_sample;
                        }
                        if (strcasecmp(keyword,SD_PKT_LEN)==0) {
                                sscanf(line,"%s%f",trash,&(me->data.pkt_len));
                                me->description_flags|=MED_PKT_LEN;
                        }
                        if (strcasecmp(keyword,SD_FRAME_RATE)==0) {
                                sscanf(line,"%s%d",trash,&(frame_rate));
                                me->description_flags|=MED_FRAME_RATE;
                        }
                        if (strcasecmp(keyword,SD_BYTE_PER_PCKT)==0) {
                                sscanf(line,"%s%d",trash,&(me->data.byte_per_pckt));
                                me->description_flags|=MED_BYTE_PER_PCKT;
                        }              
			                   
			if (strcasecmp(keyword,SD_MEDIA_SOURCE)==0) {
                                sscanf(line,"%s%10s",trash,sparam);
                                me->description_flags|=MED_MSOURCE;
                                if (strcasecmp(sparam,"STORED")==0) 
                                        track->msource=stored;
                                if (strcasecmp(sparam,"LIVE")==0) 
                                        track->msource=live;
                        }
			/*****START CC****/
			if (strcasecmp(keyword,SD_LICENSE)==0) {
				sscanf(line,"%s%s",trash,(track->track_info->commons_dead));
				me->description_flags|=MED_LICENSE;
		        }
			if (strcasecmp(keyword,SD_RDF)==0) {
				sscanf(line,"%s%s",trash,(track->track_info->rdf_page));
				me->description_flags|=MED_RDF_PAGE;
		        }                     
			if (strcasecmp(keyword,SD_TITLE)==0) {
				int i=7;
				int j=0;
				while(line[i]!='\n') {
			    		track->track_info->title[j]=line[i];
			    		i++;
			    		j++;
			   	}
			  	track->track_info->title[j]='\0';  
			  	me->description_flags|=MED_TITLE;
			}  
			 
			if (strcasecmp(keyword,SD_CREATOR)==0){
				int i=9;
				int j=0;
				while(line[i]!='\n')
			  	{
					track->track_info->author[j]=line[i];
			    		i++;
			    		j++;
			   	}
				track->track_info->author[j]='\0';  
			  	me->description_flags|=MED_CREATOR;
			}                         
			/********END CC*********/
                }/*end while !STREAM_END or eof*/

		if(me->description_flags & MED_ENCODING_NAME) {
			MediaProperties *prop = track->properties;
			// shawill: init parser functions in another way
                	// set_media_entity(track->parser->parser_type,track->parser->parser_type->encoding_name);
			if(!strcmp(track->properties->media_type, "audio")) {
				// audio_spec_prop *prop;
				// prop=malloc(sizeof(audio_spec_prop));	
				// shawill: initialize with calloc
				// prop = calloc(1, sizeof(audio_spec_prop));	
				prop->sample_rate=sample_rate;/*SamplingFrequency*/
				prop->audio_channels=audio_channels;
				prop->bit_per_sample=bit_per_sample;/*BitDepth*/
				if(me->description_flags & MED_BITRATE)
					prop->bit_rate=bit_rate; /*average if VBR or -1 is not usefull*/ 
				if(me->description_flags & MED_CODING_TYPE)
					prop->coding_type=coding_type; 
				if(me->description_flags & MED_PAYLOAD_TYPE)
					prop->payload_type=payload_type; 
				if(me->description_flags & MED_CLOCK_RATE )
					prop->clock_rate=clock_rate;
				
				// shawill: done in add_media_parser:
				// track->parser->properties=prop;
			}
			if(!strcmp(track->properties->media_type, "video")) {
				// video_spec_prop *prop;
				// prop = calloc(1, sizeof(video_spec_prop));	
				// prop->frame_rate;
				if(me->description_flags & MED_BITRATE)
					prop->bit_rate=bit_rate; /*average if VBR or -1 is not usefull*/ 
				if(me->description_flags & MED_CODING_TYPE)
					prop->coding_type=coding_type; 
				if(me->description_flags & MED_PAYLOAD_TYPE)
					prop->payload_type=payload_type; 
				if(me->description_flags & MED_CLOCK_RATE )
					prop->clock_rate=clock_rate;
				
				// shawill: done in add_media_parser:
				// track->parser->parser_type->properties=prop;
			}
		}
		else {
			fnc_log(FNC_LOG_ERR,"It's impossible to identify media_type: audio, video ...\n");
			free_track(track);
			r->num_tracks--;
			return ERR_ALLOC;
		}
		track->private_data=me;	
                if ((res = validate_track(r)) != ERR_NOERROR) {
			free_track(track);
			r->num_tracks--;
                        return res;
                }
	}while(!feof(fd));
	r->private_data=sd;

	return RESOURCE_OK;
}

static int read_header(Resource *r)
{
	return RESOURCE_OK;
}

static int read_packet(Resource *r)
{
	return RESOURCE_OK;
}

static int seek(Resource *r, long int time_msec)
{
	return RESOURCE_NOT_SEEKABLE;
}

static int uninit(Resource *r)
{
	return RESOURCE_OK;
}

