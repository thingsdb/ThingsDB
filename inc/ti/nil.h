/*
 * ti/nil.h
 */
#ifndef TI_NIL_H_
#define TI_NIL_H_

typedef struct ti_nil_s ti_nil_t;

#include <inttypes.h>
#include <tiinc.h>

struct ti_nil_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;
    void * nil;
};

ti_nil_t nil__val;

static inline ti_nil_t * ti_nil_get(void)
{
    ti_incref(&nil__val);
    return &nil__val;
}

#endif  /* TI_NIL_H_ */
