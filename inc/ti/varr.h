/*
 * ti/varr.h
 */
#ifndef TI_VARR_H_
#define TI_VARR_H_

typedef struct ti_varr_s ti_varr_t;

#include <inttypes.h>
#include <util/vec.h>
#include <ti/ex.h>

ti_varr_t * ti_varr_create(size_t sz);
void ti_varr_destroy(ti_varr_t * varr);
int ti_varr_append(ti_varr_t * to, void ** v, ex_t * e);
_Bool ti_varr_has_things(ti_varr_t * varr);
int ti_varr_to_list(ti_varr_t ** varr);
static inline _Bool ti_varr_may_have_things(ti_varr_t * varr);
static inline _Bool ti_varr_is_list(ti_varr_t * varr);
static inline _Bool ti_varr_is_tuple(ti_varr_t * varr);
static inline void ti_varr_set_assigned(ti_varr_t * varr);
static inline _Bool ti_varr_is_assigned(ti_varr_t * varr);

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

static inline void ti_varr_set_assigned(ti_varr_t * varr)
{
    varr->flags &= ~TI_VFLAG_UNASSIGNED;
}

static inline _Bool ti_varr_is_assigned(ti_varr_t * varr)
{
    return ~varr->flags & TI_VFLAG_UNASSIGNED;
}

#endif  /* TI_VARR_H_ */

