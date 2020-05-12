/*
 * ti/venum.h
 */
#ifndef TI_VENUM_H_
#define TI_VENUM_H_

#define VENUM(__x) ((ti_venum_t *) (__x))->val

#include <stdlib.h>
#include <inttypes.h>
#include <ti/val.h>
#include <ti/enum.h>

typedef struct ti_venum_s ti_venum_t;

ti_venum_t * ti_venum_create(ti_enum_t * enum_, ti_val_t * val, uint16_t idx);
void ti_venum_destroy(ti_venum_t * venum);

#define VINT(__x) ((ti_vint_t *) (__x))->int_

struct ti_venum_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t _flags;
    uint16_t idx;       /* index in enum->vec */
    ti_enum_t * enum_;
    ti_val_t * val;
};

static inline uint16_t ti_venum_id(ti_venum_t * venum)
{
    return venum->enum_->enum_id;
}

static inline const char * ti_venum_name(ti_venum_t * venum)
{
    return venum->enum_->name;
}

static inline ti_raw_t * ti_venum_get_rname(ti_venum_t * venum)
{
    ti_raw_t * rname = venum->enum_->rname;
    ti_incref(rname);
    return rname;
}
#endif  /* TI_VENUM_H_ */
