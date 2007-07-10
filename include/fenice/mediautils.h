/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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

#endif // __MEDIAUTILS_H

