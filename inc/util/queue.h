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
static inline size_t queue_space(const queue_t * queue);
static inline void * queue_get(const queue_t * queue, size_t i);
static inline void * queue_first(const queue_t * queue);
static inline void * queue_last(const queue_t * queue);
static inline void * queue_pop(queue_t * queue);
static inline void * queue_shift(queue_t * queue);
static inline void queue_clear(queue_t * queue);
queue_t * queue_dup(const queue_t * queue);
void queue_copy(const queue_t * queue, void * dest[]);
void * queue_remove(queue_t * queue, size_t idx);
void * queue_remval(queue_t * queue, void * data);
void * queue_replace(queue_t * queue, size_t idx, void * data);
int queue_reserve(queue_t ** qaddr, size_t n);
int queue_push(queue_t ** qaddr, void * data);
int queue_unshift(queue_t ** qaddr, void * data);
int queue_insert(queue_t ** qaddr, size_t idx, void * data);
int queue_extend(queue_t ** qaddr, void * data[], size_t n);
int queue_shrink(queue_t ** qaddr);
/* unsafe macro for queue_push();
 * might overwrite data if not enough space and requires at least size 1 */
#define QUEUE_push(q__, d__) \
    (q__)->data_[queue__i(q__, (q__)->n++)] = d__
/* unsafe macro for queue_unshift();
 * might overwrite data if not enough space and requires at least size 1 */
#define QUEUE_unshift(q__, d__) \
        (q__)->data_[((q__)->s_ = (((q__)->s_ ? \
                (q__)->s_ : (q__)->sz) - (!!++(q__)->n)))] = d__;

/* use queue_each in a for loop to go through all values.
 * do not change the queue while iterating, with the exception of one
 * queue_shift per iteration which is allowed. */
#define queue_each(q__, dt__, var__)\
        dt__ * var__, \
        ** b__ = (dt__ **) (q__)->data_, \
        ** v__ = b__ + (q__)->s_, \
        ** e__ = b__ + (q__)->sz, \
        ** n__ = v__ + (q__)->n; \
        (v__ < e__ && v__ < n__ && (var__ = *v__) ) || \
        (       e__ < n__ && \
                (v__ = b__) && \
                (n__ = b__ + (n__ - e__)) && \
                (var__ = *v__) \
        ); \
        v__++

struct queue_s
{
    size_t n;
    size_t sz;
    size_t s_;
    void * data_[];
};

static inline size_t queue_space(const queue_t * queue)
{
    return queue->sz - queue->n;
}

static inline void * queue_get(const queue_t * queue, size_t i)
{
    return queue->data_[queue__i(queue, i)];
}

static inline void * queue_first(const queue_t * queue)
{
    return (queue->n) ? queue->data_[queue__i(queue, 0)] : NULL;
}

static inline void * queue_last(const queue_t * queue)
{
    return (queue->n) ? queue->data_[queue__i(queue, queue->n - 1)] : NULL;
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

static inline void queue_clear(queue_t * queue)
{
    queue->n = queue->s_ = 0;
}

#endif /* QUEUE_H_ */
