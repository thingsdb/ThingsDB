/*
 * queue.h
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct queue_s  queue_t;
typedef void (*queue_destroy_cb)(void * data);

#include <stdio.h>


queue_t * queue_create(size_t sz);
void queue_destroy(queue_t * queue, queue_destroy_cb cb);
static inline void * queue_get(queue_t * queue, size_t i);
static inline void * queue_pop(queue_t * queue);
static inline void * queue_shift(queue_t * queue);
queue_t * queue_copy(queue_t * queue);
queue_t * queue_push(queue_t * queue, void * data);
queue_t * queue_unshift(queue_t * queue, void * data);
queue_t * queue_shrink(queue_t * queue);
queue_t * queue_extend(queue_t * queue, void * data[], size_t n);
#define queue_idx(q__, i__) ((q__)->s + i__) % (q__)->sz

struct queue_s
{
    size_t s;
    size_t n;
    size_t sz;
    void * data_[];
};

static inline void * queue_get(queue_t * queue, size_t i)
{
    return queue->data_[queue_idx(queue, i)];
}

static inline void * queue_pop(queue_t * queue)
{
    return (queue->n) ? queue->data_[queue_idx(queue, --queue->n)] : NULL;
}

static inline void * queue_shift(queue_t * queue)
{
    return (queue->n) ?
            (((queue->s = queue_idx(queue, 1))) ?
                    queue->sz : queue->s) - 1 : NULL;
}

#endif /* QUEUE_H_ */
