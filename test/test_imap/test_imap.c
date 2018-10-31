#include "../test.h"
#include <util/imap.h>

int main()
{
    test_start("imap");

    /* test adding values */
    {
        _assert (1 + 1 == 2);
    }


    return test_end();
}
