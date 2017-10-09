/*
 * queue.h
 *
 *  Created on: Oct 09, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef QUEUE_H_
#define QUEUE_H_

#define queue__i(q__, i__) ((q__)->s_ + i__) % (q__)->sz

typedef struct queue_s queue_t;
typedef void (*queue_destroy_cb)(void * data);

#include <stdio.h>

queue_t * queue_new(size_t sz);
void queue_destroy(queue_t * queue, queue_destroy_cb cb);
static inline void * queue_get(queue_t * queue, size_t i);
static inline void * queue_pop(queue_t * queue);
static inline void * queue_shift(queue_t * queue);
queue_t * queue_copy(queue_t * queue);
queue_t * queue_push(queue_t * queue, void * data);
queue_t * queue_unshift(queue_t * queue, void * data);
queue_t * queue_extend(queue_t * queue, void * data[], size_t n);
queue_t * queue_shrink(queue_t * queue);
/* unsafe macro for queue_push(); might overwrite data if not enough space */
#define QUEUE_push(q__, d__) \
    (q__)->data_[queue__i(q__, (q__)->n++)] = d__
/* unsafe macro for queue_unshift(); might overwrite data if not enough space */
#define QUEUE_unshift(q__, d__) \
        (q__)->data_[((q__)->s_ = (((q__)->s_ ? \
                (q__)->s_ : (q__)->sz) - (!!++(q__)->n)))] = d__;

struct queue_s
{
    size_t n;
    size_t sz;
    size_t s_;
    void * data_[];
};

static inline void * queue_get(queue_t * queue, size_t i)
{
    return queue->data_[queue__i(queue, i)];
}

static inline void * queue_pop(queue_t * queue)
{
    return (queue->n) ? queue->data_[queue__i(queue, --queue->n)] : NULL;
}

static inline void * queue_shift(queue_t * queue)
{
    return (queue->n && queue->n--) ?
            queue->data_[(((queue->s_ = queue__i(queue, 1))) ?
                    queue->s_: queue->sz ) - 1] : NULL;
}

#endif /* QUEUE_H_ */
