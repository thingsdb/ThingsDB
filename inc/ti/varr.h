/*
 * ti/varr.h
 */
#ifndef TI_VARR_H_
#define TI_VARR_H_

typedef struct ti_varr_s ti_varr_t;

#include <inttypes.h>
#include <util/vec.h>

ti_varr_t * ti_varr_create(size_t sz);

void ti_varr_destroy(ti_varr_t * varr);

struct ti_varr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _pad0;
    uint16_t _pad1;
    vec_t * vec;
};

#endif  /* TI_VARR_H_ */
