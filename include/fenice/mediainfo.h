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
		MED_LICENCE=65536,
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
	
	typedef struct {                                 /* MPEG video specific headers */
		unsigned int ffc:3;
        	unsigned int ffv:1;
        	unsigned int bfc:3;                      /* |MBZ:5|T:1|TR:10|AN:1|N:1|S:1|B:1|E:1|P:3|FBV:1|BFC:3|FFV:1|FFC:3| */
        	unsigned int fbv:1;
        	unsigned int p:3;
        	unsigned int e:1;
        	unsigned int b:1;
        	unsigned int s:1;
        	unsigned int n:1;
        	unsigned int an:1;
        	unsigned int tr:10;
        	unsigned int t:1;
        	unsigned int mbz:5;
	} video_spec_head1;

	typedef struct {
		unsigned int d:1;                        /* |X:1|E:1|F[0,0]:4|F[0,1]:4|F[1,0]:4|F[1,1]:4|DC:2|PS:2|T:1|P:1|C:1|Q:1|V:1|A:1|R:1|H:1|G:1|D:1| */
        	unsigned int g:1;
        	unsigned int h:1;
        	unsigned int r:1;
        	unsigned int a:1;
       	 	unsigned int v:1;
        	unsigned int q:1;
        	unsigned int c:1;
        	unsigned int p:1;
        	unsigned int t:1;
        	unsigned int ps:2;
        	unsigned int dc:2;
        	unsigned int f11:4;
        	unsigned int f10:4;
        	unsigned int f01:4;
        	unsigned int f00:4;
        	unsigned int e:1;
        	unsigned int x:1;
	} video_spec_head2;


	typedef enum {MPEG_1,MPEG_2,TO_PROBE} standard;

	typedef struct {
		unsigned int msb;
		unsigned int scr;
	} SCR;


	typedef struct {
		unsigned int msb;
		unsigned int pts;
	} PTS;
	
	typedef struct {
		unsigned int bufsize;
		long pkt_sent;
		int current_timestamp; 
		int next_timestamp;
	} static_H26L;
	
	typedef struct {
		video_spec_head1 vsh1;
		video_spec_head2 vsh2;
		unsigned char final_byte;
		char temp_ref;
		char hours;
		char minutes;
		char seconds;
		char picture;
		unsigned long data_total;  
		standard std;
		int fragmented;
		video_spec_head1 vsh1_aux;       				/* without modifying other variables */
		video_spec_head2 vsh2_aux;
	} static_MPEG_video;
	
	typedef struct {
		unsigned char final_byte;
		PTS pts;
		PTS next_pts;
		PTS dts;
		PTS next_dts;
		PTS pts_audio;
		SCR scr;
		unsigned int data_total;
		int offset_flag;
		int offset;
		int new_dts;
	} static_MPEG_system;
	
	
	typedef struct _media_entry {
    		me_flags flags;
    		int fd;
		void *stat;
		unsigned int data_chunk;
		
		/*Buffering with bufferpool module*/
    		unsigned char buff_data[4]; // shawill: needed for live-by-named-pipe
		unsigned int buff_size; // shawill: needed for live-by-named-pipe		
		OMSBuffer *pkt_buffer; 
		//Consumer now is in RTP_session structure
		//OMSConsumer *cons;
		
		//these time vars have transferred here
		double mtime;
		double mstart;
		double mstart_offset;
		
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
	
	typedef struct _SD_descr {
    		char filename[255];
    		int is_described;
    		description_format descr_format;
    		char descr[MAX_DESCR_LENGTH];
    		media_entry *me_list;
    		struct _SD_descr *next;
	} SD_descr;

	int parse_SD_file (char *object, SD_descr *sd_descr);	
	int get_frame (media_entry *me, double *mtime);
	int validate_stream (media_entry *me);

	int load_MPA (media_entry *me);
	int load_GSM (media_entry *me);
	int load_L16 (media_entry *me);
	int load_MP2T (media_entry *me);
	int load_MPV (media_entry *me);
	// read specific formats	
	int read_PCM (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime);
	int read_MP3 (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime);
	int read_GSM (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime);
	
	int read_H26L (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime, int *recallme);
	int read_MPEG_video (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime, int *recallme);
	int read_MPEG_system (media_entry *me, uint8 *buffer, uint32 *buffer_size, double *mtime, int *recallme);

	/*****************************************read MPEG utils*************************************************/
	
	/* returns number of bytes readen looking for start-codes, */
	int next_start_code(uint8 *, uint32 *,int fin);
	/* reads sequence header */
	int read_seq_head(uint8 *, uint32 *, int fin, char *final_byte, standard std);
	/* reads GOP header */
	int read_gop_head(uint8 *, uint32 *, int fin, char *final_byte, char *hours, char *minutes, char *seconds, char *picture, standard std);
	/* reads picture head */
	int read_picture_head(uint8 *, uint32 *, int fin, char *final_byte, char *temp_ref, video_spec_head1* vsh1, standard std);
	/* reads a slice */
	int read_slice(uint8 *, uint32 *, int fin, char *final_byte);
	/* If the sequence_extension occurs immediately */
	int probe_standard(uint8 *, uint32 *,int fin, standard *std);
	/* reads picture coding extension */
	int read_picture_coding_ext(uint8 *, uint32 *, int fin, char *final_byte,video_spec_head2* vsh2);
	/* reads pack header */
	int read_pack_head(uint8 *, uint32 *e, int fin, unsigned char *final_byte, SCR *scr);
	/* reads packet header */
	int read_packet_head(uint8 *, uint32 *, int fin, unsigned char *final_byte, int *time_set, PTS *pts, PTS *dts, int *dts_present, PTS *pts_audio);
	/* reads a packet */
	int read_packet(uint8 *, uint32 *, int fin, unsigned char *final_byte);
	int changePacketLength(float offset, media_entry *me);
	
	/*********************************************************************************************************************/

	int enum_media(char *object,media_entry **list);
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
