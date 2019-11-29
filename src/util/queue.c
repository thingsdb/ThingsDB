/*
 * util/queue.c
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
    queue_t * queue = malloc(sizeof(queue_t) + sz * sizeof(void*));
    if (!queue)
        return NULL;
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

/*
 * Returns a copy of queue with an exact fit so the new queue->sz and queue->n
 * will be equal. In case of an allocation error the return value is NULL.
 */
queue_t * queue_dup(const queue_t * queue)
{
    queue_t * q = malloc(sizeof(queue_t) + queue->n * sizeof(void*));
    if (!q)
        return NULL;

    queue_copy(queue, q->data_);

    q->sz = q->n = queue->n;
    q->s_ = 0;

    return q;
}

void queue_copy(const queue_t * queue, void * dest[])
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

void * queue_remove(queue_t * queue, size_t idx)
{
    if (idx >= queue->n)
        return NULL;

    size_t i = queue__i(queue, idx);
    void * data = queue->data_[i];

    if (i >= queue->s_)
    {
        /* walk reverse */
        for (;i > queue->s_; i--)
        {
            queue->data_[i] = queue->data_[i - 1];
        }
        queue->s_++;
    }
    else
    {
        /* walk forward */
        size_t e = queue__i(queue, queue->n);
        for (i++; i < e; i++)
        {
            queue->data_[i - 1] = queue->data_[i];
        }
    }
    queue->n--;
    return data;
}

/*
 * Removes and returns the first value in the queue which is equal to data.
 * If the value is not found, than NULL is returned.
 */
void * queue_rmval(queue_t * queue, void * data)
{
    size_t i = 0;
    for (queue_each(queue, void, d), i++)
        if (d == data)
            return queue_remove(queue, i);
    return NULL;
}

void * queue_replace(queue_t * queue, size_t idx, void * data)
{
    size_t i = queue__i(queue, idx);
    void * ret = queue->data_[i];
    queue->data_[i] = data;
    return ret;
}

/*
 * This function may resize the queue so it has space for at least n new
 * objects.
 */
int queue_reserve(queue_t ** qaddr, size_t n)
{
    queue_t * queue = *qaddr;
    size_t nn = queue->n + n;
    if (nn > queue->sz)
    {
        queue_t * q;
        size_t sz = queue->sz;

        /*
         * It is important to resize with at least x2, otherwise the memcpy
         * later might fail.
         */
        queue->sz <<= 1;

        if (nn > queue->sz)
            queue->sz = nn;

        q = realloc(queue, sizeof(queue_t) + queue->sz * sizeof(void*));

        if (!q)
        {
            /* restore original size */
            queue->sz = sz;
            return -1;
        }

        /* Until q->s_ is always initialized but we can more than required */
        memcpy(q->data_ + sz, q->data_, q->s_ * sizeof(void*));
        *qaddr = q;
    }
    return 0;
}

int queue_push(queue_t ** qaddr, void * data)
{
    queue_t * queue = *qaddr;
    if (queue->n == queue->sz)
    {
        queue_t * q = queue__grow(queue);
        if (!q)
            return -1;
        *qaddr = queue = q;
    }
    QUEUE_push(queue, data);
    return 0;
}

int queue_unshift(queue_t ** qaddr, void * data)
{
    queue_t * queue = *qaddr;
    if (queue->n == queue->sz)
    {
        queue_t * tmp = queue__grow(queue);
        if (!tmp)
            return -1;
        *qaddr = queue = tmp;
    }
    QUEUE_unshift(queue, data);
    return 0;
}

int queue_insert(queue_t ** qaddr, size_t idx, void * data)
{
    queue_t * queue = *qaddr;

    if (!idx)
        return queue_unshift(qaddr, data);
    if (idx >= queue->n)
        return queue_push(qaddr, data);

    if (queue->n == queue->sz)
    {
        queue_t * q = queue__grow(queue);
        if (!q)
            return -1;
        queue = *qaddr = q;
    }

    size_t i = queue__i(queue, idx);
    size_t e = queue__i(queue, queue->n);
    size_t x;

    if (i < e)
    {
        /* walk reverse */
        for (x = e; x != i; x--)
        {
            queue->data_[x] = queue->data_[x - 1];
        }
    }
    else
    {
        /* walk forward */
        --i;
        for (x = queue->s_;; x++)
        {
            queue->data_[x - 1] = queue->data_[x];
            if (x == i)
                break;
        }
        queue->s_ -= 1;
    }
    queue->n++;
    queue->data_[x] = data;
    return 0;
}

int queue_extend(queue_t ** qaddr, void * data[], size_t n)
{
    queue_t * q;
    size_t m, next;
    if (queue_reserve(qaddr, n))
        return -1;

    q = *qaddr;

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

    return 0;
}

/*
 * Shrinks a queue to an exact fit.
 */
int queue_shrink(queue_t ** qaddr)
{
    queue_t * queue = *qaddr;
    size_t n;
    queue_t * q;
    if (queue->n == queue->sz)
        return 0;

    n = queue->sz - queue->s_;
    n = (n <= queue->n) ? n : queue->n;

    memmove(queue->data_ + queue->n - n,
            queue->data_ + queue->s_,
            n * sizeof(void*));

    q = realloc(queue, sizeof(queue_t) + queue->n * sizeof(void*));
    if (!q)
        return -1;

    q->sz = q->n;
    q->s_ = q->n - n;

    *qaddr = q;
    return 0;
}

static queue_t * queue__grow(queue_t * queue)
{
    queue_t * q;
    size_t sz = queue->sz;

    if (!sz)
        queue->sz = 1;

    queue->sz <<= 1;

    q = realloc(queue, sizeof(queue_t) + queue->sz * sizeof(void*));
    if (!q)
    {
        /* restore original size */
        queue->sz = sz;
        return NULL;
    }

    memcpy(q->data_ + sz, q->data_, q->s_ * sizeof(void*));
    return q;
}

