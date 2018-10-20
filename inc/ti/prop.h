/*
 * prop.h
 */
#ifndef TI_PROP_H_
#define TI_PROP_H_

typedef struct ti_prop_s ti_prop_t;

#include <stdint.h>
#include <stdlib.h>

ti_prop_t * ti_prop_create(const char * name, size_t n);
ti_prop_t * ti_prop_grab(ti_prop_t * prop);
void ti_prop_drop(ti_prop_t * prop);
static inline void ti_prop_destroy(ti_prop_t * prop);

struct ti_prop_s
{
    uint32_t ref;
    uint32_t n;             /* strlen(prop->name) */
    char name[];            /* null terminated string */
};

static inline void ti_prop_destroy(ti_prop_t * prop)
{
    free(prop);
}

#endif /* TI_PROP_H_ */
