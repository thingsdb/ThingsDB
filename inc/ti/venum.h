/*
 * ti/venum.h
 */
#ifndef TI_VENUM_H_
#define TI_VENUM_H_

#define VENUM(__x) ((ti_venum_t *) (__x))->val

#include <stdlib.h>
#include <inttypes.h>
#include <ti/val.h>

typedef struct ti_venum_s ti_venum_t;

ti_venum_t * ti_venum_create(ti_enum_t * enum_, ti_val_t * val);
void ti_venum_destroy(ti_venum_t * venum);

#define VINT(__x) ((ti_vint_t *) (__x))->int_

struct ti_venum_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    ti_enum_t * enum_;
    ti_val_t * val;
};

static inline void ti_vint_free(ti_vint_t * vint)
{
    free(vint);
}

#endif  /* TI_VENUM_H_ */
