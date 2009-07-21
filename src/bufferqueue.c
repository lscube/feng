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
#include <stdio.h>

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
     * @brief Next serial to use for the added elements
     *
     * This is the next value for @ref BufferQueue_Element::serial; it
     * starts from zero and it's reset to zero each time the queue is
     * reset; each element added to the queue gets this value before
     * getting incremented; it is used by @ref bq_consumer_unseen().
     */
    gulong next_serial;

    /**
     * @brief Serial number for the queue
     *
     * This is the serial number of the queue inside the producer,
     * starts from zero and is increased each time the queue is reset.
     */
    gulong queue_serial;

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
     * @brief Serial number of the queue
     *
     * This value is the “serial number” of the producer's queue,
     * which increases each time the producer generates a new queue.
     *
     * This is taken, at each move, from @ref
     * BufferQueue_Producer::queue_serial, and is used by the
     * consumer's function to ensure no old data is used.
     */
    gulong queue_serial;

    /**
     * @brief Pointer to the current element in the list
     *
     * This pointer is used to access the "next to serve" buffer; and
     * is originally set to the head of @ref BufferQueue_Producer
     * queue.
     */
    GList *current_element_pointer;

    /**
     * @brief Last element serial returned.
     *
     * @brief This is updated at _get and makes sure that we can
     * distinguish between the situations of "all seen" and "none
     * seen".
     */
    gulong last_element_serial;
};

static inline BufferQueue_Element *GLIST_TO_BQELEM(GList *pointer)
{
    return (BufferQueue_Element*)pointer->data;
}

/**
 * @brief Ensures the validity of the current element pointer for the consume
 *
 * @param consumer The consumer to verify the pointer of
 *
 * Call this function whenever current_element_pointer is going to be
 * used; if any condition happened that would make it unreliable, set
 * it to NULL.
 */
static void bq_consumer_confirm_pointer(BufferQueue_Consumer *consumer)
{
    BufferQueue_Producer *producer = consumer->producer;
    if ( consumer->current_element_pointer == NULL )
        return;

    if ( producer->queue_serial != consumer->queue_serial ) {
        fprintf(stderr, "[%s] C:%p pointer %p reset PQS:%lu < CQS:%lu\n",
                __PRETTY_FUNCTION__,
                consumer,
                consumer->current_element_pointer,
                producer->queue_serial,
                consumer->queue_serial);
        consumer->current_element_pointer = NULL;
        return;
    }

    if ( producer->queue->head &&
         consumer->last_element_serial < GLIST_TO_BQELEM(producer->queue->head)->serial ) {
        fprintf(stderr, "[%s] C:%p pointer %p reset LES:%lu < PQHS:%lu\n",
                __PRETTY_FUNCTION__,
                consumer,
                consumer->current_element_pointer,
                consumer->last_element_serial,
                GLIST_TO_BQELEM(producer->queue->head)->serial);
        consumer->current_element_pointer = NULL;
        return;
    }
}

static inline BufferQueue_Element *BQ_OBJECT(BufferQueue_Consumer *consumer)
{
    bq_consumer_confirm_pointer(consumer);

    if ( consumer->current_element_pointer == NULL )
        return NULL;

    return (BufferQueue_Element *)consumer->current_element_pointer->data;
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

    fprintf(stderr, "[%s] Free object %p payload %p %lu\n",
            __PRETTY_FUNCTION__,
            element,
            element->payload,
            element->seen);
    free_function(element->payload);
    g_slice_free(BufferQueue_Element, element);
}

/**
 * @brief Resets a producer's queue (unlocked version)
 *
 * @param producer Producer to reset the queue of
 *
 * @internal This function does not lock the producer and should only
 *           be used by @ref bq_producer_new !
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * This function will change the currently-used queue for the
 * producer, so that a discontinuity will allow the consumers not to
 * worry about getting old buffers.
 */
static void bq_producer_reset_queue_internal(BufferQueue_Producer *producer) {
    fprintf(stderr, "[%s] Producer %p queue %p queue_serial %lu\n",
            __PRETTY_FUNCTION__,
            producer,
            producer->queue,
            producer->queue_serial);

    if ( producer->queue ) {
        g_queue_foreach(producer->queue,
                        bq_element_free_internal,
                        producer->free_function);
        g_queue_clear(producer->queue);
        g_queue_free(producer->queue);
    }

    producer->queue = g_queue_new();
    producer->queue_serial++;
    producer->next_serial = 1;
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
    fprintf(stderr, "[%s] Producer %p\n",
            __PRETTY_FUNCTION__,
            producer);

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    g_assert(!producer->stopped);

    bq_producer_reset_queue_internal(producer);

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);
}

/**
 * @brief Create a new producer for the bufferqueue
 *
 * @param free_function Function to call when removing buffers from
 *                      the queue.
 *
 * @return A new BufferQueue_Producer object that needs to be freed
 *         with @ref bq_producer_unref.
 */
BufferQueue_Producer *bq_producer_new(GDestroyNotify free_function)
{

    BufferQueue_Producer *ret = NULL;

        ret = g_slice_new0(BufferQueue_Producer);

        ret->lock = g_mutex_new();
        ret->free_function = free_function;
        ret->last_consumer = g_cond_new();

        bq_producer_reset_queue_internal(ret);

    return ret;
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
    fprintf(stderr, "[%s] Producer %p\n",
            __PRETTY_FUNCTION__,
            producer);

    g_assert_cmpuint(producer->consumers, ==, 0);

    g_cond_free(producer->last_consumer);

    if ( producer->queue ) {
        /* Destroy elements and the queue */
        g_queue_foreach(producer->queue,
                        bq_element_free_internal,
                        producer->free_function);
        g_queue_free(producer->queue);
    }

    g_mutex_unlock(producer->lock);
    g_mutex_free(producer->lock);

    g_slice_free(BufferQueue_Producer, producer);
}

/**
 * @brief Frees a producer
 *
 * @param producer Producer to be freed
 *
 */
void bq_producer_unref(BufferQueue_Producer *producer) {
    fprintf(stderr, "[%s] Producer %p\n",
            __PRETTY_FUNCTION__,
            producer);

    /* Compatibility with free(3) */
    if ( producer == NULL )
        return;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    g_assert(g_atomic_int_get(&producer->stopped) == 0);

    g_atomic_int_set(&producer->stopped, 1);

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

    g_assert(payload != NULL && payload != GINT_TO_POINTER(-1));

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    /* Make sure the producer is not stopped */
    g_assert(g_atomic_int_get(&producer->stopped) == 0);

    elem = g_slice_new(BufferQueue_Element);
    elem->payload = payload;
    elem->seen = 0;
    elem->serial = producer->next_serial++;

    fprintf(stderr, "[%s] Payload %p element %p\n", __PRETTY_FUNCTION__, payload, elem);

    g_assert_cmpint(producer->next_serial, !=, 0);

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

    fprintf(stderr, "[%s] Producer %p\n",
            __PRETTY_FUNCTION__,
            producer);

    if ( g_atomic_int_get(&producer->stopped) == 1 )
        return NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    ret = g_slice_new0(BufferQueue_Consumer);

    /* Make sure we don't overflow the consumers count; while this
     * case is most likely just hypothetical, it doesn't hurt to be
     * safe.
     */
    g_assert_cmpuint(producer->consumers, <, G_MAXULONG);

    producer->consumers++;

    ret->producer = producer;

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return ret;
}

/**
 * @brief Destroy the head of the producer queue
 *
 * @param producer Producer to destroy the head queue of
 * @param element element to destroy; this is a safety check
 */
static void bq_producer_destroy_head(BufferQueue_Producer *producer, GList *pointer) {
    BufferQueue_Element *elem = pointer->data;

    /* We can only remove the head of the queue, if we're doing
     * anything else, something is funky.
     */
    g_assert(pointer == producer->queue->head);

    /* Remove the element from the queue */
    g_queue_pop_head(producer->queue);

    /* If we reached the end of the queue, consider like it was a new
     * one */
    if ( g_queue_get_length(producer->queue) == 0 )
        producer->queue_serial++;

    bq_element_free_internal(elem, producer->free_function);
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
    BufferQueue_Element *elem;
    GList *pointer;

    bq_consumer_confirm_pointer(consumer);

    elem = BQ_OBJECT(consumer);
    pointer = consumer->current_element_pointer;

    fprintf(stderr, "[%s] C:%p PQS:%lu CQS:%lu pointer %p object %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            producer->queue_serial,
            consumer->queue_serial,
            pointer, elem);

    /* If we had no element selected, just get out of here */
    if ( elem == NULL )
        return;

    /* If we're the last one to see the element, we need to take care
     * of removing and freeing it. */
    fprintf(stderr, "[%s] C:%p pointer %p object %p payload %p seen %lu consumers %lu PQH:%p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->current_element_pointer,
            elem,
            elem->payload,
            elem->seen+1,
            producer->consumers,
            producer->queue->head);

    /* After unref we have to assume that we cannot access the pointer
     * _at all_, no matter if we actually removed it or not, otherwise
     * it wouldn't be an unref, would it? */
    consumer->current_element_pointer = NULL;

    if ( ++elem->seen < producer->consumers )
        return;

    fprintf(stderr, "[%s] C:%p object %p pointer %p next %p prev %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            pointer->data,
            pointer,
            pointer->next,
            pointer->prev);

    bq_producer_destroy_head(producer, pointer);
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

    fprintf(stderr, "[%s] C:%p LES:%lu PQHS:%lu PQH:%p pointer %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->last_element_serial,
            producer->queue->head ? GLIST_TO_BQELEM(producer->queue->head)->serial : 0,
            producer->queue->head,
            consumer->current_element_pointer);

    bq_consumer_confirm_pointer(consumer);

    if (consumer->current_element_pointer) {
        fprintf(stderr, "[%s] C:%p pointer %p object %p next %p prev %p\n",
                __PRETTY_FUNCTION__,
                consumer,
                consumer->current_element_pointer,
                consumer->current_element_pointer->data,
                consumer->current_element_pointer->next,
                consumer->current_element_pointer->prev);
        GList *next = consumer->current_element_pointer->next;

        /* If there is any element at all saved, we take care of marking
         * it as seen. We don't have to check if it's non-NULL since the
         * function takes care of that. We _have_ to do this after we
         * found the new "next" pointer.
         */
        bq_consumer_elem_unref(consumer);

        consumer->current_element_pointer = next;
    } else if ( consumer->queue_serial != producer->queue_serial ) {
        consumer->current_element_pointer = producer->queue->head;
    } else {
        GList *elem = producer->queue->head;
        fprintf(stderr, "[%s] C:%p PQH:%p\n",
                __PRETTY_FUNCTION__,
                consumer,
                producer->queue->head);

        while ( elem != NULL &&
                GLIST_TO_BQELEM(elem)->serial <= consumer->last_element_serial )
            elem = elem->next;

        consumer->current_element_pointer = elem;
    }

    /* Now we have a new "next" element and we can set it properly */
    consumer->queue_serial = producer->queue_serial;

    return ( consumer->current_element_pointer != NULL );
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

    fprintf(stderr, "[%s] C:%p pointer %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->current_element_pointer);

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
    } else {
        fprintf(stderr, "[%s] C:%p LES:%lu CQS:%lu PQS:%lu PQH:%p PQHS:%lu\n",
                __PRETTY_FUNCTION__,
                consumer,
                consumer->last_element_serial,
                consumer->queue_serial,
                producer->queue_serial,
                producer->queue->head,
                producer->queue->head ? GLIST_TO_BQELEM(producer->queue->head)->serial: 0);
        if ( consumer->last_element_serial != 0 &&
                consumer->queue_serial == producer->queue_serial &&
                producer->queue->head != NULL &&
                GLIST_TO_BQELEM(producer->queue->head)->serial <= consumer->last_element_serial ) {
        GList *it = producer->queue->head;
        BufferQueue_Element *elem;

        g_assert_cmpuint(GLIST_TO_BQELEM(producer->queue->tail)->serial, >=, consumer->last_element_serial);

        fprintf(stderr, "[%s] C:%p decrementing elements till serial %lu\n",
                __PRETTY_FUNCTION__,
                consumer,
                consumer->last_element_serial);

        /* If we have a NULL current pointer, we're at the end of the
         * queue waiting for something new (are we sure about it? :|)
         * so we'll decrease the whole queue! */
        while ( it != NULL &&
                (elem = GLIST_TO_BQELEM(it))->serial <= consumer->last_element_serial ) {
            fprintf(stderr, "[%s] C:%p decrementing counter for pointer %p object %p serial %lu seen %lu/%lu\n",
                    __PRETTY_FUNCTION__,
                    consumer,
                    it,
                    elem,
                    elem->serial,
                    elem->seen,
                    producer->consumers);

            /* we've got to have seen it! */
            g_assert_cmpuint(elem->seen, >, 0);

            elem->seen--;

            g_assert_cmpuint(elem->seen, <=, producer->consumers);

            bq_producer_destroy_head(producer, it);

            it = it->next;
        }
    }}

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    g_slice_free(BufferQueue_Consumer, consumer);
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
    gulong unseen;

    if (!producer || g_atomic_int_get(&producer->stopped) )
        return 0;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    if ( consumer->queue_serial != producer->queue_serial ) {
        unseen = producer->next_serial;
    } else {
        unseen = producer->next_serial;
        bq_consumer_confirm_pointer(consumer);

        if ( consumer->current_element_pointer != NULL ) {
            g_assert_cmpint(unseen, >, BQ_OBJECT(consumer)->serial);
            unseen -= (BQ_OBJECT(consumer)->serial + 1);
        } else if (!producer->queue->head){
            unseen = 0;
        }
    }

    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);

    return unseen;
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

    fprintf(stderr, "[%s](before) C:%p pointer %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->current_element_pointer);

    if ( g_atomic_int_get(&producer->stopped) )
        return false;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    ret = bq_consumer_move_internal(consumer);

    fprintf(stderr, "[%s](after) C:%p pointer %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->current_element_pointer);

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
    BufferQueue_Element *element = NULL;

    if ( g_atomic_int_get(&producer->stopped) )
        return NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);
    fprintf(stderr, "[%s] C:%p CQS:%lu PQS:%lu PQH: %p pointer %p\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->queue_serial,
            producer->queue_serial,
            producer->queue->head,
            consumer->current_element_pointer);

    bq_consumer_confirm_pointer(consumer);

    /* If we don't have a queue yet, like for the first read, “move
     * next” (or rather first).
     */
    if ( consumer->current_element_pointer == NULL &&
         !bq_consumer_move_internal(consumer) )
            goto end;

    element = (BufferQueue_Element*)(consumer->current_element_pointer->data);
    consumer->last_element_serial = element->serial;

    fprintf(stderr, "[%s] C:%p pointer %p object %p payload %p seen %lu/%lu\n",
            __PRETTY_FUNCTION__,
            consumer,
            consumer->current_element_pointer,
            element,
            element->payload,
            element->seen,
            producer->consumers);
 end:
    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);
    return element ? element->payload : NULL;
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
