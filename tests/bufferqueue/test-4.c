#include "bufferqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void test_4()
{
    if (!g_thread_supported ()) g_thread_init (NULL);

    int size = 10, i;
    int count = 24;
    guint64 *ret;
    guint64 *buffer = g_malloc0(sizeof(guint64));
    BufferQueue_Producer *prod = bq_producer_new(g_free);
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
