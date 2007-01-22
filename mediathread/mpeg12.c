/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
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
#include <fenice/mpeg.h>
#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>

static MediaParserInfo info = {
    "MPV",
    MP_video
};

FNC_LIB_MEDIAPARSER(mpv);

/*see ISO/IEC 11172-2:1993 and ISO/IEC 13818-2:1995 (E)*/
/*prefix*/
#if (BYTE_ORDER == BIG_ENDIAN)
    #define SHIFT_RIGHT(a,n) (a<<=n)
    #define SHIFT_LEFT(a,n) (a>>=n)
#elif (BYTE_ORDER == LITTLE_ENDIAN)
    #define SHIFT_RIGHT(a,n) (a>>=n)
    #define SHIFT_LEFT(a,n) (a<<=n)
#endif //ENDIAN

/*value*/
#if (BYTE_ORDER == BIG_ENDIAN)
    #define START_CODE 0x010000
    #define PICTURE_START_CODE 0x00000100
    #define SLICE_MIN_START_CODE 0x00000101 
    #define SLICE_MAX_START_CODE 0x000001AF
    #define USER_DATA_START_CODE 0x000001B2
    #define SEQ_START_CODE 0x000001B3
    #define SEQ_ERROR_CODE 0x000001B4
    #define EXT_START_CODE 0x000001B5 
    #define SEQ_END_CODE 0x000001B7
    #define GOP_START_CODE 0x000001B8
#elif (BYTE_ORDER == LITTLE_ENDIAN)
    #define START_CODE 0x000001
    #define PICTURE_START_CODE 0x00010000
    #define SLICE_MIN_START_CODE 0x01010000 
    #define SLICE_MAX_START_CODE 0xAF010000
    #define USER_DATA_START_CODE 0xB2010000
    #define SEQ_START_CODE 0xB3010000
    #define SEQ_ERROR_CODE 0xB4010000
    #define EXT_START_CODE 0xB5010000 
    #define SEQ_END_CODE 0xB7010000
    #define GOP_START_CODE 0xB8010000
#endif //ENDIAN

#define IS_SLICE(a) (a>=SLICE_MIN_START_CODE && a<=SLICE_MAX_START_CODE)
#define MPV_IS_SYNC(buff_data) ( (buff_data[0]==0x00) && (buff_data[1] ==0x00) && (buff_data[2] ==0x01) )

typedef struct _MPV_DATA {
    int is_buffered;
    unsigned int buffer;

    int is_fragmented;

    video_spec_head1 *vsh1;
    video_spec_head2 *vsh2;

    unsigned int hours;
    unsigned int minutes;
    unsigned int seconds;
    unsigned int pictures;
    unsigned int temp_ref; /* pictures'count mod 1024*/
    unsigned int picture_coding_type;
    standard std;
} mpv_data;

typedef struct {
    InputStream *istream; 
    uint8 *src;
    uint32 src_nbytes;
} mpv_input;

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

static float AspectRatioCode[] = {
                /*  value         height/width     video source             */
        0.0000, /*   0              forbidden                               */
        1.0000, /*   1              1,0000        pc VGA                    */
        0.6735, /*   2              0,6735                                  */
        0.7031, /*   3              0,7031        16:9 625 lines            */
        0.7615, /*   4              0,7615                                  */
        0.8055, /*   5              0,8055                                  */
        0.8437, /*   6              0,8437        16:9 525 lines            */
        0.8935, /*   7              0,8935                                  */
        0.9157, /*   8              0,9157        CCIR Rec.601 625 lines    */
        0.9815, /*   9              0,9815                                  */
        1.0255, /*  10              1,0255                                  */
        1.0695, /*  11              1,0695                                  */
        1.0950, /*  12              1,0950        CCIR Rec.601 525 lines    */
        1.1575, /*  13              1,1575                                  */
        1.2015, /*  14              1,2015                                  */
        0.0000  /*  15              reserved                                */
    };

static int mpv_read(mpv_input *in, uint8 *buf, uint32 nbytes)
{
    if (in->istream)
    {
        //printf("Reading from file n_bytes=%d\n",nbytes);
        return istream_read(in->istream, buf, nbytes);
    }
    else if (in->src) {
        uint32 to_cpy=min(nbytes, in->src_nbytes);

        if (buf)
            memcpy(buf, in->src, to_cpy);
        in->src += to_cpy;
        in->src_nbytes -= to_cpy;

        return to_cpy;
    } else 
        return ERR_EOF;
    
}

static int get_header(uint32 *header, uint8* src, mpv_data *mpv)
{
    uint8 *sync_w = (uint8 *)header;
    int ret;
    mpv_input in = {NULL, src, 4};

    if ( (ret=mpv_read(&in, sync_w, 4)) != 4 ) 
        return (ret<0) ? ERR_PARSE : ERR_EOF;
    
    while ( !MPV_IS_SYNC(sync_w) ) {
        return ERR_PARSE;
    }

    return 0;
}

static int mpv_sync(uint32 *header, mpv_input *in, mpv_data *mpv)
{
    uint8 *sync_w = (uint8 *)header;
    int ret;

    if ( (ret=mpv_read(in, sync_w, 4)) != 4 ) 
        return (ret<0) ? ERR_PARSE : ERR_EOF;
    
    while ( !MPV_IS_SYNC(sync_w) ) {
        SHIFT_RIGHT(*header,8);
        if ( (ret=mpv_read(in, &sync_w[3], 1)) != 1 ) 
            return (ret<0) ? ERR_PARSE : ERR_EOF;
    }

    return 0;
}

static int next_start_code2(uint8 *dst, uint32 dst_remained, mpv_input *in)
{
    int count=0;
    int ret;
    uint8 sync_w[4];

    if ( (ret=mpv_read(in, sync_w, 4)) != 4 ) 
        return (ret<0) ? ERR_PARSE : ERR_EOF;
    
    while ( !MPV_IS_SYNC(sync_w) ) {
        dst[count]=sync_w[0];
        //printf("count=%d - dst[count] =%X - sync_w=%X%X%X%X\n",count,dst[count],sync_w[0],sync_w[1],sync_w[2],sync_w[3]);
        count++;
        sync_w[0]=sync_w[1];
        sync_w[1]=sync_w[2];
        sync_w[2]=sync_w[3];
        if ( (ret=mpv_read(in, &sync_w[3], 1)) != 1 ) 
            return (ret<0) ? ERR_PARSE : ERR_EOF;
    }
    
    memcpy(&dst[count],sync_w,4);
    count+=4;

    return count;

}

static int seq_ext(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);

    /*---------------------*/
    return count;

}

static int read_ext(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);

    /*---------------------*/
    return count;

}

static int ext_and_user_data(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);

    /*---------------------*/
    return count;

}

static int slice(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);

    /*---------------------*/
    return count;

}

static int seq_head(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0,ret;
    uint32 header;
    uint32 off;
    int bt;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);
    if(count < 0)
        return count;
    dst_remained-=count;
    
    off=0;
    properties->PixelWidth=get_field(dst,12,&off);
    off=12;
    properties->PixelHeight=get_field(dst,12,&off);
    off=24;
    properties->AspectRatio=AspectRatioCode[get_field(dst,4,&off)];    
    off=28;
    properties->frame_rate=FrameRateCode[get_field(dst,4,&off)];
    off=32;
    bt=get_field(dst,18,&off);
    properties->bit_rate=(bt<262144)?(400*bt):0; /*if 0 -> VBR*/
    
    /*---------------------*/
    if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
        return ret;

    if(header == EXT_START_CODE) {/*MPEG_2*/
        mpv->std=MPEG_2;
        bt=seq_ext(&dst[count], dst_remained, in, properties, mpv);
        if(bt<0)
            return bt;
        count+=bt;
    }
    else mpv->std=MPEG_1;

    return count;
}

static int gop_head(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0, ret,off,bt;
    uint32 header;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);
    dst_remained-=count;

    off=1;
    mpv->hours=get_field(dst,5,&off);
    off=6;
    mpv->minutes=get_field(dst,6,&off);
    off=13;
    mpv->seconds=get_field(dst,6,&off);
    off=19;
    mpv->pictures=get_field(dst,6,&off);

    /*---------------------*/
    if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
        return ret;
    
    if(header == EXT_START_CODE) {
        bt=read_ext(&dst[count], dst_remained, in, properties, mpv);
        if(bt<0)
            return bt;
        count+=bt;
    }

    return count;
}

static int picture_coding_ext(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0, ret,off,bt;
    uint32 header;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);
    dst_remained-=count;

    off=4;
    mpv->vsh2->f00=get_field(dst,4,&off);
    off=8;
    mpv->vsh2->f01=get_field(dst,4,&off);
    off=12;
    mpv->vsh2->f10=get_field(dst,4,&off);
    off=16;
    mpv->vsh2->f11=get_field(dst,4,&off);
    off=20;
    mpv->vsh2->dc=get_field(dst,2,&off);
    off=22;
    mpv->vsh2->ps=get_field(dst,2,&off);
    off=24;
    mpv->vsh2->t=get_field(dst,1,&off);
    off=25;
    mpv->vsh2->p=get_field(dst,1,&off);
    off=26;
    mpv->vsh2->c=get_field(dst,1,&off);
    off=27;
    mpv->vsh2->q=get_field(dst,1,&off);
    off=28;
    mpv->vsh2->v=get_field(dst,1,&off);
    off=29;
    mpv->vsh2->a=get_field(dst,1,&off);
    off=30;
    mpv->vsh2->r=get_field(dst,1,&off);
    off=31;
    mpv->vsh2->h=get_field(dst,1,&off);
    off=32;
    mpv->vsh2->g=get_field(dst,1,&off);
    off=33;
    mpv->vsh2->d=get_field(dst,1,&off);


    /*---------------------*/
    
    if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
        return ret;

    if(header == EXT_START_CODE) {
        bt=ext_and_user_data(&dst[count], dst_remained, in, properties, mpv);
        if(bt<0)
            return bt;
        count+=bt;
    }

    return count;

}

static int picture_head(uint8 *dst, uint32 dst_remained, mpv_input *in, MediaProperties *properties, mpv_data *mpv)
{
    int count=0, ret,off,bt;
    uint32 header;

    /*---------------------*/
    count=next_start_code2(dst, dst_remained, in);
    dst_remained-=count;

    off=0;
    mpv->vsh1->mbz=0; /*unused*/
    mpv->vsh1->tr=mpv->temp_ref=get_field(dst,10,&off);
    
    mpv->vsh1->an=0;/*TODO*/ 
    mpv->vsh1->n=0;/*TODO*/ 
    
    off=10;
    mpv->vsh1->p=mpv->picture_coding_type=get_field(dst,3,&off);
    
    if(mpv->picture_coding_type == 2 || mpv->picture_coding_type == 3) {
        off=29;
        mpv->vsh1->ffv=get_field(dst,1,&off);
        off=30;
        mpv->vsh1->ffc=get_field(dst,3,&off);
    }
    else {
    
        mpv->vsh1->ffv=0;
        mpv->vsh1->ffc=0;
    }

    if(mpv->picture_coding_type == 3) {
        off=33;
        mpv->vsh1->fbv=get_field(dst,1,&off);
        off=34;
        mpv->vsh1->bfc=get_field(dst,3,&off);
    } 
    else {
    
        mpv->vsh1->fbv=0;
        mpv->vsh1->bfc=0;
    }
    
    /*---------------------*/
    if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
        return ret;

    if(header == EXT_START_CODE) {
        bt=picture_coding_ext(&dst[count], dst_remained, in, properties, mpv);
        if(bt<0)
            return bt;
        count+=bt;
    }

    return count;
}

/*mediaparser_module interface implementation*/
int init(MediaProperties *properties, void **private_data)
{
//    sdp_field *sdp_private;
    
    mpv_data *mpeg_video;
    *private_data = calloc(1, sizeof(mpv_data));
    mpeg_video = (mpv_data *)(*private_data);
    mpeg_video->vsh1 = calloc(1,sizeof(video_spec_head1));
    mpeg_video->vsh2 = calloc(1,sizeof(video_spec_head2));
    mpeg_video->is_buffered=0;/*false*/
#if 0 // trial for sdp private fields
    sdp_private = g_new(sdp_field, 1);
    
    sdp_private->type = fmtp;
    sdp_private->field = g_strdup("example of sdp private struct");
        
    properties->sdp_private=g_list_prepend(properties->sdp_private, sdp_private);
#endif // trial for sdp private fields
    INIT_PROPS

    return 0;
}

int get_frame2(uint8 *dst, uint32 dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data)
{
    uint32 count=0, header;
    int ret=0;
    mpv_data *mpv;
    mpv_input in={istream, NULL, 0};
    mpv=(mpv_data *)private_data;
    
    if(mpv->is_buffered) {
        header=mpv->buffer;
        ret=4;
    }
    else if ( (ret=mpv_sync(&header, &in, mpv)) ) 
        return ret;

//#if (BYTE_ORDER == BIG_ENDIAN)
    memcpy(dst,&header,4);
/*#elif (BYTE_ORDER == LITTLE_ENDIAN)
    dst[0]=((uint8 *)&header)[3];
    dst[1]=((uint8 *)&header)[2];
    dst[2]=((uint8 *)&header)[1];
    dst[3]=((uint8 *)&header)[0];
    
#endif //ENDIAN*/

    count+=4;
    dst_nbytes-=4;
    
    mpv->vsh1->s=0;/*sequence header is not present, but it is set to 1 if seq_head is called*/ 
    while(!IS_SLICE(header)) {
        /*
         *ogni funzione trova il suo start code caricato e ritornare il numero di byte letti (compreso lo start code successivo)
         *oppure ERR_EOF oppure ERR_PARSE oppure MP_PKT_TOO_SMALL 
         *
         */
        if(header == SEQ_START_CODE) {
            ret=seq_head(&dst[count], dst_nbytes, &in, properties, mpv);
            mpv->vsh1->s=1;/*sequence header is not present, but it is set to 1 if seq_head is called*/ 
        }
        else if(header == GOP_START_CODE)
            ret=gop_head(&dst[count], dst_nbytes, &in, properties, mpv);

        else if(header == PICTURE_START_CODE)
            ret=picture_head(&dst[count], dst_nbytes, &in, properties, mpv);
        
        else //return ERR_PARSE;/*TODO: SKIP: next_start_code*/
            ret=read_ext(&dst[count], dst_nbytes, &in, properties, mpv);

        if(ret<0)
            return (ret==MP_PKT_TOO_SMALL)?MP_NOT_FULL_FRAME:ret;
        dst_nbytes-=ret;
        count+=ret;
        
        if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
            return ret;
    }
    
    /*read all slices in picture*/
    while(IS_SLICE(header)) {
        ret=slice(&dst[count], dst_nbytes, &in, properties, mpv);
        if(ret<0)
            return (ret==MP_PKT_TOO_SMALL)?MP_NOT_FULL_FRAME:ret;
        dst_nbytes-=ret;
        count+=ret;
        if ( (ret=get_header(&header, &dst[count-4], mpv)) ) 
            return ret;
    }
    /*now we have the whole picture + next_start_code in dst*/
    mpv->buffer=header;
    mpv->is_buffered=1;/*yes*/

    /*seconds*/
    (*timestamp) = ((mpv->hours * 3.6e3) + (mpv->minutes * 60) + (mpv->seconds))*1000 +  (mpv->temp_ref*1000/properties->frame_rate) + (mpv->pictures*1000/properties->frame_rate);
    //(*timestamp) = ((mpv->hours * 3.6e3) + (mpv->minutes * 60) + (mpv->seconds))*1000 +  (mpv->temp_ref*40) + (mpv->pictures*40);
    
    if(header == SEQ_END_CODE)
        return count;
    return count-4; /*whole picture (i remove the next_start_code*/
}


/*see RFC 2250: RTP Payload Format for MPEG1/MPEG2 Video*/
#if MPEG2VSHE
    #define VSHCPY  vsh_tmp = (uint8 *)(mpv->vsh1); \
                dst[0] = vsh_tmp[3]; \
                dst[1] = vsh_tmp[2]; \
                dst[2] = vsh_tmp[1]; \
                dst[3] = vsh_tmp[0]; \
                if (mpv->std == MPEG_2) { \
                        vsh_tmp = (char *)(mpv->vsh2); \
                dst[4] = vsh_tmp[3]; \
                           dst[5] = vsh_tmp[2]; \
                           dst[6] = vsh_tmp[1]; \
                        dst[7] = vsh_tmp[0]; \
                    } 
#else
    #define VSHCPY  vsh_tmp = (uint8 *)(mpv->vsh1); \
                dst[0] = vsh_tmp[3]; \
                dst[1] = vsh_tmp[2]; \
                dst[2] = vsh_tmp[1]; \
                dst[3] = vsh_tmp[0]; 

#endif //MPEG2VSHE

int packetize(uint8 *dst, uint32 *dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data)
{
    int ret=0, count=0,countsrc=0;
    uint32 header;
    mpv_data *mpv;
    uint8 *vsh_tmp;
    mpv_input in = {NULL,src,src_nbytes};


    mpv=(mpv_data *)private_data;
    mpv->vsh1->b=0;/*begining of slice*/
    mpv->vsh1->e=0;/*end of slice*/

    if (mpv->std == MPEG_2)  {
#if MPEG2VSHE
        count+= 8;
        mpv->vsh1->t=1;
#else
        count += 4;
        mpv->vsh1->t=0;
#endif  /* bytes for the video specific header */
    } else {
            count += 4;
        mpv->vsh1->t=0;
    }

    while(countsrc < src_nbytes && count < *dst_nbytes) {
        if ( (ret=mpv_read(&in, &dst[count], 1)) != 1 ) {
            mpv->vsh1->e=1;/*end of slice*/
            mpv->is_fragmented=0;
            VSHCPY;
            *dst_nbytes=count;
            //return countsrc; 
            //return (ret<0) ? ERR_PARSE : ERR_EOF;
            return ERR_EOF;
        }
        count++;
        countsrc++;

        if(countsrc==4)
            if ( !(get_header(&header, &dst[count-4], mpv)) )
                mpv->vsh1->b=1;/*begining of slice*/

        if(mpv->is_fragmented) {
            if ( !(get_header(&header, &src[countsrc], mpv)) )
                break;
        }
    }

    if ( !(get_header(&header, &src[countsrc], mpv)) ) {
        mpv->vsh1->e=1;/*end of slice*/
        mpv->is_fragmented=0;
    }
    else {
        mpv->vsh1->e=0;/*end of slice*/
        mpv->is_fragmented=1;
    }

    VSHCPY;
    *dst_nbytes=count;

    mpv->vsh1->s=0;/*RESET. sequence header is not present. SEQ_HEADER can be present only in the first pkt of a frame */

    return countsrc;
}

int parse(void *track, double mtime, uint8 *data, long len, uint8 *extradata,
                 long extradata_len)
{
    Track *tr = (Track *)track;
    int ret;
    OMSSlot *slot;
    uint32 dst_len = len + 4;
    uint8 dst[dst_len];

    ret = packetize(dst, &dst_len, data, len, tr->properties,
              tr->private_data);
    if (ret == ERR_NOERROR) {
        ret = OMSbuff_write(tr->buffer, 0, mtime, 0, dst, dst_len);
        if (ret) fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
    }
    return ret;
}


int uninit(void *private_data)
{
    mpv_data *mpeg_video;
    mpeg_video=(mpv_data *)private_data;
    /*free(mpeg_video->vsh1);
    free(mpeg_video->vsh2);*/
    free(private_data);
    return 0;
}



