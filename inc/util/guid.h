/*
 * guid.h
 */
#ifndef GUID_H_
#define GUID_H_

typedef struct guid_s guid_t;

#include <stdint.h>

void guid_init(guid_t * guid, uint64_t id);

struct guid_s
{
    char guid[12];
};

#endif /* GUID_H_ */

