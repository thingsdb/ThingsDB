/*
 * ti/speci.h
 */
#ifndef TI_SPECI_H_
#define TI_SPECI_H_

#include <ti/val.h>
#include <ti/spec.h>
#include <inttypes.h>

static inline ti_spec_ret_enum_t ti_spec_check(uint16_t spec, ti_val_t * val)
{
    if ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val))
        return 0;

    spec &= TI_SPEC_MASK_NILLABLE;
    return spec == TI_SPEC_ANY ? 0 : ti__spec_check(spec, val);
}


#endif  /* TI_SPEC_H_ */
