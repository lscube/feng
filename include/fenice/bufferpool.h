/* * 
 *  $Id:$
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

#ifndef _BUFFERPOOLH
#define _BUFFERPOOLH

#include <fenice/mediainfo.h>
#include <fenice/types.h>

typedef struct  _OMSslot {
	uint16 refs; /*!  n.Consumers that must read slot*/
	uint32 timestamp;
	uint8 *data;
	struct _OMSslot *next;
} OMSSlot;
	
typedef struct _OMSbuffer {
	uint16 refs;  /*! n.Consumers that share the buffer*/
	OMSSlot *buffer_head; /*! Buffer head*/
	int fd; /*! File descriptor of incoming data*/
} OMSBuffer;

typedef struct _OMSconsumer {
	OMSSlot *read_pos; /*! read position*/
	OMSBuffer *buffer;
} OMSConsumer;

typedef struct _OMSproducer {
	OMSSlot *write_pos; /*! write position*/
	OMSBuffer *buffer;
} OMSProducer;

/*! API definitions*/
OMSBuffer *OMSbuff_new(uint32 buffer_size);
OMSConsumer *OMSbuff_ref(OMSBuffer *);
uint32 OMSbuff_unref(OMSBuffer *);
OMSSlot *OMSbuff_read();
uint32 *OMSbuff_write(OMSBuffer *, uint32 timestamp, uint8 *data);

#endif

