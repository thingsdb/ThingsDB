#include "../test.h"
#include <util/guid.h>


int main()
{
    test_start("guid");

    /* test guid */
    {
        guid_t guid;

        guid_init(&guid, 1);
        _assert ( guid.guid[sizeof(guid_t) - 1] == '\0');

        guid_init(&guid, ((uint64_t) 1 << 63) - 1);
        _assert ( strcmp(guid.guid, ".7__________") == 0);

        guid_init(&guid, 0);
        _assert ( strcmp(guid.guid, ".00000000000") == 0);

        guid_init(&guid, 63);
        _assert ( strcmp(guid.guid, ".0000000000_") == 0);

        guid_init(&guid, 64);
        _assert ( strcmp(guid.guid, ".00000000010") == 0);

    }

    return test_end();
}
