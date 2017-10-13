/*
 * test_link.c
 *
 *  Created on: Oct 11, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "test.h"
#include <util/link.h>

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

int main()
{
    test_start("link");

    link_t * link = link_create();

    /* test initial n */
    {
        assert (link->n == 0);
    }

    /* test appending values */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (link_insert(link, link->n, entries[i]) == 0);
        }
    }

    /* test each */
    {
        size_t i = 0;
        char ** entry = entries;
        link_iter_t iter = link_iter(link);
        for (link_each(iter, char, val), i++, entry++)
        {
            assert (val == *entry);
        }
        assert (i == num_entries);
    }

    /* test shift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (link_shift(link) == entries[i]);
        }
        assert (link->n == 0);

        /* restore entries */
        for (size_t i = 0; i < num_entries; i++)
        {
            assert (link_insert(link, link->n, entries[i]) == 0);
        }
    }

    /* test remove while iterate */
    {
        size_t i = 0;
        link_iter_t iter = link_iter(link);
        for (link_each(iter, char, val), i++)
        {
            if (val == entries[1] || val == entries[3])
            {
                /* return value should be the next value, so 2 / 4 */
                link_iter_remove(link, iter);
                val = (char *) link_iter_get(iter);
                i++;
            }
            assert (val == entries[i]);
        }

        assert (link-> n == num_entries - 2);

        /* test inset iter */
        iter = link_iter(link);
        for (link_each(iter, char, val))
        {
            if (val == entries[2])
            {
                assert (link_iter_insert(link, iter, entries[1]) == 0);
                link_iter_next(iter);
            }
            if (val == entries[4])
            {
                assert (link_iter_insert(link, iter, entries[3]) == 0);
                link_iter_next(iter);
            }
        }

        i = 0;
        iter = link_iter(link);
        for (link_each(iter, char, val), i++)
        {
            assert (val == entries[i]);
        }

    }

    link_destroy(link, NULL);

    test_end(0);
    return 0;
}
