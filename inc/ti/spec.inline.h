/*
 * ti/spec.inline.h
 */
#ifndef TI_SPEC_INLINE_H_
#define TI_SPEC_INLINE_H_

#include <ti/val.h>
#include <ti/spec.h>
#include <inttypes.h>

static inline ti_spec_rval_enum ti_spec_check_val(
        uint16_t spec,
        ti_val_t * val)
{
    if ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val))
        return 0;

    spec &= TI_SPEC_MASK_NILLABLE;
    return spec == TI_SPEC_ANY ? 0 : ti__spec_check_val(spec, val);
}

static inline _Bool ti_spec_maps_to_val(uint16_t spec, ti_val_t * val)
{
    if ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val))
        return true;

    spec &= TI_SPEC_MASK_NILLABLE;
    return spec == TI_SPEC_ANY ? true : ti__spec_maps_to_val(spec, val);
}

#endif  /* TI_SPEC_INLINE_H_ */
