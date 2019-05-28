/*
 * guid.c
 */
#include <stdio.h>
#include <util/guid.h>

static const char guid__map[64] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', '-', '_'
};

void guid_init(guid_t * guid, uint64_t id)
{
    char * pt = guid->guid;

    for (uint64_t i = 60; i >= 6; i -= 6, pt++)
    {
        *pt = guid__map[id / ((uint64_t) 1 << i)];
        id %= (uint64_t) 1 << i;
    }

    *pt = guid__map[id];
    pt++;
    *pt = '\0';
}
