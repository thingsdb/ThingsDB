/*
 * ti/varr.h
 */
#ifndef TI_VARR_H_
#define TI_VARR_H_

typedef struct ti_varr_s ti_varr_t;

#include <inttypes.h>
#include <util/vec.h>
#include <ex.h>

ti_varr_t * ti_varr_create(size_t sz);
ti_varr_t * ti_varr_from_vec(vec_t * vec);
ti_varr_t * ti_varr_from_slice(
        ti_varr_t * source,
        ssize_t start,
        ssize_t stop,
        ssize_t step);
void ti_varr_destroy(ti_varr_t * varr);
_Bool ti_varr_has_things(ti_varr_t * varr);
int ti_varr_to_list(ti_varr_t ** varr);
int ti_varr_val_prepare(ti_varr_t * to, void ** v, ex_t * e);
int ti_varr_set(ti_varr_t * to, void ** v, size_t idx, ex_t * e);
_Bool ti__varr_eq(ti_varr_t * varra, ti_varr_t * varrb);
static inline _Bool ti_varr_may_have_things(ti_varr_t * varr);
static inline _Bool ti_varr_is_list(ti_varr_t * varr);
static inline _Bool ti_varr_is_tuple(ti_varr_t * varr);
static inline _Bool ti_varr_eq(ti_varr_t * va, ti_varr_t * vb);
static inline int ti_varr_append(ti_varr_t * to, void ** v, ex_t * e);

struct ti_varr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t spec;
    vec_t * vec;
};

#include <ti/val.h>

static inline _Bool ti_varr_may_have_things(ti_varr_t * varr)
{
    return varr->flags & TI_VFLAG_ARR_MHT;
}

static inline _Bool ti_varr_is_list(ti_varr_t * varr)
{
    return ~varr->flags & TI_VFLAG_ARR_TUPLE;
}

static inline _Bool ti_varr_is_tuple(ti_varr_t * varr)
{
    return varr->flags & TI_VFLAG_ARR_TUPLE;
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
    if (ti_varr_val_prepare(to, v, e))
        return e->nr;

    if (vec_push(&to->vec, *v))
        ex_set_mem(e);

    return e->nr;
}


#endif  /* TI_VARR_H_ */

