/*
 * queue.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <util/queue.h>

/*
 * Returns a new queue with size sz.
 */
queue_t * queue_create(size_t sz)
{
    queue_t * queue = (queue_t *) malloc(sizeof(queue_t) + sz * sizeof(void*));
    if (!queue) return NULL;
    queue->sz = sz;
    queue->n = 0;
    queue->s = 0;
    return queue;
}

/*
 * Destroy a queue with optional callback. If callback is NULL then it is
 * just as safe to simply call free() instead of this function.
 */
void queue_destroy(queue_t * queue, queue_destroy_cb cb)
{
    if (queue && cb) for (size_t i = 0; i < queue->n; i++)
    {
        (*cb)(queue_get(queue, i));
    }
    free(queue);
}

/*
 * Returns a copy of queue with an exact fit so the new queue->sz and queue->n will
 * be equal. In case of an allocation error the return value is NULL.
 */
queue_t * queue_copy(queue_t * queue)
{
    const size_t qsz = sizeof(queue_t);
    queue_t * queue_ = (queue_t *) malloc(qsz + queue->n * sizeof(void*));
    if (!queue_) return NULL;

    size_t i = queue_idx(queue, queue->n);
    memcpy(queue_ + qsz + queue->n - i, queue + qsz, i);
    if ((i = queue->n - i)) memcpy(queue_ + qsz, queue + qsz + queue->s, i);

    queue_->sz = queue_->n = queue->n;
    queue_->s = 0;
    return queue_;
}

/*
 * Append data to queue and returns queue.
 *
 * Returns a pointer to queue. The returned queue can be equal to the original
 * queue but there is no guarantee. The return value is NULL in case of an
 * allocation error.
 */
queue_t * queue_push(queue_t * queue, void * data)
{
    if (queue->n == queue->sz)
    {
        size_t sz = queue->sz;

        if (sz < 5)
        {
            queue->sz++;
        }
        else if (sz < 64)
        {
            queue->sz *= 2;
        }
        else
        {
            queue->sz += 64;
        }

        queue_t * tmp = (queue_t *) realloc(
                queue,
                sizeof(queue_t) + queue->sz * sizeof(void*));

        if (!tmp)
        {
            /* restore original size */
            queue->sz = sz;
            return NULL;
        }

        queue = tmp;
    }

    return queue;
}

/*
 * Extends a queue with n data elements and returns the new extended queue.
 *
 * In case of an error NULL is returned.
 */
queue_t * queue_extend(queue_t * queue, void * data[], size_t n)
{
    queue->n += n;
    if (queue->n > queue->sz)
    {
        queue_t * tmp = (queue_t *) realloc(
                queue,
                sizeof(queue_t) + queue->n * sizeof(void*));
        if (!tmp)
        {
            /* restore original length */
            queue->n -= n;
            return NULL;
        }

        queue = tmp;
        queue->sz = queue->n;
    }
    memcpy(queue->data_ + (queue->n - n), data, n * sizeof(void*));
    return queue;
}

/*
 * Shrinks a queue to an exact fit.
 *
 * Returns a pointer to the new queue.
 */
queue_t * queue_shrink(queue_t * queue)
{
    if (queue->n == queue->sz) return queue;
    queue_t * tmp = (queue_t *) realloc(
            queue,
            sizeof(queue_t) + queue->n * sizeof(void*));
    if (!tmp) return NULL;
    queue = tmp;
    queue->sz = queue->n;
    return queue;
}


