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

/*
 * does not increment `*v` reference counter but the value might change to
 * a (new) tuple pointer.
 */
static inline int ti_varr_append(ti_varr_t * to, void ** v, ex_t * e)
{
    if (vec_reserve(&to->vec, 1))
    {
        ex_set_mem(e);
        return e->nr;
    }
    if (ti_varr_val_prepare(to, v, e))
        return e->nr;

    VEC_push(to->vec, *v);
    return 0;
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

/*
 * does not increment `*v` reference counter but the value might change to
 * a (new) tuple pointer.
 */
static inline int ti_varr_insert(
        ti_varr_t * to,
        void ** v,
        ex_t * e,
        uint32_t i)
{
    if (vec_reserve(&to->vec, 1))
    {
        ex_set_mem(e);
        return e->nr;
    }
    if (ti_varr_val_prepare(to, v, e))
        return e->nr;

    (void) vec_insert(&to->vec, *v, i);
    return e->nr;
}

static inline void * ti_varr_key(ti_varr_t * varr)
{
    return ti_thing_is_object(varr->parent)
            ? varr->key_
            : ((ti_field_t *) varr->key_)->name;
}

static inline int ti_varr_to_pk(ti_varr_t * varr, ti_vp_t * vp, int options)
{
    if (msgpack_pack_array(&vp->pk, varr->vec->n))
        return -1;
    for (vec_each(varr->vec, ti_val_t, v))
        if (ti_val_to_pk(v, vp, options))
            return -1;
    return 0;
}


#endif  /* TI_RAW_INLINE_H_ */
