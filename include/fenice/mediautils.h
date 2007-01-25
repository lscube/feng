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

#ifndef __MEDIAUTILS_H
#define __MEDIAUTILS_H

#define MOBJECT_COMMONS	int refs; \
			void (*destructor)(void *)

#define MObject_def(x)	typedef struct x { \
			MOBJECT_COMMONS;

#define MOBJECT(x)	((MObject *)x)
typedef struct {
	MOBJECT_COMMONS;
	char data[1];
} MObject;

#define MObject_new(type, n) (type *)MObject_malloc(n*sizeof(type))
#define MObject_new0(type, n) (type *)MObject_calloc(n*sizeof(type))
//#define MObject_newa(type, n) (type *)MObject_alloca(n*sizeof(type))
#define MObject_0(obj, type) MObject_zero(obj, sizeof(type))
#define MObject_ref(object) ++((object)->refs)
#define MObject_destructor(object, destr) (object)->destructor=destr

// void *MObject_new(size_t);
void *MObject_malloc(size_t);
void *MObject_calloc(size_t);
//inline void *MObject_alloca(size_t);
void MObject_init(MObject *);
void MObject_zero(MObject *, size_t);
void *MObject_dup(void *, size_t);
void MObject_unref(MObject *);

#ifndef _MEDIAINFOH
//TODO: Remove this legacy from mediathread!
#define MAX_DESCR_LENGTH 4096
typedef enum { stored = 0, live } media_source;

typedef enum {
	df_SDP_format = 0
} description_format;

typedef enum {
	SD_FL_TWIN = 1,
	SD_FL_MULTICAST = 2,
	SD_FL_MULTICAST_PORT = 4
} sd_descr_flags;

typedef struct _SD_descr {
	char filename[255];
//	media_entry *me_list;
	time_t last_change;
	struct _SD_descr *next;
	sd_descr_flags flags;
	char multicast[16];
	char ttl[4];
	char twin[255];
} SD_descr;
#endif

#endif // __MEDIAUTILS_H

