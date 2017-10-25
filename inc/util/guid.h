/*
 * guid.h
 *
 *  Created on: Oct 5, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef GUID_H_
#define GUID_H_

typedef struct guid_s guid_t;

#include <stdint.h>

void guid_init(guid_t * guid, uint64_t id);

struct guid_s
{
    char guid[13];
};

#endif /* GUID_H_ */

