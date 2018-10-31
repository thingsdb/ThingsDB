#include "../test.h"
#include <util/link.h>

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

int main()
{
    test_start("link");

    link_t * link = link_create();

    _assert (link);

    /* test initial n */
    {
        _assert (link->n == 0);
    }

    /* test appending values */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (link_insert(link, link->n, entries[i]) == 0);
        }
    }

    /* test each */
    {
        size_t i = 0;
        char ** entry = entries;
        link_iter_t iter = link_iter(link);
        for (link_each(iter, char, val), i++, entry++)
        {
            _assert (val == *entry);
        }
        _assert (i == num_entries);
    }

    /* test shift */
    {
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (link_shift(link) == entries[i]);
        }
        _assert (link->n == 0);

        /* restore entries */
        for (size_t i = 0; i < num_entries; i++)
        {
            _assert (link_insert(link, link->n, entries[i]) == 0);
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
            _assert (val == entries[i]);
        }

        _assert (link-> n == num_entries - 2);

        /* test inset iter */
        iter = link_iter(link);
        for (link_each(iter, char, val))
        {
            if (val == entries[2])
            {
                _assert (link_iter_insert(link, iter, entries[1]) == 0);
                link_iter_next(iter);
            }
            if (val == entries[4])
            {
                _assert (link_iter_insert(link, iter, entries[3]) == 0);
                link_iter_next(iter);
            }
        }

        i = 0;
        iter = link_iter(link);
        for (link_each(iter, char, val), i++)
        {
            _assert (val == entries[i]);
        }

    }

    link_destroy(link, NULL);

    return test_end();
}
