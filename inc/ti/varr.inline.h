/*
 * ti/varr.inline.h
 */
#ifndef TI_VARR_INLINE_H_
#define TI_VARR_INLINE_H_

#include <stdint.h>
#include <ex.h>
#include <ti/field.t.h>
#include <ti/raw.h>
#include <ti/thing.h>
#include <ti/val.h>
#include <ti/varr.h>
#include <util/mpack.h>
#include <util/vec.h>

static inline _Bool ti_varr_may_have_things(ti_varr_t * varr)
{
    return varr->flags & TI_VARR_FLAG_MHT;
}

static inline _Bool ti_varr_may_gen_ids(ti_varr_t * varr)
{
    return varr->flags & (TI_VARR_FLAG_MHT|TI_VARR_FLAG_MHR);
}

static inline _Bool ti_varr_is_list(ti_varr_t * varr)
{
    return ~varr->flags & TI_VARR_FLAG_TUPLE;
}

static inline _Bool ti_varr_is_tuple(ti_varr_t * varr)
{
    return varr->flags & TI_VARR_FLAG_TUPLE;
}

static inline _Bool ti_varr_eq(ti_varr_t * va, ti_varr_t * vb)
{
    return va == vb || (va->vec->n == vb->vec->n && ti__varr_eq(va, vb));
}

static inline uint16_t ti_varr_unsafe_spec(ti_varr_t * varr)
{
    return ti_thing_is_object(varr->parent)
            ? TI_SPEC_ANY
            : ((ti_field_t *) varr->key_)->nested_spec;
}

static inline uint16_t ti_varr_spec(ti_varr_t * varr)
{
    return ((!varr->parent) || ti_thing_is_object(varr->parent))
            ? TI_SPEC_ANY
            : ((ti_field_t *) varr->key_)->nested_spec;
}

static inline void * ti_varr_key(ti_varr_t * varr)
{
    return ti_thing_is_object(varr->parent)
            ? varr->key_
            : ((ti_field_t *) varr->key_)->name;
}

static inline void ti_varr_set_may_flags(ti_varr_t * to, ti_varr_t * from)
{
    to->flags |= from->flags & (TI_VARR_FLAG_MHT|TI_VARR_FLAG_MHR);
}

static inline _Bool ti_varr_is_stored(ti_varr_t * varr)
{
    return varr->parent && varr->parent->id;
}

#endif  /* TI_RAW_INLINE_H_ */
