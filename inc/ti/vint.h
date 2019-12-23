/*
 * ti/vint.h
 */
#ifndef TI_VINT_H_
#define TI_VINT_H_

#include <stdlib.h>
#include <inttypes.h>

typedef struct ti_vint_s ti_vint_t;

ti_vint_t * ti_vint_create(int64_t i);
static inline void ti_vint_free(ti_vint_t * vint);

#define VINT(__x) ((ti_vint_t *) (__x))->int_

struct ti_vint_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t _pad1;
    int64_t int_;
};

static inline void ti_vint_free(ti_vint_t * vint)
{
    free(vint);
}

#endif  /* TI_VINT_H_ */
