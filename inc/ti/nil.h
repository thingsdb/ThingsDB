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
    long:24;
    void * nil;
};

extern ti_nil_t nil__val;

static inline ti_nil_t * ti_nil_get(void)
{
    ti_incref(&nil__val);
    return &nil__val;
}

static inline _Bool ti_nil_no_ref(void)
{
    return nil__val.ref == 1;
}

#endif  /* TI_NIL_H_ */
