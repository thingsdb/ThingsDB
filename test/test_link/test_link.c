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
        for (size_t i = num_entries; i; i--)
        {
            _assert (link_insert(link, entries[i-1]) == 0);
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

    link_destroy(link, NULL);

    return test_end();
}
