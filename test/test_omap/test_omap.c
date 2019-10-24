#include "../test.h"
#include <util/omap.h>

static const unsigned int num_entries = 15;
static char * entries[] = {
    "Zero",
    "First entry",
    "Second entry",
    "Third entry",
    "Fourth entry",
    "Fifth entry",
    "Sixth entry",
    "Seventh entry",
    "8",
    "9",
    "entry 10",
    "entry 11",
    "entry 12",
    "entry-last",
    "age",
};



int main()
{
    test_start("omap");

    omap_t * omap = omap_create();

    _assert (omap);

    /* test adding values */
    for (unsigned int i = 0; i < num_entries; i++)
    {
        _assert (omap_add(omap, i, entries[i]) == 0);
    }

    _assert (omap->n == num_entries);

    for (unsigned int i = 0; i < num_entries; i++)
    {
        _assert (omap_get(omap, i) == entries[i]);
    }

    for (unsigned int i = 0; i < num_entries; i++)
    {
        _assert (omap_set(omap, i, entries[i]) == entries[i]);
    }

    omap_iter_t iter = omap_iter(omap);
    for (unsigned int n = omap->n; n--;)
    {
       size_t i =  iter->id_;
       char * s = iter->data_;
       iter = iter->next_;
        _assert (omap_rm(omap, i) == s);
    }

    _assert (omap->n == 0);

    omap_destroy(omap, NULL);

    return test_end();
}
