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

#include <stdbool.h>

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

    /**
     * @brief Serial number of the buffer
     *
     * This value is the “serial number” of the buffer, which, simply
     * put, is the sequence number of the current buffer in its queue.
     *
     * This is taken from @ref BufferQueue_Producer::next_serial, and
     * is used by @ref bq_consumer_unseen() to provide the number of
     * not yet seen buffers.
     */
    gulong serial;
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
     * @brief Next serial to use for the added elements
     *
     * This is the next value for @ref BufferQueue_Element::serial; it
     * starts from zero and it's reset to zero each time the queue is
     * reset; each element added to the queue gets this value before
     * getting incremented; it is used by @ref bq_consumer_unseen().
     */
    gulong next_serial;

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
     * @brief Last consumer exited condition
     *
     * This condition is signalled once the last consumer of a stopped
     * producer (see @ref stopped) exits.
     *
     * @see bq_consumer_unref
     */
    GCond *last_consumer;

    /**
     * @brief Stopped flag
     *
     * When this value is set to true, the consumer is stopped and no
     * further elements can be added. To set this flag call the
     * @ref bq_producer_unref function.
     *
     * A stopped producer cannot accept any new consumer, and will
     * wait for all the currently-connected consumers to return before
     * it is deleted.
     *
     * @note gint is used to be able to use g_atomic_int_get function.
     */
    gint stopped;

    /**
     * @brief Reset Queue flag
     *
     * When this value is set to 1, the producer will change the queue
     * at the first time a new buffer is supposed to be added. This
     * allows for an atomic switch of the queue.
     *
     * @note gint is used to be able to use g_atomic_int_get function.
     */
    gint reset_queue;
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
     * BufferQueue_Consumer::current_element_pointer might not be
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
 *         with @ref bq_producer_unref.
 */
BufferQueue_Producer *bq_producer_new(GDestroyNotify free_function) {
    BufferQueue_Producer *ret = g_slice_new(BufferQueue_Producer);

    ret->lock = g_mutex_new();
    ret->queue = NULL;
    ret->free_function = free_function;
    ret->consumers = 0;
    ret->last_consumer = g_cond_new();
    ret->stopped = 0;
    ret->reset_queue = 1;

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

    if ( producer->queue ) {
        /* Destroy elements and the queue */
        g_queue_foreach(producer->queue,
                        bq_element_free_internal,
                        producer->free_function);
        g_queue_free(producer->queue);
    }

    g_slice_free(BufferQueue_Producer, producer);
}

/**
 * @brief Frees a producer if there aren't consumers attached to it
 *
 * @param producer Producer to be freed
 *
 */
void bq_producer_unref(BufferQueue_Producer *producer) {
    /* Compatibility with free(3) */
    if ( producer == NULL )
        return;

    if ( producer->consumers > 0 ) return;

    g_assert(g_atomic_int_get(&producer->stopped) == 0);

    g_atomic_int_set(&producer->stopped, 1);

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    bq_producer_free_internal(producer);
}

/**
 * @brief Resets a producer's queue
 *
 * @param producer Producer to reset the queue of
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * This function will change the currently-used queue for the
 * producer, so that a discontinuity will allow the consumers not to
 * worry about getting old buffers.
 */
void bq_producer_reset_queue(BufferQueue_Producer *producer) {
    g_assert(!producer->stopped);
    g_atomic_int_set(&producer->reset_queue, 1);
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
    elem->serial = producer->next_serial++;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    /* Check whether we have to reset the queue */
    if ( producer->reset_queue ) {
        /** @todo we should make sure that the old queue is reaped
         * before continuing, but we can do that later */

        producer->queue = g_queue_new();
        producer->reset_queue = 0;
        producer->next_serial = 0;
    }

    g_queue_push_tail(producer->queue, elem);

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
        } else if ( producer->queue ) {
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
 * @brief Actually move to the next element in a consumer
 *
 * @retval true The move was successful
 * @retval false The move wasn't successful.
 *
 * @param consumer The consumer object to move
 */
static gboolean bq_consumer_move_internal(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer = consumer->producer;
    GList *expected_next = NULL;

    if ( consumer->last_queue != producer->queue ) {
        /* If the last used queue does not correspond to the current
         * producer's queue, we have to take the head of the new
         * queue.
         *
         * This also hits at the first request, since the last_queue
         * will be NULL.
         */
        expected_next = producer->queue->head;
    } else if ( consumer->current_element_pointer ) {
        expected_next = consumer->current_element_pointer->next;
    }

    if ( expected_next == NULL )
        return false;

    /* If there is any element at all saved, we take care of marking
     * it as seen. We don't have to check if it's non-NULL since the
     * function takes care of that. We _have_ to do this after we
     * found the new "next" pointer.
     */
    bq_consumer_elem_unref(consumer);

    /* Now we have a new "next" element and we can set it properly */
    consumer->last_queue = producer->queue;
    consumer->current_element_pointer = expected_next;
    consumer->current_element_object = (BufferQueue_Element *)consumer->current_element_pointer->data;

    return true;
}

/**
 * @brief Tells how many buffers are queued to be seen
 *
 * @param consumer The consumer object to check
 *
 * @return The number of buffers queued in the producer that have not
 *         been seen.
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 */
gulong bq_consumer_unseen(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer = consumer->producer;
    gulong serial;

    if ( g_atomic_int_get(&producer->stopped) )
        return 0;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    serial = producer->next_serial;

    if ( consumer->last_queue == producer->queue
         && consumer->current_element_object != NULL )
        serial -= consumer->current_element_object->serial;

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return serial;
}

/**
 * @brief Move to the next element in a consumer
 *
 * @param consumer The consumer object to move
 *
 * @retval true The move was successful
 * @retval false The move wasn't successful, the producer may be stopped.
 *
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * This is actually just a locking-wrapper around @ref
 * bq_consumer_move_internal.
 */
gboolean bq_consumer_move(BufferQueue_Consumer *consumer) {
    BufferQueue_Producer *producer = consumer->producer;
    gboolean ret;

    if ( g_atomic_int_get(&producer->stopped) )
        return false;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    ret = bq_consumer_move_internal(consumer);

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return ret;
}

/**
 * @brief Get the next element from the consumer list
 *
 * @param consumer The consumer object to get the data from
 *
 * @return A pointer to the payload of the newly selected element
 *
 * @retval NULL No element can be read; this might be due to no data
 *              present in the producer, or if the producer was
 *              stopped. To know which one of the two conditions
 *              happened, @ref bq_consumer_stopped should be called.
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

    if ( g_atomic_int_get(&producer->stopped) )
        return NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    /* If we don't have a queue yet, like for the first read, “move
     * next” (or rather first).
     */
    if ( consumer->last_queue == NULL &&
         !bq_consumer_move_internal(consumer) )
        goto end;

    /* Get the payload of the element */
    ret = consumer->current_element_object->payload;

 end:
    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return ret;
}

/**
 * @brief Checks if a consumer is tied to a stopped producer
 *
 * @param consumer The consumer object to get the data from
 *
 * @retval true The producer is currently stopped and no further
 *              elements will be returned by @ref bq_consumer_get.
 * @retval false The producer is still active, if @ref bq_consumer_get
 *               returned NULL, it's because there is currently no
 *               data available.
 *
 * @note This function does not lock @ref BufferQueue_Producer::lock,
 *       instead it uses atomic operations to get the @ref
 *       BufferQueue_Producer::stopped value.
 */
gboolean bq_consumer_stopped(BufferQueue_Consumer *consumer) {
    return !!g_atomic_int_get(&consumer->producer->stopped);
}

/**@}*/
