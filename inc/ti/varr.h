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
void ti_varr_destroy(ti_varr_t * varr);
int ti_varr_append(ti_varr_t * to, void ** v, ex_t * e);
_Bool ti_varr_has_things(ti_varr_t * varr);
int ti_varr_to_list(ti_varr_t ** varr);
_Bool ti__varr_eq(ti_varr_t * varra, ti_varr_t * varrb);
static inline _Bool ti_varr_may_have_things(ti_varr_t * varr);
static inline _Bool ti_varr_is_list(ti_varr_t * varr);
static inline _Bool ti_varr_is_tuple(ti_varr_t * varr);
static inline _Bool ti_varr_eq(ti_varr_t * va, ti_varr_t * vb);

struct ti_varr_s
{
    uint32_t ref;
    uint8_t tp;
    uint8_t flags;
    uint16_t _pad1;
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

#endif  /* TI_VARR_H_ */

