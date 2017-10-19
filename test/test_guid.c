/*
 * test_guid.c
 *
 *  Created on: Oct 19, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "test.h"
#include "test_guid.h"
#include <util/guid.h>


int main()
{
    test_start("guid");

    /* test guid */
    {
        guid_t guid;
        guid_init(&guid, ((uint64_t) 1 << 63) - 1);
        assert ( strcmp(guid.guid, "_7$$$$$$$$$$_") == 0);

        guid_init(&guid, 0);
        assert ( strcmp(guid.guid, "_00000000000_") == 0);

        guid_init(&guid, 63);
        assert ( strcmp(guid.guid, "_0000000000$_") == 0);

        guid_init(&guid, 64);
        assert ( strcmp(guid.guid, "_00000000010_") == 0);
    }

    test_end(0);
    return 0;
}
