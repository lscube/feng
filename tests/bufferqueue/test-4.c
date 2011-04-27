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

#include "bufferqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_4()
{
    int i;
    int count = 24;
    guint64 *ret;
    guint64 *buffer = g_malloc0(sizeof(guint64));
    BufferQueue_Producer *prod = bq_producer_new();
    BufferQueue_Consumer *cons = bq_consumer_new(prod);

    for (i = 0; i < count; i++)
        bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    for (i = 0; i < count/2; i++) {
        bq_consumer_get(cons);
        bq_consumer_move(cons);
    }

    bq_producer_reset_queue(prod);

    for (i = 0; i < count; i++)
        bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    while(count--) {
        ret = bq_consumer_get(cons);
        g_assert(ret != NULL);
        bq_consumer_move(cons);
    }

    fprintf(stderr, "---- Deallocating consumer");
    bq_consumer_free(cons);

    bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    bq_producer_unref(prod);
    g_free(buffer);
}
