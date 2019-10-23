#include "../test.h"
#include <util/queue.h>
#include <util/vec.h>

static const unsigned int num_entries = 8;
static char * entries[] = {
    "Zero",
    "First entry",
    "Second entry",
    "Third entry",
    "Fourth entry",
    "Fifth entry",
    "Sixth entry",
    "Seventh entry",
};

static void push_entries(queue_t * q)
{
    for (size_t i = 0; i < num_entries; i++)
    {
        _assert (queue_push(&q, entries[i]) == 0);
    }
}

int main()
{
    test_start("queue");

    queue_t * q = queue_new(0);

    _assert (q);

    /* test initial values */
    {
        _assert (q->sz == 0);
        _assert (q->n == 0);
        _assert (sizeof(*q) == sizeof(queue_t));
    }

    /* test insert */
    {
        size_t i = 0;
        _assert (queue_push(&q, entries[0]) == 0);
        _assert (queue_push(&q, entries[2]) == 0);
        _assert (queue_insert(&q, 1, entries[1]) == 0);

        for (queue_each(q, char, s), i++)
        {
            _assert (s == entries[i]);
        }

        for (i = 0; i < 3; i++)
        {
            _assert (queue_get(q, i) == entries[i]);
        }

        _assert (queue_shift(q) == entries[0]);
        _assert (queue_shift(q) == entries[1]);
        _assert (queue_push(&q, entries[4]) == 0);
        _assert (queue_insert(&q, 1, entries[3]) == 0);

        for (i = 0; i < 3; i++)
        {
            _assert (queue_get(q, i) == entries[i+2]);
        }

        _assert (queue_shift(q) == entries[2]);
        _assert (queue_shift(q) == entries[3]);
        _assert (queue_push(&q, entries[6]) == 0);
        _assert (queue_insert(&q, 1, entries[5]) == 0);

        for (i = 0; i < 3; i++)
        {
            _assert (queue_get(q, i) == entries[i+4]);
        }

        queue_clear(q);
    }

    /* test push */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_push(&q, entries[i]) == 0);
        }
    }

    /* test length */
    {
        _assert (q->n == num_entries);
    }

    /* test pop */
    {
        for (size_t i = 0; i < num_entries && ++i;)
        {
            _assert (queue_pop(q) == entries[num_entries - i]);
        }

        _assert (q->n == 0);
        push_entries(q);  /* restore entries */
    }

    /* test shift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_shift(q) == entries[i]);
        }
        _assert (q->n == 0);
    }

    /* test force another order in queue */
    {
        q->s_ = 4;
        push_entries(q);
        _assert (q->sz == num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i) == entries[i]);
        }
    }

    /* test each */
    {
        size_t i = 0;
        char ** entry = entries;
        for (queue_each(q, char, s), i++, entry++)
        {
            _assert (s == *entry);
        }
        _assert (i == num_entries);
    }

    /* test copy */
    {
        _assert (q->n == num_entries);
        vec_t * v = vec_new(q->n);
        queue_copy(q, v->data);
        v->n = q->n;

        for (size_t i = 0; i < v->n; i++)
        {
            _assert (vec_get(v, i) == entries[i]);
        }
        free(v);
    }

    /* test adding an extra value */
    {
        const char * extra = "extra";
        _assert (queue_push(&q, (void *) extra) == 0);
        _assert (q->sz == num_entries * 2);
        _assert (q->n == num_entries + 1);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i) == entries[i]);
        }
        _assert (queue_pop(q) == extra);
    }

    /* test dup */
    {
        queue_t * cp;
        _assert ((cp = queue_dup(q)) != NULL);
        _assert (cp->n == num_entries);
        _assert (cp->sz == num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(cp, i) == entries[i]);
        }

        free(cp);
    }

    /* test unshift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_unshift(&q, entries[i]) == 0);
        }
        _assert (q->n == num_entries * 2);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i) == entries[num_entries - i - 1]);
        }
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i + num_entries) == entries[i]);
        }
        _assert (q->sz == num_entries * 2);
    }

    /* test shrink */
    {
        size_t n = num_entries * 2;
        _assert (q->sz == n);
        _assert (q->n == q->sz);
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n - i; j++)
            {
                size_t p = (j < num_entries) ?
                    num_entries - j - 1 : j - num_entries;
                _assert (queue_get(q, j) == entries[p]);
            }
            _assert (queue_shrink(&q) == 0);
            _assert (q->sz == n - i);
            _assert (q->sz == q->n);
            _assert (queue_pop(q) != NULL);
        }
        _assert (queue_shrink(&q) == 0);
        _assert (q->sz == 0);
    }

    /* test extend */
    {
        _assert (queue_extend(&q, (void **) entries, num_entries) == 0);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i) == entries[i]);
        }
        q->n = 0;
        q->s_ = 6;
        _assert (queue_extend(&q, (void **) entries, num_entries) == 0);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_get(q, i) == entries[i]);
        }
        _assert (queue_extend(&q, (void **) entries, num_entries) == 0);
        for (size_t i = 0; i < num_entries * 2; i++)
        {
            _assert (queue_get(q, i) == entries[i % num_entries]);
        }
    }

    /* test value remove */
    {
        queue_clear(q);
        push_entries(q);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (queue_rmval(q, entries[i]) == entries[i]);
        }
        _assert (q->n == 0);
    }

    /* test null values */
    {
        queue_clear(q);
        size_t n = 4;
        for (size_t i = 0; i < n; i++)
        {
            queue_push(&q, NULL);
        }
        size_t i = 0;
        for (queue_each(q, void, t), i++)
        {
            _assert (t == NULL);
        }
        _assert (i == n);
    }

    /* test destroy */
    {
        queue_clear(q);
        for (size_t i = 0; i < num_entries; i++)
        {
            char * d = strdup(entries[i]);
            _assert (d);
            _assert (queue_push(&q, d) == 0);
        }
        queue_destroy(q, free);
    }

    return test_end();
}
