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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "mediautils.h"

/**
 * @defgroup MObject MObject interface
 *
 * @{
 */

/**
 * @brief The MObject preamble
 *
 * This structure is used to allocate the areas that are accessed by
 * multiple threads by refcounting.
 *
 * This actually is just a preamble with a final “data” entry that
 * points to the actual structure that is used by the rest of the
 * software.
 */
typedef struct {
    /** Number of threads referencing the object */
    guint refs;
    /** Size of MObject::data area */
    size_t size;
    /** Mutex for the MObject premable
     *
     * @todo It should be possible to make this recursive and provide
     *       lock/unlock functions to avoid adding further locks to
     *       the rest of the structure.
     */
    GMutex *mutex;

    /** Destructor function to call on the object
     *
     * It's important to note that this function should _never_ try to
     * free the data, since it's the MObject interface to handle that.
     */
    void (*destructor)(void *);

    /** The actual data of the structure
     */
    char data[];
} MObject;

/**
 * @brief Allocate a new MObject instance (internal interface)
 *
 * @param size The size of the memory area to allocate
 * @param destructor The function to call before freeing the object
 *
 * @return A pointer to the new MObject
 */
static MObject *MObject_alloc_internal(size_t size, void (*destructor)(void*))
{
    MObject *new_obj;

    new_obj = g_malloc0(sizeof(MObject)+size);

    fprintf(stderr, "New MObject %p [%zu + %zu]\n", new_obj, sizeof(MObject), size);

    new_obj->refs = 1;
    new_obj->size = size;
    new_obj->destructor = destructor;
    new_obj->mutex = g_mutex_new();

    return &new_obj->data;
}

/**
 * @brief Allocate a new MObject instance
 *
 * @param size The size of the memory area to allocate
 * @param destructor The function to call before freeing the object
 *
 * @return A pointer to the MObject::data element of the newly
 *         allocated object.
 */
void *MObject_alloc(size_t size, void (*destructor)(void*))
{
    MObject *new_obj = MObject_alloc_internal(size, destructor);
    return &new_obj->data;
}

/**
 * @brief Data to MObject translator
 *
 * Translates the generic pointer (MObject::data) into the actual
 * MObject.
 *
 * @param data The MObject::data pointer
 */
#define MOBJECT(data) (data - sizeof(MObject))

/**
 * @brief Duplicates the content of a MObject
 *
 * @param obj_gen The object to duplicate
 */
void *MObject_dup(void *obj_gen)
{
    MObject *obj = MOBJECT(obj_gen);
    MObject *new_obj = MObject_alloc_internal(obj->size, obj->destructor);

    memcpy(&new_obj->data, &obj->data, obj->size);

    return &new_obj->data;
}

/**
 * @brief Increases the refcount of an MObject
 *
 * @param obj_gen The object to work on
 */
void MObject_ref(void *obj_gen)
{
    MObject *obj = MOBJECT(obj_gen);

    g_mutex_lock(obj->mutex);
    obj->refs++;
    g_mutex_unlock(obj->mutex);
}

/**
 * @brief Decreases the refcount of an MObject
 *
 * @param obj_gen The object to work on
 */
void MObject_unref(void *obj_gen)
{
    MObject *obj = MOBJECT(obj_gen);

    g_mutex_lock(obj->mutex);
    if ( obj->refs-- == 0 ) {
        obj->destructor(&obj->data);
        g_mutex_unlock(obj->mutex);
        g_mutex_free(obj->mutex);

        g_free(obj);
        return;
    }

    g_mutex_unlock(obj->mutex);
}

/**
 * @}
 */

// Ripped from ffmpeg, see sdp.c

static void digit_to_char(gchar *dst, guint8 src)
{
    if (src < 10) {
        *dst = '0' + src;
    } else {
        *dst = 'A' + src - 10;
    }
}

static gchar *data_to_hex(gchar *buff, const guint8 *src, gint s)
{
    gint i;

    for(i = 0; i < s; i++) {
        digit_to_char(buff + 2 * i, src[i] >> 4);
        digit_to_char(buff + 2 * i + 1, src[i] & 0xF);
    }

    return buff;
}

gchar *extradata2config(const guint8 *extradata, gint extradata_size)
{
    gchar *config = g_malloc(extradata_size * 2 + 1);

    if (config == NULL) {
        return NULL;
    }

    data_to_hex(config, extradata, extradata_size);

    config[extradata_size * 2] = '\0';

    return config;
}
