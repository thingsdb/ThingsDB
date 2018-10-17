/*
 * prop.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Jeroen van der Heijden <jeroen@transceptor.technology>
 */
#ifndef TI_PROP_H_
#define TI_PROP_H_

typedef struct ti_prop_s ti_prop_t;

#include <stdint.h>

ti_prop_t * ti_prop_create(const char * name);
ti_prop_t * ti_prop_grab(ti_prop_t * prop);
static inline void ti_prop_destroy(ti_prop_t * prop);

struct ti_prop_s
{
    uint32_t ref;
    char name[];
};

static inline void ti_prop_destroy(ti_prop_t * prop)
{
    free(prop);
}

#endif /* TI_PROP_H_ */
