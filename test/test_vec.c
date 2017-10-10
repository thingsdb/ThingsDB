/*
 * test_vec.c
 *
 *  Created on: Oct 09, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

#include "test.h"
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


static vec_t * push_entries(vec_t * v)
{
    for (size_t i = 0; i < num_entries; i++)
    {
        assert ((v = vec_push(v, entries[i])) != NULL);
    }
    return v;
}


int main()
{
    test_start("vec");

    vec_t * v = vec_new(0);

    /* test initial values */
    {
        assert (v->sz == 0);
        assert (v->n == 0);
        assert (sizeof(*v) == sizeof(vec_t));
    }

    /* test push */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert ((v = vec_push(v, entries[i])) != NULL);
        }
    }

    /* test length */
    {
        assert (v->n == num_entries);
    }

    /* test pop */
    {
        for (size_t i = 0; i < num_entries && ++i;)
        {
            assert (vec_pop(v) == entries[num_entries - i]);
        }

        assert (v->n == 0);
        v = push_entries(v);  /* restore entries */
    }

    /* test dup */
    {
        vec_t * cp;
        assert ((cp = vec_dup(v)) != NULL);
        assert (cp->n = num_entries);
        assert (cp->sz = num_entries);
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (vec_get(cp, i) == entries[i]);
        }
        free(cp);
    }

    /* test extend */
    {
        assert ((v = vec_extend(v, (void **) entries, num_entries)) != NULL);
        for (size_t i = 0; i < num_entries * 2; i++)
        {
            assert (vec_get(v, i) == entries[i % num_entries]);
        }
    }

    /* test shrink */
    {
        size_t n = v->n;
        for (size_t i = 0; i < n; i++)
        {
            assert (vec_pop(v) == entries[num_entries - (i % 8) - 1]);
            assert ((v = vec_shrink(v)) != NULL);
            assert (v->n == v->sz);
        }
    }

    free(v);

    test_end(0);
    return 0;
}
