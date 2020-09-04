/*
 * ti/spec.inline.h
 */
#ifndef TI_SPEC_INLINE_H_
#define TI_SPEC_INLINE_H_

#include <ti/val.inline.h>
#include <ti/member.inline.h>
#include <ti/spec.h>
#include <inttypes.h>

static inline _Bool ti_spec_is_enum(uint16_t spec)
{
    return (spec & TI_SPEC_MASK_NILLABLE) >= 0x6000;
}

static inline _Bool ti_spec_is_arr_or_set(uint16_t spec)
{
    return (
        (spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_ARR ||
        (spec & TI_SPEC_MASK_NILLABLE) == TI_SPEC_SET
    );
}

static inline ti_spec_rval_enum ti_spec_check_nested_val(
        uint16_t spec,
        ti_val_t * val)
{
    return spec == TI_SPEC_ANY || ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val))
            ? TI_SPEC_RVAL_SUCCESS
            : ti__spec_check_nested_val(spec & TI_SPEC_MASK_NILLABLE, val);
}

static inline _Bool ti_spec_maps_to_nested_val(uint16_t spec, ti_val_t * val)
{
    return (
        spec == TI_SPEC_ANY ||
        ((spec & TI_SPEC_NILLABLE) && ti_val_is_nil(val)) ||
         ti__spec_maps_to_nested_val(spec & TI_SPEC_MASK_NILLABLE, val)
    );
}

static inline _Bool ti_spec_enum_eq_to_val(uint16_t spec, ti_val_t * val)
{
    return (
        ti_val_is_member(val) &&
        ti_member_enum_id((ti_member_t *) val) == (spec & TI_ENUM_ID_MASK)
    );
}

#endif  /* TI_SPEC_INLINE_H_ */
