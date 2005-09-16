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

#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>


static MediaParserInfo info = {
	"MPA",
	"A"
};

FNC_LIB_MEDIAPARSER(mpa);

typedef struct _MPA_DATA{
	uint32 frame_len;
	double pkt_len;
	uint32 is_probed;
}mpa_data;


static int probe(uint8 *dst, uint32 dst_nbytes, InputStream *istream, MediaProperties *properties, mpa_data *mpeg_audio);

static int init(MediaProperties *properties, void **private_data) 
{
	mpa_data *mpeg_audio;
	(*private_data) = malloc (sizeof(mpa_data));
	mpeg_audio = (mpa_data *)(*private_data);
	mpeg_audio->is_probed=0;
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

	if(!mpeg_audio->is_probed) {
		count=probe(dst, dst_nbytes, istream, properties, mpeg_audio);
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
		count + = istream_read(2, &dst[2], istream);	
	}
		
	N_bytes=(int)(mpeg_audio->frame_len * (float)properties->bit_rate / (float)properties->sample_rate / 8); /*2 bytes which contain 12 bit for syncword*/
	if((dst[2] & 0x02)) N_bytes++;
	dst+=count;
	dst_remaind-=count;
	count + = ret = istream_read(N_bytes - 4, dst, istream); /*4 bytes are read yet*/
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
	dst_remained=dst_nbytes - 4;

	count=min(dst_remained,src_nbytes);
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

static int probe(uint8 *dst, uint32 dst_nbytes, InputStream *istream, MediaProperties *properties, mpa_data *mpeg_audio)
{
	int ret, count,off;
	/*long*/ int tag_dim;
        int n,RowIndex, ColIndex;
	uint8 buff_data[4];
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

	if(dst_bytes < 4)
		return -1;

	/*read 4 bytes to calculate mpa parameters or to skip ID3*/
	if ( (count = istream_read(4, buff_data, istream)) != 4) return -1;

	/*look if ID3 tag is present*/
	if (!memcmp(buff_data, "ID3", 3)) { // ID3 tag present
		if ( (count = istream_read(2, buff_data, istream)) != 2) return -1; /*skip second byte of version in ID3 Header*/
		if ( (count = istream_read(4, buff_data, istream)) != 4) return -1; /*4 bytes for ID3 size*/
		off=0;
		tag_dim=get_field(buff_data,32,&off); /*chicco: if you want use: tag_dim=strtol(buff_data,(char **)NULL, 10);*/
		if ( (count = istream_read(tag_dim, buff_data, istream)) != 4) return -1; /*skip ID3*/
		
		/*read 4 bytes to calculate mpa parameters*/
		if ( (count = istream_read(4, buff_data, istream)) != 4) return -1;
	}

	if (! ((buff_data[0]==0xff) && ((buff_data[1] & 0xe0)==0xe0))) return -1; /*syncword not found*/

        switch (buff_data[1] & 0x1e) {                /* Mpeg version and Level */
                case 18: ColIndex = 4; break;  /* Mpeg-2 L3 */
                case 20: ColIndex = 4; break;  /* Mpeg-2 L2 */
                case 22: ColIndex = 3; break;  /* Mpeg-2 L1 */
                case 26: ColIndex = 2; break;  /* Mpeg-1 L3 */
                case 28: ColIndex = 1; break;  /* Mpeg-1 L2 */
                case 30: ColIndex = 0; break;  /* Mpeg-1 L1 */
                default: {
				 return -1;
			 }
        }

        RowIndex = (buff_data[2] & 0xf0) << 4;
        properties->bit_rate = BitrateMatrix[RowIndex][ColIndex];

        if (buff_data[1] & 0x08) {     /* Mpeg-1 */
                switch (buff_data[2] & 0x0c) {
                        case 0x00: properties->sample_rate=44100; break;
                        case 0x04: properties->sample_rate=48000; break;
                        case 0x08: properties->sample_rate=32000; break;
                	default: {
				 return -1;
			 }
                }
        } else {                /* Mpeg-2 */
                switch (buff_data[2] & 0x0c) {
                        case 0x00: properties->sample_rate=22050; break;
                        case 0x04: properties->sample_rate=24000; break;
                        case 0x08: properties->sample_rate=16000; break;
                	default: {
				 return -1;
			 }
                }
        }

        if ((buff_data[1] & 0x06) == 6)
		mpeg_audio->frame_len = 384;
        else
		mpeg_audio->frame_len = 1152;

        mpeg_audio->pkt_len=(double)mpeg_audio->frame_len/(double)properties->sample_rate*1000;
        //p->description.delta_mtime=p->description.pkt_len;
	dst[0]=buf_data[0];
	dst[1]=buf_data[1];
	dst[2]=buf_data[2];
	dst[3]=buf_data[3];
	
	return count; /*return 4*/
}
