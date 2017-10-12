/*
 * test_queue.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "test.h"
#include <util/queue.h>
#include <util/vec.h>

const unsigned int num_entries = 8;
char * entries[] = {
    "Zero",
    "First entry",
    "Second entry",
    "Third entry",
    "Fourth entry",
    "Fifth entry",
    "Sixth entry",
    "Seventh entry",
};

static queue_t * push_entries(queue_t * q)
{
    for (size_t i = 0; i < num_entries; i++)
    {
        assert ((q = queue_push(q, entries[i])) != NULL);
    }
    return q;
}

int main()
{
    test_start("queue");

    queue_t * q = queue_new(0);

    /* test initial values */
    {
        assert (q->sz == 0);
        assert (q->n == 0);
        assert (sizeof(*q) == sizeof(queue_t));
    }

    /* test push */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert ((q = queue_push(q, entries[i])) != NULL);
        }
    }

    /* test length */
    {
        assert (q->n == num_entries);
    }

    /* test pop */
    {
        for (size_t i = 0; i < num_entries && ++i;)
        {
            assert (queue_pop(q) == entries[num_entries - i]);
        }

        assert (q->n == 0);
        q = push_entries(q);  /* restore entries */
    }

    /* test shift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_shift(q) == entries[i]);
        }
        assert (q->n == 0);
    }

    /* test force another order in queue */
    {
        q->s_ = 4;
        q = push_entries(q);
        assert (q->sz == num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i) == entries[i]);
        }
    }

    /* test each */
    {
        size_t i = 0;
        char ** entry = entries;
        for (queue_each(q, char, s), i++, entry++)
        {
            assert (s == *entry);
        }
        assert (i == num_entries);
    }

    /* test copy */
    {
        assert (q->n = num_entries);
        vec_t * v = vec_new(q->n);
        queue_copy(q, v->data);
        v->n = q->n;

        for (size_t i = 0; i < v->n; i++)
        {
            assert (vec_get(v, i) == entries[i]);
        }
        free(v);
    }

    /* test adding an extra value */
    {
        const char * extra = "extra";
        assert ((q = queue_push(q, (void *) extra)) != NULL);
        assert (q->sz == num_entries * 2);
        assert (q->n == num_entries + 1);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i) == entries[i]);
        }
        assert (queue_pop(q) == extra);
    }

    /* test dup */
    {
        queue_t * cp;
        assert ((cp = queue_dup(q)) != NULL);
        assert (cp->n = num_entries);
        assert (cp->sz = num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(cp, i) == entries[i]);
        }

        free(cp);
    }

    /* test unshift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert ((q = queue_unshift(q, entries[i])) != NULL);
        }
        assert (q->n == num_entries * 2);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i) == entries[num_entries - i - 1]);
        }
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i + num_entries) == entries[i]);
        }
        assert (q->sz = num_entries * 2);
    }

    /* test shrink */
    {
        size_t n = num_entries * 2;
        assert (q->sz = n);
        assert (q->n = q->sz);
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n - i; j++)
            {
                size_t p = (j < num_entries) ?
                    num_entries - j - 1 : j - num_entries;
                assert (queue_get(q, j) == entries[p]);
            }
            assert ((q = queue_shrink(q)) != NULL);
            assert (q->sz == n - i);
            assert (q->sz == q->n);
            assert (queue_pop(q) != NULL);
        }
        assert ((q = queue_shrink(q)) != NULL);
        assert (q->sz == 0);
    }

    /* test extend */
    {
        assert ((q = queue_extend(q, (void **) entries, num_entries)) != NULL);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i) == entries[i]);
        }
        q->n = 0;
        q->s_ = 6;
        assert ((q = queue_extend(q, (void **) entries, num_entries)) != NULL);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (queue_get(q, i) == entries[i]);
        }
        assert ((q = queue_extend(q, (void **) entries, num_entries)) != NULL);
        for (size_t i = 0; i < num_entries * 2; i++)
        {
            assert (queue_get(q, i) == entries[i % num_entries]);
        }
    }

    /* test destroy */
    {
        queue_clear(q);
        for (size_t i = 0; i < num_entries; i++)
        {
            char * d = strdup(entries[i]);
            assert (d);
            assert ((q = queue_push(q, d)) != NULL);
        }
        queue_destroy(q, free);
    }

    test_end(0);
    return 0;
}
