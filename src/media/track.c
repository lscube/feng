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

#include "media/media.h"
#include "network/rtp.h"

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
static void bq_consumer_confirm_pointer(RTP_session *consumer)
{
    Track *producer = consumer->track;
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

static inline struct MParserBuffer *BQ_OBJECT(RTP_session *consumer)
{
    bq_consumer_confirm_pointer(consumer);

    if ( consumer->current_element_pointer == NULL )
        return NULL;

    return (struct MParserBuffer*)consumer->current_element_pointer->data;
}


static void mparser_buffer_free(struct MParserBuffer *buffer)
{
    bq_debug("Free object %p %lu",
             buffer,
             buffer->seen);

    g_free(buffer->data);
    g_slice_free(struct MParserBuffer, buffer);
}

/**
 * @brief Destroy one by one the elements in
 *        Track::queue.
 *
 * @param elem_generic Element to destroy
 * @param free_func_generic Function to use for destroying the
 *                          elements' payload.
 */
static void bq_element_free_internal(gpointer elem_generic,
                                     ATTR_UNUSED gpointer unused) {
    mparser_buffer_free((struct MParserBuffer*)elem_generic);
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
static void bq_producer_reset_queue_internal(Track *producer) {
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
void track_reset_queue(Track *producer) {
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

    mparser_buffer_free(elem);
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
static void bq_consumer_elem_unref(RTP_session *consumer) {
    Track *producer = consumer->track;
    struct MParserBuffer *elem;
    GList *pointer;

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
    bq_debug("C:%p pointer %p object %p seen %lu consumers %u PQH:%p",
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
static gboolean bq_consumer_move_internal(RTP_session *consumer) {
    Track *producer = consumer->track;

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
void bq_consumer_free(RTP_session *consumer) {
    Track *producer;

    /* Compatibility with free(3) */
    if ( consumer == NULL )
        return;

    producer = consumer->track;

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
gulong bq_consumer_unseen(RTP_session *consumer) {
    Track *producer = consumer->track;
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
gboolean bq_consumer_move(RTP_session *consumer) {
    Track *producer = consumer->track;
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
struct MParserBuffer *bq_consumer_get(RTP_session *consumer) {
    Track *producer = consumer->track;
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

    bq_debug("C:%p pointer %p object %p seen %lu/%d",
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
gboolean bq_consumer_stopped(RTP_session *consumer) {
    return !!g_atomic_int_get(&consumer->track->stopped);
}

/**@}*/

/**
 * @brief Create a new Track object
 *
 * @param name Name of the track to use (will be g_free'd); make sure
 *             to use @ref feng_str_is_unreseved before passing an
 *             user-provided string to this function.
 *
 * @return pointer to newly allocated track struct.
 */
Track *track_new(char *name)
{
    Track *t;

    t = g_slice_new0(Track);

    t->lock            = g_mutex_new();
    t->last_consumer   = g_cond_new();
    t->name            = name;
    t->sdp_description = g_string_new("");

    /* set these by default, sinze 0 might actually be a valid
       value */
    t->payload_type = -1;
    t->clock_rate = -1;
    t->media_type = MP_undef;

    g_string_append_printf(t->sdp_description,
                           "a=control:%s\r\n",
                           name);

    bq_producer_reset_queue_internal(t);

    return t;
}

/**
 * @brief Frees the resources of a Track object
 *
 * @param track Track to free
 */
void track_free(Track *track)
{
    if (!track)
        return;

    g_mutex_free(track->lock);

    g_free(track->name);
    g_free(track->encoding_name);

    g_assert_cmpuint(track->consumers, ==, 0);

    g_cond_free(track->last_consumer);

    if ( track->queue ) {
        /* Destroy elements and the queue */
        g_queue_foreach(track->queue,
                        bq_element_free_internal,
                        NULL);
        g_queue_free(track->queue);
    }

    if ( track->sdp_description )
        g_string_free(track->sdp_description, true);

    if ( track->uninit )
        track->uninit(track);

    g_slice_free(Track, track);
}

/**
 * @brief Queue a new RTP buffer into the track's queue
 *
 * @param tr The track to queue the buffer onto
 * @param buffer The RTP buffer to queue
 */
void track_write(Track *tr, struct MParserBuffer *buffer)
{
    /* Make sure the producer is not stopped */
    g_assert(g_atomic_int_get(&tr->stopped) == 0);

    /* Ensure we have the exclusive access */
    g_mutex_lock(tr->lock);

    /* do this inside the lock so that next_serial does not change */
    if ( ! buffer->seq_no )
        buffer->seq_no = tr->next_serial;

    tr->next_serial = buffer->seq_no + 1;

    g_queue_push_tail(tr->queue, buffer);

    /* Leave the exclusive access */
    g_mutex_unlock(tr->lock);
}
