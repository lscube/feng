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

#define G_LOG_DOMAIN "bufferqueue"

#include <config.h>

#include "bufferqueue.h"
#include "media/mediaparser.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* We don't enable this with !NDEBUG because it's _massive_! */
#ifdef FENG_BQ_DEBUG
# define bq_debug(fmt, ...) g_debug("[%s] " fmt, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
# define bq_debug(...)
#endif

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
    Track *producer;


    /**
     * @brief Serial number of the queue
     *
     * This value is the “serial number” of the producer's queue,
     * which increases each time the producer generates a new queue.
     *
     * This is taken, at each move, from @ref
     * Track::queue_serial, and is used by the
     * consumer's function to ensure no old data is used.
     */
    gulong queue_serial;

    /**
     * @brief Pointer to the current element in the list
     *
     * This pointer is used to access the "next to serve" buffer; and
     * is originally set to the head of @ref Track
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
    uint16_t last_element_serial;
};

static inline struct MParserBuffer *GLIST_TO_BQELEM(GList *pointer)
{
    return (struct MParserBuffer*)pointer->data;
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
    Track *producer = consumer->producer;
    if ( consumer->current_element_pointer == NULL )
        return;

    if ( producer->queue_serial != consumer->queue_serial ) {
        bq_debug("C:%p pointer %p reset PQS:%lu < CQS:%lu",
                consumer,
                consumer->current_element_pointer,
                producer->queue_serial,
                consumer->queue_serial);
        consumer->current_element_pointer = NULL;
        return;
    }

    if ( producer->queue->head &&
         consumer->last_element_serial < GLIST_TO_BQELEM(producer->queue->head)->seq_no ) {
        bq_debug("C:%p pointer %p reset LES:%lu:%u < PQHS:%lu:%u",
                consumer,
                consumer->current_element_pointer,
                consumer->queue_serial, consumer->last_element_serial,
                producer->queue_serial,
                GLIST_TO_BQELEM(producer->queue->head)->seq_no);
        consumer->current_element_pointer = NULL;
        return;
    }
}

static inline struct MParserBuffer *BQ_OBJECT(BufferQueue_Consumer *consumer)
{
    bq_consumer_confirm_pointer(consumer);

    if ( consumer->current_element_pointer == NULL )
        return NULL;

    return (struct MParserBuffer*)consumer->current_element_pointer->data;
}

/**
 * @brief Destroy one by one the elements in
 *        Track::queue.
 *
 * @param elem_generic Element to destroy
 * @param free_func_generic Function to use for destroying the
 *                          elements' payload.
 */
void bq_element_free_internal(gpointer elem_generic,
                              ATTR_UNUSED gpointer unused) {
    struct MParserBuffer *const element = (struct MParserBuffer*)elem_generic;

    bq_debug("Free object %p %lu",
            element,
            element->seen);
    g_free(element);
}

/**
 * @brief Resets a producer's queue (unlocked version)
 *
 * @param producer Producer to reset the queue of
 *
 * @internal This function does not lock the producer and should only
 *           be used by @ref add_track !
 * @note This function will require exclusive access to the producer,
 *       and will thus lock its mutex.
 *
 * This function will change the currently-used queue for the
 * producer, so that a discontinuity will allow the consumers not to
 * worry about getting old buffers.
 */
void bq_producer_reset_queue_internal(Track *producer) {
    bq_debug("Producer %p queue %p queue_serial %lu",
            producer,
            producer->queue,
            producer->queue_serial);

    if ( producer->queue ) {
        g_queue_foreach(producer->queue,
                        bq_element_free_internal,
                        NULL);
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
void bq_producer_reset_queue(Track *producer) {
    bq_debug("Producer %p",
            producer);

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    g_assert(!producer->stopped);

    bq_producer_reset_queue_internal(producer);

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
BufferQueue_Consumer *bq_consumer_new(Track *producer) {
    BufferQueue_Consumer *ret;

    bq_debug("Producer %p",
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
static void bq_producer_destroy_head(Track *producer, GList *pointer) {
    struct MParserBuffer *elem = pointer->data;

    bq_debug("P:%p PQH:%p pointer %p",
             producer,
             producer->queue->head,
             pointer);

    /* We can only remove the head of the queue, if we're doing
     * anything else, something is funky. Ibid if it hasn't been seen
     * yet.
     */
    g_assert(pointer == producer->queue->head);
    g_assert(elem->seen == producer->consumers);

    /* Remove the element from the queue */
    g_queue_pop_head(producer->queue);

    /* If we reached the end of the queue, consider like it was a new
     * one */
    if ( g_queue_get_length(producer->queue) == 0 ) {
        producer->queue_serial++;
        producer->next_serial = 1;
    }

    bq_element_free_internal(elem, NULL);
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
    Track *producer = consumer->producer;
    struct MParserBuffer *elem;
    GList *pointer;

    bq_consumer_confirm_pointer(consumer);

    elem = BQ_OBJECT(consumer);
    pointer = consumer->current_element_pointer;

    bq_debug("C:%p PQS:%lu CQS:%lu pointer %p object %p",
            consumer,
            producer->queue_serial,
            consumer->queue_serial,
            pointer, elem);

    /* If we had no element selected, just get out of here */
    if ( elem == NULL )
        return;

    g_assert_cmpint(elem->seen, !=, producer->consumers);
    /* If we're the last one to see the element, we need to take care
     * of removing and freeing it. */
    bq_debug("C:%p pointer %p object %p seen %lu consumers %lu PQH:%p",
            consumer,
            consumer->current_element_pointer,
            elem,
            elem->seen+1,
            producer->consumers,
            producer->queue->head);

    /* After unref we have to assume that we cannot access the pointer
     * _at all_, no matter if we actually removed it or not, otherwise
     * it wouldn't be an unref, would it? */
    consumer->current_element_pointer = NULL;

    if ( ++elem->seen < producer->consumers )
        return;

    bq_debug("C:%p object %p pointer %p next %p prev %p",
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
    Track *producer = consumer->producer;

    bq_debug("C:%p LES:%lu:%u PQHS:%lu:%u PQH:%p pointer %p",
            consumer,
            consumer->queue_serial, consumer->last_element_serial,
            producer->queue_serial,
            producer->queue->head ? GLIST_TO_BQELEM(producer->queue->head)->seq_no : 0,
            producer->queue->head,
            consumer->current_element_pointer);

    bq_consumer_confirm_pointer(consumer);

    if (consumer->current_element_pointer) {
        GList *next = consumer->current_element_pointer->next;

        bq_debug("C:%p pointer %p object %p next %p prev %p",
                consumer,
                consumer->current_element_pointer,
                consumer->current_element_pointer->data,
                consumer->current_element_pointer->next,
                consumer->current_element_pointer->prev);

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
        bq_debug("C:%p PQH:%p",
                consumer,
                producer->queue->head);

        while ( elem != NULL &&
                GLIST_TO_BQELEM(elem)->seq_no <= consumer->last_element_serial )
            elem = elem->next;

        consumer->current_element_pointer = elem;
    }

    /* Now we have a new "next" element and we can set it properly */
    if ( consumer->current_element_pointer ) {
        consumer->last_element_serial =
            GLIST_TO_BQELEM(consumer->current_element_pointer)->seq_no;
        consumer->queue_serial = producer->queue_serial;
    }

    return ( consumer->current_element_pointer != NULL );
}

static void bq_decrement_seen_on_free(gpointer elem,
                                      ATTR_UNUSED gpointer unused)
{
    struct MParserBuffer *const element = elem;
    element->seen--;
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
    Track *producer;

    /* Compatibility with free(3) */
    if ( consumer == NULL )
        return;

    producer = consumer->producer;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    bq_debug("C:%p pointer %p",
            consumer,
            consumer->current_element_pointer);

    /* We should never come to this point, since we are expected to
     * have symmetry between new and free calls, but just to be on the
     * safe side, make sure this never happens.
     */
    g_assert_cmpuint(producer->consumers, >,  0);

    while (bq_consumer_move_internal(consumer));

    g_queue_foreach(producer->queue,
                    bq_decrement_seen_on_free,
                    NULL);

    --producer->consumers;

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
    Track *producer = consumer->producer;
    gulong unseen;

    if (!producer || g_atomic_int_get(&producer->stopped) )
        return 0;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);

    if ( consumer->queue_serial != producer->queue_serial ) {
        unseen = producer->next_serial;
    } else {
        if (!producer->queue->head) {
            unseen = 0;
        } else {
            unseen = producer->next_serial - consumer->last_element_serial;
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
    Track *producer = consumer->producer;
    gboolean ret;

    bq_debug("(before) C:%p pointer %p",
            consumer,
            consumer->current_element_pointer);

    if ( g_atomic_int_get(&producer->stopped) )
        return false;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);
    ret = bq_consumer_move_internal(consumer);

    bq_debug("(after) C:%p pointer %p",
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
 * @return A pointer to the newly selected element
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
struct MParserBuffer *bq_consumer_get(BufferQueue_Consumer *consumer) {
    Track *producer = consumer->producer;
    struct MParserBuffer *element = NULL;

    if ( g_atomic_int_get(&producer->stopped) )
        return NULL;

    /* Ensure we have the exclusive access */
    g_mutex_lock(producer->lock);
    bq_debug("C:%p LES:%lu:%u PQHS:%lu:%u PQH:%p pointer %p",
             consumer,
             consumer->queue_serial, consumer->last_element_serial,
             producer->queue_serial,
             producer->queue->head ? GLIST_TO_BQELEM(producer->queue->head)->seq_no : 0,
             producer->queue->head,
             consumer->current_element_pointer);

    bq_consumer_confirm_pointer(consumer);

    /* If we don't have a queue yet, like for the first read, “move
     * next” (or rather first).
     */
    if ( consumer->current_element_pointer == NULL &&
         !bq_consumer_move_internal(consumer) )
            goto end;

    element = (struct MParserBuffer*)(consumer->current_element_pointer->data);

    bq_debug("C:%p pointer %p object %p seen %lu/%lu",
            consumer,
            consumer->current_element_pointer,
            element,
            element->seen,
            producer->consumers);
 end:
    /* Leave the exclusive access */
    g_mutex_unlock(producer->lock);
    return element;
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
 * @note This function does not lock @ref Track::lock,
 *       instead it uses atomic operations to get the @ref
 *       Track::stopped value.
 */
gboolean bq_consumer_stopped(BufferQueue_Consumer *consumer) {
    return !!g_atomic_int_get(&consumer->producer->stopped);
}

/**@}*/

/**
 *  Insert a rtp packet inside the track buffer queue
 *  @param tr track the packetized frames/samples belongs to
 *  @param presentation the actual packet presentation timestamp
 *         in fractional seconds, will be embedded in the rtp packet
 *  @param delivery the actual packet delivery timestamp
 *                  in fractional seconds, will be used to calculate
 *                  sending time
 *  @param duration the actual packet duration, a multiple of the
 *                  frame/samples duration
 *  @param marker tell if we are handling a frame/sample fragment
 *  @param rtp_timestamp Timestamp of the packet in RTP (0 is automatic
 *                       timestamping)
 *  @param seq_no Sequence number of the buffer (0 is automatic sequencing)
 *  @param data actual packet data
 *  @param data_size actual packet data size
 */
void mparser_buffer_write(Track *tr,
                          double presentation,
                          double delivery,
                          double duration,
                          gboolean marker,
                          uint32_t rtp_timestamp,
                          uint16_t seq_no,
                          uint8_t *data, size_t data_size)
{
    struct MParserBuffer *buffer = g_malloc(sizeof(struct MParserBuffer) + data_size);

    /* Make sure the producer is not stopped */
    g_assert(g_atomic_int_get(&tr->stopped) == 0);

    buffer->timestamp = presentation;
    buffer->rtp_timestamp = rtp_timestamp;
    buffer->delivery = delivery;
    buffer->duration = duration;
    buffer->marker = marker;
    buffer->data_size = data_size;
    buffer->seen = 0;

    memcpy(buffer->data, data, data_size);

    /* Ensure we have the exclusive access */
    g_mutex_lock(tr->lock);

    /* do this inside the lock so that next_serial does not change */
    tr->next_serial = (buffer->seq_no = (seq_no ? seq_no : tr->next_serial)) +1;

    g_queue_push_tail(tr->queue, buffer);

    /* Leave the exclusive access */
    g_mutex_unlock(tr->lock);
}
