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

#ifndef _MEDIAINFOH
#define _MEDIAINFOH

	#include <sys/types.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#include <fenice/bufferpool.h>

	#define MAX_DESCR_LENGTH 4096
	#define DIM_VIDEO_BUFFER 5 
	#define DIM_AUDIO_BUFFER 5

	/* Formati di descrizione supportati */	
	
	typedef enum {
		df_SDP_format=0
	} description_format;
		
	typedef enum {stored=0,live} media_source;	

	typedef enum {audio=0,video,application,data,control} media_type;

	typedef enum {undefined=-1,frame=0,sample=1} media_coding;

	typedef enum {
		ME_FILENAME=1,
		/*    disposable   ME=2,  */
    		ME_DESCR_FORMAT=4,
    		ME_AGGREGATE=8,
    		ME_RESERVED=16,
    		ME_FD=32
	} me_flags;
	
	typedef enum {
		SD_FL_TWIN=1,
    		SD_FL_MULTICAST=2,
    		SD_FL_MULTICAST_PORT=4
	} sd_descr_flags;

	typedef enum {
    		MED_MTYPE=1,
    		MED_MSOURCE=2,
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
    		MED_TYPE=8192,
    		MED_FRAME_RATE=16384,
    		MED_BYTE_PER_PCKT=32768,
		/*start CC*/
		MED_LICENSE=65536,
		MED_RDF_PAGE=131072,
		MED_TITLE=262144,
		MED_CREATOR=524288,
		MED_ID3=1048576
		/*end CC*/

   		/*DYN_PAYLOAD_TOKEN    	
		PACKETTIZED=
		PAYLOAD=
		FORMAT=
		CHANNELS=
		COLOR_DEPTH=
		SCREEN_WIDTH=
		SCREEN_HEIGHT=*/
	} me_descr_flags;

	typedef struct _media_entry {
    		me_flags flags;
    		int fd;
		void *stat;
		struct _media_fn *media_handler;
		unsigned int data_chunk;
		
		/*Buffering with bufferpool module*/
    		unsigned char buff_data[4]; // shawill: needed for audio-mp3 live-by-named-pipe /*TODO: to move in mp3.h*/
		unsigned int buff_size; // shawill: needed for audio-mp3 live-by-named-pipe	/*TODO: to move in mp3.h*/	
		OMSBuffer *pkt_buffer; 
		//Consumer now is in RTP_session structure
		//OMSConsumer *cons;
		
		//these time vars have transferred here
		double mtime;
		double mstart;
		double mstart_offset;
		double play_offset; /*chicco. Usefull for random access*/
		double prev_mstart_offset;/*usefull for random access*/
		int rtp_multicast_port; 

		//started has transferred itself here
		//unsigned char started;
		int reserved;
    		char filename[255];
    		char aggregate[50];
    		description_format descr_format;		
    	
		struct {
			/*start CC*/
			char commons_dead[255]; 
              		char rdf_page[255];
			char title[80];
			char author[80];	
			int tag_dim;    
			/*end CC*/
        		me_descr_flags flags;	    	    	    		
        		media_source msource;
        		media_type mtype;
        		int payload_type;
        		int clock_rate;
        		int sample_rate;
        		short audio_channels;
        		int bit_per_sample;
        		char encoding_name[11];	    	
        		media_coding coding_type;
        		int frame_len;
        		int bitrate;
        		int priority;
        		float pkt_len; /*packet length*/
        		float delta_mtime;
        		int frame_rate;
        		int byte_per_pckt;
			
        		/* Not used yet*/
			/*	int encoding;    	
				int color_depth;
				int screen_width;
				int screen_height;*/
    		} description;    	
    		struct _media_entry *next;
	} media_entry;
	
	/* 
	 * \struct media_fn
	 */
	typedef struct _media_fn{
		
		/*
		 * \fn load_media
		 *	\brief Function pointer to load media.
		 *	\param media_entry *me .
		 *
		 */
		int (*load_media)(media_entry *);

		/*
		 * \fn read_media
		 * 	\brief Function pointer to read bitstream.
		 * 	\param media_entry *me .
		 * 	\param uint8 *buffer .
		 * 	\param uint32 *buffer_size .
		 * 	\param double *mtime .
		 * 	\param int *recallme .
		 * 	\param uint8 *marker; 
		 *
		 */	
		int (*read_media)(media_entry *, uint8 *, uint32 *, double *, int *, uint8 *);

		/*
		 * \fn free_media
		 * 	\brief Function pointer to free static_X structure used in read_X.
		 *	\param void *stat .
		 *	\warnings Don't free structures allocated by load_X because it is recalled only if .sd changes.
		 */
		int (*free_media)(void *);
	
	} media_fn;

	typedef struct _SD_descr {
    		char filename[255];
    		media_entry *me_list;
    		time_t last_change;
		struct _SD_descr *next;
		sd_descr_flags flags;
		char multicast[16];
		char ttl[4];
    		char twin[255];
				
	} SD_descr;

	int parse_SD_file (char *object, SD_descr *sd_descr);	
	int get_frame (media_entry *me, double *mtime);
	int validate_stream (media_entry *me,SD_descr **sd_descr);

	int register_media(media_entry *);
	int mediaopen(media_entry *);
	int mediaclose(media_entry *);

	/*****************************************read common utils*************************************************/
	
	/* returns number of bytes readen looking for start-codes, */
	int next_start_code(uint8 *, uint32 *,int fin);
	uint32 random_access(media_entry *me);
	int changePacketLength(float offset, media_entry *me);

	/*********************************************************************************************************************/

	int enum_media(char *object, SD_descr **d);
	// Alloca e restituisce una lista di media disponibili per la URL specificata.

	int media_is_empty(media_entry *me);
	// Restituisce 1 se il media e' vuoto, cioe' non specifica richieste. 0 Altrimenti.
	
	media_entry *default_selection_criterion(media_entry *list);
	// Restituisce un elemento per default, oppure NULL se la lista e' vuota
	
	media_entry *search_media(media_entry *req,media_entry *list);
	// Cerca il media richiesto nella lista
	
	int get_media_entry(media_entry *req,media_entry *list,media_entry **result);
	// Restituisce l'elemento della lista dei media (list) che corrisponde alle caratteristiche specificate (sel).
	// req==NULL oppure req->flags==0 indica che non c'e' una preferenza sul media, ma va scelto quello di default.
	
	//int get_media_descr(char *url,media_entry *req,media_entry *media,char *descr);
	// Reperisce la descrizione del media; shawill: moved in sdp.h
	
	uint32 mediacpy(media_entry **, media_entry **);
	unsigned long msec2tick(double mtime,media_entry *me);
	
	// ID3v2
	int calculate_skip(int byte_value,double *skip,int peso); 
	int read_dim(int file,long int *dim);

#endif
