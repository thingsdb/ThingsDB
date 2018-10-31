#include "../test.h"
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


static void push_entries(vec_t ** v)
{
    for (size_t i = 0; i < num_entries; i++)
    {
        _assert (vec_push(v, entries[i]) == 0);
    }
}


int main()
{
    test_start("vec");

    vec_t * v = vec_new(0);

    _assert (v);

    /* test initial values */
    {
        _assert (v->sz == 0);
        _assert (v->n == 0);
        _assert (sizeof(*v) == sizeof(vec_t));
    }

    /* test push */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (vec_push(&v, entries[i]) == 0);
        }
    }

    /* test length */
    {
        _assert (v->n == num_entries);
    }

    /* test pop */
    {
        for (size_t i = 0; i < num_entries && ++i;)
        {
            _assert (vec_pop(v) == entries[num_entries - i]);
        }

        _assert (v->n == 0);
        push_entries(&v);  /* restore entries */
    }

    /* test dup */
    {
        vec_t * cp;
        _assert ((cp = vec_dup(v)) != NULL);
        _assert (cp->n == num_entries);
        _assert (cp->sz == num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (vec_get(cp, i) == entries[i]);
        }
        free(cp);
    }

    /* test extend */
    {
        _assert (vec_extend(&v, (void **) entries, num_entries) == 0);
        for (size_t i = 0; i < num_entries * 2; i++)
        {
            _assert (vec_get(v, i) == entries[i % num_entries]);
        }
    }

    /* test shrink */
    {
        size_t n = v->n;
        for (size_t i = 0; i < n; i++)
        {
            _assert (vec_pop(v) == entries[num_entries - (i % 8) - 1]);
            _assert (vec_shrink(&v) == 0);
            _assert (v->n == v->sz);
        }
    }

    /* test vec_each loop */
    {
        push_entries(&v); /* restore some points */
        char ** e = entries;
        size_t n = 0;
        for (vec_each(v, char, s), n++, e++)
        {
            _assert (s == *e);
        }
        _assert (n == num_entries); /* make sure we have hit all entries */
    }

    /* test vec_remove loop */
    {
        vec_remove(v, 4);
        char ** e = entries;
        size_t n = 0;

        _assert (v->n == num_entries - 1);

        for (vec_each(v, char, s), n++, e++)
        {
            if (n == 4)
            {
                e++;
            }
            _assert (s == *e);
        }

        _assert (n == num_entries - 1);
    }

    /* test null values */
    {
        vec_clear(v);
        size_t n = 4;
        for (size_t i = 0; i < n; i++)
        {
            vec_push(&v, NULL);
        }
        size_t i = 0;
        for (vec_each(v, void, t), i++)
        {
            _assert (t == NULL);
        }
        _assert (i == n);
    }

    free(v);

    return test_end();
}
