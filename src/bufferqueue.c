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

/**
 * @defgroup bufferqueue Buffer Queue
 *
 * @brief Producer/consumer buffer queue structure
 *
 * The buffer queue is a data structure that allows one producer
 * (i.e.: a demuxer) to read data and feed it to multiple consumers
 * (i.e.: the RTSP clients).
 *
 * The structure is implemented in a very simple way: there is one big
 * double-ended queue for each producer, the elements of this queue
 * are wrappers around the actual buffer structures. Each time the
 * producer reads a buffer, it is added to its queue, with a “seen
 * count” of zero.
 *
 * Each consumer gets, at any time, the pointer to the start of the
 * queue and as it moves on to the following element, the “seen count”
 * gets incremented. Once the seen count reaches the amount of
 * consumers, the element is deleted from the queue.
 *
 * @{
 */

/**
 * @brief Element structure
 *
 * This is the structure that is put in the list for each buffer
 * produced by the producer.
 */
typedef struct {
    /**
     * @brief Element data
     *
     * Pointer to the actual data represented by the element. This is
     * what the reading function returns to the caller.
     *
     * When the element is deleted, this is passed as parameter to the
     * @ref BufferQueue_Producer::free_function function.
     */
    gpointer payload;

    /**
     * @brief Seen count
     *
     * Reverse reference counter, that tells how many consumers have
     * seen the buffer already. Once the buffer has been seen by all
     * the consumers of its producer, the buffer is deleted and the
     * queue is shifted further on.
     *
     * @note Since once the counter reaches the number of consumers
     *       the element is deleted and freed, before increasing the
     *       counter it is necessary to hold the @ref
     *       BufferQueue_Producer lock.
     */
    gulong seen;
} BufferQueue_Element;

/**
 * @brief Producer structure
 *
 * This is the structure that keeps all the information related to the
 * buffer queue producer. Which means the main lock for the queue and
 * the queue itself, as well as the eventual functions to call for
 * events and the consumers count.
 */
struct BufferQueue_Producer {
    /**
     * @brief Lock for the producer.
     *
     * This lock must be held in almost every case:
     *
     * @li when a new consumer is registered;
     * @li when the consumer request a new buffer,
     * @li when the producer adds a new buffer to the queue;
     * @li when a consumer is removed.
     */
    GMutex *lock;

    /**
     * @brief The actual buffer queue
     *
     * This double-ended queue contains the actual buffer elements
     * that the BufferQueue framework deals with.
     */
    GQueue *queue;

    /**
     * @brief Function to free elements
     *
     * Since each element in the queue is freed once all the consumers
     * have seen it, its payload needs to be freed at that time as
     * well.
     *
     * To make the framework independent from the actual function to
     * use to free the buffers, this has to be set by the consumer
     * itself.
     */
    GDestroyNotify free_function;

    /**
     * @brief Count of registered consumers
     *
     * This attribute keeps the updated number of registered
     * consumers; each buffer needs to reach the same count before it
     * gets removed from the queue.
     */
    gulong consumers;

    /**
     * @brief New data available for blocked threads
     *
     * This condition is signalled once new data is available from the
     * producer.
     *
     * @see bq_consuper_put
     */
    GCond *new_data;

    /**
     * @brief Last consumer exited condition
     *
     * This condition is signalled once the last consumer of a stopped
     * producer (see @ref stopped) exits.
     *
     * @see bq_consumer_stop
     */
    GCond *last_consumer;

    /**
     * @brief Stopped flag
     *
     * When this value is set to true, the consumer is stopped and no
     * further elements can be added. To set this flag call the
     * @ref bq_producer_stop function.
     *
     * A stopped producer cannot accept any new consumer, and will
     * wait for all the currently-connected consumers to return before
     * it is deleted.
     *
     * @note gint is used to be able to use g_atomic_int_get function.
     */
    gint stopped;
};

/**
 * @brief Consumer structure
 *
 * This is the structure that each consumer gets and that is used as
 * context for all the buffer queue interfaces.
 */
struct BufferQueue_Consumer {
    /**
     * @brief Link to the producer
     *
     * Each consumer has a direct link to the producer, to be able to
     * access its data without duplication.
     */
    BufferQueue_Producer *producer;

    /**
     * @brief Last used producer queue
     *
     * Each producer can drop its queue and create a new one, for
     * instance if there is a discontinuity of any kind in the
     * produced stream. For this reason, the consumer needs to track
     * whether its references are still valid or not, by checking the
     * last used queue and the current one.
     */
    GQueue *last_queue;

    /**
     * @brief Pointer to the current element in the list
     *
     * This pointer is used to access the "next to serve" buffer; and
     * is originally set to the head of @ref BufferQueue_Producer
     * queue.
     */
    GList *current_element_pointer;

    /**
     * @brief Pointer to the last seen element
     *
     * Since a producer can change queue, the @ref
     * BufferQueue_Consumer::current_element pointer might not be
     * valid any longer; to make sure that the element is not deleted
     * before time, a copy of it is kept here.
     */
    BufferQueue_Element *current_element_object;
};

/**
 * @brief Create a new producer for the bufferqueue
 *
 * @param free_function Function to call when removing buffers from
 *                      the queue.
 *
 * @return A new BufferQueue_Producer object that needs to be freed
 *         with @ref bq_producer_stop.
 */
BufferQueue_Producer *bq_producer_new(GDestroyNotify free_function) {
    BufferQueue_Producer *ret = g_slice_new(BufferQueue_Producer);

    ret->lock = g_mutex_new();
    ret->queue = g_queue_new();
    ret->free_function = free_function;
    ret->consumers = 0;
    ret->new_data = g_cond_new();
    ret->last_consumer = g_cond_new();
    ret->stopped = 0;

    return ret;
}

/**
 * @brief Destroy one by one the elements in
 *        BufferQueue_Producer::queue.
 *
 * @param elem_generic Element to destroy
 * @param free_func_generic Function to use for destroying the
 *                          elements' payload.
 */
static void bq_element_free_internal(gpointer elem_generic,
                                     gpointer free_func_generic) {
    BufferQueue_Element *const element = (BufferQueue_Element*)elem_generic;
    const GDestroyNotify free_function = (GDestroyNotify)free_func_generic;

    free_function(element->payload);
    g_slice_free(BufferQueue_Element, element);
}

/**
 * @brief Destroy a producer
 *
 * @param producer Producer to destroy.
 *
 * This function recursively destroys all the elements of the
 * producer, included the producer itself.
 *
 * @warning This function should only be called when all consumers
 *          have been unregistered. If that's not the case the
 *          function will cause the program to abort.
 */
static void bq_producer_free_internal(BufferQueue_Producer *producer) {
    g_assert_cmpuint(producer->consumers, ==, 0);

    g_mutex_unlock(producer->lock);
    g_mutex_free(producer->lock);

    /* Destroy elements and the queue */
    g_queue_foreach(producer->queue,
                    bq_element_free_internal,
                    producer->free_function);
    g_queue_free(producer->queue);

    g_slice_free(BufferQueue_Producer, producer);
}

/**
 * @brief Stop a producer
 *
 * @param producer Producer to stop
 *
 * This function marks a producer as stopped, refusing further
 * consumers from connecting, then waits for all of the consumer to
 * disconnect before deleting the producer object itself.
 */
void bq_producer_stop(BufferQueue_Producer *producer) {
    /* Compatibility with free(3) */
    if ( producer == NULL )
        return;

    g_assert(g_atomic_int_get(&producer->stopped) == 0);

    g_atomic_int_set(&producer->stopped, 1);

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    if ( producer->consumers > 0 )
        g_cond_wait(producer->last_consumer, producer->lock);

    bq_producer_free_internal(producer);
}

/**
 * @brief Adds a new buffer to the producer
 *
 * @param producer The producer to add the element to
 * @param payload The buffer to link in the element
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * @note The @p payload pointer will be freed with @ref
 *       BufferQueue_Producer::free_function.
 *
 * @note The memory area provided as @p payload parameter should be
 *       considered used and should not be freed manually.
 */
void bq_producer_put(BufferQueue_Producer *producer, gpointer payload) {
    BufferQueue_Element *elem;

    /* Make sure the producer is not stopped */
    g_assert(g_atomic_int_get(&producer->stopped) == 0);

    elem = g_slice_new(BufferQueue_Element);
    elem->payload = payload;
    elem->seen = 0;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    g_queue_push_tail(producer->queue, elem);

    g_cond_broadcast(producer->new_data);

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);
}

/**
 * @brief Create a new consumer
 *
 * @param producer The producer to link the consumer to.
 * @return A new BufferQueue_Consumer object that needs to be freed
 *         with @ref bq_consumer_free
 *
 * @retval NULL The producer is stopped and new consumers cannot be
 *              attached.
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 */
BufferQueue_Consumer *bq_consumer_new(BufferQueue_Producer *producer) {
    BufferQueue_Consumer *ret;

    if ( g_atomic_int_get(&producer->stopped) == 1 )
        return NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    ret = g_slice_new(BufferQueue_Consumer);

    /* Make sure we don't overflow the consumers count; while this
     * case is most likely just hypothetical, it doesn't hurt to be
     * safe.
     */
    g_assert_cmpuint(producer->consumers, <, G_MAXULONG);

    producer->consumers++;

    ret->producer = producer;
    ret->last_queue = NULL;
    ret->current_element_pointer = NULL;
    ret->current_element_object = NULL;

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return ret;
}

/**
 * @brief Remove reference from the current element
 *
 * @param consumer The consumer that has seen the element
 *
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 */
static void bq_consumer_elem_unref(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer = consumer->producer;
    BufferQueue_Element *elem = consumer->current_element_object;

    /* If we had no element selected, just get out of here */
    if ( elem == NULL )
        return;

    /* If we're the last one to see the element, we need to take care
     * of removing and freeing it. */
    if ( ++elem->seen < producer->consumers )
        return;

    bq_element_free_internal(elem, producer->free_function);

    /* Make sure to lose reference to it */
    consumer->current_element_object = NULL;

    /* Only remove the element from the queue if it's still the
     * one used by the producer.
     */
    if ( producer->queue != consumer->last_queue )
        return;

    /* We can only remove the head of the queue, if we're doing
     * anything else, something is funky.
     */
    g_assert(consumer->current_element_pointer == producer->queue->head);

    /* Remove the element from the queue */
    g_queue_pop_head(producer->queue);
}

/**
 * @brief Destroy a consumer
 *
 * @param consumer The consumer object to destroy
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 */
void bq_consumer_free(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer;

    /* Compatibility with free(3) */
    if ( consumer == NULL )
        return;

    producer = consumer->producer;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    /* We should never come to this point, since we are expected to
     * have symmetry between new and free calls, but just to be on the
     * safe side, make sure this never happens.
     */
    g_assert_cmpuint(producer->consumers, >,  0);

    bq_consumer_elem_unref(consumer);

    if ( --producer->consumers == 0 ) {
        if ( producer->stopped ) {
            g_cond_signal(producer->last_consumer);
        } else {
            /* Decrement consumers and check, if we're the latest consumer, we
             * want to clean the queue up entirely!
             */
            g_queue_foreach(producer->queue,
                            bq_element_free_internal,
                            producer->free_function);
            g_queue_clear(producer->queue);
        }
    } else if ( consumer->current_element_object != NULL ) {
        /* If we haven't removed the current selected element, it
         * means that we have to decrease the seen count for all the
         * elements before it.
         */

        GList *it = consumer->current_element_pointer;
        do {
            BufferQueue_Element *elem = (BufferQueue_Element*)(it->data);

            /* If we were the last one to see this we would have
             * deleted it, since we haven't, we expect that there will
             * be at least another consumer waiting on it.
             *
             * But let's be safe and assert this.
             */
            g_assert_cmpuint(elem->seen, <, producer->consumers);

            elem->seen--;
        } while ( (it = it->prev) != NULL );
    }

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    g_slice_free(BufferQueue_Consumer, consumer);
}

/**
 * @brief Get the next element from the consumer list
 *
 * @param consumer The consumer object to get the data from
 *
 * @return A pointer to the payload of the newly selected element
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * This function marks as seen the previously-selected element, if
 * any; the new selection is not freed until the cursor is moved or
 * the consumer is deleted.
 */
gpointer bq_consumer_get(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer = consumer->producer;
    gpointer ret = NULL;
    GList *expected_next = NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    while ( expected_next == NULL ) {
        if ( producer->stopped )
            goto end;

        if ( consumer->last_queue != producer->queue ) {
            /* If the last used queue does not correspond to the current
             * producer's queue, we have to take the head of the new
             * queue.
             *
             * This also hits at the first request, since the last_queue
             * will be NULL.
             */
            consumer->last_queue = producer->queue;
            expected_next = producer->queue->head;
        } else {
            expected_next = consumer->current_element_pointer->next;
        }

        /* If at this point we don't have an element to read, it means
         * that the producer is either stopped or it hasn't data to
         * work with, so let's wait.
         */
        if ( expected_next == NULL )
            g_cond_wait(producer->new_data, producer->lock);
    }

    /* If there is any element at all saved, we take care of marking
     * it as seen. We don't have to check if it's non-NULL since the
     * function takes care of that. We _have_ to do this after we
     * found the new "next" pointer.
     */
    bq_consumer_elem_unref(consumer);

    /* Now we have a new "next" element and we can set it properly */
    consumer->current_element_pointer = expected_next;
    consumer->current_element_object = (BufferQueue_Element *)consumer->current_element_pointer->data;

    /* Get the payload of the element */
    ret = consumer->current_element_object->payload;

 end:
    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return ret;
}

/**@}*/
