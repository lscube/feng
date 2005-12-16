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
#include <fenice/debug.h>

static MediaParserInfo info = {
	"MPA",
	MP_audio
};

FNC_LIB_MEDIAPARSER(mpa);

#define ID3v2_HDRLEN 10 // basic header lenght for id3 tag

// inspired by id3lib
#define MASK(bits) ((1 << (bits)) - 1)

#define MPA_IS_SYNC(buff_data) ((buff_data[0]==0xff) && ((buff_data[1] & 0xe0)==0xe0))

#define MPA_RTPSUBHDR 4

#define BITRATEMATRIX { \
	{0,     0,     0,     0,     0     }, \
	{32000, 32000, 32000, 32000, 8000  }, \
	{64000, 48000, 40000, 48000, 16000 }, \
	{96000, 56000, 48000, 56000, 24000 }, \
	{128000,64000, 56000, 64000, 32000 }, \
	{160000,80000, 64000, 80000, 40000 }, \
	{192000,96000, 80000, 96000, 48000 }, \
	{224000,112000,96000, 112000,56000 }, \
	{256000,128000,112000,128000,64000 }, \
	{288000,160000,128000,144000,80000 }, \
	{320000,192000,160000,160000,96000 }, \
	{352000,224000,192000,176000,112000}, \
	{384000,256000,224000,192000,128000}, \
	{416000,320000,256000,224000,144000}, \
	{448000,384000,320000,256000,160000}, \
	{0,     0,     0,     0,     0     } \
}

#define SRATEMATRIX { \
	{44100,	22050,	11025	}, \
	{48000,	24000,	12000	}, \
	{32000,	16000,	8000	}, \
	{-1,	-1,	-1	} \
}

typedef struct {
	enum {MPA_MPEG_2_5=0, MPA_MPEG_RES, MPA_MPEG_2, MPA_MPEG_1} id;
	enum {MPA_LAYER_RES=0, MPA_LAYER_III, MPA_LAYER_II, MPA_LAYER_I} layer;
	uint32 frame_size;
	// double pkt_len;
	uint32 pkt_len;
	uint32 probed;
	double time;
	// fragmentation suuport:
	uint8 fragmented;
	uint8 *frag_src;
	uint32 frag_src_nbytes;
	uint32 frag_offset;
} mpa_data;

typedef struct {
	uint8 id[3];
	uint8 major;
	uint8 rev;
	uint8 flags;
	uint8 size[4];
	// not using extended header
} id3v2_hdr;

typedef struct {
	InputStream *istream;
	uint8 *src;
	uint32 src_nbytes;
} mpa_input;

static int mpa_read(uint32, uint8 *, mpa_input *);
static uint32 dec_synchsafe_int(uint8 [4]);
// static int mpa_read_id3v2(id3v2_hdr *, InputStream *, mpa_data *); // for now only skipped.
static int mpa_read_id3v2(id3v2_hdr *, mpa_input *, mpa_data *); // for now only skipped.
// static int mpa_sync(uint32 *, InputStream *, mpa_data *);
static int mpa_sync(uint32 *, mpa_input *, mpa_data *);
static int mpa_decode_header(uint32 header, MediaProperties *properties, mpa_data *mpa);
#ifdef DEBUG
// debug function to diplay MPA header information
static void mpa_info(mpa_data *, MediaProperties *);
#endif // DEBUG

static int init(MediaProperties *properties, void **private_data) 
{
	if ( !(*private_data = calloc (1, sizeof(mpa_data))) )
		return ERR_ALLOC;

	INIT_PROPS

	return 0;
}

// at the moment get_frame do not support packet fragmentation
static int get_frame2(uint8 *dst, uint32 dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data) 
{
	mpa_input in={istream, NULL, 0};
	mpa_data *mpa;
	int ret;
	uint32 header;

	mpa = (mpa_data *)private_data;

	do {
		if ( (ret=mpa_sync(&header, &in, mpa)) )
			return ret;
		ret=mpa_decode_header(header, properties, mpa);
	} while(ret);

	if (dst_nbytes < mpa->pkt_len)
		return MP_PKT_TOO_SMALL;

	memcpy(dst, &header, sizeof(header));

	if ( (uint32)(ret = istream_read(mpa->pkt_len - 4, dst + 4, istream)) < mpa->pkt_len-4 )
		return (ret>=0) ? MP_NOT_FULL_FRAME : ret;
	
	// (*timestamp)+=mpa->pkt_len; /*it was negative at the beginning so it is zero at the first time*/
	*timestamp = mpa->time;
	mpa->time += (double)mpa->frame_size/(double)properties->sample_rate;
	fnc_log(FNC_LOG_DEBUG, "[MPA] time: %fs\n", mpa->time);

	return ret + sizeof(header);
}

// packet fragmentation supported
static int packetize(uint8 *dst, uint32 *dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data)
{
	mpa_input in={ NULL, src, src_nbytes };
	uint32 header, mpa_header=0;
	mpa_data *mpa=(mpa_data *)private_data;
	int ret;
	uint32 dst_offset=0;

	uint32 to_cpy, end_frm_dist;
	// uint8 tmp[3];

	if ( (mpa->fragmented) && (mpa->frag_src == src) && (mpa->frag_src_nbytes == src_nbytes) ) { // last frame was fragmented
		end_frm_dist = mpa->pkt_len - mpa->frag_offset;
		to_cpy = min(end_frm_dist, (mpa->frag_src_nbytes-mpa->frag_offset));
		src += mpa->frag_offset;
		if (to_cpy==end_frm_dist)
			mpa->fragmented=0;
		else
			mpa->frag_offset += to_cpy;
		mpa_header = mpa->frag_offset & 0x00FF;
	} else { // last frame wasn't fragmented
		if ( (ret=mpa_sync(&header, &in, mpa)) )
			return ret;
		// mpa_header = 0; // already initialized
		memcpy(dst + 4, &header, sizeof(header));
		dst_offset+=4;
		to_cpy = min(mpa->pkt_len, *dst_nbytes);
		if (to_cpy<mpa->pkt_len) {
			mpa->fragmented = 1;
			mpa->frag_src = src;
			mpa->frag_src_nbytes = src_nbytes;
			mpa->frag_offset = to_cpy;
		} else
			mpa->fragmented = 0;
	}

	// rtp MPA sub-header
	memcpy(dst, &mpa_header, sizeof(mpa_header));

	ret = mpa_read(to_cpy, dst+sizeof(mpa_header)+dst_offset, &in);
	
	return ret+dst_offset;
}

static int uninit(void *private_data)
{
	free(private_data);
	return 0;
}

// internal functions

static uint32 dec_synchsafe_int(uint8 encoded[4])
{
	const unsigned char bitsused = 7;
	uint32 decoded =/*(uint32)*/encoded[0] & MASK(bitsused);
	unsigned i;

	for (i=1; i<sizeof(encoded); i++) {
		decoded <<= bitsused;
		decoded |= encoded[i] & MASK(bitsused);
	}

	return decoded;
}

// for the moment we just skip the ID3v2 tag.
// static int mpa_read_id3v2(id3v2_hdr *id3hdr, InputStream *i_stream, mpa_data *mpa)
static int mpa_read_id3v2(id3v2_hdr *id3hdr, mpa_input *in, mpa_data *mpa)
{
	uint32 tagsize;
	int ret;

	tagsize=dec_synchsafe_int(id3hdr->size);

	// if ( (ret=istream_read(tagsize, NULL, i_stream)) != (int)tagsize )
	if ( (ret=mpa_read(tagsize, NULL, in)) != (int)tagsize )
		return (ret<0) ? ERR_PARSE : ERR_EOF;

	return 0;
}

// static int mpa_sync(uint32 *header, InputStream *i_stream, mpa_data *mpa)
static int mpa_sync(uint32 *header, mpa_input *in, mpa_data *mpa)
{
	uint8 *sync_w = (uint8 *)header;
	int ret;

	// if ( (ret=istream_read(4, sync_w, i_stream)) != 4 )
	if ( (ret=mpa_read(4, sync_w, in)) != 4 )
		return (ret<0) ? ERR_PARSE : ERR_EOF;

	if (!mpa->probed) {
		/*look if ID3 tag is present*/
		if (!memcmp(sync_w, "ID3", 3)) { // ID3 tag present
			id3v2_hdr id3hdr;

			// fnc_log(FNC_LOG_DEBUG, "ID3v2 tag present in %s\n", i_stream->name);

			memcpy(&id3hdr, sync_w, 4);
			// if ( (ret = istream_read(ID3v2_HDRLEN - 4, &id3hdr.rev, i_stream)) != ID3v2_HDRLEN - 4 )
			if ( (ret = mpa_read(ID3v2_HDRLEN - 4, &id3hdr.rev, in)) != ID3v2_HDRLEN - 4 )
				return (ret<0) ? ERR_PARSE : ERR_EOF;
			// if ( (ret=mpa_read_id3v2(&id3hdr, i_stream, mpa)) )
			if ( (ret=mpa_read_id3v2(&id3hdr, in, mpa)) )
				return ret;
			// if ( (ret=istream_read(4, sync_w, i_stream)) != 4 )
			if ( (ret=mpa_read(4, sync_w, in)) != 4 )
				return (ret<0) ? ERR_PARSE : ERR_EOF;
		}
	}
	while ( !MPA_IS_SYNC(sync_w) ) {
		*header >>= 8;
		// sync_w[0]=sync_w[1];
		// sync_w[1]=sync_w[2];
		// sync_w[2]=sync_w[3];

		// if ( (ret=istream_read(1, &sync_w[3], i_stream)) != 1 )
		if ( (ret=mpa_read(1, &sync_w[3], in)) != 1 )
			return (ret<0) ? ERR_PARSE : ERR_EOF;
		fnc_log(FNC_LOG_DEBUG, "[MPA] sync: %X%X%X%X\n", sync_w[0], sync_w[1], sync_w[2], sync_w[3]);
	}

	return 0;
}

static int mpa_decode_header(uint32 header, MediaProperties *properties, mpa_data *mpa)
{
	int RowIndex,ColIndex;
	uint8 *buff_data = (uint8 * )&header;
	int padding;
	int BitrateMatrix[16][5] = BITRATEMATRIX;
	float SRateMatrix[4][3] = SRATEMATRIX;

	if ( !MPA_IS_SYNC(buff_data) ) return -1; /*syncword not found*/

	mpa->id = (buff_data[1] & 0x18) >> 3;
	mpa->layer = (buff_data[1] & 0x06) >> 1;

	switch (mpa->id<< 2 | mpa->layer) {
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
	if (RowIndex == 0xF) // bad bitrate
		return -1;
	if (RowIndex == 0) // free bitrate: not supported
		return -1;
	properties->bit_rate = BitrateMatrix[RowIndex][ColIndex];

	switch (mpa->id) {
		case MPA_MPEG_1: ColIndex = 0; break;
		case MPA_MPEG_2: ColIndex = 1; break;
		case MPA_MPEG_2_5: ColIndex = 2; break;
		default:
			return -1;
			break;
	}

	RowIndex = (buff_data[2] & 0x0c) >> 2;
	if (RowIndex == 3) // reserved
		return -1;
	properties->sample_rate = SRateMatrix[RowIndex][ColIndex];

	// padding
	padding = (buff_data[2] & 0x02) >> 1;

	// packet len
	if (mpa->layer == MPA_LAYER_I) { // layer 1
		mpa->frame_size = 384;
		mpa->pkt_len=((12 * properties->bit_rate)/properties->sample_rate + padding)* 4;
	} else { // layer 2 or 3
		mpa->frame_size = 1152;
		mpa->pkt_len= 144 * properties->bit_rate /properties->sample_rate + padding;
	}
	
#if DEBUG
	mpa_info(mpa, properties);
#endif // DEBUG
	
	return 0;// count; /*return 4*/
}

#if DEBUG
// debug function to diplay MPA header information
static void mpa_info(mpa_data *mpa, MediaProperties *properties)
{
	switch (mpa->id) {
		case MPA_MPEG_1:
			fnc_log(FNC_LOG_DEBUG, "[MPA] MPEG1\n");
			break;
		case MPA_MPEG_2:
			fnc_log(FNC_LOG_DEBUG, "[MPA] MPEG2\n");
			break;
		case MPA_MPEG_2_5:
			fnc_log(FNC_LOG_DEBUG, "[MPA] MPEG2.5\n");
			break;
		default:
			fnc_log(FNC_LOG_DEBUG, "[MPA] MPEG reserved (bad)\n");
			return;
			break;
	}
	switch (mpa->layer) {
		case MPA_LAYER_I:
			fnc_log(FNC_LOG_DEBUG, "[MPA] Layer I\n");
			break;
		case MPA_LAYER_II:
			fnc_log(FNC_LOG_DEBUG, "[MPA] Layer II\n");
			break;
		case MPA_LAYER_III:
			fnc_log(FNC_LOG_DEBUG, "[MPA] Layer III\n");
			break;
		default:
			fnc_log(FNC_LOG_DEBUG, "[MPA] Layer reserved (bad)\n");
			return;
			break;
	}
	fnc_log(FNC_LOG_DEBUG, "[MPA] bitrate: %d; sample rate: %3.0f; pkt_len: %d\n", properties->bit_rate, properties->sample_rate, mpa->pkt_len);
}
#endif // DEBUG

static int mpa_read(uint32 nbytes, uint8 *buf, mpa_input *in)
{
	if (in->istream)
		return istream_read(nbytes, buf, in->istream);
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

