/*
 * queue.c
 *
 *  Created on: Oct 09, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#include <string.h>
#include <stdlib.h>
#include <util/queue.h>

static queue_t * queue__grow(queue_t * queue);

/*
 * Returns a new queue with size sz.
 */
queue_t * queue_new(size_t sz)
{
    queue_t * queue = (queue_t *) malloc(sizeof(queue_t) + sz * sizeof(void*));
    if (!queue) return NULL;
    queue->sz = sz;
    queue->n = 0;
    queue->s_ = 0;
    return queue;
}

/*
 * Destroy a queue with optional callback. If callback is NULL then it is
 * just as safe to simply call free() instead of this function.
 */
void queue_destroy(queue_t * queue, queue_destroy_cb cb)
{
    if (queue && cb)
    {
        for (queue_each(queue, void, obj), (*cb)(obj));
    }
    free(queue);
}

void queue_copy(queue_t * queue, void * dest[])
{
    size_t m = queue->sz - queue->s_;
    if (m < queue->n)
    {
        memcpy(dest, queue->data_ + queue->s_, m * sizeof(void*));
        memcpy(dest + m, queue->data_, (queue->n - m) * sizeof(void*));
    }
    else
    {
        memcpy(dest, queue->data_ + queue->s_, queue->n * sizeof(void*));
    }
}

/*
 * This function may resize the queue so it has space for at least n new
 * objects.
 */
queue_t * queue_reserve(queue_t * queue, size_t n)
{
    size_t nn = queue->n + n;
    if (nn > queue->sz)
    {
        queue_t * q;
        size_t sz = queue->sz;
        queue->sz = (nn > (queue->sz += queue->s_)) ? nn : queue->sz;

        q = (queue_t *) realloc(
                queue,
                sizeof(queue_t) + queue->sz * sizeof(void*));

        if (!q)
        {
            /* restore original size */
            queue->sz = sz;
            return NULL;
        }

        /* until q->s_ is always initialized but we can more than required */
        memcpy(q->data_ + sz, q->data_, q->s_ * sizeof(void*));
        queue = q;
    }
    return queue;
}

/*
 * Returns a copy of queue with an exact fit so the new queue->sz and queue->n
 * will be equal. In case of an allocation error the return value is NULL.
 */
queue_t * queue_dup(queue_t * queue)
{
    queue_t * q =
            (queue_t *) malloc(sizeof(queue_t) + queue->n * sizeof(void*));
    if (!q) return NULL;

    queue_copy(queue, q->data_);

    q->sz = q->n = queue->n;
    q->s_ = 0;

    return q;
}

/*
 * Push data to the queue and returns an address to the new queue.
 *
 * The returned queue can be equal to the original queue but there is no
 * guarantee. The return value is NULL in case of an allocation error.
 */
queue_t * queue_push(queue_t * queue, void * data)
{
    if (queue->n == queue->sz)
    {
        queue_t * tmp = queue__grow(queue);
        if (!tmp) return NULL;
        queue = tmp;
    }
    QUEUE_push(queue, data);
    return queue;
}

queue_t * queue_unshift(queue_t * queue, void * data)
{
    if (queue->n == queue->sz)
    {
        queue_t * tmp = queue__grow(queue);
        if (!tmp) return NULL;
        queue = tmp;
    }
    QUEUE_unshift(queue, data);

    return queue;
}

queue_t * queue_extend(queue_t * queue, void * data[], size_t n)
{
    size_t m, next;
    queue_t * q = queue_reserve(queue, n);
    if (!q) return NULL;

    next = queue__i(q, q->n);
    if (next < q->s_ || (n <= (m = q->sz - next)))
    {
        memcpy(q->data_ + next, data, n * sizeof(void*));
    }
    else
    {
        memcpy(q->data_ + next, data, m * sizeof(void*));
        memcpy(q->data_, data + m, (n - m) * sizeof(void*));
    }

    q->n += n;
    return q;
}

/*
 * Shrinks a queue to an exact fit.
 *
 * Returns a pointer to the new queue.
 */
queue_t * queue_shrink(queue_t * queue)
{
    size_t n;
    queue_t * q;
    if (queue->n == queue->sz) return queue;

    n = queue->sz - queue->s_;
    n = (n <= queue->n) ? n : queue->n;

    memmove(queue->data_ + queue->n - n,
            queue->data_ + queue->s_,
            n * sizeof(void*));

    q = (queue_t *) realloc(
            queue,
            sizeof(queue_t) + queue->n * sizeof(void*));
    if (!q) return NULL;

    q->sz = q->n;
    q->s_ = q->n - n;

    return q;
}

static queue_t * queue__grow(queue_t * queue)
{
    queue_t * q;
    size_t sz = queue->sz;

    queue->sz *= 2;
    queue->sz += !queue->sz;

    q = (queue_t *) realloc(
            queue,
            sizeof(queue_t) + queue->sz * sizeof(void*));

    if (!q)
    {
        /* restore original size */
        queue->sz = sz;
        return NULL;
    }

    memcpy(q->data_ + sz, q->data_, q->s_ * sizeof(void*));
    return q;
}

