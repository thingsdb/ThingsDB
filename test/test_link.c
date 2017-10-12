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

        for (link_each(link, char, val), i++, entry++)
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

    /* test pop current */
    {
        size_t i = 0;
        for (link_each(link, char, val), i++)
        {
            if (val == entries[1] || val == entries[3])
            {
                /* return value should be the next value, so 2 / 4 */
                val = link_pop_current(link);
                i++;
            }
            assert (val == entries[i]);
        }

        assert (link-> n == num_entries - 2);

        /* test inset before */
        for (link_each(link, char, val))
        {
            if (val == entries[0])
            {
                assert (link_insert_after(link, entries[1]) == 0);
            }
            if (val == entries[4])
            {
                assert (link_insert_before(link, entries[3]) == 0);
                break;  /* stop because of recursion */
            }
        }

        i = 0;
        for (link_each(link, char, val), i++)
        {
            assert (val == entries[i]);
        }

    }

    link_destroy(link, NULL);

    test_end(0);
    return 0;
}
