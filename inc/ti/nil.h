/*
 * ti/nil.h
 */
#ifndef TI_NIL_H_
#define TI_NIL_H_

typedef struct ti_nil_s ti_nil_t;

#include <inttypes.h>

struct ti_nil_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad8;
    uint16_t _pad16;
    void * nil;
};

ti_nil_t * ti_nil_get(void);

#endif  /* TI_NIL_H_ */
