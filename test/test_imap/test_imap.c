#include "../test.h"
#include <util/imap.h>

static const unsigned int num_batches = 5;
static const unsigned int num_entries = 8;


int main()
{
    test_start("imap");

    imap_t * imap = imap_create();
    uint64_t id = 0;

    _assert (imap);

    /* test adding values */
    for (size_t j = 0; j < num_batches; ++j)
    {
        for (size_t i = 1; i <= num_entries ; ++i)
        {
            id = (j*100) + i + (i*j);
            _assert (imap_add(
                imap, id,
                (void *) (uintptr_t)
                &id) == IMAP_SUCCESS);
        }
    }

    _assert (imap->n == num_batches * num_entries);

    while ((id = imap_unused_id(imap, 0x1000)) != 0x1000)
    {
        _assert (imap_add(imap, id, (void *) (uintptr_t) &id) == IMAP_SUCCESS);
    }

    _assert (imap->n == 0x1000);

    imap_destroy(imap, NULL);

    return test_end();
}
