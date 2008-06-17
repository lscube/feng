/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
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

#include <stdlib.h>
#ifdef ENABLE_DUMA
#include <duma.h>
#endif
#include <string.h>
#include <glib.h>

#include <fenice/mediautils.h>

void *MObject_calloc(size_t size)
{
    MObject *new_obj;
    
    new_obj = g_malloc0(size);
    new_obj->refs=1;
    MObject_destructor(new_obj, g_free);

    return new_obj;
}

void *MObject_dup(void *obj, size_t size)
{
    MObject *new_obj;

    if ( (new_obj = g_memdup(obj, size)) )
        new_obj->refs=1;

    return new_obj;
}

void MObject_init(MObject *obj)
{
    obj->refs = 1;
    obj->destructor = NULL;
}

void MObject_zero(MObject *obj, size_t size)
{
    size_t obj_hdr_size;

    // set to zero object data part
    obj_hdr_size = obj->data - (char *)obj;
    memset(obj->data, 0, size-obj_hdr_size);
}

void MObject_unref(MObject *mobject)
{
    if ( mobject && !--mobject->refs && mobject->destructor)
        mobject->destructor(mobject);
}

