/*
 * ti/opr.h
 */
#ifndef TI_OPR_H_
#define TI_OPR_H_

#include <cleri/cleri.h>
#include <ex.h>
#include <ti/val.h>

typedef int (*opr_enum_cb) (ti_val_t *, ti_val_t **, ex_t *);
typedef int (*opr_enum_inplace_cb) (ti_val_t *, ti_val_t **, ex_t *, _Bool);


int ti_opr_a_to_b(ti_val_t * a, cleri_node_t * nd, ti_val_t ** b, ex_t * e);
_Bool ti__opr_eq_(ti_val_t * a, ti_val_t * b);
int ti_opr_compare(ti_val_t * a, ti_val_t * b, ex_t * e);
int opr_on_enum(ti_val_t * a, ti_val_t ** enum_b, ex_t * e, opr_enum_cb cb);
int opr_on_enum_inplace(
        ti_val_t * a,
        ti_val_t ** enum_b,
        ex_t * e,
        _Bool inplace,
        opr_enum_inplace_cb cb);

static inline int ti_opr_compare_desc(ti_val_t * a, ti_val_t * b, ex_t * e)
{
    return ti_opr_compare(b, a, e);
}

static inline _Bool ti_opr_eq(ti_val_t * a, ti_val_t * b)
{
    return a == b || ti__opr_eq_(a, b);
}

#endif  /* TI_OPR_H_ */
