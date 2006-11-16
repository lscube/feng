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

 /*
  * Fenice's bufferpool is projected to support ONE AND ONLY ONE producer and many consumers.
  * Each consumer have it's own OMSConsumer structure that is NOT SHARED with others readers.
  * So the only struct that must be locked with mutexes is OMSBuffer.
  * */

#ifndef _BUFFERPOOLH
#define _BUFFERPOOLH

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fenice/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>

#define omsbuff_min(x,y) ((x) < (y) ? (x) : (y))
#define omsbuff_max(x,y) ((x) > (y) ? (x) : (y))

#define OMSBUFF_MEM_PAGE 9

#define OMSBUFFER_DEFAULT_DIM OMSBUFF_MEM_PAGE

#define OMSSLOT_DATASIZE 65000

#define OMSBUFF_SHM_CTRLNAME "Buffer"
#define OMSBUFF_SHM_SLOTSNAME "Slots"
#define OMSBUFF_SHM_PAGE OMSBUFF_MEM_PAGE	// 9


#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define OMSbuff_lock(buffer) pthread_mutex_lock(&buffer->control->syn)
#define OMSbuff_unlock(buffer) pthread_mutex_unlock(&buffer->control->syn)

typedef ptrdiff_t OMSSlotPtr;

#define OMSSLOT_COMMON	uint16 refs; \
			uint64 slot_seq; /* monotone identifier of slot (NOT RTP seq) */ \
			double timestamp; \
			double sendts; /* send time of pkt */ \
			uint8 data[OMSSLOT_DATASIZE]; \
			uint32 data_size; \
			uint8 marker; \
			ptrdiff_t next;

typedef struct _OMSslot {
OMSSLOT_COMMON} OMSSlot;

typedef struct _OMSControl {
	uint16 refs;		/*! n.Consumers that share the buffer */
	uint32 nslots;
	OMSSlotPtr write_pos;	/*! last write position */
	OMSSlotPtr valid_read_pos;	/*! valid read position for new consumers */
	pthread_mutex_t syn;
} OMSControl;

typedef enum { buff_local = 0, buff_shm } OMSBufferType;

/*!
 * Buffer struct.
 * the pointers have different meaning if we use normally allocated memory or shared memory.
 * Using a shared memory buffer pointer will be offsets relative to the beginning of SHM.
 * This way the two processes can add the address returned by mmap to obtain the absolute address.
 * */
typedef struct _OMSbuffer {
	OMSBufferType type;	//! whether buffer is on shared memory or not;
	OMSControl *control;
	OMSSlot *slots;		/*! Buffer head */
	uint32 known_slots;	/*!< last known number of slots. This member is only useful for SHM buffer. */
	// SHM object file
	char filename[PATH_MAX];
	// int fd; /*! pointer to File descriptor of incoming data*/
} OMSBuffer;

typedef struct _OMSconsumer {
	OMSSlotPtr read_pos;	/*! read position */
	OMSSlotPtr last_read_pos;	/*! last read position . used for managing the slot addition */
	uint64 last_seq;
	OMSBuffer *buffer;
	int32 frames;
	double firstts;
	// pthread_mutex_t mutex;
} OMSConsumer;

/*! This structure is usefull if you need to do some syncronization among different correlated buffers.
 * */
typedef struct _OMSAggregate {
	OMSBuffer *buffer;
	struct _OMSAggregate *next;
} OMSAggregate;

/*! API definitions*/
OMSBuffer *OMSbuff_new(uint32);
OMSConsumer *OMSbuff_ref(OMSBuffer *);
void OMSbuff_unref(OMSConsumer *);
int32 OMSbuff_read(OMSConsumer *, uint32 *, uint8 *, uint8 *, uint32 *);
OMSSlot *OMSbuff_getreader(OMSConsumer *);
int32 OMSbuff_gotreader(OMSConsumer *);
int32 OMSbuff_write(OMSBuffer *, uint64, uint32, uint8, uint8 *, uint32);
OMSSlot *OMSbuff_getslot(OMSBuffer *);
OMSSlot *OMSbuff_addpage(OMSBuffer *, OMSSlot *);
//OMSSlot *OMSbuff_slotadd(OMSBuffer *, OMSSlot *);
void OMSbuff_free(OMSBuffer *);

int OMSbuff_isempty(OMSConsumer *);
double OMSbuff_nextts(OMSConsumer *);

// Shared Memory Bufferpool
OMSBuffer *OMSbuff_shm_create(char *, uint32);
OMSBuffer *OMSbuff_shm_map(char *);
OMSSlot *OMSbuff_shm_addpage(OMSBuffer *);
int OMSbuff_shm_remap(OMSBuffer *);
// int OMSbuff_shm_refresh(OMSBuffer *);
#define OMSbuff_shm_refresh(oms_buffer)	\
	(((oms_buffer->type == buff_shm) && (oms_buffer->known_slots != oms_buffer->control->nslots)) ? \
		OMSbuff_shm_remap(oms_buffer) : 0)
int OMSbuff_shm_unmap(OMSBuffer *);
int OMSbuff_shm_destroy(OMSBuffer *);

// shared memory names
char *fnc_ipc_name(const char *, const char *);

// OMSSlotPtr manipulation
#define OMStoSlot(b, p) ((p<0) ? NULL : (&b->slots[p]))	// used when a pointer could be NULL, otherwise we'll use buffe->slots[index]
#define OMStoSlotPtr(b, p) (p ? p - b->slots : -1)

// syncronization of aggregate consumers
OMSAggregate *OMBbuff_aggregate(OMSAggregate *, OMSBuffer *);
int OMSbuff_sync(OMSAggregate *);

// dump for debug 
#if ENABLE_DUMPBUFF
void OMSbuff_dump(OMSConsumer *, OMSBuffer *);
#else
#define OMSbuff_dump(x, y)
#endif


#endif
