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

#include <stdio.h>
#include <string.h>

#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

static MediaParserInfo info = {
	"MPA",
	"A"
};

FNC_LIB_MEDIAPARSER(mpa);

#define ID3v2_HDRLEN 10 // basic header lenght for id3 tag

// inspired by id3lib
#define MASK(bits) ((1 << (bits)) - 1)

#define MPA_IS_SYNC(buff_data) ((buff_data[0]==0xff) && ((buff_data[1] & 0xe0)==0xe0))

typedef struct {
	enum {MPA_MPEG_2_5=0, MPA_MPEG_RES, MPA_MPEG_2, MPA_MPEG_1} id;
	enum {MPA_LAYER_RES=0, MPA_LAYER_III, MPA_LAYER_II, MPA_LAYER_I} layer;
	uint32 frame_size;
	// double pkt_len;
	uint32 pkt_len;
	uint32 probed;
} mpa_data;


static uint32 dec_synchsafe_int(uint8 [4]);
static int mpa_read_id3v2(InputStream *, mpa_data *); // for now only skipped.
static int mpa_sync(uint32 *, InputStream *);
static int mpa_decode_header(uint8 *dst, uint32 dst_nbytes, InputStream *istream, MediaProperties *properties, mpa_data *mpeg_audio);

static int init(MediaProperties *properties, void **private_data) 
{
	mpa_data *mpeg_audio;
	(*private_data) = malloc (sizeof(mpa_data));
	mpeg_audio = (mpa_data *)(*private_data);
	mpeg_audio->probed=0;
	return 0;
}

static int get_frame2(uint8 *dst, uint32 dst_nbytes, int64 *timestamp, InputStream *istream, MediaProperties *properties, void *private_data) 
{
	mpa_data *mpeg_audio;
	int count=0, ret;
	uint32 N_bytes;
	uint8 buf_data[2];
	uint32 sync_found=0;
	
	mpeg_audio = (mpa_data *)private_data;

	if(!mpeg_audio->probed) {
		count=mpa_decode_header(dst, dst_nbytes, istream, properties, mpeg_audio);
		if(count<0)
			return -1; /*EOF or syncword not found or header error*/
		(*timestamp)-=mpeg_audio->pkt_len;/*i set it negative so at the first time it is zero (see at the end of this function)*/
	}
	else {
		count = istream_read(2, buf_data, istream);	
		if(count<0)
			return -1;
		sync_found=0;
		while(!sync_found){
			if (!(buf_data[0]==0xff) && ((buf_data[1] & 0xe0)==0xe0)) { /* no syncword at the beginning*/
				if (!(buf_data[1]==0xff) && ((buf_data[0] & 0x07)==0x07)) { /* no syncword at the beginning*/
					buf_data[0]=buf_data[1];
					ret = istream_read(1, &buf_data[1], istream);	
					if(ret<0)
						return -1;
				}
				else sync_found=1;
			}
			else sync_found=1;
		}
		dst[0]=buf_data[0];
		dst[1]=buf_data[1];
		count += istream_read(2, &dst[2], istream);	
	}
		
	N_bytes=(int)(mpeg_audio->frame_size * (float)properties->bit_rate / (float)properties->sample_rate / 8); /*2 bytes which contain 12 bit for syncword*/
	if((dst[2] & 0x02)) N_bytes++;
	dst+=count;
	// dst_remained-=count;
	dst_nbytes-=count;
	count += ret = istream_read(N_bytes - 4, dst, istream); /*4 bytes are read yet*/
	if(ret<0)
		return -1;
	
	(*timestamp)+=mpeg_audio->pkt_len; /*it was negative at the beginning so it is zero at the first time*/

	return count;
}

static int packetize(uint8 *dst, uint32 dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data)
{
	uint32 count;
	uint8 tmp[3];

	/*4 bytes for rtp encapsulation*/	
	// uint32 dst_remained=dst_nbytes - 4;

	// count=min(dst_remained,src_nbytes);
	count=min(dst_nbytes, src_nbytes);
	memcpy(dst+4,src,count);
	dst[0]=0;
	dst[1]=0;
	if(count<src_nbytes) {
		sprintf(tmp,"%d",count);
		dst[2]=tmp[0];
		dst[3]=tmp[1];
	}
	else {
		dst[2]=0;
		dst[3]=0;
	}
	
	return count + 4;
}

static int uninit(void *private_data)
{
	free(private_data);
	return 0;
}

static uint32 dec_synchsafe_int(uint8 encoded[4])
{
	uint32 decoded=/*(uint32)*/encoded[sizeof(encoded)-1];
	const unsigned char bitsused = 7;
	int i;

	for (i=sizeof(encoded)-2; i>=0; i--) {
		decoded <<= bitsused;
		decoded = encoded[i] & MASK(bitsused);
	}

	return decoded;
}

// for the moment we just skip the ID3v2 tag.
static int mpa_read_id3v2(InputStream *i_stream, mpa_data *mpeg_audio)
{
	return 0;
}

static int mpa_sync(uint32 *header, InputStream *i_stream)
{
	uint8 *sync_w = (uint8 *)header;
	int ret;

	if ( (ret=istream_read(2, sync_w, i_stream)) != 2 )
		return ret;
	while ( !MPA_IS_SYNC(sync_w) ) {
		sync_w[0]=sync_w[1];
		if ( (ret=istream_read(1, &sync_w[1], i_stream)) != 1 )
			return ret;
	}

	if ( (ret=istream_read(2, &sync_w[2], i_stream)) != 2 )
		return ret;

	return 0;
}

static int mpa_decode_header(uint8 *dst, uint32 dst_nbytes, InputStream *istream, MediaProperties *properties, mpa_data *mpeg_audio)
{
	int ret, count,off;
	/*long*/ int tag_dim;
        int n,RowIndex, ColIndex;
	uint8 buff_data[4];
	int padding;
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

	float SRateMatrix[4][3] = {
		{44100,	22050,	11025	},
		{48000,	24000,	12000	},
		{32000,	16000,	8000	},
		{-1,	-1,	-1	}
	};

	if(dst_nbytes < 4)
		return -1;

	/*read 4 bytes to calculate mpa parameters or to skip ID3*/
	if ( (count = istream_read(4, buff_data, istream)) != 4) return -1;

	/*look if ID3 tag is present*/
	if (!memcmp(buff_data, "ID3", 3)) { // ID3 tag present
		if ( (count = istream_read(2, buff_data, istream)) != 2) return -1; /*skip second byte of version in ID3 Header and flags*/
		if ( (count = istream_read(4, buff_data, istream)) != 4) return -1; /*4 bytes for ID3 size*/
		off=0;
		tag_dim=get_field(buff_data,32,&off); /*chicco: if you want use: tag_dim=strtol(buff_data,(char **)NULL, 10);*/
		if ( (count = istream_read(tag_dim, buff_data, istream)) != tag_dim) return -1; /*skip ID3*/
		
		/*read 4 bytes to calculate mpa parameters*/
		if ( (count = istream_read(4, buff_data, istream)) != 4) return -1;
	}

	if ( !((buff_data[0]==0xff) && ((buff_data[1] & 0xe0)==0xe0)) ) return -1; /*syncword not found*/

	mpeg_audio->id = (buff_data[1] & 0x18) >> 3;
	mpeg_audio->layer = (buff_data[1] & 0x06) >> 1;

	switch (mpeg_audio->id<< 2 | mpeg_audio->layer) {
		case MPA_MPEG_1<<2|MPA_LAYER_I:		// MPEG 1 layer I
			ColIndex = 0;
			break;
		case MPA_MPEG_1<<2|MPA_LAYER_II:	// MPEG 1 layer II
			ColIndex = 1;
			break;
		case MPA_MPEG_1<<2|MPA_LAYER_III:	// MPEG 1 layer III
			ColIndex = 2;
			break;
		case MPA_MPEG_2<<2|MPA_LAYER_I:		// MPEG 2 layer I
		case MPA_MPEG_2_5<<2|MPA_LAYER_I:	// MPEG 2.5 layer I
			ColIndex = 3;
			break;
		case MPA_MPEG_2<<2|MPA_LAYER_II:	// MPEG 2 layer II
		case MPA_MPEG_2<<2|MPA_LAYER_III:	// MPEG 2 layer III
		case MPA_MPEG_2_5<<2|MPA_LAYER_II:	// MPEG 2.5 layer II
		case MPA_MPEG_2_5<<2|MPA_LAYER_III:	// MPEG 2.5 layer III
			ColIndex = 4;
			break;
		default: return -1; break;
	}

        RowIndex = (buff_data[2] & 0xf0) >> 4;
        properties->bit_rate = BitrateMatrix[RowIndex][ColIndex];

	switch (mpeg_audio->id) {
		case MPA_MPEG_1: ColIndex = 0; break;
		case MPA_MPEG_2: ColIndex = 1; break;
		case MPA_MPEG_2_5: ColIndex = 2; break;
		default:
			return -1;
			break;
	}

	RowIndex = (buff_data[2] & 0x0c) >> 2;
        properties->sample_rate = SRateMatrix[RowIndex][ColIndex];

	// padding
	padding = buff_data[2] & 0x02 >> 2;
	fnc_log(FNC_LOG_DEBUG, "[MT] padding: %d\n", padding);

        // if ((buff_data[1] & 0x06) == 6)
        if (mpeg_audio->layer == 1) { // layer 1
		mpeg_audio->frame_size = 384;
		mpeg_audio->pkt_len=((12 * properties->bit_rate)/properties->sample_rate + padding)* 4;
	} else { // layer 2 or 3
		mpeg_audio->frame_size = 1152;
		mpeg_audio->pkt_len=(144 * properties->bit_rate)/properties->sample_rate + padding;
	}

	fnc_log(FNC_LOG_DEBUG, "[MT] pkt_len: %d\n", mpeg_audio->pkt_len);

        // mpeg_audio->pkt_len=(double)mpeg_audio->frame_size/(double)properties->sample_rate*1000;
        //p->description.delta_mtime=p->description.pkt_len;
	dst[0]=buff_data[0];
	dst[1]=buff_data[1];
	dst[2]=buff_data[2];
	dst[3]=buff_data[3];
	
	return count; /*return 4*/
}
