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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <fenice/mpeg.h>
#include <fenice/mpeg_utils.h>
#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>

static MediaParserInfo info = {
	"MPV",
	"V"
};

FNC_LIB_MEDIAPARSER(mpv);

/*see ISO/IEC 11172-2:1993 and ISO/IEC 13818-2:1995 (E)*/
/*prefix*/
#define START_CODE 0x000001

/*value*/
#define PICTURE_START_CODE 0x00
#define SLICE_START_CODE /*0x01 to 0xAF*/
#define USER_DATA_START_CODE 0xB2
#define SEQ_START_CODE 0xB3
#define SEQ_ERROR_CODE 0xB4
#define EXT_START_CODE 0xB5 
#define SEQ_END_CODE 0xB7
#define GOP_START_CODE 0xB8

typedef struct _MPV_DATA{
	uint32 is_buffered;
	uint8 buf[4];
	video_spec_head1 *vsh1;
	video_spec_head2 *vsh2;
	uint32 hours;
	uint32 minutes;
	uint32 seconds;
	uint32 pictures;
	uint32 temp_ref; /* pictures'count mod 1024*/
	uint32 picture_coding_type;
	standard std;
}mpv_data;

static int seq_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, MediaProperties *properties, mpv_data *mpeg_video);
static int read_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/);
static int seq_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/);
static int ext_and_user_data(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/);
static int gop_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video);
static int picture_coding_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video);
static int picture_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video);
static int slice(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video);

static float FrameRateCode[] = {
		0.000,
		23.976,
		24.000,
		25.000,
		29.970,
		30.000,
		50.000,
		59.940,
		60.000,
		0.000,
		0.000,
		0.000,
		0.000,
		0.000,
		0.000
	};

static float AspectRatioCode[] = {		/* value 	height/width	video source		*/
		0.0000,                         /*  0		forbidden				*/
		1.0000,	                        /*  1		1,0000		pc VGA			*/
		0.6735,                         /*  2		0,6735					*/
		0.7031,	                        /*  3		0,7031		16:9 625 lines		*/
		0.7615,                         /*  4		0,7615					*/
		0.8055,                         /*  5		0,8055					*/
		0.8437,	                        /*  6		0,8437		16:9 525 lines		*/
		0.8935,                         /*  7		0,8935					*/
		0.9157,	                        /*  8		0,9157		CCIR Rec.601 625 lines	*/
		0.9815,                         /*  9		0,9815					*/
		1.0255,                         /*  10		1,0255					*/
		1.0695,                         /*  11		1,0695					*/
		+1.0950,	                        /*  12		1,0950		CCIR Rec.601 525 lines	*/
		1.1575,                         /*  13		1,1575					*/
		1.2015,                         /*  14		1,2015					*/
		0.0000                          /*  15		reserved				*/
	};                                      /*							*/

/*mediaparser_module interface implementation*/
static int init(MediaProperties *properties, void **private_data)
{
	mpv_data *mpeg_video;
	*private_data = malloc(sizeof(mpv_data));
	mpeg_video = (mpv_data *)(*private_data);
	mpeg_video->vsh1 = malloc(sizeof(video_spec_head1));
	mpeg_video->vsh2 = malloc(sizeof(video_spec_head2));

	return 0;
}

#define GET_UNTIL_PICTURE_START_CODE \
	do { \
		while ( !((buf_aux[0] == 0x00) && (buf_aux[1]==0x00) && (buf_aux[2]==0x01))) { \
    			dst[count]=buf_aux[0]; \
			count++; \
			buf_aux[0]=buf_aux[1]; \
			buf_aux[1]=buf_aux[2]; \
      			if ( istream_read(1,&buf_aux[2],istream) < 1) { \
    				for (i=0;i<3;i++) { \
    					dst[count]=buf_aux[i]; \
      					count++; \
    				} \
				mpeg_video->is_buffered=0; \
				return count; \
			} \
  	  	} \
		ret=istream_read(1, &final_byte, istream); \
		mpeg_video->is_buffered=1; \
		mpeg_video->buf[0]=buf_aux[0]; \
		mpeg_video->buf[1]=buf_aux[1]; \
		mpeg_video->buf[2]=buf_aux[2]; \
		mpeg_video->buf[2]=final_byte; \
		buf_aux[0]=buf_aux[1]; \
		buf_aux[1]=buf_aux[2]; \
		buf_aux[2]=final_byte; \
	} while(final_byte!=PICTURE_START_CODE && count < dst_nbytes); 

static int get_frame2(uint8 *dst, uint32 dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data)
{
	uint32 count=0, ret;
	uint8 final_byte;
	uint8 buf_aux[3];                                               	
	int i;
	mpv_data *mpeg_video;
	uint32 was_buffered;
	uint8 *trash;

	mpeg_video=(mpv_data *)private_data;
	was_buffered=mpeg_video->is_buffered;	
		
	
	if(was_buffered) {
		dst[0]=mpeg_video->buf[0];
		dst[1]=mpeg_video->buf[1];
		dst[2]=mpeg_video->buf[2];
		dst[3]=mpeg_video->buf[3];
		count=4;
	}
	if ( istream_read(3, buf_aux, istream) <3) 
		return -1;
		
	GET_UNTIL_PICTURE_START_CODE;
	if(count >= dst_nbytes)
		return count;
	
	if(!was_buffered) {
		buf_aux[0]=buf_aux[1];
		buf_aux[1]=buf_aux[2];
      		if ( istream_read(1,&buf_aux[2],istream) < 1) 
			return count;
		GET_UNTIL_PICTURE_START_CODE;

		if(count >= dst_nbytes)
			return count;
	}

	trash=malloc(1024); /* i call packetize to fill the time dates*/
	ret=packetize(trash, 1024, dst, count, properties, private_data);
	free(trash);
		
	// (*timestamp) = (mpeg_video->hours * 3.6e6) + (mpeg_video->minutes * 6e4) + (mpeg_video->seconds * 1000) +  (mpeg_video->temp_ref*1024*1000/properties->frame_rate) + (mpeg_video->pictures*1000/properties->frame_rate); // milliseconds
	(*timestamp) = (mpeg_video->hours * 3.6e3) + (mpeg_video->minutes * 60) + (mpeg_video->seconds) +  (mpeg_video->temp_ref*1024/properties->frame_rate) + (mpeg_video->pictures/properties->frame_rate); // seconds

	return count;
}


/*see RFC 2250: RTP Payload Format for MPEG1/MPEG2 Video*/
#ifdef MPEG2VSHE
	#define VSHCPY  vsh_tmp = (uint8 *)(mpeg_video->vsh1); \
        		init_dst[0] = vsh_tmp[3]; \
        		init_dst[1] = vsh_tmp[2]; \
        		init_dst[2] = vsh_tmp[1]; \
        		init_dst[3] = vsh_tmp[0]; \
        		if (s->std == MPEG_2 && !flag) { \
        		        vsh_tmp = (char *)(mpeg_video->vsh2); \
				init_dst[4] = vsh_tmp[3]; \
        	       		init_dst[5] = vsh_tmp[2]; \
        	       		init_dst[6] = vsh_tmp[1]; \
        	        	init_dst[7] = vsh_tmp[0]; \
       		 	} 
#else
	#define VSHCPY  vsh_tmp = (uint8 *)(mpeg_video->vsh1); \
        		init_dst[0] = vsh_tmp[3]; \
        		init_dst[1] = vsh_tmp[2]; \
        		init_dst[2] = vsh_tmp[1]; \
        		init_dst[3] = vsh_tmp[0]; 

#endif

#define SHFRET	if (ret!=-1) { \
			dst_remained-=ret; \
			src_remained-=ret; \
			dst+=ret; \
			src+=ret; \
			final_byte=src[3]; \
		}
/*
 * A possible use is:
 *
 get_frame2();
 do {
	ret=packetize(dst, dst_nbytes, src, src_nbytes, properties, *private_data);
	src+=ret;
	src_nbytes-=ret;
	rtp_marker_bit = !(src_nbytes>0);
	// use dst
 } while(src_nbytes);

 *
 */
static int packetize(uint8 *dst, uint32 dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data)
{
	int ret=0;
	int dst_remained=dst_nbytes;
	int src_remained=src_nbytes;
	uint8 final_byte;
	mpv_data *mpeg_video;
	uint8* init_dst;
	uint8 *vsh_tmp;

	init_dst=dst;
	mpeg_video=(mpv_data *)private_data;
	mpeg_video->vsh1->s=0;/*sequence header is not present, but it is set to 1 if seq_head is called*/ 
	mpeg_video->vsh1->b=0;/*begining of slice*/
	mpeg_video->vsh1->e=0;/*end of slice*/

	if (mpeg_video->std == MPEG_2)  {/*the first time is TO_PROBE, so for the first time is it impossible to use the optional vsh2*/
		#ifdef MPEG2VSHE
		dst += 8;
		dst_remained -= 8;
		mpeg_video->vsh1->t=1;
		#else
		*dst += 4;
		dst_remained -= 4;
		mpeg_video->vsh1->t=0;
		#endif                               				/* bytes for the video specific header */
	} else {
        	*dst += 4;
		dst_remained -= 4;
		mpeg_video->vsh1->t=0;
	}
	do{
		ret=next_start_code2(dst,dst_remained,src,src_remained);
		if(ret==-1) {
			VSHCPY;
			return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
		}
		SHFRET;

		if(final_byte==SEQ_START_CODE) {
			ret=seq_head(dst,dst_remained,src,src_remained,properties,mpeg_video);
			if(ret==-1) {
				VSHCPY;
				return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
			}
			SHFRET;
			if(final_byte==EXT_START_CODE) {
				/*means MPEG2*/
				mpeg_video->std=MPEG_2;
				ret=seq_ext(dst,dst_remained,src,src_remained/*,mpeg_video*/);
				if(ret==-1) {
					VSHCPY;
					return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
				}
				SHFRET;
			}
			else
				mpeg_video->std=MPEG_1;
		}
		else if(final_byte==EXT_START_CODE) {
			ret=read_ext(dst,dst_remained,src,src_remained/*,mpeg_video*/);
			if(ret==-1) {
				VSHCPY;
				return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
			}
			SHFRET;
		}
		else if(final_byte==GOP_START_CODE) {
			ret=gop_head(dst,dst_remained,src,src_remained,mpeg_video);
			if(ret==-1) {
				VSHCPY;
				return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
			}
			SHFRET;
		}
		else if(final_byte==USER_DATA_START_CODE) {
			ret=ext_and_user_data(dst,dst_remained,src,src_remained/*,mpeg_video*/);
			if(ret==-1) {
				VSHCPY;
				return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
			}
			SHFRET;
		}
		else if(final_byte==PICTURE_START_CODE) {
			ret=picture_head(dst,dst_remained,src,src_remained,mpeg_video);
			if(ret==-1) {
				VSHCPY;
				return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
			}
			SHFRET;
			if(mpeg_video->std == MPEG_2 && final_byte== EXT_START_CODE) {
				ret=picture_coding_ext(dst,dst_remained,src,src_remained,mpeg_video);
				if(ret==-1) {
					VSHCPY;
					return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
				}
				SHFRET;
			}
			do {
				ret=slice(dst,dst_remained,src,src_remained, mpeg_video);
				SHFRET;
			 } while(ret!=-1 && final_byte>=0x01 && final_byte<=0xAF /*SLICE_START_CODE*/);
		}
		else if(final_byte>=0x01 && final_byte<=0xAF /*SLICE_START_CODE*/) {
			if(ret!=0 ){ 
				/*means that the end of a fragmented slice is arreached*/
				VSHCPY;
				return ret;
			}
			else {
			 	do {
					ret=slice(dst,dst_remained,src,src_remained, mpeg_video);
					SHFRET;
				 } while(ret!=-1 && final_byte>=0x01 && final_byte<=0xAF /*SLICE_START_CODE*/);
			}
		}
		else if(final_byte==SEQ_END_CODE) {
			/*adding SEQ_END_CODE*/	
			if(dst_remained>=4) {
				dst[0]=0x00;
				dst[1]=0x00;
				dst[2]=0x01;
				dst[3]=SEQ_END_CODE;
			}
			else {
				/*i must send another packet??!?*/
			}
			/*in this case the video sequence should be finished, so min(dst_nbytes,src_nbytes) = src_nbytes*/
		}
		else {
			/*unknow start_code*/
			dst[0]=0x00;
			dst[1]=0x00;
			dst[2]=0x01;
			dst[3]=final_byte;
			dst+=4;
			src+=4;
			dst_remained-=4;
			src_remained-=4;
		 }
	}while(ret!=-1);
	
	VSHCPY;
	return min(dst_nbytes,src_nbytes); /*if src_nbytes => marker=1 because src contains a frame*/
}

static int uninit(void *private_data)
{
	mpv_data *mpeg_video;
	mpeg_video=(mpv_data *)private_data;
	free(mpeg_video->vsh1);
	free(mpeg_video->vsh2);
	free(private_data);
	return 0;
}

/*usefull function to parse*/
static int seq_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, MediaProperties *properties, mpv_data *mpeg_video)
{
	int count=0;
	int i;
	int bt;
	uint32 off;
	uint8 final_byte;
	/*
	 *read bitstream and increment dst and src, decrement dst_remained and src_remained
	 * */

	mpeg_video->vsh1->s=1; 
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;
	off=0;
	properties->PixelWidth=get_field(src,12,&off);
	off=12;
	properties->PixelHeight=get_field(src,12,&off);
	for(i=0;i<3;i++)
		dst[i]=src[i]; 
	src+=3; 
	dst+=3;
	count+=3;
	dst_remained-=3;
	src_remained-=3;
	off=0;
	properties->AspectRatio=AspectRatioCode[get_field(src,4,&off)];
	off=4;
	properties->frame_rate=FrameRateCode[get_field(src,4,&off)];
	dst[0]=src[0]; 
	src+=1; 
	dst+=1;
	count+=1;
	dst_remained-=1;
	src_remained-=1;
	off=0;
	bt=get_field(src,18,&off);
	properties->bit_rate=(bt<262143)?(400*bt):0; /*if 0 -> VBR*/
	off=18;
	for(i=0;i<3;i++)
		dst[i]=src[i]; 
	src+=3; 
	dst+=3;
	count+=3;
	dst_remained-=3;
	src_remained-=3;
	
	count+=bt=next_start_code2(dst,dst_remained,src,src_remained);
	if(bt==-1)
		return -1;
	if(mpeg_video->std==MPEG_1) {
		dst_remained-=bt;
		src_remained-=bt;
		dst+=bt;
		src+=bt;
		final_byte=src[3];
		if(final_byte==EXT_START_CODE) {
			count+=bt=read_ext(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return  -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			final_byte=src[3];
		}
		if(final_byte==USER_DATA_START_CODE) {
			count+=bt=ext_and_user_data(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			//final_byte=src[3];
		}
	}
	return count;
}

static int read_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/)
{
	int count=0;
	int i;
	/*
	 *read bitstream and increment dst and src, decrement dst_remained and src_remained
	 * */
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;
	count+=i=next_start_code2(dst,dst_remained,src,src_remained);
	if(i==-1)
		return -1;
	return count;
}


static int seq_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/)
{
	return read_ext(dst, dst_remained, src, src_remained/*, mpeg_video*/);
}

static int ext_and_user_data(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained/*, mpv_data *mpeg_video*/)
{
	return read_ext(dst, dst_remained, src, src_remained/*, mpeg_video*/);
}

static int gop_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video)
{
	int count=0;
	int i,bt;
	uint32 off;
	uint8 final_byte;
	/*
	 *read bitstream and increment dst and src, decrement dst_remained and src_remained
	 * */
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;
	off=1;
	mpeg_video->hours=get_field(src,5,&off);
	off=6;
	mpeg_video->minutes=get_field(src,6,&off);
	off=13;
	mpeg_video->seconds=get_field(src,6,&off);
	off=19;
	mpeg_video->pictures=get_field(src,6,&off);

	for(i=0;i<3;i++)
		dst[i]=src[i]; 
	src+=3; 
	dst+=3;
	count+=3;
	dst_remained-=3;
	src_remained-=3;
	
	count+=bt=next_start_code2(dst,dst_remained,src,src_remained);
	if(bt==-1)
		return -1;
	if(mpeg_video->std==MPEG_1) {
		dst_remained-=bt;
		src_remained-=bt;
		dst+=bt;
		src+=bt;
		final_byte=src[3];
		if(final_byte==EXT_START_CODE) {
			count+=bt=read_ext(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return  -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			final_byte=src[3];
		}
		if(final_byte==USER_DATA_START_CODE) {
			count+=bt=ext_and_user_data(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			//final_byte=src[3];
		}
	}
	return count;
}

static int picture_coding_ext(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video)
{
	int count=0;
	int i,bt;
	uint32 off;
	/*
	 *read bitstream and increment dst and src, decrement dst_remained and src_remained
	 * */
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;
	/*---*/
	mpeg_video->vsh2->x=0;
	mpeg_video->vsh2->e=0; /*TODO extension data*/
	
	off=4;
	mpeg_video->vsh2->f00=get_field(src,4,&off);
	off=8;
	mpeg_video->vsh2->f01=get_field(src,4,&off);
	off=12;
	mpeg_video->vsh2->f10=get_field(src,4,&off);
	off=16;
	mpeg_video->vsh2->f11=get_field(src,4,&off);
	off=20;
	mpeg_video->vsh2->dc=get_field(src,2,&off);
	off=22;
	mpeg_video->vsh2->ps=get_field(src,2,&off);
	off=24;
	mpeg_video->vsh2->t=get_field(src,1,&off);
	off=25;
	mpeg_video->vsh2->p=get_field(src,1,&off);
	off=26;
	mpeg_video->vsh2->c=get_field(src,1,&off);
	off=27;
	mpeg_video->vsh2->q=get_field(src,1,&off);
	off=28;
	mpeg_video->vsh2->v=get_field(src,1,&off);
	off=29;
	mpeg_video->vsh2->a=get_field(src,1,&off);
	off=30;
	mpeg_video->vsh2->r=get_field(src,1,&off);
	off=31;
	mpeg_video->vsh2->h=get_field(src,1,&off);
	off=32;
	mpeg_video->vsh2->g=get_field(src,1,&off);
	off=33;
	mpeg_video->vsh2->d=get_field(src,1,&off);

	/*---*/
	count+=bt=next_start_code2(dst,dst_remained,src,src_remained);
	if(bt==-1)
		return -1;

	return count;
}

/*chicco: continue.....*/
static int picture_head(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video)
{
	int bt;
	int count=0;
	int i;
	uint32 off;
	uint8 final_byte;
	/*
	 *read bitstream and increment dst, src and count, decrement dst_remained and src_remained
	 * */
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;

	off=0;
	mpeg_video->vsh1->mbz=0; /*unused*/
	mpeg_video->vsh1->tr=mpeg_video->temp_ref=get_field(src,10,&off);
	
	mpeg_video->vsh1->an=0;/*TODO*/ 
	mpeg_video->vsh1->n=0;/*TODO*/ 
	
	off=10;
	mpeg_video->vsh1->p=mpeg_video->picture_coding_type=get_field(src,3,&off);
	
	if(mpeg_video->picture_coding_type == 2 || mpeg_video->picture_coding_type == 3) {
		off=29;
		mpeg_video->vsh1->ffv=get_field(src,1,&off);
		off=30;
		mpeg_video->vsh1->ffc=get_field(src,3,&off);
	}
	else {
	
		mpeg_video->vsh1->ffv=0;
		mpeg_video->vsh1->ffc=0;
	}

	if(mpeg_video->picture_coding_type == 3) {
		off=33;
		mpeg_video->vsh1->fbv=get_field(src,1,&off);
		off=34;
		mpeg_video->vsh1->bfc=get_field(src,3,&off);
	} 
	else {
	
		mpeg_video->vsh1->fbv=0;
		mpeg_video->vsh1->bfc=0;
	}

	count+=bt=next_start_code2(dst,dst_remained,src,src_remained);
	if(bt==-1)
		return -1;
	if(mpeg_video->std==MPEG_1) {
		dst_remained-=bt;
		src_remained-=bt;
		dst+=bt;
		src+=bt;
		final_byte=src[3];
		if(final_byte==EXT_START_CODE) {
			count+=bt=read_ext(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return  -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			final_byte=src[3];
		}
		if(final_byte==USER_DATA_START_CODE) {
			count+=bt=ext_and_user_data(dst, dst_remained, src, src_remained/*, mpeg_video*/);
			if(bt==-1)
				return -1;
			dst_remained-=bt;
			src_remained-=bt;
			dst+=bt;
			src+=bt;
			//final_byte=src[3];
		}
	}

	return count;
}

static int slice(uint8 *dst, uint32 dst_remained, uint8 *src, uint32 src_remained, mpv_data *mpeg_video)
{
	int i;
	int count=0;

	mpeg_video->vsh1->b=1;
	for(i=0;i<4;i++)
		dst[i]=src[i]; /*cp start_code*/
	src+=4; 
	dst+=4;
	count+=4;
	dst_remained-=4;
	src_remained-=4;
	count+=i=next_start_code2(dst,dst_remained,src,src_remained);
	if(i==-1) {
		if(dst_remained<src_remained)
			mpeg_video->vsh1->e=0;
		else
			mpeg_video->vsh1->e=1; /*FIXME: if the initial src isn't a whole frame???*/
		return -1;
	}
	return count;
}

