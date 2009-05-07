/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#ifndef FN_MEDIAUTILS_H
#define FN_MEDIAUTILS_H

#include <glib.h>

void *MObject_alloc(size_t size, void (*destructor)(void *));
#define MObject_new(type, destructor) (type *)MObject_alloc(sizeof(type), destructor)

void *MObject_dup(void *obj_gen);

void MObject_ref(void *obj_gen);
void MObject_unref(void *obj_gen);

gchar *extradata2config(const guint8 *extradata, gint extradata_size);

#endif // FN_MEDIAUTILS_H

