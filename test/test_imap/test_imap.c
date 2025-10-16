#include "../test.h"
#include <util/imap.h>

int main()
{
    test_start("imap");

    imap_t * imap = imap_create();
    uint64_t id = 0;
    uint64_t x = 0;
    uint64_t t = 0x1fff;

    _assert (imap);

    while ((id = imap_unused_id(imap, t)) != t)
    {
        _assert (x < t);
        _assert (id < t);
        _assert (imap_add(imap, id, (void *) (uintptr_t) &id) == IMAP_SUCCESS);
        x += 1;
    }
    _assert (imap->n == t);

    imap_destroy(imap, NULL);

    return test_end();
}
