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

void test_3()
{
    int size = 10, i, j;
    int count = 24;
    guint64 *ret;
    guint64 *buffer = g_malloc0(sizeof(guint64));
    BufferQueue_Consumer *cons[size];
    BufferQueue_Producer *prod = bq_producer_new(g_free);

//init
    for (i = 0; i < size/2; i++) {
        fprintf(stderr,"---- Allocating consumer %d ", i);
        cons[i] = bq_consumer_new(prod);
        fprintf(stderr,"%p\n", cons[i]);
    }

    for (i = 0; i < count/2; i++) {
        *buffer = i;
        fprintf(stderr, "---- Putting %d\n", i);
        bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));
    }

    for (j = 0; j < count; j++)
        for (i = 0; i < size/2-1; i++) {
            ret = bq_consumer_get(cons[i]);
            g_assert(ret != NULL );
            g_assert_cmpuint(*ret, ==, 0);
        }

    for (i = size/2; i < size; i++)
        cons[i] = bq_consumer_new(prod);

    for (j = 0; j < count; j++)
        for (i = size/2-1; i < size; i++) {
            ret = bq_consumer_get(cons[i]);
            g_assert(ret != NULL );
            g_assert_cmpuint(*ret, ==, 0);
        }

    for (i = size/2; i < size; i++) {
        bq_consumer_free(cons[i]);
    }

    for (j = 0; j < count; j++)
        for (i = 0; i < size/2; i++) {
            ret = bq_consumer_get(cons[i]);
            if (ret) g_assert_cmpuint(*ret, ==, j);
            else g_assert_cmpint(j, >=, count/2);

            if (bq_consumer_move(cons[i])) {
                ret = bq_consumer_get(cons[i]);
                if (ret) g_assert_cmpuint(*ret, ==, j+1);
                else g_assert_cmpint(j+1, >=, count/2);
            }
        }

    for (i = size/2; i < size; i++)
        cons[i] = bq_consumer_new(prod);

    for (j = 0; j < count; j++)
        for (i = size/2-1; i < size; i++) {
            ret = bq_consumer_get(cons[i]);
            g_assert(ret == NULL);

            g_assert(!bq_consumer_move(cons[i]));
        }


    for (i = 0; i < size; i++)
        if (cons[i]) {
            fprintf(stderr, "---- Deallocating consumer %d %p\n",
                          i, cons[i]);
            bq_consumer_free(cons[i]);
        }

    bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    bq_producer_unref(prod);
    g_free(buffer);
}
