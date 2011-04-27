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

static int awake = 0;
static int stop_fill = 0;

static GMutex *mux;

static void fill_cb(gpointer cons_p, gpointer prod_p)
{
    BufferQueue_Consumer *consumer = cons_p;
    BufferQueue_Producer *producer = prod_p;

    g_mutex_lock(mux);
    while (bq_consumer_unseen(consumer) < 16) {
        bq_producer_put(producer, g_malloc0 (sizeof(guint64)));
        if(awake++ > 11) {
            sleep(1);
            fprintf(stderr, "Sleeping %p\n", consumer);
            awake = 0;
        }
        if (stop_fill) goto exit;
    }
    exit:
    g_mutex_unlock(mux);
}

void test_5()
{
    int size = 1, i;
    int count = 2000, count_reset = 0;
    int reset = 5;
    BufferQueue_Consumer *cons[size];
    BufferQueue_Producer *prod[size];
    GThreadPool *pool[size];
    mux = g_mutex_new();

//init
    for (i = 0; i< size; i++) {
        prod[i] = bq_producer_new();
        cons[i] = bq_consumer_new(prod[i]);
        pool[i] = g_thread_pool_new(fill_cb, prod[i], 6, FALSE, NULL);
    }

    while(count--) {
        for (i = 0; i < size; i++) {
            void *ret = bq_consumer_get(cons[i]);
            fprintf(stderr, "Foo :%d %p\n", i, ret);
            if(!bq_consumer_move(cons[i]))
                fprintf(stderr, "  %d no next\n", i);
            else
                fprintf(stderr, "  %d next %p\n", i, bq_consumer_get(cons[i]));
            g_thread_pool_push (pool[i], cons[i], NULL);
            if (count_reset++>reset) {
                count_reset = 0;
                bq_producer_reset_queue(prod[i]);
            }
        }
    }
    fprintf(stderr, "---- Killing _put thread\n");
    stop_fill = 1;
    for (i = 0; i < size; i++) {
        g_thread_pool_free(pool[i], TRUE, TRUE);
        if (cons[i]) {
            fprintf(stderr, "---- Deallocating consumer %d %p\n",
                          i, cons[i]);
            bq_consumer_free(cons[i]);
        }
        bq_producer_unref(prod[i]);
    }
    g_mutex_free(mux);
}
