/*
 * test_imap.c
 *
 *  Created on: Oct 11, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "test.h"
#include <util/imap.h>



int main()
{
    test_start("imap");

    /* test adding values */
    {
        assert (1 + 1 == 2);
    }

    test_end(0);
    return 0;
}
