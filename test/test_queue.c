/*
 * test_queue.c
 *
 *  Created on: Sep 30, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */

#include "test.h"
#include <util/queue.h>



int main()
{
    test_start("queue");

    /* test adding values */
    {
        assert (1 + 1 == 2);
    }

    test_end(0);
    return 0;
}
