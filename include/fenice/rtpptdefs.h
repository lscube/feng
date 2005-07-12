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
 
#ifndef __RTP_PT_DEFS_H
#define __RTP_PT_DEFS_H

	#include <config.h>
	#include <fenice/mediainfo.h>

	#ifndef GLOBAL_RTP_DEFS
	#define RTP_DEFS_EXTERN extern
	#else /* GLOBAL_RTP_DEFS */
	#define RTP_DEFS_EXTERN
	#endif /* GLOBAL_RTP_DEFS */


	typedef struct _RTP_static_payload {
		int PldType;
	        char *EncName;
	        int ClockRate;      // In Hz
        	short Channels;
	        int BitPerSample;
	        media_coding Type;  // FRAME/SAMPLE
	        float PktLen;       // In msec
	} RTP_static_payload;	
	

	RTP_DEFS_EXTERN RTP_static_payload RTP_payload[]
	#ifdef GLOBAL_RTP_DEFS
		={
	        // Payload Type, Encode Name, Clock Rate, Audio Channels, Bit per Sample, Type, Pkt Lenght, Audio: 0-23
		
	    	{0 ,"PCMU"   ,8000 ,1 ,8 ,sample,20},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{2 ,"G726_32",8000 ,1 ,4 ,sample,20},	{3 ,"GSM"    ,8000 ,1 ,-1,frame ,20},
	        {4 ,"G723"   ,8000 ,1 ,-1,frame ,30},	{5 ,"DVI4"   ,8000 ,1 ,4 ,sample,20},	{6 ,"DVI4"   ,16000,1 ,4 ,sample,20},	{7 ,"LPC"    ,8000 ,1 ,-1,frame ,20},
	        {8 ,"PCMA"   ,8000 ,1 ,8 ,sample,20},	{9 ,"G722"   ,8000 ,1 ,8 ,sample,20},	{10,"L16"    ,44100,2 ,16,sample,20},	{11,"L16"    ,44100,1 ,16,sample,20},
	        {12,"QCELP"  ,8000 ,1 ,-1,frame ,20},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{14,"MPA"    ,90000,1 ,-1,frame ,-1},	{15,"G728"   ,8000 ,1 ,-1,frame ,20},
	        {16,"DVI4"   ,11025,1 ,4 ,sample,20},	{17,"DVI4"   ,22050,1 ,4 ,sample,20},	{18,"G729"   ,8000 ,1 ,-1,frame ,20},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
	        {-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
	    	// Video: 24-95 - Pkt_len in milliseconds is not specified and will be calculated in such a way
		// that each RTP packet contains a video frame (but no more than 536 byte, for UDP limitations)
	        {-1,""       ,-1   ,-1,-1,-1    ,-1},	{25,"CelB"   ,90000,0 ,-1,frame ,-1},	{26,"JPEG"   ,90000,0 ,-1,frame ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
	        {28,"nv"     ,90000,0 ,-1,frame ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{31,"H261"   ,90000,0 ,-1,frame ,-1},
	        {32,"MPV"    ,90000,0 ,-1,frame ,-1},	{33,"MP2T"   ,90000,0 ,-1,frame ,-1},	{34,"H263"   ,90000,0 ,-1,frame ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
	        // Dynamic: 96-127
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},
		{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1},	{-1,""       ,-1   ,-1,-1,-1    ,-1}
	}
	#endif /* GLOBAL_RTP_DEFS */
	;

#endif /* __RTP_PT_DEFS_H */
