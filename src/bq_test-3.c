#include "bufferqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int stop_fill = 0;

int main(void)
{
    if (!g_thread_supported ()) g_thread_init (NULL);

    int size = 10, i;
    int count = 24;
    guint64 *ret;
    guint64 *buffer = g_malloc0(sizeof(guint64));
    BufferQueue_Consumer *cons[size];
    BufferQueue_Producer *prod = bq_producer_new(g_free);

//init
    for (i = 0; i < size; i++) {
        fprintf(stderr,"---- Allocating consumer %d ", i);
        cons[i] = bq_consumer_new(prod);
        fprintf(stderr,"%p\n", cons[i]);
    }

    for (i = 0; i < count; i++)
        bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    fprintf(stderr, "---- Killing _put thread\n");
    stop_fill = 1;
    for (i = 0; i < size/2; i++)
        bq_consumer_get(cons[i]);
    for (i = size/2; i < size; i++) {
        bq_consumer_get(cons[i]);
        bq_consumer_move(cons[i]);
    }

    for (i = 0; i < count; i++)
        bq_producer_put(prod, g_memdup (buffer, sizeof(guint64)));

    for (i = 0; i < size; i++) {
        bq_consumer_get(cons[i]);
        bq_consumer_move(cons[i]);
    }

    while(count--)
        for (i = 0; i < size; i++) {
            bq_consumer_get(cons[i]);
            bq_consumer_move(cons[i]);
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
    return 0;
}
