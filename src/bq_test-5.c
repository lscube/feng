#include "bufferqueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int awake = 0;
int stop_fill = 0;

GMutex *mux;

static void fill_cb(gpointer cons_p, gpointer prod_p)
{
    g_mutex_lock(mux);
    BufferQueue_Consumer *consumer = cons_p;
    BufferQueue_Producer *producer = prod_p;

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

int main(void)
{
    if (!g_thread_supported ()) g_thread_init (NULL);

    int size = 1, i;
    int count = 2000, count_reset = 0;
    int reset = 5;
    BufferQueue_Consumer *cons[size];
    BufferQueue_Producer *prod[size];
    mux = g_mutex_new();
    GThreadPool *pool[size];

//init
    for (i = 0; i< size; i++) {
        prod[i] = bq_producer_new(g_free);
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
    return 0;
}
