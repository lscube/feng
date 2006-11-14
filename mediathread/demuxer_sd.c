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

#include <fenice/multicast.h>	/*is_valid_multicast_address */
#include <fenice/rtsp.h>	/*parse_url */
#include <fenice/rtpptdefs.h>	/*payload type definitions */

#include <fenice/demuxer_module.h>

static DemuxerInfo info = {
	"Source Description",
	"sd",
	"OMSP Team",
	"",
	"sd"
};

FNC_LIB_DEMUXER(sd);

typedef struct __FLAGS_DATA {
	struct __DATA {
		int frame_len;	// i need to move it in Parser
		int priority;	//i need to move it. Where? Selector, Track 
		float pkt_len;	//i need to move it in Parser
		int byte_per_pckt;	//i need to move it in Parser
		char aggregate[80];
	} data;
} FlagsData;

// private functions definition:
static int validate_audio_track(Track *);
static int validate_video_track(Track *);
static int validate_track(Resource *);

static int probe(InputStream * i_stream)
{
	char *ext;

	if ((ext = strrchr(i_stream->name, '.')) && (!strcmp(ext, ".sd"))) {
		return RESOURCE_OK;
	}
	return RESOURCE_DAMAGED;
}

static int init(Resource * r)
{
	char keyword[80], line[256], sparam[10];
	Track *track;
	FlagsData *me;
	char object[255], server[255];
	unsigned short port;
	int res;
	char content_base[256] = "", *separator, track_file[256];

	media_source msource = stored;
	FILE *fd;

	MediaProperties props_hints;
	TrackInfo trackinfo;

	fnc_log(FNC_LOG_DEBUG, "SD init function\n");
	fd = fdopen(r->i_stream->fd, "r");

	// content base
	// if ( (seaparator=strrchr(r->i_stream->name, '/')) )
	// using GLib it becomes:
	if ((separator = strrchr(r->i_stream->name, G_DIR_SEPARATOR))) {
		if ((unsigned) (separator - r->i_stream->name + 1) >= sizeof(content_base)) {
			fnc_log(FNC_LOG_ERR, "content base string too long\n");
			return ERR_GENERIC;
		} else {
			strncpy(content_base, r->i_stream->name, separator - r->i_stream->name + 1);
			fnc_log(FNC_LOG_DEBUG, "content base: %s\n", content_base);
		}
	}
	MObject_init(MOBJECT(&props_hints));
	MObject_init(MOBJECT(&trackinfo));
	MObject_0(MOBJECT(&props_hints), MediaProperties);
	MObject_0(MOBJECT(&trackinfo), TrackInfo);

	do {
		*keyword = '\0';
		while (strcasecmp(keyword, SD_STREAM) && !feof(fd)) {
			fgets(line, sizeof(line), fd);
			sscanf(line, "%s", keyword);
			/*
			 *- validate multicast
			 *- validate twin
			 * */
			if (!strcasecmp(keyword, SD_TWIN)) {
				sscanf(line, "%*s%s", r->info->twin);
				parse_url(r->info->twin, server, sizeof(server), &port, object, sizeof (object));	//FIXME
			} else if (!strcasecmp(keyword, SD_MULTICAST)) {
				sscanf(line, "%*s%s", r->info->multicast);
				// if (!is_valid_multicast_address(sd->multicast))
				if (!is_valid_multicast_address(r->info->multicast))
					//strcpy(sd->multicast, DEFAULT_MULTICAST_ADDRESS);
					strcpy(r->info->multicast, DEFAULT_MULTICAST_ADDRESS);
			}
		}
		if (feof(fd))
			return RESOURCE_OK;

		/* Allocate and cast TRACK PRIVATE DATA foreach track.
		 * (in this case track = elementary stream media file)
		 * */
		// memset(keyword,0,sizeof(keyword));
		*keyword = '\0';
		while (strcasecmp(keyword, SD_STREAM_END) && !feof(fd)) {
			fgets(line, sizeof(line), fd);
			sscanf(line, "%s", keyword);
			if (!strcasecmp(keyword, SD_FILENAME)) {
				// SD_FILENAME
				sscanf(line, "%*s%255s", track_file);
				// if ( *track_file == '/')
				if ((*track_file == G_DIR_SEPARATOR)
				    || (separator = strstr(track_file, FNC_PROTO_SEPARATOR)))
					// is filename absolute path or complete mrl?
					trackinfo.mrl = g_strdup(track_file);
				else
					trackinfo.mrl = g_strdup_printf("%s%s", content_base, track_file);

				if ((separator = strrchr(track_file, G_DIR_SEPARATOR)))
					g_strlcpy(trackinfo.name, separator + 1, sizeof(trackinfo.name));
				else
					g_strlcpy(trackinfo.name, track_file, sizeof(trackinfo.name));

#if 0
				if (!(track->i_stream = istream_open(track->info->mrl))) {
					free_track(track, r);
					return ERR_ALLOC;
				}
#endif
			} else if (!strcasecmp(keyword, SD_ENCODING_NAME)) {
				// SD_ENCODING_NAME
				sscanf(line, "%*s%10s", props_hints.encoding_name);
			} else if (!strcasecmp(keyword, SD_PRIORITY)) {
				// SD_PRIORITY // shawill XXX: probably to be moved in properties or infos
				sscanf(line, "%*s %d\n", &me->data.priority);
			} else if (!strcasecmp(keyword, SD_BITRATE)) {
				// SD_BITRATE
				sscanf(line, "%*s %d\n", &props_hints.bit_rate);
			} else if (!strcasecmp(keyword, SD_PAYLOAD_TYPE)) {
				// SD_PAYLOAD_TYPE
				sscanf(line, "%*s %u\n", &props_hints.payload_type);
			} else if (!strcasecmp(keyword, SD_CLOCK_RATE)) {
				// SD_CLOCK_RATE
				sscanf(line, "%*s %u\n", &props_hints.clock_rate);
			} else if (!strcasecmp(keyword, SD_AUDIO_CHANNELS)) {
				// SD_AUDIO_CHANNELS
				sscanf(line, "%*s %hd\n", &props_hints.audio_channels);
			} else if (!strcasecmp(keyword, SD_AGGREGATE)) {
				// SD_AGGREGATE
				sscanf(line, "%*s%50s", me->data.aggregate);
			} else if (!strcasecmp(keyword, SD_SAMPLE_RATE)) {
				// SD_SAMPLE_RATE
				sscanf(line, "%*s%f", &props_hints.sample_rate);
			} else if (!strcasecmp(keyword, SD_BIT_PER_SAMPLE)) {
				// SD_BIT_PER_SAMPLE
				sscanf(line, "%*s%u", &props_hints.bit_per_sample);
			} else if (!strcasecmp(keyword, SD_FRAME_LEN)) {
				// SD_FRAME_LEN
				sscanf(line, "%*s%d", &me->data.frame_len);
			} else if (!strcasecmp(keyword, SD_CODING_TYPE)) {
				// SD_CODING_TYPE
				sscanf(line, "%*s%10s", sparam);
				if (strcasecmp(sparam, "FRAME") == 0)
					props_hints.coding_type = mc_frame;
				else if (strcasecmp(sparam, "SAMPLE") == 0)
					props_hints.coding_type = mc_sample;
			} else if (!strcasecmp(keyword, SD_PKT_LEN)) {
				// SD_PKT_LEN
				sscanf(line, "%*s%f", &me->data.pkt_len);
			} else if (!strcasecmp(keyword, SD_FRAME_RATE)) {
				// SD_FRAME_RATE
				sscanf(line, "%*s%u", &props_hints.frame_rate);
			} else if (!strcasecmp(keyword, SD_BYTE_PER_PCKT)) {
				// SD_BYTE_PER_PCKT
				sscanf(line, "%*s%d", &me->data.byte_per_pckt);
			} else if (!strcasecmp(keyword, SD_MEDIA_SOURCE)) {
				// SD_MEDIA_SOURCE
				sscanf(line, "%*s%10s", sparam);
				if (strcasecmp(sparam, "STORED") == 0)
					msource = stored;
				if (strcasecmp(sparam, "LIVE") == 0)
					msource = live;
			} else if (!strcasecmp(keyword, SD_LICENSE)) {

				/*******START CC********/
				// SD_LICENSE
				sscanf(line, "%*s%s", trackinfo.commons_deed);
			} else if (!strcasecmp(keyword, SD_RDF)) {
				// SD_RDF
				sscanf(line, "%*s%s", trackinfo.rdf_page);
			} else if (!strcasecmp(keyword, SD_TITLE)) {
				// SD_TITLE
				int i = 0;

				while (line[i] != '\n') {
					trackinfo.title[i] = line[i + 7];
					i++;
				}
				trackinfo.title[i] = '\0';
			} else if (!strcasecmp(keyword, SD_CREATOR)) {
				// SD_CREATOR
				int i = 0;

				while (line[i] != '\n') {
					trackinfo.author[i] = line[i + 9];
					i++;
				}
				trackinfo.author[i] = '\0';
			}

			/********END CC*********/
		}		/*end while !STREAM_END or eof */

		if (!(track = add_track(r, &trackinfo, &props_hints)))
			return ERR_ALLOC;

#if 0
		if (me->description_flags & MED_ENCODING_NAME) {
			// MediaProperties *prop = &props_hints;
			// shawill: init parser functions in another way
			// set_media_entity(track->parser->parser_type,track->parser->parser_type->encoding_name);
#    if 0
			if (!(track->parser = mparser_find(props_hints.encoding_name)))
				return ERR_GENERIC;
			if (track->parser->init(&props_hints, &track->parser_private))
				return ERR_GENERIC;
#    endif

#    if 0
			// shawill: just for parser trying:
			{
				uint8 tmp_dst[512];
				double timest;

				fnc_log(FNC_LOG_DEBUG, "[MT] demuxer sd init done.\n");

				track->parser->get_frame(tmp_dst, sizeof(tmp_dst), &timest,
							 track->i_stream, track->properties, track->parser_private);
			}
#    endif
#    if 0
			switch (props_hints.media_type) {
			case MP_audio:
				// audio_spec_prop *prop;
				// prop=malloc(sizeof(audio_spec_prop));        
				// shawill: initialize with calloc
				// prop = calloc(1, sizeof(audio_spec_prop));   
				prop->sample_rate = sample_rate;	/*SamplingFrequency */
				prop->audio_channels = audio_channels;
				prop->bit_per_sample = bit_per_sample;	/*BitDepth */
				if (me->description_flags & MED_BITRATE)
					prop->bit_rate = bit_rate;	/*average if VBR or -1 is not usefull */
				if (me->description_flags & MED_CODING_TYPE)
					prop->coding_type = coding_type;
				if (me->description_flags & MED_PAYLOAD_TYPE)
					prop->payload_type = payload_type;
				if (me->description_flags & MED_CLOCK_RATE)
					prop->clock_rate = clock_rate;

				// shawill: done in add_media_parser:
				// track->parser->properties=prop;
				break;
			case MP_video:
				// video_spec_prop *prop;
				// prop = calloc(1, sizeof(video_spec_prop));   
				// prop->frame_rate;
				if (me->description_flags & MED_BITRATE)
					prop->bit_rate = bit_rate;	/*average if VBR or -1 is not usefull */
				if (me->description_flags & MED_CODING_TYPE)
					prop->coding_type = coding_type;
				if (me->description_flags & MED_PAYLOAD_TYPE)
					prop->payload_type = payload_type;
				if (me->description_flags & MED_CLOCK_RATE)
					prop->clock_rate = clock_rate;

				// shawill: done in add_media_parser:
				// track->parser->parser_type->properties=prop;
				break;
				// TODO other media types, if needed.
			default:
				fnc_log(FNC_LOG_ERR, "It's impossible to identify media_type: audio, video ...\n");
				free_track(track, r);
				r->num_tracks--;
				return ERR_ALLOC;
				break;
			}
#    endif
		}
#endif
		track->msource = msource;	// XXX shawill: this variable should not be set here
		//FIXME validate???
	} while (!feof(fd));

	return RESOURCE_OK;
}

static int read_header(Resource * r)
{
	return RESOURCE_OK;
}

static int read_packet(Resource * r)
{
	return RESOURCE_OK;
}

static int seek(Resource * r, long int time_msec)
{
	return RESOURCE_NOT_SEEKABLE;
}

static int uninit(Resource * r)
{
	return RESOURCE_OK;
}
