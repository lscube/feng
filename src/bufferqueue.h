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

#ifndef FENG_BUFFERQUEUE_H
#define FENG_BUFFERQUEUE_H

#include <glib.h>

typedef struct BufferQueue_Producer BufferQueue_Producer;
typedef struct BufferQueue_Consumer BufferQueue_Consumer;

BufferQueue_Producer *bq_producer_new(GDestroyNotify free_function, gchar *key);
void bq_producer_put(BufferQueue_Producer *producer, gpointer payload);
void bq_producer_reset_queue(BufferQueue_Producer *producer);
void bq_producer_unref(BufferQueue_Producer *producer);

void bq_init();
BufferQueue_Consumer *bq_consumer_new(BufferQueue_Producer *producer);
gpointer bq_consumer_get(BufferQueue_Consumer *consumer);
gulong bq_consumer_unseen(BufferQueue_Consumer *consumer);
gboolean bq_consumer_move(BufferQueue_Consumer *consumer);
gboolean bq_consumer_stopped(BufferQueue_Consumer *consumer);
void bq_consumer_free(BufferQueue_Consumer *consumer);

#endif
